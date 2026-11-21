#include "types.h"
#include "battle/b_app_tool.h"
#include "battle/b_bag_bmp.h"
#include "battle/b_bag_main.h"
#include "constants/items.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/battle_bag.h"

// The battle bag's windows and text (overlay 286). The ROM doesn't name this file: b_bag_bmp.c is a guess after the
// ROM's b_bag_main.c and the b_bag_bmp.c of Diamond and Pearl. The names are ours.

#define BBAG_COLOR PRINT_COLOR(15, 2, 0)

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} BBagWinData;

static void BBagBmp_CreateWindows(BBagWork *work, u8 page);
static void BBagBmp_FreeWindows(BBagWork *work);
static void BBagBmp_PrintCentered(BBagWork *work, u32 winIdx, u32 msgId, u32 y, u16 color);
static void BBagBmp_DrawPocketPage(BBagWork *work);
static void BBagBmp_PrintSlotName(BBagWork *work, u32 index, u32 slot, u32 winIdx, u32 x, u32 y);
static void BBagBmp_PrintSlotCount(BBagWork *work, u32 index, u32 slot, u32 winIdx, u32 x, u32 y, u32 unused);
static void BBagBmp_DrawSlot(BBagWork *work, u32 slot);
static void BBagBmp_PrintPocketName(BBagWork *work);
static void BBagBmp_DrawItemPage(BBagWork *work);
static void BBagBmp_PrintChosenName(BBagWork *work, u32 index);
static void BBagBmp_PrintDescription(BBagWork *work, u32 index);
static void BBagBmp_DrawChosenItemPage(BBagWork *work);
static void BBagBmp_StartMessageStream(BBagWork *work);

static const u8 sBBagUsePageWindows[] = { BBAG_WINDOW_ITEM_NAME, BBAG_WINDOW_ITEM_COUNT, BBAG_WINDOW_ITEM_DESCRIPTION,
                                          BBAG_WINDOW_USE, BBAG_WINDOW_LIST_END };
static const u8 sBBagPocketPageWindows[] = { BBAG_WINDOW_POCKET_HP_PP,    BBAG_WINDOW_POCKET_STATUS,
                                             BBAG_WINDOW_POCKET_BALLS,    BBAG_WINDOW_POCKET_BATTLE_ITEMS,
                                             BBAG_WINDOW_LAST_ITEM_LABEL, BBAG_WINDOW_LIST_END };
static const u8 sBBagSecondListWindows[] = {
    BBAG_WINDOW_LIST_SECOND,     BBAG_WINDOW_LIST_SECOND + 1,  BBAG_WINDOW_LIST_SECOND + 2,
    BBAG_WINDOW_LIST_SECOND + 3, BBAG_WINDOW_LIST_SECOND + 4,  BBAG_WINDOW_LIST_SECOND + 5,
    BBAG_WINDOW_LIST_SECOND + 6, BBAG_WINDOW_LIST_SECOND + 7,  BBAG_WINDOW_LIST_SECOND + 8,
    BBAG_WINDOW_LIST_SECOND + 9, BBAG_WINDOW_LIST_SECOND + 10, BBAG_WINDOW_LIST_SECOND + 11,
    BBAG_WINDOW_POCKET_NAME,     BBAG_WINDOW_PAGE_NUMBER,      BBAG_WINDOW_LIST_END
};
static const u8 sBBagFirstListWindows[] = {
    BBAG_WINDOW_LIST_FIRST,     BBAG_WINDOW_LIST_FIRST + 1, BBAG_WINDOW_LIST_FIRST + 2,  BBAG_WINDOW_LIST_FIRST + 3,
    BBAG_WINDOW_LIST_FIRST + 4, BBAG_WINDOW_LIST_FIRST + 5, BBAG_WINDOW_LIST_FIRST + 6,  BBAG_WINDOW_LIST_FIRST + 7,
    BBAG_WINDOW_LIST_FIRST + 8, BBAG_WINDOW_LIST_FIRST + 9, BBAG_WINDOW_LIST_FIRST + 10, BBAG_WINDOW_LIST_FIRST + 11,
    BBAG_WINDOW_POCKET_NAME,    BBAG_WINDOW_PAGE_NUMBER,    BBAG_WINDOW_LIST_END
};

// By slot, the messages of the item's name and of its count
static const u32 sBBagSlotMessages[6][2] = {
    { BattleBag_Text_Empty_3, BattleBag_Text_X },   { BattleBag_Text_Empty_4, BattleBag_Text_X_2 },
    { BattleBag_Text_Empty_5, BattleBag_Text_X_3 }, { BattleBag_Text_Empty_6, BattleBag_Text_X_4 },
    { BattleBag_Text_Empty_7, BattleBag_Text_X_5 }, { BattleBag_Text_Empty_8, BattleBag_Text_X_6 },
};

static const BBagWinData sBBagWindowLayouts[BBAG_WINDOW_MAX] = {
    { 4, 2, 4, 12, 4, 12 },  { 4, 2, 13, 12, 4, 12 },  { 4, 18, 5, 12, 2, 12 }, { 4, 18, 14, 12, 2, 12 },
    { 4, 6, 21, 18, 2, 12 }, { 5, 1, 1, 14, 3, 12 },   { 5, 8, 4, 5, 3, 12 },   { 5, 17, 1, 14, 3, 12 },
    { 5, 24, 4, 5, 3, 12 },  { 5, 1, 7, 14, 3, 12 },   { 5, 8, 10, 5, 3, 12 },  { 5, 17, 7, 14, 3, 12 },
    { 5, 24, 10, 5, 3, 12 }, { 5, 1, 13, 14, 3, 12 },  { 5, 8, 16, 5, 3, 12 },  { 5, 17, 13, 14, 3, 12 },
    { 5, 24, 16, 5, 3, 12 }, { 5, 1, 1, 14, 3, 12 },   { 5, 8, 4, 5, 3, 12 },   { 5, 17, 1, 14, 3, 12 },
    { 5, 24, 4, 5, 3, 12 },  { 5, 1, 7, 14, 3, 12 },   { 5, 8, 10, 5, 3, 12 },  { 5, 17, 7, 14, 3, 12 },
    { 5, 24, 10, 5, 3, 12 }, { 5, 1, 13, 14, 3, 12 },  { 5, 8, 16, 5, 3, 12 },  { 5, 17, 13, 14, 3, 12 },
    { 5, 24, 16, 5, 3, 12 }, { 5, 11, 19, 10, 5, 12 }, { 5, 21, 20, 5, 3, 12 }, { 5, 7, 4, 12, 2, 12 },
    { 5, 20, 4, 5, 2, 12 },  { 5, 2, 9, 28, 6, 12 },   { 5, 8, 21, 11, 2, 12 },
};

// The message window. Declared after the window table for MWCC's sort to lay the data out in the ROM's order
static const BBagWinData sBBagMessageWindowLayout = { 4, 1, 19, 30, 4, 12 };

void BBagBmp_Init(BBagWork *work) {
    work->msgWin = BmpWin_CreateDynamic(sBBagMessageWindowLayout.bg, sBBagMessageWindowLayout.x,
                                        sBBagMessageWindowLayout.y, sBBagMessageWindowLayout.width,
                                        sBBagMessageWindowLayout.height, sBBagMessageWindowLayout.palette, TRUE);
    BBagBmp_CreateWindows(work, work->page);
}

// page isn't used: every window is created at once
static void BBagBmp_CreateWindows(BBagWork *work, u8 page) {
    const BBagWinData *data = sBBagWindowLayouts;
    u32 i;

    for (i = 0; i < BBAG_WINDOW_MAX; i++) {
        work->windows[i].window =
            BmpWin_CreateDynamic(data->bg, data->x, data->y, data->width, data->height, data->palette, TRUE);
        data++;
    }
}

static void BBagBmp_FreeWindows(BBagWork *work) {
    u32 i;

    for (i = 0; i < BBAG_WINDOW_MAX; i++) {
        BmpWin_Free(work->windows[i].window);
    }
}

void BBagBmp_Exit(BBagWork *work) {
    BBagBmp_FreeWindows(work);
    BmpWin_Free(work->msgWin);
}

void BBagBmp_DrawPage(BBagWork *work, u8 page) {
    switch (page) {
    case BBAG_PAGE_POCKETS:
        BBagBmp_DrawPocketPage(work);
        break;
    case BBAG_PAGE_ITEMS:
        BBagBmp_DrawItemPage(work);
        break;
    case BBAG_PAGE_USE:
        BBagBmp_DrawChosenItemPage(work);
        break;
    }
}

static void BBagBmp_PrintCentered(BBagWork *work, u32 winIdx, u32 msgId, u32 y, u16 color) {
    BmpWin *win = work->windows[winIdx].window;
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, msgId);
    u32 width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    u32 x = (BmpWin_GetSizeX(win) * 8 - width) / 2;

    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, y, str, work->param->font, color);
    GFL_StrBufFree(str);
}

static void BBagBmp_DrawPocketPage(BBagWork *work) {
    u32 i;

    for (i = 0; i <= 4; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[i].window), 0);
    }
    BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_HP_PP, BattleBag_Text_HpPp, 0, BBAG_COLOR);
    BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_HP_PP, BattleBag_Text_Restore, 16, BBAG_COLOR);
    BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_STATUS, BattleBag_Text_Status, 0, BBAG_COLOR);
    BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_STATUS, BattleBag_Text_Restore_2, 16, BBAG_COLOR);
    BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_BALLS, BattleBag_Text_PokeBalls, 0, BBAG_COLOR);
    BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_BATTLE_ITEMS, BattleBag_Text_BattleItems, 0, BBAG_COLOR);

    if (work->lastItem != ITEM_NONE) {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, BattleBag_Text_LastUsedItem);

        PrintWindow_Print(&work->windows[BBAG_WINDOW_LAST_ITEM_LABEL], work->printQueue, 0, 0, str, work->param->font,
                          BBAG_COLOR);
        GFL_StrBufFree(str);
    } else {
        BmpWin_FlushChar(work->windows[BBAG_WINDOW_LAST_ITEM_LABEL].window);
    }
    work->flushList = sBBagPocketPageWindows;
}

// x and y aren't used: the name is centred at y 7
static void BBagBmp_PrintSlotName(BBagWork *work, u32 index, u32 slot, u32 winIdx, u32 x, u32 y) {
    BmpWin *win = work->windows[winIdx].window;
    StrBuf *tmpl;
    u32 width;
    u32 nameX;

    GFL_BitmapFill(BmpWin_GetBitmap(win), 0);
    if (work->items[work->pocket][index].item != ITEM_NONE) {
        tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, sBBagSlotMessages[slot][BBAG_SLOT_MESSAGE_NAME]);
        loadItemNameToStrbuf(work->wordSet, 0, work->items[work->pocket][index].item);
        GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, tmpl);
        width = GFL_FontGetBlockWidth(work->strBuf, work->param->font, 0);
        nameX = (BmpWin_GetSizeX(win) * 8 - width) / 2;
        PrintWindow_Print(&work->windows[winIdx], work->printQueue, nameX, 7, work->strBuf, work->param->font,
                          BBAG_COLOR);
        GFL_StrBufFree(tmpl);
    } else {
        BmpWin_FlushChar(win);
    }
}

// x and the last argument aren't used: the count is printed at x 0
static void BBagBmp_PrintSlotCount(BBagWork *work, u32 index, u32 slot, u32 winIdx, u32 x, u32 y, u32 unused) {
    BmpWin *win = work->windows[winIdx].window;
    StrBuf *tmpl;

    GFL_BitmapFill(BmpWin_GetBitmap(win), 0);
    if (work->items[work->pocket][index].count != 0 && work->param->mode != BBAG_MODE_SHOOTER) {
        tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, sBBagSlotMessages[slot][BBAG_SLOT_MESSAGE_COUNT]);
        WordSetNumber(work->wordSet, 0, work->items[work->pocket][index].count, 3, NUM_PAD_NONE, TRUE);
        GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, tmpl);
        PrintWindow_Print(&work->windows[winIdx], work->printQueue, 0, y, work->strBuf, work->param->font, BBAG_COLOR);
        GFL_StrBufFree(tmpl);
    } else {
        BmpWin_FlushChar(win);
    }
}

static void BBagBmp_DrawSlot(BBagWork *work, u32 slot) {
    u32 index = work->param->pages[work->pocket] * BBAG_ITEMS_PER_PAGE;
    u32 winIdx = BBAG_WINDOW_LIST_FIRST;

    if (work->listBuf != 0) {
        winIdx = BBAG_WINDOW_LIST_SECOND;
    }
    BBagBmp_PrintSlotName(work, slot + index, slot, winIdx + slot * 2, 0, 0);
    BBagBmp_PrintSlotCount(work, slot + index, slot, winIdx + 1 + slot * 2, 0, 4, 0);
}

void BBagBmp_DrawSlots(BBagWork *work) {
    u16 i;

    GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 19, 17);
    for (i = 0; i < 6; i++) {
        BBagBmp_DrawSlot(work, i);
    }
    if (work->listBuf == 0) {
        work->flushList = sBBagFirstListWindows;
    } else {
        work->flushList = sBBagSecondListWindows;
    }
    work->listBuf ^= 1;
}

void BBagBmp_PrintPageNumber(BBagWork *work) {
    BmpWin *win = work->windows[BBAG_WINDOW_PAGE_NUMBER].window;
    GFLBitmap *bitmap = BmpWin_GetBitmap(win);
    StrBuf *str;
    u32 slashWidth;
    u32 x;
    u32 pageWidth;

    GFL_BitmapFill(bitmap, 0);
    str = GFL_MsgDataLoadStrbufNew(work->msgData, BattleBag_Text_Empty_10);
    slashWidth = GFL_FontGetBlockWidth(str, work->param->font, 0);
    x = (BmpWin_GetSizeX(win) * 8 - slashWidth) / 2;
    PrintQueue_PrintColor(work->printQueue, bitmap, x, 4, str, work->param->font, BBAG_COLOR);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(work->msgData, BattleBag_Text_Empty_11);
    WordSetNumber(work->wordSet, 0, work->lastPage[work->pocket] + 1, 2, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
    PrintQueue_PrintColor(work->printQueue, bitmap, x + slashWidth, 4, work->strBuf, work->param->font, BBAG_COLOR);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(work->msgData, BattleBag_Text_Empty_12);
    WordSetNumber(work->wordSet, 0, work->param->pages[work->pocket] + 1, 2, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
    pageWidth = GFL_FontGetBlockWidth(work->strBuf, work->param->font, 0);
    PrintWindow_Print(&work->windows[BBAG_WINDOW_PAGE_NUMBER], work->printQueue, x - pageWidth, 4, work->strBuf,
                      work->param->font, BBAG_COLOR);
    GFL_StrBufFree(str);
}

static void BBagBmp_PrintPocketName(BBagWork *work) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[BBAG_WINDOW_POCKET_NAME].window), 0);
    if (work->param->mode != BBAG_MODE_SHOOTER) {
        switch (work->pocket) {
        case BBAG_POCKET_HP_PP:
            BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_NAME, BattleBag_Text_HpPp_2, 4, BBAG_COLOR);
            BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_NAME, BattleBag_Text_Restore_3, 20, BBAG_COLOR);
            break;
        case BBAG_POCKET_STATUS:
            BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_NAME, BattleBag_Text_Status_2, 4, BBAG_COLOR);
            BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_NAME, BattleBag_Text_Restore_4, 20, BBAG_COLOR);
            break;
        case BBAG_POCKET_BALLS:
            BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_NAME, BattleBag_Text_PokeBalls_2, 12, BBAG_COLOR);
            break;
        case BBAG_POCKET_BATTLE_ITEMS:
            BBagBmp_PrintCentered(work, BBAG_WINDOW_POCKET_NAME, BattleBag_Text_BattleItems_2, 12, BBAG_COLOR);
            break;
        }
    } else {
        BmpWin_FlushChar(work->windows[BBAG_WINDOW_POCKET_NAME].window);
    }
}

static void BBagBmp_DrawItemPage(BBagWork *work) {
    BBagBmp_DrawSlots(work);
    BBagBmp_PrintPocketName(work);
    BBagBmp_PrintPageNumber(work);
}

static void BBagBmp_PrintChosenName(BBagWork *work, u32 index) {
    BmpWin *win = work->windows[BBAG_WINDOW_ITEM_NAME].window;
    StrBuf *tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, BattleBag_Text_Empty_3);

    loadItemNameToStrbuf(work->wordSet, 0, work->items[work->pocket][index].item);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, tmpl);
    GFL_BitmapFill(BmpWin_GetBitmap(win), 0);
    PrintWindow_Print(&work->windows[BBAG_WINDOW_ITEM_NAME], work->printQueue, 0, 0, work->strBuf, work->param->font,
                      BBAG_COLOR);
    GFL_StrBufFree(tmpl);
}

static void BBagBmp_PrintDescription(BBagWork *work, u32 index) {
    StrBuf *str = GFL_StrBufCreate(130, work->param->heapId);

    setItemDescriptionTextToStrbuf(str, work->items[work->pocket][index].item, work->param->heapId);
    PrintWindow_Print(&work->windows[BBAG_WINDOW_ITEM_DESCRIPTION], work->printQueue, 4, 0, str, work->param->font,
                      BBAG_COLOR);
    GFL_StrBufFree(str);
}

static void BBagBmp_DrawChosenItemPage(BBagWork *work) {
    u32 i;
    s16 row;
    s16 page;

    for (i = 31; i <= 34; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[i].window), 0);
    }
    row = work->param->rows[work->pocket];
    page = work->param->pages[work->pocket];
    BBagBmp_PrintChosenName(work, row + page * BBAG_ITEMS_PER_PAGE);
    BBagBmp_PrintSlotCount(work, row + page * BBAG_ITEMS_PER_PAGE, 0, BBAG_WINDOW_ITEM_COUNT, 0, 0, 0);
    BBagBmp_PrintDescription(work, row + page * BBAG_ITEMS_PER_PAGE);
    BBagBmp_PrintCentered(work, BBAG_WINDOW_USE, BattleBag_Text_Use, 0, BBAG_COLOR);
    work->flushList = sBBagUsePageWindows;
}

void BBagBmp_OpenMessage(BBagWork *work) {
    BmpWin_DrawFrame(work->msgWin, WINFRAME_TRANSFER_NONE, 1, 11);
    GFL_BitmapFill(BmpWin_GetBitmap(work->msgWin), 15);
    BBagBmp_StartMessageStream(work);
}

static void BBagBmp_StartMessageStream(BBagWork *work) {
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    work->printStream = PrintStream_Create(work->msgWin, 0, 0, work->strBuf, work->param->font,
                                           TextSpeed_GetCurrentWait(), work->tcbExMgr, 10, work->param->heapId, 15);
    BmpWin_Transfer(work->msgWin);
}

void BBagBmp_FlushWindows(BBagWork *work) {
    BAppTool_FlushPrintWindows(work->windows, work->printQueue, BBAG_WINDOW_MAX);
}

void BBagBmp_TransferPage(BBagWork *work) {
    BAppTool_QueueWindowScreens(work->windows, work->flushList);
}
