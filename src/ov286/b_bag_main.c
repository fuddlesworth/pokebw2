#include "battle/b_bag_main.h"
#include "battle/b_app_tool.h"
#include "battle/b_bag_anm.h"
#include "battle/b_bag_bmp.h"
#include "battle/b_bag_item.h"
#include "battle/b_bag_obj.h"
#include "battle/b_bag_ui.h"
#include "battle/btl_pokeparam.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_finger_cursor.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/item.h"
#include "save/bag.h"
#include "system/bgwinfrm.h"
#include "system/bmp_winframe.h"
#include "system/cursor_move.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/b_bag_main.h"

// The battle bag's entry, task and states (b_bag_main.c, named by the ROM's embedded string): the bag runs one state
// function a frame, each returning the next state, until state 20 frees everything and sets the parameter's done.
// The pages are 0 the pockets, 1 a pocket's items and 2 one item.

static void BBagMain_Task(TCB *tcb, void *data);
static int BBagMain_StateInit(BBagWork *work);
static int BBagMain_StateInitLauncher(BBagWork *work);
static int BBagMain_StateStartWipe(BBagWork *work);
static int BBagMain_StateWaitWipe(BBagWork *work);
static int BBagMain_StatePocketPage(BBagWork *work);
static int BBagMain_StateItemPage(BBagWork *work);
static int BBagMain_StateStepPage(BBagWork *work);
static int BBagMain_StateWaitStepPage(BBagWork *work);
static int BBagMain_StateUsePage(BBagWork *work);
static int BBagMain_CheckUse(BBagWork *work);
static int BBagMain_StateToPocketPage(BBagWork *work);
static int BBagMain_StateToItemPage(BBagWork *work);
static int BBagMain_StateToUsePage(BBagWork *work);
static int BBagMain_StateWaitPagePrint(BBagWork *work);
static int BBagMain_StateCloseMessage(BBagWork *work);
static int BBagMain_StatePrintMessage(BBagWork *work);
static int BBagMain_StateWaitMessageKey(BBagWork *work);
static int BBagMain_StateWaitButtonAnm(BBagWork *work);
static int BBagMain_StateStartExit(BBagWork *work);
static int BBagMain_StateWaitFadeOut(BBagWork *work);
static BOOL BBagMain_Exit(TCB *tcb, BBagWork *work);
static int BBagMain_StateDemo(BBagWork *work);
static void BBagMain_InitBG(BBagWork *work);
static void BBagMain_ExitBG(void);
static void BBagMain_LoadGraphics(BBagWork *work);
static void BBagMain_InitText(BBagWork *work);
static void BBagMain_ExitText(BBagWork *work);
static void BBagMain_ScrollPage(BBagWork *work, u8 page);
static void BBagMain_ChangePage(BBagWork *work, u8 page);
static BOOL BBagMain_CheckQuit(BBagWork *work);
static void BBagMain_PlaySE(BBagWork *work, u32 se);
static void BBagMain_PlayMessageSE(BBagWork *work, u32 se);

static int (*const sBBagStateFuncs[])(BBagWork *work) = {
    BBagMain_StateInit,          BBagMain_StateInitLauncher, BBagMain_StateStartWipe,     BBagMain_StateWaitWipe,
    BBagMain_StatePocketPage,    BBagMain_StateItemPage,     BBagMain_StateUsePage,       BBagMain_StateToPocketPage,
    BBagMain_StateToItemPage,    BBagMain_StateToUsePage,    BBagMain_StateWaitPagePrint, BBagMain_StateStepPage,
    BBagMain_StateWaitStepPage,  BBagMain_StateCloseMessage, BBagMain_StatePrintMessage,  BBagMain_StateWaitMessageKey,
    BBagMain_StateWaitButtonAnm, BBagMain_StateDemo,         BBagMain_StateStartExit,     BBagMain_StateWaitFadeOut,
};

void BBagMain_Start(BBagParam *param) {
    BBagWork *work = GFL_HeapAllocate(param->heapId, sizeof(BBagWork), TRUE, "b_bag_main.c", 177);
    u32 i;

    GFL_TCBMgrAddTask(BtlvEffect_GetTCBManager(), BBagMain_Task, work, 100);
    work->param = param;
    work->paletteFade = BtlvEffect_GetPaletteFade();
    work->initialized = FALSE;
    if (work->param->mode == BBAG_MODE_SHOOTER) {
        work->page = BBAG_PAGE_ITEMS;
        work->seq = BBAG_STATE_INIT_LAUNCHER;
        work->pocket = BBAG_POCKET_SHOOTER;
    } else if (work->param->mode == BBAG_MODE_BALLS) {
        work->page = BBAG_PAGE_ITEMS;
        work->seq = BBAG_STATE_INIT;
        work->pocket = BBAG_POCKET_BALLS;
    } else {
        work->page = BBAG_PAGE_POCKETS;
        work->seq = BBAG_STATE_INIT;
        work->pocket = BBAG_POCKET_HP_PP;
    }
    if (work->param->mode != BBAG_MODE_SHOOTER) {
        for (i = 0; i < 4; i++) {
            BagCursor_GetBattlePocketPosition(work->param->bagCursor, i, &work->param->rows[i], &work->param->pages[i]);
        }
        work->lastItem = BagCursor_GetBattleLastItem(work->param->bagCursor);
        work->lastPocket = BagCursor_GetBattleLastPocket(work->param->bagCursor);
        BBagItem_CheckLastItem(work);
    } else {
        for (i = 0; i < 4; i++) {
            work->param->rows[i] = 0;
            work->param->pages[i] = 0;
        }
    }
}

static void BBagMain_Task(TCB *tcb, void *data) {
    BBagWork *work = data;

    if (work->param->abort == 1) {
        work->param->item = ITEM_NONE;
        work->param->cost = 0;
        work->seq = BBAG_STATE_EXIT;
    }
    if (work->seq != BBAG_STATE_EXIT) {
        work->seq = sBBagStateFuncs[work->seq](work);
    }
    if (work->seq == BBAG_STATE_EXIT && BBagMain_Exit(tcb, work) == TRUE) {
        return;
    }
    GFL_TCBExMgrUpdate(work->tcbExMgr);
    BBagAnm_MainButtonAnm(work);
    BGWinFrame_UpdateMoves(work->buttons);
    BBagBmp_FlushWindows(work);
}

static int BBagMain_StateInit(BBagWork *work) {
    G2S_BlendNone();
    GXS_SetMasterBrightness(-16);
    work->tcbExMgr = GFL_TCBExMgrCreate(work->param->heapId, work->param->heapId, 1, 4);
    work->cursor = BAppCursor_Create(work->param->heapId);
    BBagMain_InitBG(work);
    BBagMain_LoadGraphics(work);
    BBagMain_InitText(work);
    if (work->param->mode == BBAG_MODE_CAPTURE_DEMO) {
        BBagItem_MakeDemoList(work);
    } else {
        BBagItem_MakePocketLists(work);
    }
    BBagBmp_Init(work);
    BBagBmp_DrawPage(work, work->page);
    BBagBmp_TransferPage(work);
    BBagAnm_CreateButtons(work);
    BBagAnm_PutPageButtons(work, work->page);
    BBagObj_Init(work);
    BBagObj_SetPage(work, work->page);
    if (*work->param->usingKeys == 1) {
        work->cursorVisible = TRUE;
    } else {
        work->cursorVisible = FALSE;
    }
    BAppCursor_SetVisible(work->cursor, work->cursorVisible);
    if (work->page == BBAG_PAGE_POCKETS) {
        BBagUi_CreateCursor(work, work->page, work->lastPocket);
    } else {
        BBagUi_CreateCursor(work, work->page, 0);
    }
    GFL_NetSetIconScreen(FALSE, work->param->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, PALFADE_VRAM_SUB_OBJ, 0xe0, 0x20);
    PaletteFade_StartFade(work->paletteFade, 0xa, 0xffff, 0, 16, 0, 0, BtlvEffect_GetTCBManager());
    if (work->param->mode == BBAG_MODE_BALLS) {
        BBagMain_ScrollPage(work, work->page);
    }
    work->initialized = TRUE;
    return BBAG_STATE_START_WIPE;
}

static int BBagMain_StateInitLauncher(BBagWork *work) {
    G2S_BlendNone();
    GXS_SetMasterBrightness(-16);
    work->tcbExMgr = GFL_TCBExMgrCreate(work->param->heapId, work->param->heapId, 1, 4);
    work->cursor = BAppCursor_Create(work->param->heapId);
    BBagMain_InitBG(work);
    BBagMain_LoadGraphics(work);
    BBagMain_InitText(work);
    BBagItem_MakeShooterList(work);
    BBagMain_ScrollPage(work, work->page);
    BBagBmp_Init(work);
    BBagBmp_DrawPage(work, work->page);
    BBagBmp_TransferPage(work);
    BBagAnm_CreateButtons(work);
    BBagAnm_PutPageButtons(work, work->page);
    BBagObj_Init(work);
    BBagObj_SetPage(work, work->page);
    if (*work->param->usingKeys == 1) {
        work->cursorVisible = TRUE;
    } else {
        work->cursorVisible = FALSE;
    }
    BAppCursor_SetVisible(work->cursor, work->cursorVisible);
    BBagUi_CreateCursor(work, work->page, 0);
    GFL_NetSetIconScreen(FALSE, work->param->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, PALFADE_VRAM_SUB_OBJ, 0xe0, 0x20);
    PaletteFade_StartFade(work->paletteFade, 0xa, 0xffff, 0, 16, 0, 0, BtlvEffect_GetTCBManager());
    work->initialized = TRUE;
    return BBAG_STATE_START_WIPE;
}

static int BBagMain_StateStartWipe(BBagWork *work) {
    if (PrintQueue_IsEmpty(work->printQueue) == TRUE) {
        GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, work->param->heapId);
        return BBAG_STATE_WAIT_WIPE;
    }
    return BBAG_STATE_START_WIPE;
}

static int BBagMain_StateWaitWipe(BBagWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        if (work->param->mode == BBAG_MODE_CAPTURE_DEMO) {
            return BBAG_STATE_DEMO;
        }
        if (work->param->mode == BBAG_MODE_SHOOTER) {
            return BBAG_STATE_ITEM_PAGE;
        }
        if (work->param->mode == BBAG_MODE_BALLS) {
            return BBAG_STATE_ITEM_PAGE;
        }
        return BBAG_STATE_POCKET_PAGE;
    }
    return BBAG_STATE_WAIT_WIPE;
}

static int BBagMain_StatePocketPage(BBagWork *work) {
    u32 ret;

    if (PaletteFade_GetActiveMask(work->paletteFade) == 0) {
        if (BBagMain_CheckQuit(work) == TRUE) {
            return BBAG_STATE_START_EXIT;
        }
        ret = CursorMove_Update(work->cursorMove);
        switch (ret) {
        case BBAG_POCKET_HP_PP:
        case BBAG_POCKET_STATUS:
        case BBAG_POCKET_BALLS:
        case BBAG_POCKET_BATTLE_ITEMS:
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->pocket = ret;
            work->nextSeq = BBAG_STATE_TO_ITEM_PAGE;
            BBagAnm_StartButtonAnm(work, ret);
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case BBAG_UI_POCKET_LAST_ITEM:
            if (work->lastItem == ITEM_NONE) {
                break;
            }
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->pocket = work->lastPocket;
            work->nextSeq = BBAG_STATE_TO_USE_PAGE;
            BBagItem_SetCursorToLastItem(work);
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_ACTION);
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case BBAG_UI_POCKET_CANCEL:
            BBagMain_PlaySE(work, SEQ_SE_CANCEL2);
            work->param->item = ITEM_NONE;
            work->param->pocket = BBAG_POCKET_NONE;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_BACK);
            return BBAG_STATE_START_EXIT;
        case CURSOR_MOVE_CANCEL:
            BBagMain_PlaySE(work, SEQ_SE_CANCEL2);
            work->param->item = ITEM_NONE;
            work->param->pocket = BBAG_POCKET_NONE;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_BACK);
            return BBAG_STATE_START_EXIT;
        case CURSOR_MOVE_MOVED:
        case CURSOR_MOVE_CURSOR_ON:
            BBagMain_PlaySE(work, SEQ_SE_SELECT1);
            break;
        case CURSOR_MOVE_NONE:
        case CURSOR_MOVE_EDGE_RIGHT:
        case CURSOR_MOVE_EDGE_LEFT:
        case CURSOR_MOVE_EDGE_DOWN:
        case CURSOR_MOVE_EDGE_UP:
            break;
        }
    }
    return BBAG_STATE_POCKET_PAGE;
}

static int BBagMain_StateItemPage(BBagWork *work) {
    u32 ret;

    if (PaletteFade_GetActiveMask(work->paletteFade) == 0) {
        if (BBagMain_CheckQuit(work) == TRUE) {
            return BBAG_STATE_START_EXIT;
        }
        ret = CursorMove_Update(work->cursorMove);
        switch (ret) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            if (BBagItem_GetSlotItem(work, ret) == ITEM_NONE) {
                break;
            }
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->param->rows[work->pocket] = (u8)ret;
            work->nextSeq = BBAG_STATE_TO_USE_PAGE;
            BBagAnm_StartButtonAnm(work, ret + BBAG_BUTTON_ITEM);
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case BBAG_UI_ITEM_CANCEL:
            BBagMain_PlaySE(work, SEQ_SE_CANCEL2);
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_BACK);
            if (work->param->mode == BBAG_MODE_SHOOTER || work->param->mode == BBAG_MODE_BALLS) {
                work->param->item = ITEM_NONE;
                work->param->pocket = BBAG_POCKET_NONE;
                return BBAG_STATE_START_EXIT;
            }
            work->nextSeq = BBAG_STATE_TO_POCKET_PAGE;
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case CURSOR_MOVE_CANCEL:
            BBagMain_PlaySE(work, SEQ_SE_CANCEL2);
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_BACK);
            if (work->param->mode == BBAG_MODE_SHOOTER || work->param->mode == BBAG_MODE_BALLS) {
                work->param->item = ITEM_NONE;
                work->param->pocket = BBAG_POCKET_NONE;
                return BBAG_STATE_START_EXIT;
            }
            work->nextSeq = BBAG_STATE_TO_POCKET_PAGE;
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case BBAG_UI_ITEM_PREV_PAGE:
            if (work->lastPage[work->pocket] == 0) {
                break;
            }
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->param->rows[work->pocket] = 0;
            work->nextSeq = BBAG_STATE_STEP_PAGE;
            work->pageStep = -1;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_PREV_PAGE);
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case BBAG_UI_ITEM_NEXT_PAGE:
            if (work->lastPage[work->pocket] == 0) {
                break;
            }
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->param->rows[work->pocket] = 0;
            work->nextSeq = BBAG_STATE_STEP_PAGE;
            work->pageStep = 1;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_NEXT_PAGE);
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case CURSOR_MOVE_MOVED:
        case CURSOR_MOVE_CURSOR_ON:
            BBagMain_PlaySE(work, SEQ_SE_SELECT1);
            break;
        case CURSOR_MOVE_EDGE_LEFT:
            CursorMove_GetPos(work->cursorMove); // unused
            if (work->lastPage[work->pocket] == 0) {
                break;
            }
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->nextSeq = BBAG_STATE_STEP_PAGE;
            work->pageStep = -1;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_PREV_PAGE);
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case CURSOR_MOVE_EDGE_RIGHT:
            CursorMove_GetPos(work->cursorMove); // unused
            if (work->lastPage[work->pocket] == 0) {
                break;
            }
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->nextSeq = BBAG_STATE_STEP_PAGE;
            work->pageStep = 1;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_NEXT_PAGE);
            return BBAG_STATE_WAIT_BUTTON_ANM;
        case CURSOR_MOVE_NONE:
        case CURSOR_MOVE_EDGE_DOWN:
        case CURSOR_MOVE_EDGE_UP:
            break;
        }
    }
    return BBAG_STATE_ITEM_PAGE;
}

static int BBagMain_StateStepPage(BBagWork *work) {
    s8 page = work->param->pages[work->pocket];

    page += work->pageStep;
    if (page > work->lastPage[work->pocket]) {
        work->param->pages[work->pocket] = 0;
    } else if (page < 0) {
        work->param->pages[work->pocket] = work->lastPage[work->pocket];
    } else {
        work->param->pages[work->pocket] = page;
    }
    BBagBmp_DrawSlots(work);
    BBagBmp_PrintPageNumber(work);
    return BBAG_STATE_WAIT_STEP_PAGE;
}

static int BBagMain_StateWaitStepPage(BBagWork *work) {
    if (PrintQueue_IsEmpty(work->printQueue) == TRUE) {
        BBagBmp_TransferPage(work);
        BBagObj_SetPage(work, work->page);
        BBagAnm_PutPageButtons(work, work->page);
        return BBAG_STATE_ITEM_PAGE;
    }
    return BBAG_STATE_WAIT_STEP_PAGE;
}

static int BBagMain_StateUsePage(BBagWork *work) {
    if (BBagMain_CheckQuit(work) == TRUE) {
        return BBAG_STATE_START_EXIT;
    }
    switch (CursorMove_Update(work->cursorMove)) {
    case BBAG_UI_USE_ITEM:
        BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
        work->param->item = BBagItem_GetSlotItem(work, work->param->rows[work->pocket]);
        work->param->pocket = work->pocket;
        if (work->param->mode == BBAG_MODE_SHOOTER) {
            work->param->cost = BBagItem_GetShooterCost(work->param->item);
        } else {
            work->param->cost = 0;
        }
        BBagAnm_StartButtonAnm(work, BBAG_BUTTON_ACTION);
        return BBagMain_CheckUse(work);
    case BBAG_UI_USE_CANCEL:
        BBagMain_PlaySE(work, SEQ_SE_CANCEL2);
        work->nextSeq = BBAG_STATE_TO_ITEM_PAGE;
        BBagAnm_StartButtonAnm(work, BBAG_BUTTON_BACK);
        return BBAG_STATE_WAIT_BUTTON_ANM;
    case CURSOR_MOVE_CANCEL:
        BBagMain_PlaySE(work, SEQ_SE_CANCEL2);
        work->nextSeq = BBAG_STATE_TO_ITEM_PAGE;
        BBagAnm_StartButtonAnm(work, BBAG_BUTTON_BACK);
        return BBAG_STATE_WAIT_BUTTON_ANM;
    case CURSOR_MOVE_MOVED:
    case CURSOR_MOVE_CURSOR_ON:
        BBagMain_PlaySE(work, SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_NONE:
    case CURSOR_MOVE_EDGE_RIGHT:
    case CURSOR_MOVE_EDGE_LEFT:
    case CURSOR_MOVE_EDGE_DOWN:
    case CURSOR_MOVE_EDGE_UP:
        break;
    }
    return BBAG_STATE_USE_PAGE;
}

// The refusals of USE: the Launcher's energy, why a Poké Ball can't be thrown, and an escape item outside a wild
// battle. Each prints its message and returns to the item page
static int BBagMain_CheckUse(BBagWork *work) {
    BBagParam *param = work->param;

    if (param->mode == BBAG_MODE_SHOOTER && param->shooterEnergy < BBagItem_GetShooterCost(param->item)) {
        GFL_MsgDataLoadStrbuf(work->msgData, BBagMain_Text_ItsNotChargedEnough, work->strBuf);
        BBagBmp_OpenMessage(work);
        work->nextSeq = BBAG_STATE_CLOSE_MESSAGE;
        return BBAG_STATE_PRINT_MESSAGE;
    }
    if (work->pocket == BBAG_POCKET_BALLS) {
        if (param->ballError == BBAG_BALL_BOXES_FULL) {
            GFL_MsgDataLoadStrbuf(work->msgData, BBagMain_Text_ThereNoRoomLeft, work->strBuf);
            BBagBmp_OpenMessage(work);
            work->nextSeq = BBAG_STATE_CLOSE_MESSAGE;
            return BBAG_STATE_PRINT_MESSAGE;
        }
        if (param->ballError == BBAG_BALL_TWO_TARGETS) {
            GFL_MsgDataLoadStrbuf(work->msgData, BBagMain_Text_ItsNoGoodIts, work->strBuf);
            BBagBmp_OpenMessage(work);
            work->nextSeq = BBAG_STATE_CLOSE_MESSAGE;
            return BBAG_STATE_PRINT_MESSAGE;
        }
        if (param->ballError == BBAG_BALL_NOT_FOCUSED) {
            GFL_MsgDataLoadStrbuf(work->msgData, BBagMain_Text_ItsImpossibleAimWithout, work->strBuf);
            BBagBmp_OpenMessage(work);
            work->nextSeq = BBAG_STATE_CLOSE_MESSAGE;
            return BBAG_STATE_PRINT_MESSAGE;
        }
        if (param->ballError == BBAG_BALL_HIDDEN) {
            GFL_MsgDataLoadStrbuf(work->msgData, BBagMain_Text_ItsNoGoodIts_2, work->strBuf);
            BBagBmp_OpenMessage(work);
            work->nextSeq = BBAG_STATE_CLOSE_MESSAGE;
            return BBAG_STATE_PRINT_MESSAGE;
        }
        if (param->ballError == BBAG_BALL_BLOCKED) {
            GFL_MsgDataLoadStrbuf(work->msgData, BBagMain_Text_ItsImpossibleUsePoke, work->strBuf);
            BBagBmp_OpenMessage(work);
            work->nextSeq = BBAG_STATE_CLOSE_MESSAGE;
            return BBAG_STATE_PRINT_MESSAGE;
        }
    }
    if (param->isWild == FALSE &&
        GetItemParam(param->item, ITEM_PARAM_BATTLE_FUNC, param->heapId) == ITEM_BATTLE_FUNC_ESCAPE) {
        GFL_MsgDataLoadStrbuf(work->msgData, BBagMain_Text_JunipersWordsEchoedTheres, work->strBuf);
        BBagBmp_OpenMessage(work);
        work->nextSeq = BBAG_STATE_CLOSE_MESSAGE;
        return BBAG_STATE_PRINT_MESSAGE;
    }
    return BBAG_STATE_START_EXIT;
}

static int BBagMain_StateToPocketPage(BBagWork *work) {
    BBagBmp_DrawPage(work, BBAG_PAGE_POCKETS);
    work->page = BBAG_PAGE_POCKETS;
    work->nextSeq = BBAG_STATE_POCKET_PAGE;
    return BBAG_STATE_WAIT_PAGE_PRINT;
}

static int BBagMain_StateToItemPage(BBagWork *work) {
    BBagBmp_DrawPage(work, BBAG_PAGE_ITEMS);
    work->page = BBAG_PAGE_ITEMS;
    work->nextSeq = BBAG_STATE_ITEM_PAGE;
    return BBAG_STATE_WAIT_PAGE_PRINT;
}

static int BBagMain_StateToUsePage(BBagWork *work) {
    BBagBmp_DrawPage(work, BBAG_PAGE_USE);
    work->page = BBAG_PAGE_USE;
    work->nextSeq = BBAG_STATE_USE_PAGE;
    return BBAG_STATE_WAIT_PAGE_PRINT;
}

static int BBagMain_StateWaitPagePrint(BBagWork *work) {
    if (PrintQueue_IsEmpty(work->printQueue) == TRUE) {
        BBagMain_ChangePage(work, work->page);
        return work->nextSeq;
    }
    return BBAG_STATE_WAIT_PAGE_PRINT;
}

static int BBagMain_StateCloseMessage(BBagWork *work) {
    BmpWin_ClearFrame(work->msgWin, 0);
    return BBAG_STATE_USE_PAGE;
}

static int BBagMain_StatePrintMessage(BBagWork *work) {
    switch (PrintStream_GetState(work->printStream)) {
    case PRINT_STREAM_RUNNING:
        if (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            PrintStream_SkipWait(work->printStream, 0);
        }
        work->streamAdvanced = FALSE;
        break;
    case PRINT_STREAM_PAUSED:
        if (work->streamAdvanced == FALSE &&
            (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)))) {
            BBagMain_PlayMessageSE(work, SEQ_SE_MESSAGE);
            PrintStream_Continue(work->printStream);
            work->streamAdvanced = TRUE;
        }
        break;
    case PRINT_STREAM_DONE:
        PrintStream_Delete(work->printStream);
        work->printStream = NULL;
        work->streamAdvanced = FALSE;
        return BBAG_STATE_WAIT_MESSAGE_KEY;
    }
    return BBAG_STATE_PRINT_MESSAGE;
}

static int BBagMain_StateWaitMessageKey(BBagWork *work) {
    if (BBagMain_CheckQuit(work) == TRUE) {
        return BBAG_STATE_START_EXIT;
    }
    if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed() == TRUE) {
        return work->nextSeq;
    }
    return BBAG_STATE_WAIT_MESSAGE_KEY;
}

static int BBagMain_StateWaitButtonAnm(BBagWork *work) {
    if (work->animActive == FALSE) {
        return work->nextSeq;
    }
    return BBAG_STATE_WAIT_BUTTON_ANM;
}

static int BBagMain_StateStartExit(BBagWork *work) {
    PaletteFade_StartFade(work->paletteFade, 0xa, 0xffff, 0, 0, 16, 0, BtlvEffect_GetTCBManager());
    return BBAG_STATE_WAIT_FADE_OUT;
}

static int BBagMain_StateWaitFadeOut(BBagWork *work) {
    if (PaletteFade_GetActiveMask(work->paletteFade) == 0) {
        return BBAG_STATE_EXIT;
    }
    return BBAG_STATE_WAIT_FADE_OUT;
}

static BOOL BBagMain_Exit(TCB *tcb, BBagWork *work) {
    if (work->initialized == TRUE) {
        if (PrintQueue_IsEmpty(work->printQueue) == FALSE) {
            if (work->param->abort == 1) {
                PrintQueue_Clear(work->printQueue);
            } else {
                return FALSE;
            }
        }
        if (GFL_WipeIsFinished() == FALSE) {
            if (work->param->abort == 1) {
                GFL_WipeForceEnd();
            } else {
                return FALSE;
            }
        }
        if (work->printStream != NULL) {
            PrintStream_Delete(work->printStream);
        }
        BBagAnm_DeleteButtons(work);
        BBagObj_Exit(work);
        BBagBmp_Exit(work);
        BBagMain_ExitText(work);
        BBagMain_ExitBG();
        GFL_TCBExMgrFree(work->tcbExMgr);
        BBagUi_DeleteCursor(work);
        BAppCursor_Delete(work->cursor);
        if (work->cursorVisible == TRUE) {
            *work->param->usingKeys = TRUE;
        } else {
            *work->param->usingKeys = FALSE;
        }
    }
    work->param->done = TRUE;
    GFL_TCBRemove(tcb);
    GFL_HeapFree(work);
    return TRUE;
}

static int BBagMain_StateDemo(BBagWork *work) {
    switch (work->demoSeq) {
    case BBAG_DEMO_WAIT_POCKET_TOUCH:
        if (BtlvFingerCursor_IsTouched(work->fingerCursor) == TRUE) {
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->pocket = BBAG_POCKET_BALLS;
            work->nextSeq = BBAG_STATE_DEMO;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_POCKET_BALLS);
            work->demoSeq++;
            return BBAG_STATE_WAIT_BUTTON_ANM;
        }
        break;
    case BBAG_DEMO_WAIT_POCKET_RELEASE:
        if (BtlvFingerCursor_IsTouched(work->fingerCursor) == FALSE) {
            BBagMain_StateToItemPage(work);
            work->demoSeq++;
            work->nextSeq = BBAG_STATE_DEMO;
            return BBAG_STATE_WAIT_PAGE_PRINT;
        }
        break;
    case BBAG_DEMO_WAIT_ITEM_TOUCH:
        if (BtlvFingerCursor_IsTouched(work->fingerCursor) == TRUE) {
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->param->rows[work->pocket] = 0;
            work->nextSeq = BBAG_STATE_DEMO;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_ITEM);
            work->demoSeq++;
            return BBAG_STATE_WAIT_BUTTON_ANM;
        }
        break;
    case BBAG_DEMO_WAIT_ITEM_RELEASE:
        if (BtlvFingerCursor_IsTouched(work->fingerCursor) == FALSE) {
            BBagMain_StateToUsePage(work);
            work->demoSeq++;
            work->nextSeq = BBAG_STATE_DEMO;
            return BBAG_STATE_WAIT_PAGE_PRINT;
        }
        break;
    case BBAG_DEMO_WAIT_USE_TOUCH:
        if (BtlvFingerCursor_IsTouched(work->fingerCursor) == TRUE) {
            BBagMain_PlaySE(work, SEQ_SE_DECIDE2);
            work->param->item = BBagItem_GetSlotItem(work, work->param->rows[work->pocket]);
            work->param->pocket = work->pocket;
            work->param->cost = 0;
            BBagAnm_StartButtonAnm(work, BBAG_BUTTON_ACTION);
            work->demoSeq++;
        }
        break;
    case BBAG_DEMO_WAIT_USE_RELEASE:
        if (BtlvFingerCursor_IsTouched(work->fingerCursor) == FALSE) {
            return BBagMain_CheckUse(work);
        }
        break;
    }
    return BBAG_STATE_DEMO;
}

static void BBagMain_InitBG(BBagWork *work) {
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

        GFL_BGSysSetLCDConfigForEngine(&lcdConfig, BGSYS_ENGINE_SUB);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xc800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_23,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x2000,
            0,
            BGRES_512x512,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xd000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_23,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x4000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
    }
    GFL_BGSysClearCharCore(5, BGSYS_TILE_SIZE_16, 0, work->param->heapId);
    GFL_BGSysClearCharCore(4, BGSYS_TILE_SIZE_16, 0, work->param->heapId);
    GFL_BGSysQueueScrLoad(5);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
}

static void BBagMain_ExitBG(void) {
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ,
                           FALSE);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
}

static void BBagMain_LoadGraphics(BBagWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_B_BAG, HEAPID_TAIL(work->param->heapId));

    GFL_BGSysLoadArcNCGRStatic(arc, 0, 6, 0, 0, TRUE, work->param->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 6, 0, 0, TRUE, work->param->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, 7, 0, 0, TRUE, work->param->heapId);
#ifdef BLACK2
    PaletteFade_LoadArcNCLR(work->paletteFade, arc, 2, work->param->heapId, PALFADE_BUFFER_SUB_BG, 0xe0, 0);
#else
    PaletteFade_LoadArcNCLR(work->paletteFade, arc, 1, work->param->heapId, PALFADE_BUFFER_SUB_BG, 0xe0, 0);
#endif
    GFL_ArcToolFree(arc);
    LoadSysMsgBox(4, 1, 11, 0, work->param->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, PALFADE_VRAM_SUB_BG, 0xb0, 0x20);
    PaletteFade_LoadNCLR(work->paletteFade, ARCID_FONT, 5, work->param->heapId, PALFADE_BUFFER_SUB_BG, 0x20, 0xc0);
}

static void BBagMain_InitText(BBagWork *work) {
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_B_BAG_MAIN, work->param->heapId);
    work->wordSet = GFL_WordSetSystemCreateDefault(work->param->heapId);
    work->printQueue = PrintQueue_CreateDefault(work->param->heapId);
    work->strBuf = GFL_StrBufCreate(0x200, work->param->heapId);
}

static void BBagMain_ExitText(BBagWork *work) {
    GFL_MsgDataFree(work->msgData);
    GFL_WordSetSystemFree(work->wordSet);
    PrintQueue_Free(work->printQueue);
    GFL_StrBufFree(work->strBuf);
}

static void BBagMain_ScrollPage(BBagWork *work, u8 page) {
    switch (page) {
    case BBAG_PAGE_POCKETS:
        GFL_BGSysMoveBGReq(6, BG_MOVE_SET_X, 0);
        GFL_BGSysMoveBGReq(6, BG_MOVE_SET_Y, 0);
        break;
    case BBAG_PAGE_ITEMS:
        GFL_BGSysMoveBGReq(6, BG_MOVE_SET_X, 0x100);
        GFL_BGSysMoveBGReq(6, BG_MOVE_SET_Y, 0);
        break;
    case BBAG_PAGE_USE:
        GFL_BGSysMoveBGReq(6, BG_MOVE_SET_X, 0);
        GFL_BGSysMoveBGReq(6, BG_MOVE_SET_Y, 0x100);
        break;
    }
}

static void BBagMain_ChangePage(BBagWork *work, u8 page) {
    BBagMain_ScrollPage(work, page);
    GFL_BGSysFillScrAsync(4, 0);
    GFL_BGSysFillScrAsync(5, 0);
    BBagBmp_TransferPage(work);
    BBagAnm_PutPageButtons(work, page);
    BBagUi_ChangeCursorPage(work, page, 0);
    BBagObj_SetPage(work, page);
}

static BOOL BBagMain_CheckQuit(BBagWork *work) {
    if (work->param->quit == 1) {
        work->param->item = ITEM_NONE;
        work->param->cost = 0;
        return TRUE;
    }
    return FALSE;
}

static void BBagMain_PlaySE(BBagWork *work, u32 se) {
    if (work->param->playSound == 1) {
        GFL_SndSEPlay(se);
    }
}

static void BBagMain_PlayMessageSE(BBagWork *work, u32 se) {
    if (work->param->playSound == 1) {
        GFL_SndSEPlay(se);
    }
}
