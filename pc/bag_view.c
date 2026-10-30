#include "bag_view.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "ui_assets.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static void ItemName(const PcBagView *view, u32 item, float x, float y, float right, u32 color, PcUiTextRole role) {
    u16 text[128];
    GFL_MsgDataLoadRawStr(view->itemNames, item, text, SDL_arraysize(text));
    PcUi_LineText(view->font, text, x, y, right, color, role);
}

void PcBagView_Draw(const PcBagView *view, PcBagViewCache *cache) {
    Font *font = view->font;
    PcUiFrame_Draw(font, PC_MENU_BAG, "BAG", view->section, view->hint);
    float stride = 1230.0f / view->pocketCount;
    for (u32 i = 0; i < view->pocketCount; i++)
        PcUi_Tab(font, view->pockets[i], (PcUiRect){ 26 + i * stride, 101, stride - 2, 43 }, i == view->selectedPocket,
                 (PcUiAction){ view->tabsEnabled ? PC_UI_ACTION_TAB : PC_UI_ACTION_NONE, i });
    PcUi_RoleLabel(font, view->heading, 52, 168, 642, 202, PC_UI_MUTED, PC_UI_TEXT_BODY);
    for (u32 i = 0; i < view->rowCount; i++) {
        const PcBagViewRow *row = &view->rows[i];
        float y = 213 + i * 65;
        PcUi_SkinControl(PC_MENU_ROW, (PcUiRect){ 42, y, 608, 58 }, row->selected,
                         (PcUiAction){ row->enabled ? view->rowAction : PC_UI_ACTION_NONE, row->value });
        u32 color = row->selected ? PC_UI_INK : PC_UI_WHITE;
        if (row->label)
            PcUi_RoleLabel(font, row->label, 66, y + 15, 530, y + 52, color, PC_UI_TEXT_BODY);
        else if (row->item) {
            PcUi_ItemIcon(row->item, i, 56, y + 5, 48);
            ItemName(view, row->item, 119, y + 15, 535, color, PC_UI_TEXT_VALUE);
        } else
            continue;
        if (row->item || (row->label && i < view->pocketCount)) {
            char quantity[32];
            SDL_snprintf(quantity, sizeof(quantity), row->label ? "%u" : "×%u", row->count);
            PcUi_RoleLabel(font, quantity, 555, y + 17, 640, y + 52, row->selected ? PC_UI_INK : PC_UI_MUTED,
                           PC_UI_TEXT_BODY);
        }
    }
    if (view->pocketList)
        return;
    if (!view->selectedItem) {
        PcUi_RoleLabel(font, "This pocket is empty.", 80, 350, 640, 390, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
        PcUi_RoleLabel(font, "NO ITEMS", 730, 186, 1220, 230, PC_UI_INK, PC_UI_TEXT_CARD_TITLE);
        return;
    }
    PcUi_ItemIcon(view->selectedItem, 6, 718, 167, 88);
    ItemName(view, view->selectedItem, 822, 180, 1212, PC_UI_INK, PC_UI_TEXT_CARD_TITLE);
    if (view->showQuantity) {
        char quantity[48];
        SDL_snprintf(quantity, sizeof(quantity), "%u in Bag", view->selectedCount);
        PcUi_RoleLabel(font, quantity, 824, 219, 1210, 253, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
    }
    if (!cache->valid || cache->item != view->selectedItem) {
        StrBuf *description = GFL_StrBufCreate(SDL_arraysize(cache->description), HEAPID_USER);
        if (!description)
            return;
        setItemDescriptionTextToStrbuf(description, view->selectedItem, HEAPID_USER);
        const u16 *text = GFL_StrBufGetStringPtr(description);
        u32 i = 0;
        while (i + 1 < SDL_arraysize(cache->description) && text[i] != 0xffff) {
            cache->description[i] = text[i];
            i++;
        }
        cache->description[i] = 0xffff;
        GFL_StrBufFree(description);
        cache->item = view->selectedItem;
        cache->valid = true;
    }
    PcUi_RoleParagraph(font, cache->description, 730, 271, 1222, 390, PC_UI_INK, PC_UI_TEXT_BODY);
}
