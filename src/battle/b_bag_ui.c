#include "battle/b_bag_ui.h"
#include "battle/b_app_tool.h"
#include "battle/b_bag_main.h"
#include "system/cursor_move.h"

// The battle bag's key cursor: a CursorMove on the page's table of positions, whose callbacks move and show overlay
// 285's cursor. The file name is our guess, after the ROM's b_bag_main.c and Diamond and Pearl's b_bag_* files.

static void BBagUi_SetCursorVisible(BBagWork *work, BOOL visible);
static void BBagUi_PutCursor(BBagWork *work, int pos);
static void BBagUi_CallbackCursorOn(void *work, int pos, int prevPos);
static void BBagUi_CallbackCursorOff(void *work, int pos, int prevPos);
static void BBagUi_CallbackTouch(void *work, int pos, int prevPos);
static void BBagUi_CallbackMove(void *work, int pos, int prevPos);
static void BBagUi_CallbackItemsCursorOn(void *work, int pos, int prevPos);

// The pockets
static const CursorMoveData sBBagPocketCursorData[7] = {
    { 64, 42, 132, 60, BBAG_POCKET_HP_PP, BBAG_POCKET_STATUS, BBAG_POCKET_HP_PP, BBAG_POCKET_BALLS, { 8, 71, 0, 127 } },
    { 64,
      114,
      132,
      60,
      BBAG_POCKET_HP_PP,
      BBAG_UI_POCKET_LAST_ITEM,
      BBAG_POCKET_STATUS,
      BBAG_POCKET_BATTLE_ITEMS,
      { 80, 143, 0, 127 } },
    { 192,
      42,
      132,
      60,
      BBAG_POCKET_BALLS,
      BBAG_POCKET_BATTLE_ITEMS,
      BBAG_POCKET_HP_PP,
      BBAG_POCKET_BALLS,
      { 8, 71, 128, 255 } },
    { 192,
      114,
      132,
      60,
      BBAG_POCKET_BALLS,
      BBAG_UI_POCKET_CANCEL,
      BBAG_POCKET_STATUS,
      BBAG_POCKET_BATTLE_ITEMS,
      { 80, 143, 128, 255 } },
    { 108,
      176,
      204,
      40,
      BBAG_POCKET_STATUS,
      BBAG_UI_POCKET_LAST_ITEM,
      BBAG_UI_POCKET_LAST_ITEM,
      BBAG_UI_POCKET_CANCEL,
      { 152, 191, 8, 207 } },
    { 236,
      174,
      40,
      40,
      CURSOR_MOVE_LINK_RETURN | BBAG_POCKET_BATTLE_ITEMS,
      BBAG_UI_POCKET_CANCEL,
      BBAG_UI_POCKET_LAST_ITEM,
      BBAG_UI_POCKET_CANCEL,
      { 152, 191, 216, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

// A pocket's items; 7 and 8, the page arrows, are touch only
static const CursorMoveData sBBagItemCursorData[10] = {
    { 64, 32, 132, 54, 0, 2, 0, 1, { 8, 55, 0, 127 } },
    { 192, 32, 132, 54, 1, 3, 0, 1, { 8, 55, 128, 255 } },
    { 64, 80, 132, 54, 0, 4, 2, 3, { 56, 103, 0, 127 } },
    { 192, 80, 132, 54, 1, 5, 2, 3, { 56, 103, 128, 255 } },
    { 64, 128, 132, 54, 2, BBAG_UI_ITEM_CANCEL, 4, 5, { 104, 151, 0, 127 } },
    { 192, 128, 132, 54, 3, BBAG_UI_ITEM_CANCEL, 4, 5, { 104, 151, 128, 255 } },
    { 236,
      174,
      40,
      40,
      CURSOR_MOVE_LINK_RETURN | 5,
      BBAG_UI_ITEM_CANCEL,
      BBAG_UI_ITEM_CANCEL,
      BBAG_UI_ITEM_CANCEL,
      { 152, 191, 216, 255 } },
    { 0,
      0,
      20,
      16,
      BBAG_UI_ITEM_PREV_PAGE,
      BBAG_UI_ITEM_PREV_PAGE,
      BBAG_UI_ITEM_PREV_PAGE,
      BBAG_UI_ITEM_PREV_PAGE,
      { 152, 191, 0, 39 } },
    { 0,
      0,
      20,
      16,
      BBAG_UI_ITEM_NEXT_PAGE,
      BBAG_UI_ITEM_NEXT_PAGE,
      BBAG_UI_ITEM_NEXT_PAGE,
      BBAG_UI_ITEM_NEXT_PAGE,
      { 152, 191, 40, 79 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

// One item
static const CursorMoveData sBBagUseCursorData[3] = {
    { 108,
      176,
      204,
      40,
      BBAG_UI_USE_ITEM,
      BBAG_UI_USE_ITEM,
      BBAG_UI_USE_ITEM,
      BBAG_UI_USE_CANCEL,
      { 152, 191, 8, 207 } },
    { 236,
      174,
      40,
      40,
      BBAG_UI_USE_CANCEL,
      BBAG_UI_USE_CANCEL,
      BBAG_UI_USE_ITEM,
      BBAG_UI_USE_CANCEL,
      { 152, 191, 216, 255 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static const CursorMoveCallbacks sBBagItemCursorCallbacks = {
    BBagUi_CallbackItemsCursorOn,
    BBagUi_CallbackCursorOff,
    BBagUi_CallbackMove,
    BBagUi_CallbackTouch,
};

static const CursorMoveCallbacks sBBagPocketCursorCallbacks = {
    BBagUi_CallbackCursorOn,
    BBagUi_CallbackCursorOff,
    BBagUi_CallbackMove,
    BBagUi_CallbackTouch,
};

static const CursorMoveCallbacks sBBagUseCursorCallbacks = {
    BBagUi_CallbackCursorOn,
    BBagUi_CallbackCursorOff,
    BBagUi_CallbackMove,
    BBagUi_CallbackTouch,
};

static const CursorMoveData *const sBBagPageCursorData[BBAG_PAGE_MAX] = {
    sBBagPocketCursorData,
    sBBagItemCursorData,
    sBBagUseCursorData,
};

static const CursorMoveCallbacks *const sBBagPageCursorCallbacks[BBAG_PAGE_MAX] = {
    &sBBagPocketCursorCallbacks,
    &sBBagItemCursorCallbacks,
    &sBBagUseCursorCallbacks,
};

void BBagUi_CreateCursor(BBagWork *work, u8 page, int pos) {
    work->cursorMove = CursorMove_Create(sBBagPageCursorData[page], sBBagPageCursorCallbacks[page], work,
                                         work->cursorVisible, pos, work->param->heapId);
    CursorMove_SetHideOnTouch(work->cursorMove);
    CursorMove_SetBOnlyShowsCursor(work->cursorMove);
    BBagUi_PutCursor(work, pos);
}

void BBagUi_DeleteCursor(BBagWork *work) {
    CursorMove_Delete(work->cursorMove);
}

void BBagUi_ChangeCursorPage(BBagWork *work, u8 page, int pos) {
    if (work->cursorVisible == FALSE) {
        pos = 0;
    } else {
        switch (page) {
        case BBAG_PAGE_POCKETS:
            pos = work->pocket;
            break;
        case BBAG_PAGE_ITEMS:
            pos = work->param->rows[work->pocket];
            break;
        case BBAG_PAGE_USE:
            pos = 0;
            break;
        }
    }
    BBagUi_DeleteCursor(work);
    BBagUi_CreateCursor(work, page, pos);
}

static void BBagUi_SetCursorVisible(BBagWork *work, BOOL visible) {
    work->cursorVisible = visible;
    BAppCursor_SetVisible(work->cursor, visible);
}

static void BBagUi_PutCursor(BBagWork *work, int pos) {
    BAppCursor_SetPos(work->cursor, CursorMove_GetData(work->cursorMove, pos));
}

static void BBagUi_CallbackCursorOn(void *work, int pos, int prevPos) {
    BBagUi_PutCursor(work, pos);
    BBagUi_SetCursorVisible(work, TRUE);
}

static void BBagUi_CallbackCursorOff(void *work, int pos, int prevPos) {
    BBagUi_SetCursorVisible(work, FALSE);
}

static void BBagUi_CallbackTouch(void *work, int pos, int prevPos) {
    BBagUi_SetCursorVisible(work, FALSE);
}

static void BBagUi_CallbackMove(void *work, int pos, int prevPos) {
    BBagUi_PutCursor(work, pos);
}

// On the items page, the cursor shown on a page arrow goes to the first item
static void BBagUi_CallbackItemsCursorOn(void *wk, int pos, int prevPos) {
    BBagWork *work = wk;

    if (pos == BBAG_UI_ITEM_PREV_PAGE || pos == BBAG_UI_ITEM_NEXT_PAGE) {
        CursorMove_SetPos(work->cursorMove, 0);
        pos = 0;
    }
    BBagUi_PutCursor(work, pos);
    BBagUi_SetCursorVisible(work, TRUE);
}
