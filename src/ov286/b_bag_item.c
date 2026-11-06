#include "battle/b_bag_item.h"
#include "battle/b_bag_main.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "system/shooter_item.h"

// The battle bag's item lists (b_bag_item.c, our guess after the ROM's b_bag_main.c and Diamond and Pearl's
// b_bag_item.c): the four battle pockets filled from the bag, from the Wonder Launcher's items or with the catching
// demo's Poké Balls, the last used item, and the item in a slot of the shown page

// The bag pockets the battle pockets are filled from: Items, Medicine and Berries
static const u8 sBBagSourcePockets[3] = { BAG_POCKET_ITEMS, BAG_POCKET_MEDICINE, BAG_POCKET_BERRIES };
// The battle pocket of each bit of an item's byte in archive 228
static const u8 sBBagPocketsByItemBit[4] = { BBAG_POCKET_BALLS, BBAG_POCKET_BATTLE_ITEMS, BBAG_POCKET_HP_PP,
                                             BBAG_POCKET_STATUS };

// Clears the last used item when the bag no longer holds it
BOOL BBagItem_CheckLastItem(BBagWork *work) {
    if (work->lastItem == ITEM_NONE) {
        return FALSE;
    }
    if (BagSave_CheckAmount(work->param->bag, work->lastItem, 1, work->param->heapId) == FALSE) {
        work->lastItem = ITEM_NONE;
        work->lastPocket = BBAG_POCKET_HP_PP;
        return FALSE;
    }
    return TRUE;
}

// Puts the cursor on the last used item in the pocket
void BBagItem_SetCursorToLastItem(BBagWork *work) {
    u32 i;

    for (i = 0; i < BBAG_ITEMS_PER_POCKET; i++) {
        if (work->lastItem == work->items[work->pocket][i].item) {
            work->param->rows[work->pocket] = i % BBAG_ITEMS_PER_PAGE;
            work->param->pages[work->pocket] = i / BBAG_ITEMS_PER_PAGE;
            return;
        }
    }
}

// Fills the battle pockets from the bag: archive 228 has a byte per item whose bits are the battle pockets it shows in
void BBagItem_MakePocketLists(BBagWork *work) {
    BagItem *slot;
    u8 *pockets;
    u32 i, j, k;
    u8 pocket;
    u8 bits;

    pockets = GFL_ArcSysReadHeapNew(ARCID_B_BAG_ITEM, 0, work->param->heapId);
    for (i = 0; i < 3; i++) {
        for (j = 0;; j++) {
            slot = BagSave_GetItemIndexHandle(work->param->bag, sBBagSourcePockets[i], j);
            if (slot == NULL) {
                break;
            }
            if (slot->item == ITEM_NONE || slot->count == 0) {
                continue;
            }
            bits = pockets[slot->item];
            for (k = 0; k < 4; k++) {
                if (bits & (1 << k)) {
                    pocket = sBBagPocketsByItemBit[k];
                    work->items[pocket][work->itemCount[pocket]] = *slot;
                    work->itemCount[pocket]++;
                }
            }
        }
    }
    GFL_HeapFree(pockets);

    for (i = 0; i < 4; i++) {
        if (work->itemCount[i] == 0) {
            work->lastPage[i] = 0;
        } else {
            work->lastPage[i] = (work->itemCount[i] - 1) / BBAG_ITEMS_PER_PAGE;
        }
        if (work->lastPage[i] < work->param->pages[i]) {
            work->param->pages[i] = work->lastPage[i];
        }
    }
}

// Fills the first pocket with the Wonder Launcher's enabled items
void BBagItem_MakeShooterList(BBagWork *work) {
    u32 i;

    for (i = 0; i < SHOOTER_ITEM_COUNT; i++) {
        if (ShooterItem_IsEnabled(work->param->shooterDisabled, i) == TRUE) {
            work->items[BBAG_POCKET_SHOOTER][work->itemCount[BBAG_POCKET_SHOOTER]].item = ShooterItem_GetItem(i);
            work->items[BBAG_POCKET_SHOOTER][work->itemCount[BBAG_POCKET_SHOOTER]].count = 1;
            work->itemCount[BBAG_POCKET_SHOOTER]++;
        }
    }
    work->lastPage[BBAG_POCKET_SHOOTER] = (work->itemCount[BBAG_POCKET_SHOOTER] - 1) / BBAG_ITEMS_PER_PAGE;
}

// The catching demo's bag: 30 Poké Balls
void BBagItem_MakeDemoList(BBagWork *work) {
    work->items[BBAG_POCKET_BALLS][0].item = ITEM_POKE_BALL;
    work->items[BBAG_POCKET_BALLS][0].count = 30;
    work->itemCount[BBAG_POCKET_BALLS] = 1;
    work->lastPage[BBAG_POCKET_BALLS] = 0;
}

// The item in a slot of the shown page, 0 if the slot is empty
u16 BBagItem_GetSlotItem(BBagWork *work, int slot) {
    u32 i = work->param->pages[work->pocket] * BBAG_ITEMS_PER_PAGE + slot;

    if (work->items[work->pocket][i].item == ITEM_NONE || work->items[work->pocket][i].count == 0) {
        return ITEM_NONE;
    }
    return work->items[work->pocket][i].item;
}

u16 BBagItem_GetShooterCost(u16 item) {
    return ShooterItem_GetCost(item);
}
