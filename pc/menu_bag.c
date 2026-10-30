#include "app/itemmenu.h"
#include "bag_view.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/ui.h"
#include "menu_apps.h"
#include "ui_widgets.h"
static PcUiInput input;
static MsgData *itemNames;
static PcBagViewCache cache;
static u32 listStart, actionStart;
static const char *const pockets[] = { "ITEMS", "MEDICINE", "TMs & HMs", "BERRIES", "KEY ITEMS", "FREE SPACE" };
static void Sync(ItemMenuWork *work) {
    PcUiInput_Sync(&input, work, ItemMenu_GetNativePhase(work));
}
void PcMenuBag_Reset(void) {
    if (itemNames)
        GFL_MsgDataFree(itemNames);
    itemNames = NULL;
    cache = (PcBagViewCache){ 0 };
    input = (PcUiInput){ 0 };
    listStart = actionStart = 0;
}
bool PcMenuBag_Draw(void) {
    ItemMenuWork *wk = ItemMenu_GetNativeWork();
    if (!wk)
        return false;
    Sync(wk);
    Font *font = wk->font;
    if (!itemNames)
        itemNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, HEAPID_USER);
    u32 phase = ItemMenu_GetNativePhase(wk);
    PcUi_BeginControls(input.owner, input.phase);
    if (phase == ITEMMENU_NATIVE_MESSAGE)
        PcUi_Control((PcUiShape){ .rect = { 26, 145, 1228, 500 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
    u32 count = ItemMenu_GetItemCount(wk);
    u32 cursor = count ? (u32)ItemMenu_GetCursorIndex(wk) : 0;
    if (cursor < listStart)
        listStart = cursor;
    if (cursor >= listStart + 6)
        listStart = cursor - 5;
    if (listStart + 6 > count)
        listStart = count > 6 ? count - 6 : 0;
    char text[96];
    SDL_snprintf(text, sizeof(text), "%u %s", count, count == 1 ? "ITEM" : "ITEMS");
    PcBagView view = { .font = font,
                       .itemNames = itemNames,
                       .section = wk->mode == 2 ? "GIVE ITEM" : "INVENTORY",
                       .hint = "Q/W: Pocket   T: Sort/filter   A: Move   D: Register",
                       .heading = text,
                       .pockets = pockets,
                       .pocketCount = 6,
                       .selectedPocket = wk->pocket,
                       .tabsEnabled = phase == ITEMMENU_NATIVE_LIST,
                       .showQuantity = true,
                       .rowAction = PC_UI_ACTION_ROW };
    for (u32 row = 0; row < 6 && listStart + row < count; row++) {
        u32 index = listStart + row;
        BagItem *slot = ItemMenu_GetSlot(wk, index);
        view.rows[view.rowCount++] = (PcBagViewRow){ .item = slot->item,
                                                     .count = slot->count,
                                                     .value = index,
                                                     .selected = index == cursor,
                                                     .enabled = phase == ITEMMENU_NATIVE_LIST };
    }
    if (count) {
        BagItem *slot = ItemMenu_GetSlot(wk, cursor);
        view.selectedItem = slot->item;
        view.selectedCount = slot->count;
    }
    PcBagView_Draw(&view, &cache);
    if (phase == ITEMMENU_NATIVE_MENU) {
        PcUi_RoleParagraph(font, wk->nativeMessage, 735, 434, 1224, 485, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
        PcMenu_Actions(font, wk->taskMenu, &actionStart, (PcUiRect){ 734, 492, 490, 36 }, 4);
    } else if (phase == ITEMMENU_NATIVE_QUANTITY) {
        PcUi_Control((PcUiShape){ .rect = { 735, 495, 242, 59 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_KEY_DOWN });
        PcUi_Control((PcUiShape){ .rect = { 978, 495, 242, 59 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_KEY_UP });
        PcUi_LineLabel(font, "QUANTITY", 735, 448, 1220, PC_UI_MUTED, PC_UI_TEXT_BODY);
        SDL_snprintf(text, sizeof(text), "−        %03d        +", wk->quantity);
        PcUi_RoleCenteredLabel(font, text, 735, 495, 1220, 554, PC_UI_CYAN, PC_UI_TEXT_HERO_VALUE);
        PcUi_LineLabel(font, "Arrows: Adjust   Z: Confirm   X: Cancel", 735, 576, 1220, PC_UI_WHITE, PC_UI_TEXT_LABEL);
    } else if (phase == ITEMMENU_NATIVE_MESSAGE) {
        PcUi_RoleParagraph(font, wk->nativeMessage, 735, 451, 1220, 601, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "Z: Continue", 735, 602, 1220, PC_UI_CYAN, PC_UI_TEXT_LABEL);
    } else {
        PcUi_LineLabel(font,
                       wk->movingItem  ? "MOVE ITEM"
                       : wk->mode == 2 ? "GIVE TO POKÉMON"
                                       : "CHOOSE AN ITEM",
                       735, 451, 1220, PC_UI_CYAN, PC_UI_TEXT_SECTION);
        PcUi_LineLabel(font,
                       wk->movingItem  ? "Move the item, then press Z to finish."
                       : wk->mode == 2 ? "Select an item for the chosen Pokémon."
                                       : "Select an item to see its available actions.",
                       735, 493, 1220, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
        PcUi_LineLabel(font, "Mouse wheel or arrows to browse.", 735, 571, 1220, PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
    }
    return true;
}

bool PcMenuBag_HandleEvent(PcVideo *video, const SDL_Event *event) {
    ItemMenuWork *work = ItemMenu_GetNativeWork();
    if (!work)
        return false;
    Sync(work);
    return PcUiInput_Event(&input, video, event);
}
Uint16 PcMenuBag_FilterKeys(Uint16 keys) {
    ItemMenuWork *work = ItemMenu_GetNativeWork();
    if (!work)
        return keys;
    Sync(work);
    Uint16 pressed;
    PcUiAction action;
    keys = PcUiInput_Read(&input, keys, &pressed, &action);
    if (keys & PAD_BUTTON_L)
        keys = (keys & ~PAD_BUTTON_L) | PAD_KEY_LEFT;
    if (keys & PAD_BUTTON_R)
        keys = (keys & ~PAD_BUTTON_R) | PAD_KEY_RIGHT;
    if (action.kind == PC_UI_ACTION_TAB)
        ItemMenu_SetNativePocket(work, action.value);
    if (action.kind == PC_UI_ACTION_ROW && ItemMenu_SetNativeIndex(work, action.value))
        keys |= PAD_BUTTON_A;
    if (action.kind == PC_UI_ACTION_MENU) {
        AppTaskMenuNativeState state;
        if (AppTaskMenu_GetNativeState(work->taskMenu, &state) && state.ready && (u32)action.value < state.count) {
            AppTaskMenu_SetCursorPos(work->taskMenu, action.value);
            keys |= PAD_BUTTON_A;
        }
    }
    if (keys)
        GCTX_HIDSetUsingTouch(FALSE);
    return keys;
}
