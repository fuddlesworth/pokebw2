#include "party_view.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "dex_media.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gx_backend.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "system/poke_icon.h"
#include "ui_assets.h"
#include "ui_components.h"
#include "ui_draw.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static MsgData *itemNames, *abilityNames, *natureNames, *moveNames;
static u32 animationFrame;
static struct {
    u32 file, palette;
    bool loaded;
    u16 pixels[32 * 64];
} icons[PARTY_SIZE];

void PcPartyView_ReadMember(PcPartyMember *member, PartyPkm *pkm) {
    *member = (PcPartyMember){ .pkm = pkm };
    if (!pkm)
        return;
    member->hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
    member->maxHp = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
    member->level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    member->status = GetStatusCond(pkm);
    member->type1 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL);
    member->type2 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL);
    member->ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
    member->item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    member->attack = PokeParty_GetParam(pkm, PKM_PARAM_ATTACK, NULL);
    member->defense = PokeParty_GetParam(pkm, PKM_PARAM_DEFENSE, NULL);
    member->spAttack = PokeParty_GetParam(pkm, PKM_PARAM_SP_ATTACK, NULL);
    member->spDefense = PokeParty_GetParam(pkm, PKM_PARAM_SP_DEFENSE, NULL);
    member->speed = PokeParty_GetParam(pkm, PKM_PARAM_SPEED, NULL);
    for (u32 i = 0; i < 4; i++) {
        member->moves[i] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);
        member->pp[i] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + i, NULL);
        member->maxPp[i] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_MAX_PP + i, NULL);
    }
}

static void DrawIcon(PartyPkm *pkm, u32 slot, float y, bool selected) {
    u32 file = PokeIcon_GetFileOfPkm(PokeParty_GetBoxPkm(pkm));
    u32 paletteIndex = PokeIcon_GetPaletteOfPkm(PokeParty_GetBoxPkm(pkm));
    if (!icons[slot].loaded || icons[slot].file != file || icons[slot].palette != paletteIndex) {
        NNSG2dCharacterData *chars;
        NNSG2dPaletteData *palette;
        void *charFile = GFL_G2DIOReadOBJNCGR(ARCID_POKEICON, file, FALSE, &chars, HEAPID_USER);
        void *paletteFile = GFL_G2DIOReadNCLR(ARCID_POKEICON, PokeIcon_GetPaletteFile(), &palette, HEAPID_USER);
        SDL_zero(icons[slot]);
        icons[slot].file = file;
        icons[slot].palette = paletteIndex;
        if (charFile && paletteFile && chars->szByte >= 1024 && palette->szByte >= (paletteIndex + 1) * 32) {
            const u8 *tiles = chars->pRawData;
            const u16 *colors = (const u16 *)palette->pRawData + paletteIndex * 16;
            for (u32 py = 0; py < 64; py++) {
                for (u32 px = 0; px < 32; px++) {
                    u32 offset = ((py / 8) * 4 + px / 8) * 32 + (py & 7) * 4 + (px & 7) / 2;
                    u32 index = (tiles[offset] >> ((px & 1) * 4)) & 15;
                    icons[slot].pixels[py * 32 + px] = index ? colors[index] | 0x8000 : 0;
                }
            }
            icons[slot].loaded = true;
        }
        if (charFile)
            GFL_HeapFree(charFile);
        if (paletteFile)
            GFL_HeapFree(paletteFile);
    }
    u32 tx = slot * 32;
    u32 frame = selected && GetStatusCond(pkm) == 0 && PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL)
                    ? (animationFrame / 15) % 2
                    : 0;
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    for (u32 row = 0; row < 32; row++)
        SDL_memcpy(atlas + (160 + row) * PC_UI_ATLAS_WIDTH + tx, icons[slot].pixels + (row + frame * 32) * 32,
                   32 * sizeof(u16));
    PcUi_Quad(48, y + 13, 64, 64, -0.45f, 1, 1, 1, PcUi_FontTexture(), tx, 160, 32, 32);
}

static const char *Status(const PcPartyMember *member) {
    PartyPkm *pkm = member->pkm;
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL))
        return "Egg";
    if (!member->hp)
        return "Fainted";
    static const char *const names[] = { "", "Sleep", "Poison", "Burn", "Freeze", "Paralysis" };
    u32 status = member->status;
    return status < SDL_arraysize(names) ? names[status] : "";
}

static u32 HpColor(u32 hp, u32 maximum) {
    return hp * 5 <= maximum ? PC_UI_DANGER : hp * 2 <= maximum ? PC_UI_WARNING : 0x68dfa2;
}

static void DrawRoster(const PcPartyView *view) {
    Font *font = view->font;
    for (u32 slot = 0; slot < PARTY_SIZE; slot++) {
        float y = 90 + slot * 89;
        char number[32];
        if (!view->members[slot].pkm) {
            SDL_snprintf(number, sizeof(number), "%02u", slot + 1);
            PcUi_RoleLabel(font, number, 63, y + 29, 105, y + 62, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
            PcUi_RoleLabel(font, "Empty", 124, y + 12, 340, y + 37, PC_UI_INK_MUTED, PC_UI_TEXT_LABEL);
            continue;
        }
        if (view->interactiveRows)
            PcUi_Control((PcUiShape){ .rect = { 2, y + 4, 358, 82 }, .topRight = 20 },
                         (PcUiAction){ view->rowAction, (int)slot });
        const PcPartyMember *member = &view->members[slot];
        PartyPkm *pkm = member->pkm;
        u16 name[32];
        DrawIcon(pkm, slot, y, slot == view->selected);
        bool egg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
        if (egg)
            PcUi_RoleLabel(font, "Egg", 122, y + 8, 281, y + 35, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        else {
            PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME_RAW, name);
            PcUi_RoleText(font, name, 122, y + 8, 281, y + 35, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        }
        if (!egg) {
            PcGx.party_hp_rows |= 1u << slot;
            SDL_snprintf(number, sizeof(number), "Lv.%u", member->level);
            float width = PcUi_RoleLabelWidth(font, number, PC_UI_TEXT_CAPTION);
            PcUi_RoleLabel(font, number, 343 - width, y + 12, 345, y + 35, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CAPTION);
            u32 hp = member->hp;
            u32 maxHp = member->maxHp;
            if (maxHp && hp)
                PcUi_Solid(124, y + 45, 216 * SDL_min(hp, maxHp) / (float)maxHp, 7, HpColor(hp, maxHp), 1);
            SDL_snprintf(number, sizeof(number), "%u / %u HP", hp, maxHp);
            PcUi_RoleLabel(font, number, 122, y + 59, 260, y + 81, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CAPTION);
        }
        const char *status = member->inBattle && member->hp ? "In battle" : Status(member);
        float width = PcUi_RoleLabelWidth(font, status, PC_UI_TEXT_CAPTION);
        PcUi_RoleLabel(font, status, 343 - width, y + 59, 345, y + 81, PC_UI_WARNING, PC_UI_TEXT_CAPTION);
    }
}

static void DrawOverviewStats(const PcPartyView *view, PartyPkm *pkm) {
    const PcPartyMember *member = &view->members[view->selected];
    const u32 values[] = { member->hp,       member->speed,   member->attack,
                           member->spAttack, member->defense, member->spDefense };
    static const struct {
        const char *label;
        PkmField field;
    } stats[] = {
        { "HP", PKM_PARAM_HP },           { "Speed", PKM_PARAM_SPEED },
        { "Attack", PKM_PARAM_ATTACK },   { "Sp. Atk", PKM_PARAM_SP_ATTACK },
        { "Defense", PKM_PARAM_DEFENSE }, { "Sp. Def", PKM_PARAM_SP_DEFENSE },
    };
    for (u32 i = 0; i < SDL_arraysize(stats); i++) {
        float left = i % 2 ? 1060 : 864;
        float right = i % 2 ? 1234 : 1034;
        float y = 259 + (i / 2) * 41;
        char text[32];
        u32 value = values[i];
        if (stats[i].field == PKM_PARAM_HP)
            SDL_snprintf(text, sizeof(text), "%u/%u", value, member->maxHp);
        else
            SDL_snprintf(text, sizeof(text), "%u", value);
        PcUi_StatRow(view->font, stats[i].label, text, (PcUiRect){ left, y, right - left, 27 }, true);
    }
    u16 nature[40];
    GFL_MsgDataLoadRawStr(natureNames, PokeParty_GetParam(pkm, PKM_PARAM_NATURE, NULL), nature, SDL_arraysize(nature));
    PcUi_RoleLabel(view->font, "NATURE", 864, 381.6f, 964, 406, PC_UI_INK_MUTED, PC_UI_TEXT_CAPTION);
    PcUi_RoleText(view->font, nature, 970, 378, 1234, 406, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_STAT_VALUE);
}

void PcPartyView_DrawMoves(const PcPartyView *view) {
    if (view->selected >= PARTY_SIZE || !view->members[view->selected].pkm)
        return;
    const PcPartyMember *member = &view->members[view->selected];
    PartyPkm *pkm = member->pkm;
    bool egg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    PcUi_RoleLabel(view->font, "MOVES", 850, 449, 1150, 476, PC_UI_MUTED, PC_UI_TEXT_STAT_LABEL);
    if (egg) {
        PcUi_RoleLabel(view->font, "Stats and moves are available", 850, 485, 1238, 514, PC_UI_WHITE,
                       PC_UI_TEXT_STAT_VALUE);
        PcUi_RoleLabel(view->font, "after this Egg hatches.", 850, 516, 1238, 545, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
        return;
    }
    PcUi_RoleCenteredLabel(view->font, "PP", 1150, 449, 1238, 476, PC_UI_MUTED, PC_UI_TEXT_STAT_LABEL);
    for (u32 slot = 0; slot < 4; slot++) {
        float y = 479 + slot * 34;
        u32 move = member->moves[slot];
        if (!move) {
            PcUi_RoleLabel(view->font, "—", 862, y, 1130, y + 30, 0x697982, PC_UI_TEXT_STAT_VALUE);
            continue;
        }
        u16 name[40];
        char text[24];
        GFL_MsgDataLoadRawStr(moveNames, move, name, SDL_arraysize(name));
        PcUi_RoleText(view->font, name, 862, y, 1130, y + 30, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
        u32 pp = member->pp[slot];
        u32 maximum = member->maxPp[slot];
        SDL_snprintf(text, sizeof(text), "%u/%u", pp, maximum);
        PcUi_RoleCenteredLabel(view->font, text, 1150, y + 1.2f, 1238, y + 30, pp ? PC_UI_CYAN : PC_UI_DANGER,
                               PC_UI_TEXT_LABEL);
    }
    u32 level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    u32 exp = PokeParty_GetParam(pkm, PKM_PARAM_EXP, NULL);
    char text[64];
    SDL_snprintf(text, sizeof(text), "Level %u  /  %u EXP", level, exp);
    PcUi_RoleCenteredLabel(view->font, text, 417, 551, 770, 580, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
    if (level < 100) {
        u32 next = PML_UtilGetPkmLvExp(PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                       PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL), level + 1);
        SDL_snprintf(text, sizeof(text), "Next level: %u EXP", next > exp ? next - exp : 0);
    } else
        SDL_strlcpy(text, "Maximum level", sizeof(text));
    PcUi_RoleCenteredLabel(view->font, text, 417, 583, 770, 612, PC_UI_CYAN, PC_UI_TEXT_LABEL);
}

void PcPartyView_DrawDetails(const PcPartyView *view) {
    if (view->selected >= PARTY_SIZE || !view->members[view->selected].pkm)
        return;
    const PcPartyMember *member = &view->members[view->selected];
    PartyPkm *pkm = member->pkm;
    Font *font = view->font;
    u16 name[80];
    char text[40];
    u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    bool egg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    if (egg) {
        name[0] = 'E';
        name[1] = 'g';
        name[2] = 'g';
        name[3] = 0xffff;
    } else
        PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME_RAW, name);
    for (u32 i = 0; name[i] != 0xffff && name[i]; i++) {
        if (name[i] >= 'a' && name[i] <= 'z')
            name[i] -= 'a' - 'A';
    }
    PcUi_RoleText(font, name, 893, 168, 1248, 209, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_PANEL_TITLE);
    if (!egg) {
        GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, species, name, SDL_arraysize(name));
        PcUi_RoleText(font, name, 893, 217, 1248, 245, PC_UI_INK, PC_UI_TEXT_BODY);
        if (view->overview) {
            DrawOverviewStats(view, pkm);
            return;
        }
        SDL_snprintf(text, sizeof(text), "LEVEL %u", member->level);
        PcUi_RoleLabel(font, text, 893, 257, 1170, 290, PC_UI_INK, PC_UI_TEXT_BODY);
        PcGx.party_sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
        u32 type = member->type1;
        u32 type2 = member->type2;
        PcUi_TypeBadge(type, 0, 6, 897, 297, 132, 40);
        if (type2 != type)
            PcUi_TypeBadge(type2, 1, 6, 1041, 297, 132, 40);
        u32 hp = member->hp;
        u32 maximum = member->maxHp;
        PcUi_Solid(896, 354, 337, 13, 0x27353b, 1);
        if (maximum && hp)
            PcUi_Solid(899, 357, 331 * SDL_min(hp, maximum) / (float)maximum, 7, HpColor(hp, maximum), 1);
        SDL_snprintf(text, sizeof(text), "%u / %u HP", hp, maximum);
        PcUi_RoleLabel(font, text, 893, 377, 1110, 405, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_STAT_VALUE);
        const char *status = Status(member);
        if (!*status)
            status = "Healthy";
        float width = PcUi_RoleLabelWidth(font, status, PC_UI_TEXT_STAT_LABEL);
        PcUi_RoleLabel(font, status, 1234 - width, 379, 1240, 405, PC_UI_INK, PC_UI_TEXT_STAT_LABEL);
    } else {
        PcUi_RoleLabel(font, "Not yet hatched", 893, 257, 1248, 290, PC_UI_INK, PC_UI_TEXT_BODY);
        PcUi_RoleLabel(font, "Check Summary for its condition.", 850, 355, 1248, 400, PC_UI_INK, PC_UI_TEXT_LABEL);
    }
}

void PcPartyView_DrawHeldItem(const PcPartyView *view) {
    if (view->selected >= PARTY_SIZE || !view->members[view->selected].pkm)
        return;
    const PcPartyMember *member = &view->members[view->selected];
    PartyPkm *pkm = member->pkm;
    PcUi_RoleLabel(view->font, "HELD ITEM", 850, 449, 1238, 475, PC_UI_MUTED, PC_UI_TEXT_STAT_LABEL);
    u32 item = member->item;
    u16 name[80];
    if (item) {
        GFL_MsgDataLoadRawStr(itemNames, item, name, SDL_arraysize(name));
        PcUi_RoleText(view->font, name, 850, 480, 1238, 514, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
    } else
        PcUi_RoleLabel(view->font, "None", 850, 480, 1238, 514, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
    if (!PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
        PcUi_RoleLabel(view->font, "ABILITY", 850, 531, 1238, 557, PC_UI_MUTED, PC_UI_TEXT_STAT_LABEL);
        GFL_MsgDataLoadRawStr(abilityNames, member->ability, name, SDL_arraysize(name));
        PcUi_RoleText(view->font, name, 850, 560, 1238, 594, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
    }
}

bool PcPartyView_Draw(const PcPartyView *view) {
    if (!itemNames) {
        itemNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, HEAPID_SYSTEM);
        abilityNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_NAMES, HEAPID_SYSTEM);
        natureNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_NATURES, HEAPID_SYSTEM);
        moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_SYSTEM);
    }
    PcGx.vertex_count = 0;
    PcGx.menu_layer_count = 0;
    PcGx.composite_scene = false;
    PcGx.battle_ui = false;
    PcGx.party_ui = true;
    PcGx.party_menu_row = 0xff;
    PcGx.party_sex = 2;
    PcGx.party_hp_rows = 0;
    PcGx.party_occupied = 0;
    for (u32 i = 0; i < PARTY_SIZE; i++)
        if (view->members[i].pkm)
            PcGx.party_occupied |= 1u << i;
    bool hasSelection = view->selected < PARTY_SIZE && view->members[view->selected].pkm;
    PcGx.party_selected = view->selected;
    PcGx.party_marked = view->marked;
    PcGx.party_overview = view->overview;
    PcGx.party_overview_data = view->overview && hasSelection &&
                               !PokeParty_GetParam(view->members[view->selected].pkm, PKM_PARAM_IS_EGG, NULL);
    if (!PcUi_Begin()) {
        PcGx.failed = true;
        SDL_strlcpy(PcGx.error, "Unable to load the bundled menu font", sizeof(PcGx.error));
        return false;
    }
    animationFrame++;
    PcUiFrame_Header(view->font, "POKÉMON", view->section);
    char text[48];
    SDL_snprintf(text, sizeof(text), "%u / 6 POKÉMON", view->count);
    PcUi_RoleLabel(view->font, text, 779, 32, 1080, 65, PC_UI_MUTED, PC_UI_TEXT_VALUE);
    const PcUiShape tabs[] = { { .rect = { 796, 102, 192, 40 }, .topRight = 40 },
                               { .rect = { 958, 102, 306, 40 }, .topRight = 40, .bottomLeft = 40 } };
    PcUi_TabLabel(view->font, "PARTY", tabs[0], !view->overview);
    PcUi_TabLabel(view->font, "OVERVIEW", tabs[1], view->overview);
    if (view->interactiveTabs) {
        for (u32 i = 0; i < SDL_arraysize(tabs); i++)
            PcUi_Control(tabs[i], (PcUiAction){ PC_UI_ACTION_TAB, (int)i });
    }
    DrawRoster(view);
    if (hasSelection) {
        const PcPartyMember *member = &view->members[view->selected];
        PartyPkm *pkm = member->pkm;
        bool egg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
        PcDexMedia_SetPokemon(0, PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL),
                              egg ? 0 : PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL),
                              egg ? 0 : PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL), egg ? 0 : PokeParty_IsRare(pkm),
                              PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL), egg, view->backView && !egg);
        PcDexMedia_Play(!egg && member->hp && !member->status);
        PcDexMedia_Update();
        PcDexMedia_DrawPokemon(0, 585, 490, 190, 218);
    }
    return true;
}
