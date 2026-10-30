#include "app/p_status.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "dex_media.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "menu_apps.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "pml/move.h"
#include "ui_assets.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static Font *font;
static PcUiInput input;
static MsgData *itemNames, *moveNames, *abilityNames, *abilityDescriptions, *moveDescriptions;
static u32 ribbonCursor;
static bool backView, ribbonFocus;
static u32 Param(PartyPkm *pkm, u32 param) {
    return PokeParty_GetParam(pkm, param, NULL);
}

static void DrawSummaryPortrait(PStatusNativeState *state) {
    PartyPkm *pkm = state->pkm;
    bool egg = Param(pkm, PKM_PARAM_IS_EGG);
    u16 text[64];
    PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME_RAW, text);
    PcUi_RoleCenteredText(font, text, 44, 146, 379, 192, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_CARD_TITLE);
    char label[96];
    SDL_snprintf(label, sizeof(label), "Lv. %u    %u / %u", Param(pkm, PKM_PARAM_LEVEL), state->partyIndex + 1,
                 state->param->partyCount);
    if (!egg)
        PcUi_RoleCenteredLabel(font, label, 44, 187, 379, 218, PC_UI_MUTED, PC_UI_TEXT_BODY);
    u32 species = Param(pkm, PKM_PARAM_SPECIES);
    PcDexMedia_SetPokemon(0, species, egg ? 0 : Param(pkm, PKM_PARAM_FORM), egg ? 0 : Param(pkm, PKM_PARAM_SEX),
                          egg ? 0 : PokeParty_IsRare(pkm), Param(pkm, PKM_PARAM_PID), egg, !egg && backView);
    PcDexMedia_Play(!egg);
    PcDexMedia_Update();
    PcDexMedia_DrawPokemon(0, 211, 417, 170, 190);
    if (!egg)
        PcUi_Control((PcUiShape){ .rect = { 41, 227, 340, 190 } }, (PcUiAction){ PC_UI_ACTION_CUSTOM, 0 });
    if (!egg) {
        u32 type1 = Param(pkm, PKM_PARAM_TYPE1), type2 = Param(pkm, PKM_PARAM_TYPE2);
        PcUi_CenteredTypeBadge(type1, 0, 6, type1 == type2 ? 150 : 79, 453, 122, 34);
        if (type1 != type2)
            PcUi_CenteredTypeBadge(type2, 1, 6, 221, 453, 122, 34);
        SDL_snprintf(label, sizeof(label), "HP  %u / %u", Param(pkm, PKM_PARAM_HP), Param(pkm, PKM_PARAM_MAX_HP));
        PcUi_RoleCenteredLabel(font, label, 44, 500, 379, 530, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        PcUi_Solid(72, 535, 278, 7, 0x30434e, 1);
        u32 maxHp = Param(pkm, PKM_PARAM_MAX_HP);
        if (maxHp)
            PcUi_Solid(72, 535, 278.0f * Param(pkm, PKM_PARAM_HP) / maxHp, 7, PC_UI_CYAN, 1);
        static const char *const marks[] = { "●", "▲", "■", "♥", "★", "◆" };
        u32 markings = Param(pkm, PKM_PARAM_MARKINGS);
        for (u32 i = 0; i < 6; i++) {
            float x = 91 + i * 40;
            PcUi_SkinControl(PC_MENU_MARKING, (PcUiRect){ x, 567, 32, 32 }, (markings >> i) & 1,
                             (PcUiAction){ state->ready && state->param->mode != PSTATUS_MODE_LOCK_MARKINGS
                                               ? PC_UI_ACTION_MARK
                                               : PC_UI_ACTION_NONE,
                                           i });
            PcUi_RoleCenteredLabel(font, marks[i], x, 571, x + 32, 601,
                                   (markings & (1u << i)) ? PC_UI_INK : PC_UI_MUTED, PC_UI_TEXT_BODY);
        }
        PcUi_RoleCenteredLabel(font, "Click a marking to toggle", 44, 609, 379, 638, PC_UI_MUTED,
                               PC_UI_TEXT_STAT_LABEL);
    }
}

static void DrawInfo(PStatusNativeState *state) {
    PartyPkm *pkm = state->pkm;
    bool egg = Param(pkm, PKM_PARAM_IS_EGG);
    u16 text[64];
    PcUi_LineLabel(font, egg ? "EGG" : "POKÉMON INFO", 450, 171, 1200, PC_UI_INK, PC_UI_TEXT_CARD_TITLE);
    if (!egg) {
        GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, Param(pkm, PKM_PARAM_SPECIES), text, SDL_arraysize(text));
        PcUi_LineText(font, text, 450, 207, 800, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
        char value[96];
        PcUi_LineLabel(font, "Original Trainer", 450, 261, 735, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
        PokeParty_GetParam(pkm, PKM_PARAM_OT_NAME_RAW, text);
        PcUi_LineText(font, text, 720, 261, 1000, PC_UI_INK, PC_UI_TEXT_VALUE);
        SDL_snprintf(value, sizeof(value), "ID No. %05u", Param(pkm, PKM_PARAM_ID) & 0xffff);
        PcUi_LineLabel(font, value, 1000, 261, 1230, PC_UI_INK, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "Held item", 450, 303, 710, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
        u32 item = Param(pkm, PKM_PARAM_ITEM);
        if (item) {
            PcUi_ItemIcon(item, 0, 710, 296, 40);
            PcMenu_Name(font, itemNames, item, 759, 303, 1220, PC_UI_INK, PC_UI_TEXT_VALUE);
        } else
            PcUi_LineLabel(font, "None", 720, 303, 1220, PC_UI_INK, PC_UI_TEXT_VALUE);
        PcUi_LineLabel(font, "Experience", 450, 349, 700, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
        u32 level = Param(pkm, PKM_PARAM_LEVEL), exp = Param(pkm, PKM_PARAM_EXP);
        SDL_snprintf(value, sizeof(value), "%u EXP", exp);
        PcUi_LineLabel(font, value, 720, 349, 970, PC_UI_INK, PC_UI_TEXT_VALUE);
        if (level < 100) {
            u32 next = PML_UtilGetPkmLvExp(Param(pkm, PKM_PARAM_SPECIES), Param(pkm, PKM_PARAM_FORM), level + 1);
            SDL_snprintf(value, sizeof(value), "%u to Lv. %u", next > exp ? next - exp : 0, level + 1);
        } else
            SDL_strlcpy(value, "Maximum level", sizeof(value));
        PcUi_LineLabel(font, value, 990, 349, 1230, PC_UI_INK, PC_UI_TEXT_BODY);
    } else
        PcUi_LineLabel(font, "A mysterious Pokémon Egg.", 450, 261, 1220, PC_UI_INK, PC_UI_TEXT_VALUE);
    PcUi_LineLabel(font, "TRAINER MEMO", 463, 449, 1220, PC_UI_CYAN, PC_UI_TEXT_BODY);
    PcUi_LineText(font, state->memo[0], 463, 483, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    PcUi_RoleParagraph(font, state->memo[1], 463, 514, 1220, 592, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    PcUi_RoleParagraph(font, state->memo[2], 463, 597, 1220, 635, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
}

static void DrawMoveDetail(u32 move, float y) {
    u32 power = PML_MoveGetParam(move, MOVE_PARAM_POWER);
    u32 accuracy = PML_MoveGetParam(move, MOVE_PARAM_ACCURACY);
    static const char *const categories[] = { "Status", "Physical", "Special" };
    s32 category = PML_MoveGetCategory(move);
    char powerText[16], accuracyText[16];
    if (power > 1)
        SDL_snprintf(powerText, sizeof(powerText), "%u", power);
    else
        SDL_strlcpy(powerText, "—", sizeof(powerText));
    if (accuracy && !PML_MoveIsAlwaysHit(move))
        SDL_snprintf(accuracyText, sizeof(accuracyText), "%u", accuracy);
    else
        SDL_strlcpy(accuracyText, "—", sizeof(accuracyText));
    PcUi_LineLabel(font, category >= 0 && category < 3 ? categories[category] : "", 463, y, 690, PC_UI_CYAN,
                   PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, "Power", 717, y, 840, PC_UI_MUTED, PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, powerText, 850, y, 950, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, "Accuracy", 971, y, 1115, PC_UI_MUTED, PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, accuracyText, 1125, y, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    PcMenu_Paragraph(font, moveDescriptions, move, 463, y + 35, 1220, y + 111, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
}

static void DrawMoves(PStatusNativeState *state) {
    bool forget = state->page == PSTATUS_PAGE_FORGET;
    PcUi_LineLabel(font, forget ? "CHOOSE A MOVE TO FORGET" : "MOVES", 450, 170, 1220, PC_UI_DEFAULT_COLOR,
                   PC_UI_TEXT_SECTION);
    for (u32 i = 0; i < (forget && state->param->move ? 5u : 4u); i++) {
        u32 move = i == 4 ? state->param->move : Param(state->pkm, PKM_PARAM_MOVE1 + i);
        float y = 213 + i * (forget ? 37 : 47);
        bool selected = !state->browsing &&
                        (state->moveCursor == i || ((state->confirming || state->swapping) && state->swapSource == i));
        PcUi_SkinControl(PC_MENU_LIGHT_ROW, (PcUiRect){ 446, y, 772, forget ? 37 : 46 }, selected,
                         (PcUiAction){ move && state->ready ? PC_UI_ACTION_ROW : PC_UI_ACTION_NONE, i });
        if (!move) {
            PcUi_LineLabel(font, "—", 466, y + 6, 800, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
            continue;
        }
        u16 moveName[64];
        GFL_MsgDataLoadRawStr(moveNames, move, moveName, SDL_arraysize(moveName));
        PcUi_RoleText(font, moveName, 466, y + 6, 880, y + 40, PC_UI_INK, PC_UI_TEXT_BODY);
        PcUi_TypeBadge(PML_MoveGetType(move), 2 + i, 6, 903, y + 7, 97, 28);
        char text[40];
        if (i == 4)
            SDL_strlcpy(text, "NEW", sizeof(text));
        else
            SDL_snprintf(text, sizeof(text), "%u / %u PP", Param(state->pkm, PKM_PARAM_MOVE1_PP + i),
                         Param(state->pkm, PKM_PARAM_MOVE1_MAX_PP + i));
        PcUi_LineLabel(font, text, 1040, y + 8, 1200, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_STAT_VALUE);
    }
    if (!state->browsing && state->moveCursor < 5) {
        u32 move = state->moveCursor == 4 ? state->param->move : Param(state->pkm, PKM_PARAM_MOVE1 + state->moveCursor);
        if (move)
            DrawMoveDetail(move, 449);
        if (state->moveMessage[0] && state->moveMessage[0] != 0xffff)
            PcUi_RoleParagraph(font, state->moveMessage, 463, 557, 1220, 598, PC_UI_CYAN, PC_UI_TEXT_LABEL);
        else if (state->confirming && state->swapSource < 4) {
            PcUi_LineLabel(font, "REPLACING", 463, 563, 641, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
            PcMenu_Name(font, moveNames, Param(state->pkm, PKM_PARAM_MOVE1 + state->swapSource), 663, 563, 1220,
                        PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
        }
        PcUi_LineLabel(font,
                       state->confirming ? "Z: Forget selected move and learn the new move   X: Cancel"
                       : state->swapping ? "Choose a destination, then Z to swap. X cancels."
                       : forget          ? "Z: Choose move   X: Cancel learning"
                                         : "Z: Pick up move to reorder   X: Return to stats",
                       463, 600, 1230, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
    } else {
        static const u32 params[] = { PKM_PARAM_MAX_HP,    PKM_PARAM_ATTACK,     PKM_PARAM_DEFENSE,
                                      PKM_PARAM_SP_ATTACK, PKM_PARAM_SP_DEFENSE, PKM_PARAM_SPEED };
        static const char *const labels[] = { "HP", "Attack", "Defense", "Sp. Atk", "Sp. Def", "Speed" };
        char text[48];
        for (u32 i = 0; i < 6; i++) {
            float x = 463 + (i % 3) * 254, y = 449 + (i / 3) * 32;
            SDL_snprintf(text, sizeof(text), "%u", Param(state->pkm, params[i]));
            PcUi_StatRow(font, labels[i], text, (PcUiRect){ x, y, 240, 30 }, false);
        }
        u32 ability = Param(state->pkm, PKM_PARAM_ABILITY);
        PcMenu_Name(font, abilityNames, ability, 463, 525, 1220, PC_UI_CYAN, PC_UI_TEXT_BODY);
        PcMenu_Paragraph(font, abilityDescriptions, ability, 463, 559, 1220, 625, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
    }
}

static void DrawRibbons(void) {
    u16 name[100], description[256];
    u32 icon, palette;
    u32 count = PStatus_GetNativeRibbon(0, name, 100, description, 256, &icon, &palette);
    if (ribbonCursor >= count)
        ribbonCursor = 0;
    PcUi_LineLabel(font, "RIBBONS", 450, 170, 1220, PC_UI_INK, PC_UI_TEXT_CARD_TITLE);
    if (!count) {
        PcUi_LineLabel(font, "No Ribbons yet.", 450, 271, 1220, PC_UI_INK, PC_UI_TEXT_VALUE);
        PcUi_LineLabel(font, "Ribbons earned by this Pokémon appear here.", 463, 483, 1220, PC_UI_DEFAULT_COLOR,
                       PC_UI_TEXT_BODY);
        return;
    }
    u32 start = ribbonCursor > 3 ? ribbonCursor - 3 : 0;
    for (u32 row = 0; row < 4 && start + row < count; row++) {
        u32 i = start + row;
        PStatus_GetNativeRibbon(i, name, 100, description, 256, &icon, &palette);
        PcUi_SkinControl(PC_MENU_LIGHT_ROW, (PcUiRect){ 446, 213 + row * 47, 772, 46 }, i == ribbonCursor,
                         (PcUiAction){ PC_UI_ACTION_ROW, i });
        PcUi_RibbonIcon(icon, palette, row, 459, 216 + row * 47, 40);
        PcUi_RoleText(font, name, 515, 220 + row * 47, 1200, 260 + row * 47, PC_UI_INK, PC_UI_TEXT_BODY);
    }
    PStatus_GetNativeRibbon(ribbonCursor, name, 100, description, 256, &icon, &palette);
    PcUi_LineText(font, name, 463, 449, 1220, PC_UI_CYAN, PC_UI_TEXT_BODY);
    PcUi_RoleParagraph(font, description, 463, 489, 1220, 591, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    PcUi_LineLabel(font, "Click a Ribbon or use Z, then Up/Down to browse.", 463, 601, 1220, PC_UI_DEFAULT_COLOR,
                   PC_UI_TEXT_LABEL);
}

static void DrawSummary(PStatusNativeState *state) {
    font = state->font;
    PcUiFrame_Draw(font, PC_MENU_SUMMARY, "POKÉMON", "SUMMARY", "Q/W: Page   Up/Down: Pokémon   V: Front/back");
    PcUi_Tab(font, "INFO", (PcUiRect){ 416, 101, 203, 43 }, state->page == PSTATUS_PAGE_INFO,
             (PcUiAction){ PC_UI_ACTION_TAB, 0 });
    PcUi_Tab(font, "MOVES", (PcUiRect){ 631, 101, 203, 43 },
             state->page == PSTATUS_PAGE_SKILL || state->page == PSTATUS_PAGE_FORGET,
             (PcUiAction){ PC_UI_ACTION_TAB, 1 });
    PcUi_Tab(font, "RIBBONS", (PcUiRect){ 846, 101, 203, 43 }, state->page == PSTATUS_PAGE_RIBBON,
             (PcUiAction){ PC_UI_ACTION_TAB, 2 });
    DrawSummaryPortrait(state);
    if (state->page == PSTATUS_PAGE_INFO)
        DrawInfo(state);
    else if (state->page == PSTATUS_PAGE_RIBBON)
        DrawRibbons();
    else
        DrawMoves(state);
}

static void Sync(const PStatusNativeState *state) {
    unsigned phase = state->page | (state->browsing << 8) | (state->confirming << 9) | (state->swapping << 10) |
                     (state->ready << 11);
    PcUiInput_Sync(&input, PStatus_GetNativeWork(), phase);
}
void PcMenuSummary_Reset(void) {
    MsgData **banks[] = { &itemNames, &moveNames, &abilityNames, &abilityDescriptions, &moveDescriptions };
    for (u32 i = 0; i < SDL_arraysize(banks); i++) {
        if (*banks[i])
            GFL_MsgDataFree(*banks[i]);
        *banks[i] = NULL;
    }
    input = (PcUiInput){ 0 };
    ribbonCursor = 0;
    ribbonFocus = backView = false;
}
bool PcMenuSummary_Draw(void) {
    PStatusNativeState state;
    if (!PStatus_GetNativeState(&state))
        return false;
    Sync(&state);
    if (!itemNames) {
        itemNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, HEAPID_USER);
        moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_USER);
        abilityNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_NAMES, HEAPID_USER);
        abilityDescriptions =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_DESCRIPTIONS, HEAPID_USER);
        moveDescriptions = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_13, HEAPID_USER);
    }
    PcUi_BeginControls(input.owner, input.phase);
    DrawSummary(&state);
    return true;
}
bool PcMenuSummary_HandleEvent(PcVideo *video, const SDL_Event *event) {
    PStatusNativeState state;
    if (!PStatus_GetNativeState(&state))
        return false;
    Sync(&state);
    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat && event->key.scancode == SDL_SCANCODE_V)
        backView = !backView;
    return PcUiInput_Event(&input, video, event);
}
Uint16 PcMenuSummary_FilterKeys(Uint16 keys) {
    PStatusNativeState state;
    if (!PStatus_GetNativeState(&state))
        return keys;
    Sync(&state);
    Uint16 pressed;
    PcUiAction action;
    keys = PcUiInput_Read(&input, keys, &pressed, &action);
    int tab = action.kind == PC_UI_ACTION_TAB ? action.value : -1;
    if (pressed & PAD_BUTTON_L)
        tab = state.page ? state.page - 1 : 2;
    if (pressed & PAD_BUTTON_R)
        tab = state.page < 2 ? state.page + 1 : 0;
    keys &= ~(PAD_BUTTON_L | PAD_BUTTON_R);
    if (tab >= 0 && PStatus_SetNativePage(tab))
        ribbonFocus = false;
    if (action.kind == PC_UI_ACTION_ROW) {
        if (state.page == PSTATUS_PAGE_RIBBON) {
            ribbonCursor = action.value;
            ribbonFocus = true;
        } else
            PStatus_SetNativeMove(action.value);
    }
    if (action.kind == PC_UI_ACTION_CUSTOM)
        backView = !backView;
    if (action.kind == PC_UI_ACTION_MARK && action.value >= 0 && action.value < 6 && state.ready &&
        state.param->mode != PSTATUS_MODE_LOCK_MARKINGS && !Param(state.pkm, PKM_PARAM_IS_EGG))
        PokeParty_SetParam(state.pkm, PKM_PARAM_MARKINGS, Param(state.pkm, PKM_PARAM_MARKINGS) ^ (1u << action.value));
    if (state.page == PSTATUS_PAGE_RIBBON) {
        if (pressed & PAD_BUTTON_A)
            ribbonFocus = true;
        if (ribbonFocus) {
            u16 name[100], desc[256];
            u32 icon, palette;
            u32 count = PStatus_GetNativeRibbon(0, name, 100, desc, 256, &icon, &palette);
            if (count && (pressed & PAD_KEY_DOWN))
                ribbonCursor = (ribbonCursor + 1) % count;
            if (count && (pressed & PAD_KEY_UP))
                ribbonCursor = (ribbonCursor + count - 1) % count;
            keys &= ~(PAD_KEY_UP | PAD_KEY_DOWN);
            if (pressed & PAD_BUTTON_B) {
                ribbonFocus = false;
                keys &= ~PAD_BUTTON_B;
            }
        }
        keys &= ~PAD_BUTTON_A;
    }
    return keys;
}
