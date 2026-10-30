#include "battle_menu.h"
#include "bag_view.h"
#include "battle/b_bag_item.h"
#include "battle/b_bag_main.h"
#include "battle/b_plist_main.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "dex_media.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gx_backend.h"
#include "menu_skin.h"
#include "party_view.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "pml/move.h"
#include "ui_assets.h"
#include "ui_components.h"
#include "ui_draw.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static BBagWork *bagOwner;
static BPlistWork *partyOwner;
static Font *font;
static MsgData *itemNames, *moveNames, *moveDescriptions;
static Uint16 pendingKeys, previousKeys;
static int pendingChoice = -1, pendingPocket = -1, pendingTab = -1;
static u32 heldFrames, shownParty;
static bool backView;
static unsigned inputPhase;
static PcBagViewCache bagCache;
static const char *const pockets[] = { "HP / PP", "STATUS", "POKÉ BALLS", "BATTLE ITEMS" };

bool PcBattleMenu_IsActive(void) {
    return BBagMain_GetNativeWork() || BPlistMain_GetNativeWork();
}

static void Reset(BBagWork *bag, BPlistWork *party) {
    unsigned phase = bag     ? (unsigned)BBagMain_GetNativePhase(bag)
                     : party ? (unsigned)BPlistMain_GetNativePhase(party)
                             : 0;
    if (bagOwner == bag && partyOwner == party) {
        if (inputPhase != phase) {
            pendingKeys = 0;
            pendingChoice = -1;
            inputPhase = phase;
        }
        return;
    }
    inputPhase = phase;
    bagCache = (PcBagViewCache){ 0 };
    if (partyOwner)
        PcDexMedia_Free();
    bagOwner = bag;
    partyOwner = party;
    pendingKeys = previousKeys = 0;
    pendingChoice = pendingPocket = pendingTab = -1;
    heldFrames = shownParty = 0;
    backView = false;
}

static u32 PartyButtons(BPlistNativePhase phase) {
    switch (phase) {
    case BPLIST_NATIVE_PARTY:
    case BPLIST_NATIVE_POSITION:
        return 7;
    case BPLIST_NATIVE_ACTIONS:
    case BPLIST_NATIVE_SUMMARY:
        return 4;
    case BPLIST_NATIVE_MOVES:
        return 8;
    case BPLIST_NATIVE_MOVE_INFO:
    case BPLIST_NATIVE_PP:
        return 5;
    case BPLIST_NATIVE_FORGET:
        return 6;
    case BPLIST_NATIVE_CONFIRM:
        return 2;
    default:
        return 0;
    }
}

Uint16 PcBattleMenu_FilterKeys(Uint16 keys) {
    BBagWork *bag = BBagMain_GetNativeWork();
    BPlistWork *party = BPlistMain_GetNativeWork();
    Reset(bag, party);
    if (!bag && !party)
        return keys;
    keys |= pendingKeys;
    pendingKeys = 0;
    if (keys & PAD_BUTTON_START)
        keys = (keys & ~PAD_BUTTON_START) | PAD_BUTTON_A;
    Uint16 directions = keys & (PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT);
    if (directions != (previousKeys & (PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT)))
        heldFrames = 0;
    bool step = directions && (heldFrames == 0 || (heldFrames >= 20 && heldFrames % 5 == 0));
    u16 pressed = keys & ~previousKeys;
    previousKeys = keys;
    heldFrames++;
    keys &= ~(PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT | PAD_BUTTON_L | PAD_BUTTON_R);
    int direction = directions & (PAD_KEY_UP | PAD_KEY_LEFT) ? -1 : 1;
    if (bag) {
        BBagNativePhase phase = BBagMain_GetNativePhase(bag);
        u32 count = phase == BBAG_NATIVE_POCKETS ? 6
                    : phase == BBAG_NATIVE_ITEMS ? 7
                    : phase == BBAG_NATIVE_USE   ? 2
                                                 : 0;
        if (pressed & (PAD_BUTTON_L | PAD_BUTTON_R))
            pendingPocket = (bag->pocket + ((pressed & PAD_BUTTON_L) ? 3 : 1)) % 4;
        if (pendingPocket >= 0 && count) {
            if (phase == BBAG_NATIVE_POCKETS) {
                pendingChoice = pendingPocket;
                pendingPocket = -1;
            } else
                keys |= PAD_BUTTON_B;
        }
        if (count && !CursorMove_IsCursorVisible(bag->cursorMove))
            CursorMove_SetCursorVisible(bag->cursorMove, TRUE);
        if (step && count) {
            if (phase == BBAG_NATIVE_ITEMS && (directions & (PAD_KEY_LEFT | PAD_KEY_RIGHT))) {
                pendingChoice = direction < 0 ? 7 : 8;
            } else {
                int pos = CursorMove_GetPos(bag->cursorMove);
                do {
                    pos = (pos + count + direction) % count;
                } while ((phase == BBAG_NATIVE_ITEMS && pos < 6 && !BBagItem_GetSlotItem(bag, pos)) ||
                         (phase == BBAG_NATIVE_POCKETS && pos == 4 && !bag->lastItem));
                BBagMain_SetNativeCursor(bag, pos);
            }
        }
        if (pendingChoice >= 0) {
            if (BBagMain_SetNativeCursor(bag, pendingChoice))
                keys |= PAD_BUTTON_A;
            pendingChoice = -1;
        }
        if (phase == BBAG_NATIVE_WAIT)
            keys &= ~(PAD_BUTTON_A | PAD_BUTTON_B);
    } else {
        BPlistNativePhase phase = BPlistMain_GetNativePhase(party);
        u32 count = PartyButtons(phase);
        if (party->param->unk1F != 3 && (pressed & (PAD_BUTTON_L | PAD_BUTTON_R)))
            pendingTab = (pressed & PAD_BUTTON_R) ? 1 : 0;
        if (count && !CursorMove_IsCursorVisible(party->cursorMove))
            CursorMove_SetCursorVisible(party->cursorMove, TRUE);
        if (pendingTab >= 0 && count) {
            if (pendingTab == 0) {
                if (phase == BPLIST_NATIVE_PARTY)
                    pendingTab = -1;
                else
                    keys |= PAD_BUTTON_B;
            } else if (phase == BPLIST_NATIVE_PARTY) {
                pendingChoice = shownParty;
            } else if (phase == BPLIST_NATIVE_ACTIONS) {
                pendingChoice = 1;
                pendingTab = -1;
            } else if (phase == BPLIST_NATIVE_SUMMARY)
                pendingTab = -1;
            else
                keys |= PAD_BUTTON_B;
        }
        if (phase == BPLIST_NATIVE_SUMMARY) {
            if (step)
                pendingChoice = direction < 0 ? 0 : 1;
            else if (pressed & PAD_BUTTON_A)
                pendingChoice = 2;
            step = false;
        }
        if (step && count) {
            int pos = CursorMove_GetPos(party->cursorMove);
            do {
                pos = (pos + count + direction) % count;
            } while ((phase == BPLIST_NATIVE_PARTY || phase == BPLIST_NATIVE_POSITION) && pos < 6 &&
                     !BPlistMain_CheckPos(party, pos));
            BPlistMain_SetNativeCursor(party, pos);
        }
        if (pendingChoice >= 0) {
            if (BPlistMain_SetNativeCursor(party, pendingChoice))
                keys |= PAD_BUTTON_A;
            pendingChoice = -1;
        }
        if (phase == BPLIST_NATIVE_WAIT)
            keys &= ~(PAD_BUTTON_A | PAD_BUTTON_B);
    }
    return keys;
}

bool PcBattleMenu_HandleEvent(PcVideo *video, const SDL_Event *event) {
    BBagWork *bag = BBagMain_GetNativeWork();
    BPlistWork *party = BPlistMain_GetNativeWork();
    Reset(bag, party);
    if (!bag && !party)
        return false;
    Uint16 key = PcUi_EventKey(event);
    if (key & PAD_BUTTON_B)
        pendingPocket = pendingTab = -1;
    pendingKeys |= key;
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (!event->key.repeat) {
            if (event->key.scancode == SDL_SCANCODE_TAB && party && party->param->unk1F != 3)
                pendingTab = party->page == 2 ? 0 : 1;
            if (event->key.scancode == SDL_SCANCODE_V && party)
                backView = !backView;
        }
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_WHEEL)
        return true;
    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN || event->button.button != SDL_BUTTON_LEFT)
        return false;
    PcUiAction action = PcUi_PointerAction(video, event, bag ? (void *)bag : (void *)party, inputPhase);
    if (action.kind == PC_UI_ACTION_KEY) {
        pendingKeys |= action.value;
        if (action.value & PAD_BUTTON_B)
            pendingPocket = pendingTab = -1;
    } else if (action.kind == PC_UI_ACTION_TAB) {
        if (bag)
            pendingPocket = action.value;
        else
            pendingTab = action.value;
    } else if (action.kind == PC_UI_ACTION_MENU)
        pendingChoice = action.value;
    else if (action.kind == PC_UI_ACTION_CUSTOM && party)
        backView = !backView;
    return true;
}

static void Name(MsgData *data, u32 id, float x, float y, float right, u32 color, PcUiTextRole role) {
    u16 text[128];
    GFL_MsgDataLoadRawStr(data, id, text, SDL_arraysize(text));
    PcUi_LineText(font, text, x, y, right, color, role);
}

static void DrawBag(BBagWork *work) {
    BBagNativePhase phase = BBagMain_GetNativePhase(work);
    u32 cursor = CursorMove_GetPos(work->cursorMove);
    font = work->param->font;
    char heading[80];
    PcBagView view = { .font = font,
                       .itemNames = itemNames,
                       .section = "BATTLE",
                       .hint = "Q/W: Pocket   Left/Right: Page   Arrows: Select",
                       .heading = heading,
                       .pockets = pockets,
                       .pocketCount = 4,
                       .selectedPocket = work->page == 0 ? cursor : work->pocket,
                       .tabsEnabled =
                           work->param->mode != 1 && phase != BBAG_NATIVE_WAIT && phase != BBAG_NATIVE_MESSAGE,
                       .rowAction = PC_UI_ACTION_MENU,
                       .pocketList = work->page == 0,
                       .rowCount = 6 };
    if (phase == BBAG_NATIVE_MESSAGE)
        PcUi_Control((PcUiShape){ .rect = { 26, 145, 1228, 500 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
    if (work->page == 0) {
        SDL_strlcpy(heading, "BATTLE POCKETS", sizeof(heading));
        for (u32 i = 0; i < 6; i++)
            view.rows[i] = (PcBagViewRow){ .label = i < 4    ? pockets[i]
                                                    : i == 4 ? "LAST USED ITEM"
                                                             : "BACK TO BATTLE",
                                           .count = i < 4 ? work->itemCount[i] : 0,
                                           .value = i,
                                           .selected = i == cursor,
                                           .enabled = phase == BBAG_NATIVE_POCKETS && (i != 4 || work->lastItem) };
    } else {
        u32 page = work->param->pages[work->pocket];
        SDL_snprintf(heading, sizeof(heading), "PAGE %u / %u", page + 1, work->lastPage[work->pocket] + 1);
        u32 row = work->page == 1 && cursor < 6 ? cursor : (u32)work->param->rows[work->pocket];
        for (u32 i = 0; i < 6; i++) {
            BagItem *item = &work->items[work->pocket][page * 6 + i];
            view.rows[i] = (PcBagViewRow){ .item = item->item,
                                           .count = item->count,
                                           .value = i,
                                           .selected = row == i && item->item,
                                           .enabled = phase == BBAG_NATIVE_ITEMS && item->item };
        }
        view.selectedItem = row < 6 ? BBagItem_GetSlotItem(work, row) : 0;
    }
    PcBagView_Draw(&view, &bagCache);
    if (work->page == 0) {
        PcUi_LineLabel(font, "CHOOSE A POCKET", 730, 186, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
        PcUi_LineLabel(font, "Items available in this battle", 730, 242, 1220, PC_UI_INK, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "LAST USED", 735, 451, 1220, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
        if (work->lastItem) {
            PcUi_ItemIcon(work->lastItem, 6, 734, 500, 64);
            Name(itemNames, work->lastItem, 818, 512, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
        } else
            PcUi_LineLabel(font, "No item used yet.", 735, 500, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
    } else {
        if (phase == BBAG_NATIVE_MESSAGE) {
            PcUi_RoleParagraph(font, GFL_StrBufGetStringPtr(work->strBuf), 735, 451, 1220, 600, PC_UI_DEFAULT_COLOR,
                               PC_UI_TEXT_BODY);
            PcUi_LineLabel(font, "Z: Continue", 735, 605, 1220, PC_UI_CYAN, PC_UI_TEXT_LABEL);
        } else if (work->page == 2) {
            PcUi_LineLabel(font, "USE THIS ITEM?", 735, 451, 1220, PC_UI_CYAN, PC_UI_TEXT_VALUE);
            for (u32 i = 0; i < 2; i++)
                PcUi_ButtonLabel(font, i ? "Back" : "Use", (PcUiRect){ 734, 500 + i * 48, 490, 44 }, cursor == i,
                                 (PcUiAction){ phase == BBAG_NATIVE_USE ? PC_UI_ACTION_MENU : PC_UI_ACTION_NONE, i });
        } else {
            PcUi_LineLabel(font, "CHOOSE AN ITEM", 735, 451, 1220, PC_UI_CYAN, PC_UI_TEXT_SECTION);
            PcUi_LineLabel(font, "Select an item to use in battle.", 735, 500, 1220, PC_UI_DEFAULT_COLOR,
                           PC_UI_TEXT_BODY);
            PcUi_ButtonLabel(font, "Back to pockets", (PcUiRect){ 734, 605, 490, 36 }, cursor == 6,
                             (PcUiAction){ phase == BBAG_NATIVE_ITEMS ? PC_UI_ACTION_MENU : PC_UI_ACTION_NONE, 6 });
        }
        PcUi_ButtonLabel(font, "< Previous", (PcUiRect){ 42, 605, 190, 36 }, false,
                         (PcUiAction){ phase == BBAG_NATIVE_ITEMS ? PC_UI_ACTION_MENU : PC_UI_ACTION_NONE, 7 });
        PcUi_ButtonLabel(font, "Next >", (PcUiRect){ 460, 605, 190, 36 }, false,
                         (PcUiAction){ phase == BBAG_NATIVE_ITEMS ? PC_UI_ACTION_MENU : PC_UI_ACTION_NONE, 8 });
    }
}

static void DrawPartyActions(BPlistWork *work, BPlistNativePhase phase, u32 cursor) {
    static const char *const actions[] = { "Switch in", "Summary", "Moves", "Back" };
    static const char *const summary[] = { "Previous Pokémon", "Next Pokémon", "Moves", "Back" };
    static const char *const moves[] = { "Previous Pokémon", "Next Pokémon", "Summary", "Back" };
    const char *const *labels = phase == BPLIST_NATIVE_ACTIONS   ? actions
                                : phase == BPLIST_NATIVE_SUMMARY ? summary
                                                                 : moves;
    if (phase == BPLIST_NATIVE_MESSAGE) {
        PcUi_RoleParagraph(font, GFL_StrBufGetStringPtr(work->strBuf), 850, 449, 1238, 608, PC_UI_DEFAULT_COLOR,
                           PC_UI_TEXT_BODY);
    } else if (phase == BPLIST_NATIVE_ACTIONS || phase == BPLIST_NATIVE_SUMMARY || phase == BPLIST_NATIVE_MOVES) {
        u32 start = phase == BPLIST_NATIVE_MOVES ? 4 : 0;
        PcGx.party_menu_row = cursor >= start && cursor < start + 4 ? cursor - start : 0xff;
        for (u32 i = 0; i < 4; i++) {
            PcUi_Control((PcUiShape){ .rect = { 850, 447 + i * 34, 398, 34 } },
                         (PcUiAction){ PC_UI_ACTION_MENU, start + i });
            PcUi_LineLabel(font, labels[i], 872, 450 + i * 34, 1230, cursor == start + i ? PC_UI_CYAN : PC_UI_WHITE,
                           PC_UI_TEXT_BODY);
        }
    } else if (phase == BPLIST_NATIVE_CONFIRM) {
        PcGx.party_menu_row = cursor < 2 ? cursor : 0xff;
        for (u32 i = 0; i < 2; i++)
            PcUi_Control((PcUiShape){ .rect = { 850, 447 + i * 34, 398, 34 } }, (PcUiAction){ PC_UI_ACTION_MENU, i });
        PcUi_LineLabel(font, "Learn this move", 872, 450, 1230, cursor == 0 ? PC_UI_CYAN : PC_UI_WHITE,
                       PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "Cancel", 872, 484, 1230, cursor == 1 ? PC_UI_CYAN : PC_UI_WHITE, PC_UI_TEXT_BODY);
    } else if (work->page >= 3 && work->page <= 7) {
        u32 slot = work->page == 4 || work->page == 7 ? work->param->slot : cursor;
        BPlistPokemon *mon = &work->pokemon[BPlistMain_GetPartySlot(work, shownParty)];
        u32 move = slot < 4 ? mon->moves[slot].move : slot == 4 && work->page == 6 ? work->param->move : 0;
        if (move) {
            char value[96];
            SDL_snprintf(value, sizeof(value), "Power %u     Accuracy %u", PML_MoveGetParam(move, MOVE_PARAM_POWER),
                         PML_MoveGetParam(move, MOVE_PARAM_ACCURACY));
            PcUi_LineLabel(font, value, 850, 449, 1238, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
            u16 description[256];
            GFL_MsgDataLoadRawStr(moveDescriptions, move, description, SDL_arraysize(description));
            PcUi_RoleParagraph(font, description, 850, 483, 1238, 581, PC_UI_WHITE, PC_UI_TEXT_LABEL);
        }
        PcUi_Control((PcUiShape){ .rect = { 850, 589, 398, 28 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_B });
        PcUi_LineLabel(font, "X: Back", 850, 589, 1238, PC_UI_CYAN, PC_UI_TEXT_LABEL);
    } else {
        PcUi_LineLabel(font, work->param->unk1F == 3 ? "CHOOSE A TARGET" : "CHOOSE A POKÉMON", 850, 449, 1238,
                       PC_UI_CYAN, PC_UI_TEXT_BODY);
        if (work->param->unk1F == 3) {
            Name(itemNames, work->param->item, 850, 493, 1238, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
            PcUi_LineLabel(font, "Select the Pokémon to use it on.", 850, 541, 1238, PC_UI_MUTED,
                           PC_UI_TEXT_STAT_LABEL);
        } else
            PcUi_LineLabel(font, "Z: Switch, summary or moves", 850, 500, 1238, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
    }
}

static void DrawParty(BPlistWork *work) {
    font = work->param->font;
    BPlistNativePhase phase = BPlistMain_GetNativePhase(work);
    u32 cursor = CursorMove_GetPos(work->cursorMove);
    u32 selected = work->page == 0 || work->page == 8 ? cursor : work->param->partyIndex;
    if (selected < 6 && BPlistMain_CheckPos(work, selected))
        shownParty = selected;
    PcPartyView view = { .font = font,
                         .selected = shownParty,
                         .marked = 0xff,
                         .overview = work->page == 2,
                         .backView = backView,
                         .section = "BATTLE",
                         .interactiveRows = phase == BPLIST_NATIVE_PARTY || phase == BPLIST_NATIVE_POSITION,
                         .interactiveTabs =
                             work->param->unk1F != 3 && phase != BPLIST_NATIVE_WAIT && phase != BPLIST_NATIVE_MESSAGE,
                         .rowAction = PC_UI_ACTION_MENU };
    for (u32 i = 0; i < 6; i++) {
        BPlistPokemon *mon = &work->pokemon[BPlistMain_GetPartySlot(work, i)];
        if (!mon->pkm)
            continue;
        view.count++;
        PcPartyMember *member = &view.members[i];
        PcPartyView_ReadMember(member, mon->pkm);
        member->hp = mon->hp;
        member->maxHp = mon->maxHp;
        member->level = mon->level;
        member->status = mon->status;
        member->type1 = mon->type1;
        member->type2 = mon->type2;
        member->ability = mon->ability;
        member->item = mon->item;
        member->attack = mon->attack;
        member->defense = mon->defense;
        member->speed = mon->speed;
        member->spAttack = mon->spAttack;
        member->spDefense = mon->spDefense;
        member->inBattle = BPlistMain_CheckPos(work, i) == 1;
        for (u32 j = 0; j < 4; j++) {
            member->moves[j] = mon->moves[j].move;
            member->pp[j] = mon->moves[j].pp;
            member->maxPp[j] = mon->moves[j].maxPp;
        }
    }
    if (shownParty >= 6 || !view.members[shownParty].pkm) {
        for (u32 i = 0; i < 6; i++)
            if (view.members[i].pkm) {
                shownParty = i;
                break;
            }
        view.selected = shownParty;
    }
    if (!PcPartyView_Draw(&view) || !view.count)
        return;
    BPlistPokemon *mon = &work->pokemon[BPlistMain_GetPartySlot(work, shownParty)];
    if (work->page >= 3 && work->page <= 7) {
        char text[96];
        PcUi_LineLabel(font,
                       work->page == 5   ? "RESTORE PP"
                       : work->page >= 6 ? "LEARN A MOVE"
                                         : "MOVES",
                       893, 166, 1238, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
        u32 rows = work->page == 6 ? 5 : 4;
        for (u32 i = 0; i < rows; i++) {
            u32 move = i == 4 ? work->param->move : mon->moves[i].move;
            float y = 260 + i * (150 / rows);
            if (phase == BPLIST_NATIVE_MOVES || phase == BPLIST_NATIVE_MOVE_INFO || phase == BPLIST_NATIVE_PP ||
                phase == BPLIST_NATIVE_FORGET)
                PcUi_Control((PcUiShape){ .rect = { 850, y, 398, 150 / rows } }, (PcUiAction){ PC_UI_ACTION_MENU, i });
            PcUi_LineLabel(font, cursor == i ? ">" : "", 850, y + 3, 875, PC_UI_LINK, PC_UI_TEXT_BODY);
            if (move) {
                Name(moveNames, move, 877, y, 1130, PC_UI_INK, PC_UI_TEXT_LABEL);
                PcUi_TypeBadge(PML_MoveGetType(move), i, 6, 1142, y + 2, 94, 24);
                SDL_snprintf(text, sizeof(text), i == 4 ? "New move" : "PP %u / %u", i == 4 ? 0 : mon->moves[i].pp,
                             i == 4 ? 0 : mon->moves[i].maxPp);
                PcUi_LineLabel(font, text, 877, y + 17, 1128, PC_UI_INK_MUTED, PC_UI_TEXT_CAPTION);
            } else
                PcUi_LineLabel(font, "—", 877, y, 1130, PC_UI_INK_MUTED, PC_UI_TEXT_LABEL);
        }

    } else {
        PcPartyView_DrawDetails(&view);
    }
    if (phase == BPLIST_NATIVE_SUMMARY) {
        PcPartyView_DrawMoves(&view);
        PcUi_Control((PcUiShape){ .rect = { 850, 447, 398, 170 } }, (PcUiAction){ PC_UI_ACTION_MENU, 2 });
    } else if (phase == BPLIST_NATIVE_PARTY && work->param->unk1F != 3) {
        PcPartyView_DrawHeldItem(&view);
        PcUi_Control((PcUiShape){ .rect = { 850, 447, 398, 170 } }, (PcUiAction){ PC_UI_ACTION_MENU, shownParty });
        PcUi_LineLabel(font, "Z: Switch, summary or moves", 850, 603, 1238, PC_UI_CYAN, PC_UI_TEXT_STAT_LABEL);
    } else {
        DrawPartyActions(work, phase, cursor);
    }
    PcUiFrame_Footer(font,
                     phase == BPLIST_NATIVE_SUMMARY ? "Left/Right: Pokémon   Z: Moves   X: Back"
                     : work->param->unk1F == 3      ? "Arrows: Target   V: Front / Back"
                                                    : "Tab: Page   V: Front / Back",
                     "Select", true, true);
}

bool PcBattleMenu_Draw(void) {
    BBagWork *bag = BBagMain_GetNativeWork();
    BPlistWork *party = BPlistMain_GetNativeWork();
    Reset(bag, party);
    if (!bag && !party)
        return false;
    if (!itemNames) {
        itemNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, HEAPID_SYSTEM);
        moveNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_SYSTEM);
        moveDescriptions = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_13, HEAPID_SYSTEM);
    }
    PcGx.vertex_count = 0;
    PcGx.menu_layer_count = 0;
    PcGx.composite_scene = false;
    PcGx.battle_ui = false;
    PcUi_BeginControls(bag ? (void *)bag : (void *)party, inputPhase);
    if (party && BPlistMain_GetNativePhase(party) == BPLIST_NATIVE_MESSAGE)
        PcUi_Control((PcUiShape){ .rect = { 26, 145, 1228, 500 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
    if (bag) {
        if (!PcUi_Begin())
            return false;
        DrawBag(bag);
    } else
        DrawParty(party);
    return true;
}
