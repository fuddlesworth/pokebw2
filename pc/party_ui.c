#include "party_ui.h"
#include "app/pokelist.h"
#include "dex_media.h"
#include "gfl/key.h"
#include "gx_backend.h"
#include "party_view.h"
#include "pml/poke_party.h"
#include "ui_components.h"
#include "ui_draw.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static PokeListWork *owner;
static Uint16 pendingKeys, previousDirections, previousTabKeys;
static u32 heldFrames, menuStart;
static s32 pendingSlot = -1, pendingMenu = -1;
static u32 shownSlot;
static bool backView;
static PcPartyTab activeTab;
static s32 pendingTab = -1;
static bool cycleTab;
static PokeListWork *CurrentWork(void) {
    PokeListWork *wk = PokeList_GetNativeWork();
    return wk && wk->font && wk->param && !PokeList_IsBattle(wk) ? wk : NULL;
}

bool PcPartyUi_IsActive(void) {
    return CurrentWork() != NULL;
}

Uint16 PcPartyUi_FilterKeys(Uint16 keys) {
    PokeListWork *wk = CurrentWork();
    if (!wk) {
        pendingKeys = previousDirections = 0;
        pendingSlot = pendingMenu = -1;
        heldFrames = 0;
        previousTabKeys = 0;
        pendingTab = -1;
        cycleTab = false;
        return keys;
    }
    keys |= pendingKeys;
    pendingKeys = 0;
    if (keys & PAD_BUTTON_START)
        keys = (keys & ~PAD_BUTTON_START) | PAD_BUTTON_A;
    PokeListNativePhase phase = PokeList_GetNativePhase(wk);
    Uint16 tabKeys = keys & (PAD_BUTTON_L | PAD_BUTTON_R);
    if (phase == POKELIST_NATIVE_BROWSE) {
        if ((tabKeys & ~previousTabKeys) & PAD_BUTTON_L)
            activeTab = PC_PARTY_PARTY;
        if ((tabKeys & ~previousTabKeys) & PAD_BUTTON_R)
            activeTab = PC_PARTY_OVERVIEW;
        if (cycleTab)
            activeTab = activeTab == PC_PARTY_PARTY ? PC_PARTY_OVERVIEW : PC_PARTY_PARTY;
        if (pendingTab >= 0)
            activeTab = (PcPartyTab)pendingTab;
    }
    previousTabKeys = tabKeys;
    pendingTab = -1;
    cycleTab = false;
    keys &= ~(PAD_BUTTON_L | PAD_BUTTON_R);
    if (pendingSlot >= 0) {
        if (PokeList_SetNativeCursor(wk, pendingSlot) && activeTab == PC_PARTY_PARTY)
            keys |= PAD_BUTTON_A;
        pendingSlot = -1;
    }
    if (pendingMenu >= 0) {
        if (PokeListMenu_SetNativeCursor(wk->menu, pendingMenu))
            keys |= PAD_BUTTON_A;
        pendingMenu = -1;
    }
    if (phase == POKELIST_NATIVE_BROWSE && (keys & PAD_BUTTON_A))
        activeTab = PC_PARTY_PARTY;
    if (wk->touch && (phase == POKELIST_NATIVE_BROWSE || phase == POKELIST_NATIVE_TARGET))
        PokeList_SetNativeCursor(wk, wk->cursorPos);
    Uint16 directions = keys & (PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT);
    if (directions != previousDirections)
        heldFrames = 0;
    if (directions && (heldFrames == 0 || (heldFrames >= 20 && heldFrames % 4 == 0)) &&
        (phase == POKELIST_NATIVE_BROWSE || phase == POKELIST_NATIVE_TARGET)) {
        u32 count = PokeParty_GetPkmCount(wk->param->party);
        if (count) {
            int direction = directions & (PAD_KEY_UP | PAD_KEY_LEFT) ? -1 : 1;
            PokeList_SetNativeCursor(wk, ((int)wk->cursorPos + count + direction) % count);
        }
    }
    previousDirections = directions;
    heldFrames++;
    if (phase == POKELIST_NATIVE_BROWSE || phase == POKELIST_NATIVE_TARGET)
        keys &= ~(PAD_KEY_UP | PAD_KEY_DOWN | PAD_KEY_LEFT | PAD_KEY_RIGHT);
    return keys;
}

bool PcPartyUi_HandleEvent(PcVideo *video, const SDL_Event *event) {
    PokeListWork *wk = CurrentWork();
    if (!wk)
        return false;
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_MOUSE_WHEEL) {
        pendingKeys |= PcUi_EventKey(event);
        if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
            if (event->key.scancode == SDL_SCANCODE_V)
                backView = !backView;
            if (event->key.scancode == SDL_SCANCODE_TAB)
                cycleTab = true;
        }
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        PcUiAction action = PcUi_PointerAction(video, event, wk, PokeList_GetNativePhase(wk));
        switch (action.kind) {
        case PC_UI_ACTION_KEY:
            pendingKeys |= action.value;
            break;
        case PC_UI_ACTION_TAB:
            pendingTab = action.value;
            break;
        case PC_UI_ACTION_ROW:
            pendingSlot = action.value;
            break;
        case PC_UI_ACTION_MENU:
            pendingMenu = action.value;
            break;
        default:
            break;
        }
        return true;
    }
    return false;
}

static void DrawActions(PokeListWork *wk, const PcPartyView *view) {
    PokeListNativeMenu menu;
    if (PokeListMenu_GetNativeState(wk->menu, &menu)) {
        menuStart = menu.cursor >= 5 ? menu.cursor - 4 : 0;
        PcGx.party_menu_row = menu.cursor - menuStart;
        for (u32 row = 0; row < 5 && menuStart + row < menu.count; row++) {
            PcUi_Control((PcUiShape){ .rect = { 848, 447 + row * 34, 400, 32 } },
                         (PcUiAction){ PC_UI_ACTION_MENU, (int)(menuStart + row) });
            bool selected = menuStart + row == menu.cursor;
            PcUi_RoleLabel(wk->font, selected ? ">" : "", 862, 450 + row * 34, 882, 481 + row * 34, PC_UI_CYAN,
                           PC_UI_TEXT_BODY);
            PcUi_RoleText(wk->font, menu.labels[menuStart + row], 894, 450 + row * 34, 1230, 481 + row * 34,
                          selected ? PC_UI_CYAN : PC_UI_WHITE, PC_UI_TEXT_BODY);
        }
        return;
    }
    if (PokeList_GetNativePhase(wk) == POKELIST_NATIVE_MESSAGE) {
        PcUi_Control((PcUiShape){ .rect = { 796, 424, 484, 222 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
        const u16 *message = PokeListMessage_GetNativeText(wk->message);
        if (message)
            PcUi_RoleParagraph(wk->font, message, 850, 449, 1238, 608, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        return;
    }
    PcPartyView_DrawHeldItem(view);
    if (PokeList_GetNativePhase(wk) == POKELIST_NATIVE_BROWSE) {
        PcUi_Control((PcUiShape){ .rect = { 848, 447, 400, 185 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
        PcUi_RoleLabel(wk->font, "Z: Choose an action", 850, 603, 1210, 632, PC_UI_CYAN, PC_UI_TEXT_STAT_LABEL);
    }
}

bool PcPartyUi_Draw(void) {
    PokeListWork *wk = CurrentWork();
    if (!wk) {
        if (owner) {
            PcDexMedia_Free();
            owner = NULL;
        }
        return false;
    }
    if (owner != wk) {
        owner = wk;
        PcDexMedia_Free();
        activeTab = PC_PARTY_PARTY;
        shownSlot = 0;
        menuStart = 0;
        backView = false;
    }
    PokeListNativePhase phase = PokeList_GetNativePhase(wk);
    if (phase != POKELIST_NATIVE_BROWSE && phase != POKELIST_NATIVE_WAIT)
        activeTab = PC_PARTY_PARTY;
    u32 count = PokeParty_GetPkmCount(wk->param->party);
    if (wk->cursorPos >= 0 && (u32)wk->cursorPos < count)
        shownSlot = wk->cursorPos;
    shownSlot = shownSlot < count ? shownSlot : 0;
    PcUi_BeginControls(wk, phase);
    PcPartyView view = { .font = wk->font,
                         .count = count,
                         .selected = shownSlot,
                         .marked = wk->selectPos >= 0 ? (u32)wk->selectPos : 0xff,
                         .overview = activeTab == PC_PARTY_OVERVIEW,
                         .backView = backView,
                         .interactiveRows = phase == POKELIST_NATIVE_BROWSE || phase == POKELIST_NATIVE_TARGET,
                         .interactiveTabs = phase == POKELIST_NATIVE_BROWSE,
                         .rowAction = PC_UI_ACTION_ROW,
                         .section = "PARTY" };
    for (u32 i = 0; i < count; i++)
        PcPartyView_ReadMember(&view.members[i], PokeParty_GetPkm(wk->param->party, i));
    if (!PcPartyView_Draw(&view))
        return true;
    if (count) {
        PcPartyView_DrawDetails(&view);
        if (view.overview)
            PcPartyView_DrawMoves(&view);
        else
            DrawActions(wk, &view);
    }
    const u16 *message = PokeListMessage_GetNativeText(wk->message);
    if (message && phase != POKELIST_NATIVE_MESSAGE && !view.overview)
        PcUi_RoleParagraph(wk->font, message, 417, 551, 770, 631, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
    PcUiFrame_Footer(
        wk->font, phase == POKELIST_NATIVE_TARGET ? "Choose a destination.  X: Cancel" : "Tab: Page   V: Front / Back",
        "Select", true, true);
    return true;
}
