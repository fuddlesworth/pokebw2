#include "battle/b_bag_anm.h"
#include "battle/b_bag_item.h"
#include "battle/b_bag_main.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "system/bgwinfrm.h"

// The battle bag's buttons (b_bag_anm.c, our guess after the ROM's b_bag_main.c and Diamond and Pearl's
// b_bag_anm.c): the 14 window frames on BG 6, from members of archive 98, which pages show which, their palette rows
// for normal, pressed and disabled, and the press animation that draws a button pressed and then released

// A button's member of archive 98 and its size in tiles
typedef struct {
    u16 member;
    u8 width;
    u8 height;
} BBagButton;

// The buttons: 0-3 the pockets, 4-9 the item slots, 10 back, 11 and 12 the previous and next page, 13 the wide bar
static const BBagButton sBBagButtonLayouts[BBAG_BUTTON_MAX] = {
    { 5, 16, 8 }, { 6, 16, 8 }, { 7, 16, 8 }, { 8, 16, 8 }, { 9, 16, 6 }, { 9, 16, 6 }, { 9, 16, 6 },
    { 9, 16, 6 }, { 9, 16, 6 }, { 9, 16, 6 }, { 12, 5, 5 }, { 10, 5, 5 }, { 11, 5, 5 }, { 13, 25, 5 },
};

void BBagAnm_CreateButtons(BBagWork *work) {
    ArcTool *arc;
    u32 i;

    work->buttons = BGWinFrame_Create(2, BBAG_BUTTON_MAX, work->param->heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_B_BAG, HEAPID_TAIL(work->param->heapId));
    for (i = 0; i < BBAG_BUTTON_MAX; i++) {
        BGWinFrame_InitFrame(work->buttons, i, 6, sBBagButtonLayouts[i].width, sBBagButtonLayouts[i].height);
        BGWinFrame_SetArea(work->buttons, i, 0, 64, 0, 64);
        BGWinFrame_LoadScreenArc(work->buttons, i, arc, sBBagButtonLayouts[i].member, TRUE);
    }
    GFL_ArcToolFree(arc);

    BGWinFrame_Put(work->buttons, BBAG_BUTTON_POCKET_HP_PP, 0, 1);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_POCKET_STATUS, 0, 10);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_POCKET_BALLS, 16, 1);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_POCKET_BATTLE_ITEMS, 16, 10);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_ITEM, 32, 1);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_ITEM + 1, 48, 1);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_ITEM + 2, 32, 7);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_ITEM + 3, 48, 7);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_ITEM + 4, 32, 13);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_ITEM + 5, 48, 13);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_PREV_PAGE, 32, 19);
    BGWinFrame_Put(work->buttons, BBAG_BUTTON_NEXT_PAGE, 37, 19);
}

void BBagAnm_DeleteButtons(BBagWork *work) {
    BGWinFrame_Delete(work->buttons);
}

// Shows the buttons of a page, each in its palette row: 1 or 4 normal, 3 or 6 disabled
void BBagAnm_PutPageButtons(BBagWork *work, u8 page) {
    u32 i;

    for (i = 0; i < BBAG_BUTTON_MAX; i++) {
        BGWinFrame_Hide(work->buttons, i);
    }

    switch (page) {
    case BBAG_PAGE_POCKETS:
        BGWinFrame_Put(work->buttons, BBAG_BUTTON_BACK, 27, 19);
        BGWinFrame_Put(work->buttons, BBAG_BUTTON_ACTION, 1, 19);
        if (work->lastItem == ITEM_NONE) {
            BGWinFrame_SetPalette(work->buttons, BBAG_BUTTON_ACTION, 0, 0, 25, 5, BBAG_CONTROL_PALETTE_DISABLED);
        } else {
            BGWinFrame_SetPalette(work->buttons, BBAG_BUTTON_ACTION, 0, 0, 25, 5, BBAG_CONTROL_PALETTE_NORMAL);
        }
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_POCKET_HP_PP);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_POCKET_STATUS);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_POCKET_BALLS);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_POCKET_BATTLE_ITEMS);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_BACK);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_ACTION);
        break;
    case BBAG_PAGE_ITEMS:
        BGWinFrame_Put(work->buttons, BBAG_BUTTON_BACK, 59, 19);
        for (i = 0; i < 6; i++) {
            if (BBagItem_GetSlotItem(work, i) == ITEM_NONE) {
                BGWinFrame_SetPalette(work->buttons, i + BBAG_BUTTON_ITEM, 0, 0,
                                      sBBagButtonLayouts[i + BBAG_BUTTON_ITEM].width,
                                      sBBagButtonLayouts[i + BBAG_BUTTON_ITEM].height, BBAG_GRID_PALETTE_DISABLED);
            } else {
                BGWinFrame_SetPalette(work->buttons, i + BBAG_BUTTON_ITEM, 0, 0,
                                      sBBagButtonLayouts[i + BBAG_BUTTON_ITEM].width,
                                      sBBagButtonLayouts[i + BBAG_BUTTON_ITEM].height, BBAG_GRID_PALETTE_NORMAL);
            }
        }
        if (work->lastPage[work->pocket] == 0) {
            BGWinFrame_SetPalette(work->buttons, BBAG_BUTTON_PREV_PAGE, 0, 0, 5, 5, BBAG_CONTROL_PALETTE_DISABLED);
            BGWinFrame_SetPalette(work->buttons, BBAG_BUTTON_NEXT_PAGE, 0, 0, 5, 5, BBAG_CONTROL_PALETTE_DISABLED);
        } else {
            BGWinFrame_SetPalette(work->buttons, BBAG_BUTTON_PREV_PAGE, 0, 0, 5, 5, BBAG_CONTROL_PALETTE_NORMAL);
            BGWinFrame_SetPalette(work->buttons, BBAG_BUTTON_NEXT_PAGE, 0, 0, 5, 5, BBAG_CONTROL_PALETTE_NORMAL);
        }
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_ITEM);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_ITEM + 1);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_ITEM + 2);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_ITEM + 3);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_ITEM + 4);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_ITEM + 5);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_PREV_PAGE);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_NEXT_PAGE);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_BACK);
        break;
    case BBAG_PAGE_USE:
        BGWinFrame_SetPalette(work->buttons, BBAG_BUTTON_ACTION, 0, 0, 25, 5, BBAG_CONTROL_PALETTE_NORMAL);
        BGWinFrame_Put(work->buttons, BBAG_BUTTON_BACK, 27, 51);
        BGWinFrame_Put(work->buttons, BBAG_BUTTON_ACTION, 1, 51);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_BACK);
        BGWinFrame_Show(work->buttons, BBAG_BUTTON_ACTION);
        break;
    }
}

void BBagAnm_StartButtonAnm(BBagWork *work, u8 button) {
    work->animSeq = BBAG_BUTTON_ANM_PRESS;
    work->animCount = 0;
    work->animButton = button;
    work->animActive = TRUE;
}

// Draws the pressed button in the pressed palette row (2, or 5 for the pockets and items) for 5 frames, then in the
// normal one for 2
void BBagAnm_MainButtonAnm(BBagWork *work) {
    u8 palette;

    if (work->animActive == FALSE) {
        return;
    }

    switch (work->animSeq) {
    case BBAG_BUTTON_ANM_PRESS:
        if (work->animButton >= BBAG_BUTTON_BACK) {
            palette = BBAG_CONTROL_PALETTE_PRESSED;
        } else {
            palette = BBAG_GRID_PALETTE_PRESSED;
        }
        BGWinFrame_SetPalette(work->buttons, work->animButton, 0, 0, sBBagButtonLayouts[work->animButton].width,
                              sBBagButtonLayouts[work->animButton].height, palette);
        BGWinFrame_Show(work->buttons, work->animButton);
        work->animSeq++;
        break;
    case BBAG_BUTTON_ANM_WAIT_PRESS:
        if (work->animCount == 4) {
            if (work->animButton >= BBAG_BUTTON_BACK) {
                palette = BBAG_CONTROL_PALETTE_NORMAL;
            } else {
                palette = BBAG_GRID_PALETTE_NORMAL;
            }
            BGWinFrame_SetPalette(work->buttons, work->animButton, 0, 0, sBBagButtonLayouts[work->animButton].width,
                                  sBBagButtonLayouts[work->animButton].height, palette);
            BGWinFrame_Show(work->buttons, work->animButton);
            work->animCount = 0;
            work->animSeq++;
        } else {
            work->animCount++;
        }
        break;
    case BBAG_BUTTON_ANM_WAIT_RELEASE:
        if (work->animCount == 1) {
            work->animCount = 0;
            work->animSeq = BBAG_BUTTON_ANM_PRESS;
            work->animActive = FALSE;
        } else {
            work->animCount++;
        }
        break;
    }
}
