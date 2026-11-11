#include "app/battle_championship_flow.h"
#include "types.h"
#include "app/digitalcardcheck.h"
#include "app/livebattlematch_flow.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/backup_system.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/script/global_10520.h"
#include "text/system/battle_championship_flow.h"

enum {
    CHAMP_TOP_CHOICE_WIFI = 0,
    CHAMP_TOP_CHOICE_LIVE = 1,
    CHAMP_TOP_CHOICE_INFO = 2,
    CHAMP_TOP_CHOICE_EXIT = 3,
};

enum {
    CHAMP_ENTRY_CHOICE_ENTER = 0,
    CHAMP_ENTRY_CHOICE_INFO = 1,
    CHAMP_ENTRY_CHOICE_CARD = 2,
    CHAMP_ENTRY_CHOICE_BACK = 3,
};

enum {
    CHAMP_FADE_IN_START = 0,
    CHAMP_FADE_IN_WAIT = 1,
    CHAMP_FADE_IN_DISPATCH = 2,
};

enum {
    CHAMP_FADE_OUT_START = 0,
    CHAMP_FADE_OUT_WAIT = 1,
    CHAMP_FADE_OUT_END = 2,
};

enum {
    CHAMP_MENU_CREATE = 0,
    CHAMP_MENU_UPDATE = 1,
    CHAMP_MENU_EXIT = 2,
    CHAMP_MENU_START_LIVE = 3,
};

enum {
    CHAMP_LIVE_CREATE = 0,
    CHAMP_LIVE_UPDATE = 1,
    CHAMP_LIVE_EXIT = 2,
    CHAMP_LIVE_RETURN_MENU = 3,
};

enum {
    CHAMP_TOP_SHOW_PROMPT = 0,
    CHAMP_TOP_WAIT_PROMPT = 1,
    CHAMP_TOP_OPEN_MENU = 2,
    CHAMP_TOP_WAIT_MENU = 3,
};

enum {
    CHAMP_INTRO_SHOW = 0,
    CHAMP_INTRO_WAIT = 1,
};

enum {
    CHAMP_WIFI_SHOW_PROMPT = 0,
    CHAMP_WIFI_WAIT_PROMPT = 1,
    CHAMP_WIFI_OPEN_MENU = 2,
    CHAMP_WIFI_WAIT_MENU = 3,
};

enum {
    CHAMP_WIFI_INFO_SHOW = 0,
    CHAMP_WIFI_INFO_WAIT = 1,
};

enum {
    CHAMP_WIFI_CARD_CREATE = 0,
    CHAMP_WIFI_CARD_UPDATE = 1,
    CHAMP_WIFI_CARD_DELETE = 2,
};

enum {
    CHAMP_LIVE_CARD_CREATE = 0,
    CHAMP_LIVE_CARD_UPDATE = 1,
    CHAMP_LIVE_CARD_DELETE = 2,
};

enum {
    CHAMP_LIVE_MENU_SHOW_PROMPT = 0,
    CHAMP_LIVE_MENU_WAIT_PROMPT = 1,
    CHAMP_LIVE_MENU_OPEN_MENU = 2,
    CHAMP_LIVE_MENU_WAIT_MENU = 3,
};

enum {
    CHAMP_LIVE_INFO_SHOW = 0,
    CHAMP_LIVE_INFO_WAIT = 1,
};

enum {
    CHAMP_NO_WIRELESS_SHOW = 0,
    CHAMP_NO_WIRELESS_WAIT = 1,
};

enum {
    CHAMP_WIFI_ENTRY_CHECK_RECORD = 0,
    CHAMP_WIFI_ENTRY_MARK_STARTED = 1,
    CHAMP_WIFI_ENTRY_SHOW_OTHER_CONSOLE = 2,
    CHAMP_WIFI_ENTRY_WAIT_OTHER_CONSOLE = 3,
    CHAMP_WIFI_ENTRY_SHOW_INTERRUPTED = 4,
    CHAMP_WIFI_ENTRY_WAIT_INTERRUPTED = 5,
    CHAMP_WIFI_ENTRY_SHOW_SAVING = 6,
    CHAMP_WIFI_ENTRY_START_SAVE = 7,
    CHAMP_WIFI_ENTRY_WAIT_SAVE = 8,
    CHAMP_WIFI_ENTRY_ENTER = 9,
    CHAMP_WIFI_ENTRY_RETURN_MENU = 10,
};

enum {
    CHAMP_REWARD_CHECK_POINTS = 0,
    CHAMP_REWARD_SHOW_INTRO = 1,
    CHAMP_REWARD_WAIT_INTRO = 2,
    CHAMP_REWARD_SHOW_POINTS = 3,
    CHAMP_REWARD_WAIT_FANFARE = 4,
    CHAMP_REWARD_ADD_POINTS = 5,
    CHAMP_REWARD_SHOW_SAVING = 6,
    CHAMP_REWARD_START_SAVE = 7,
    CHAMP_REWARD_WAIT_SAVE = 8,
    CHAMP_REWARD_RETURN_MENU = 9,
};

enum {
    CHAMP_LAYOUT_CENTER = 0,
    CHAMP_LAYOUT_RIGHT = 1,
};

#define HEAPID_BATTLE_CHAMPIONSHIP_FLOW 0x4e

// What the menu's list holds
enum {
    LIST_TOP,
    LIST_WIFI,
    LIST_LIVE_MENU,
    LIST_LIVE,
};

// What the menu is for
enum {
    MENU_MODE_TOP,
    MENU_MODE_REWARD,
    MENU_MODE_LIVE,
};

// What the menu ends with
enum {
    MENU_RESULT_NONE,
    MENU_RESULT_EXIT,
    MENU_RESULT_WIFI,
    MENU_RESULT_LIVE,
};

// What the live competition's flow ends with
enum {
    LIVE_RESULT_NONE,
    LIVE_RESULT_MENU,
};

typedef struct {
    u16 heapId;
    WifiBattleMatchTask *task;
    WifiBattleMatchMsgBox *msgBox;
    Font *font;
    // The Entree Forest's messages
    MsgData *msgData;
    PrintQueue *printQueue;
    u32 unk18;
    u32 unk1C;
    WifiBattleMatchRes *seq;
    WifiBattleMatchGraphic *net;
    BattleChampionshipFlowParam *param;
    // The menu or the live competition's flow, whichever is running
    void *flow;
    // Set when the live competition sends the player back to the menu
    u32 backToMenu;
} BattleChampionshipFlow;

// What the menu is given
typedef struct {
    u32 mode;
    WifiBattleMatchMsgBox *msgBox;
    Font *font;
    MsgData *msgData;
    PrintQueue *printQueue;
    WifiBattleMatchRes *seq;
    WifiBattleMatchGraphic *net;
    GameData *gameData;
} BattleChampionshipMenuParam;

typedef struct {
    WifiBattleMatchTask *task;
    WifiBattleMatchMenu *list;
    u32 listKind;
    // Global script messages
    MsgData *scriptMsgData;
    u32 result;
    BattleChampionshipMenuParam param;
    u16 heapId;
    WordSet *wordSet;
    StrBuf *strBuf;
    DigitalCardCheck *cardCheck;
} BattleChampionshipMenu;

static BOOL BattleChampionshipFlow_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BattleChampionshipFlow_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BattleChampionshipFlow_Main(GameProc *proc, u32 *state, void *param, void *work);
static void BattleChampionshipFlow_CreateNet(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_DeleteNet(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_UpdateNet(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_CreateSeq(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_DeleteSeq(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_CreateUI(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_DeleteUI(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_UpdateUI(BattleChampionshipFlow *wk);
static void BattleChampionshipFlow_TaskDispatch(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskFadeIn(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskFadeOut(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskMenu(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_TaskLive(WifiBattleMatchTask *task, u32 *seq, void *work);
static BattleChampionshipMenu *BattleChampionshipFlow_MenuCreate(const BattleChampionshipMenuParam *param,
                                                                 HeapID heapId);
static void BattleChampionshipFlow_MenuDelete(BattleChampionshipMenu *menu);
static void BattleChampionshipFlow_MenuUpdate(BattleChampionshipMenu *menu);
static u32 BattleChampionshipFlow_MenuGetResult(BattleChampionshipMenu *menu);
static void BattleChampionshipFlow_MenuTaskStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskTop(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskIntro(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskWifi(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskWifiInfo(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskWifiCard(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskLiveCard(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskLive(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskLiveInfo(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskNoWireless(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskWifiEntry(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuTaskReward(WifiBattleMatchTask *task, u32 *seq, void *work);
static void BattleChampionshipFlow_MenuOpenList(BattleChampionshipMenu *menu, u32 kind);
static void BattleChampionshipFlow_MenuCloseList(BattleChampionshipMenu *menu);
static s32 BattleChampionshipFlow_MenuGetListChoice(BattleChampionshipMenu *menu);
static void BattleChampionshipFlow_MenuPrint(BattleChampionshipMenu *menu, u32 msgId);
static void BattleChampionshipFlow_MenuPrintScript(BattleChampionshipMenu *menu, u32 msgId);
static BOOL BattleChampionshipFlow_MenuIsPrinted(BattleChampionshipMenu *menu);
static void BattleChampionshipFlow_MenuFinish(BattleChampionshipMenu *menu, u32 result);

const GameProcFunctions BATTLE_CHAMPIONSHIP_FLOW_PROC_FUNCTIONS = {
    BattleChampionshipFlow_Init,
    BattleChampionshipFlow_Main,
    BattleChampionshipFlow_Exit,
};

static BOOL BattleChampionshipFlow_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BattleChampionshipFlow *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BATTLE_CHAMPIONSHIP_FLOW, 0x30200);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BattleChampionshipFlow), HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
    MI_CpuFill8(wk, 0, sizeof(BattleChampionshipFlow));
    wk->param = param;
    wk->heapId = HEAPID_BATTLE_CHAMPIONSHIP_FLOW;
    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_OvlLoad(OVERLAY_ID(262));
    BattleChampionshipFlow_CreateNet(wk);
    BattleChampionshipFlow_CreateSeq(wk);
    BattleChampionshipFlow_CreateUI(wk);
    wk->task = WifiBattleMatchTask_Create(wk, BattleChampionshipFlow_TaskFadeIn, HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
    return TRUE;
}

static BOOL BattleChampionshipFlow_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    BattleChampionshipFlow *wk = work;

    WifiBattleMatchTask_Free(wk->task);
    BattleChampionshipFlow_DeleteUI(wk);
    BattleChampionshipFlow_DeleteSeq(wk);
    BattleChampionshipFlow_DeleteNet(wk);
    GFL_OvlUnload(OVERLAY_ID(262));
    GFL_OvlUnload(OVERLAY_ID(139));
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
    return TRUE;
}

static BOOL BattleChampionshipFlow_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BattleChampionshipFlow *wk = work;

    BattleChampionshipFlow_UpdateNet(wk);
    BattleChampionshipFlow_UpdateUI(wk);
    WifiBattleMatchTask_Run(wk->task);
    if (WifiBattleMatchTask_IsEnded(wk->task)) {
        return TRUE;
    }
    return FALSE;
}

static void BattleChampionshipFlow_CreateNet(BattleChampionshipFlow *wk) {
    wk->net = WifiBattleMatchGraphic_Create(TRUE, wk->heapId);
}

static void BattleChampionshipFlow_DeleteNet(BattleChampionshipFlow *wk) {
    WifiBattleMatchGraphic_Free(wk->net);
    wk->net = NULL;
}

static void BattleChampionshipFlow_UpdateNet(BattleChampionshipFlow *wk) {
    if (wk->net != NULL) {
        WifiBattleMatchGraphic_Update(wk->net);
        WifiBattleMatchRes_Update(wk->seq);
    }
}

static void BattleChampionshipFlow_CreateSeq(BattleChampionshipFlow *wk) {
    wk->seq = WifiBattleMatchRes_Create(WifiBattleMatchGraphic_GetClActUnit(wk->net), WIFIBATTLEMATCH_BG_CHAMPIONSHIP,
                                        wk->heapId);
}

static void BattleChampionshipFlow_DeleteSeq(BattleChampionshipFlow *wk) {
    WifiBattleMatchRes_Free(wk->seq);
}

static void BattleChampionshipFlow_CreateUI(BattleChampionshipFlow *wk) {
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BATTLE_CHAMPIONSHIP_FLOW, wk->heapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    wk->msgBox = WifiBattleMatchMsgBox_Create(0, 15, 14, 10, wk->printQueue, wk->font, wk->heapId);
}

static void BattleChampionshipFlow_DeleteUI(BattleChampionshipFlow *wk) {
    WifiBattleMatchMsgBox_Free(wk->msgBox);
    PrintQueue_Free(wk->printQueue);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
}

static void BattleChampionshipFlow_UpdateUI(BattleChampionshipFlow *wk) {
    WifiBattleMatchMsgBox_Update(wk->msgBox);
    PrintQueue_Main(wk->printQueue);
}

static void BattleChampionshipFlow_TaskDispatch(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipFlow *wk = work;

    switch (wk->param->mode) {
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_MENU:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_REWARD:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_MENU_2:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskMenu);
        break;
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_JOIN:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_AFTER_MATCH:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_QUIT:
    case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_MENU:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskLive);
        break;
    }
}

static void BattleChampionshipFlow_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

static void BattleChampionshipFlow_TaskFadeIn(WifiBattleMatchTask *task, u32 *seq, void *work) {
    switch (*seq) {
    case CHAMP_FADE_IN_START:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        *seq = CHAMP_FADE_IN_WAIT;
        break;
    case CHAMP_FADE_IN_WAIT:
        if (!GFL_FadeIsRunning()) {
            *seq = CHAMP_FADE_IN_DISPATCH;
        }
        break;
    case CHAMP_FADE_IN_DISPATCH:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskDispatch);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
}

static void BattleChampionshipFlow_TaskFadeOut(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipFlow *wk = work;
    u32 mode;

    switch (*seq) {
    case CHAMP_FADE_OUT_START:
        if (wk->param->result == BATTLE_CHAMPIONSHIP_FLOW_RESULT_EXIT) {
            GFL_SndBGMFadeOut(6);
            mode = FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE;
        } else {
            mode = FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK;
        }
        GFL_FadeSet(mode, 0, 16, 0);
        *seq = CHAMP_FADE_OUT_WAIT;
        break;
    case CHAMP_FADE_OUT_WAIT:
        if (!GFL_FadeIsRunning()) {
            *seq = CHAMP_FADE_OUT_END;
        }
        break;
    case CHAMP_FADE_OUT_END:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskEnd);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
}

static void BattleChampionshipFlow_TaskMenu(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipFlow *wk = work;
    BattleChampionshipMenuParam param;

    switch (*seq) {
    case CHAMP_MENU_CREATE:
        MI_CpuFill8(&param, 0, sizeof(BattleChampionshipMenuParam));
        param.msgBox = wk->msgBox;
        param.font = wk->font;
        param.msgData = wk->msgData;
        param.printQueue = wk->printQueue;
        param.seq = wk->seq;
        param.net = wk->net;
        param.gameData = wk->param->gameData;
        if (wk->backToMenu != 0) {
            wk->backToMenu = 0;
            param.mode = MENU_MODE_LIVE;
        } else {
            switch (wk->param->mode) {
            case BATTLE_CHAMPIONSHIP_FLOW_MODE_MENU:
                param.mode = MENU_MODE_TOP;
                break;
            case BATTLE_CHAMPIONSHIP_FLOW_MODE_REWARD:
                param.mode = MENU_MODE_REWARD;
                break;
            }
        }
        wk->flow = BattleChampionshipFlow_MenuCreate(&param, HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
        (*seq)++;
        break;
    case CHAMP_MENU_UPDATE:
        BattleChampionshipFlow_MenuUpdate(wk->flow);
        switch (BattleChampionshipFlow_MenuGetResult(wk->flow)) {
        case MENU_RESULT_EXIT:
            wk->param->result = BATTLE_CHAMPIONSHIP_FLOW_RESULT_EXIT;
            *seq = CHAMP_MENU_EXIT;
            break;
        case MENU_RESULT_WIFI:
            wk->param->result = BATTLE_CHAMPIONSHIP_FLOW_RESULT_WIFI;
            *seq = CHAMP_MENU_EXIT;
            break;
        case MENU_RESULT_LIVE:
            wk->param->mode = BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_JOIN;
            *seq = CHAMP_MENU_START_LIVE;
            break;
        }
        break;
    case CHAMP_MENU_EXIT:
        BattleChampionshipFlow_MenuDelete(wk->flow);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskFadeOut);
        break;
    case CHAMP_MENU_START_LIVE:
        BattleChampionshipFlow_MenuDelete(wk->flow);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskLive);
        break;
    }
}

static void BattleChampionshipFlow_TaskLive(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipFlow *wk = work;
    LiveBattleMatchFlowParam param;

    switch (*seq) {
    case CHAMP_LIVE_CREATE:
        MI_CpuFill8(&param, 0, sizeof(LiveBattleMatchFlowParam));
        param.msgBox = wk->msgBox;
        param.font = wk->font;
        param.printQueue = wk->printQueue;
        param.seq = wk->seq;
        param.net = wk->net;
        param.gameData = wk->param->gameData;
        param.playerData = wk->param->playerData;
        param.partnerData = wk->param->partnerData;
        param.matchResult = wk->param->matchResult;
        switch (wk->param->mode) {
        case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_JOIN:
            param.mode = LIVE_FLOW_MODE_JOIN;
            break;
        case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_AFTER_MATCH:
            param.mode = LIVE_FLOW_MODE_AFTER_MATCH;
            break;
        case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_QUIT:
            param.mode = LIVE_FLOW_MODE_QUIT;
            break;
        case BATTLE_CHAMPIONSHIP_FLOW_MODE_LIVE_MENU:
            param.mode = LIVE_FLOW_MODE_MENU;
            break;
        default:
            param.mode = LIVE_FLOW_MODE_JOIN;
            break;
        }
        wk->flow = LiveBattleMatchFlow_Create(&param, HEAPID_BATTLE_CHAMPIONSHIP_FLOW);
        (*seq)++;
        break;
    case CHAMP_LIVE_UPDATE:
        LiveBattleMatchFlow_Main(wk->flow);
        switch (LiveBattleMatchFlow_GetResult(wk->flow)) {
        case LIVE_RESULT_MENU:
            *seq = CHAMP_LIVE_RETURN_MENU;
            break;
        case LIVE_FLOW_RESULT_BATTLE:
            wk->param->result = LIVE_FLOW_RESULT_BATTLE;
            *seq = CHAMP_LIVE_EXIT;
            break;
        case LIVE_FLOW_RESULT_RECORDED:
            wk->param->result = LIVE_FLOW_RESULT_RECORDED;
            *seq = CHAMP_LIVE_EXIT;
            break;
        case LIVE_FLOW_RESULT_REPLAY:
            wk->param->result = LIVE_FLOW_RESULT_REPLAY;
            *seq = CHAMP_LIVE_EXIT;
            break;
        }
        break;
    case CHAMP_LIVE_EXIT:
        LiveBattleMatchFlow_Delete(wk->flow);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskFadeOut);
        break;
    case CHAMP_LIVE_RETURN_MENU:
        wk->backToMenu = 1;
        LiveBattleMatchFlow_Delete(wk->flow);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_TaskMenu);
        break;
    }
}

static BattleChampionshipMenu *BattleChampionshipFlow_MenuCreate(const BattleChampionshipMenuParam *param,
                                                                 HeapID heapId) {
    BattleChampionshipMenu *menu =
        GFL_HeapAllocate(heapId, sizeof(BattleChampionshipMenu), FALSE, "battle_championship_flow.c", 185);

    MI_CpuFill8(menu, 0, sizeof(BattleChampionshipMenu));
    menu->param = *param;
    menu->heapId = heapId;
    menu->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    menu->strBuf = GFL_StrBufCreate(128, heapId);
    menu->scriptMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, heapId);
    menu->task = WifiBattleMatchTask_Create(menu, BattleChampionshipFlow_MenuTaskStart, heapId);
    return menu;
}

static void BattleChampionshipFlow_MenuDelete(BattleChampionshipMenu *menu) {
    WifiBattleMatchTask_Free(menu->task);
    GFL_MsgDataFree(menu->scriptMsgData);
    GFL_WordSetSystemFree(menu->wordSet);
    GFL_StrBufFree(menu->strBuf);
    GFL_HeapFree(menu);
}

static void BattleChampionshipFlow_MenuUpdate(BattleChampionshipMenu *menu) {
    WifiBattleMatchTask_Run(menu->task);
}

static u32 BattleChampionshipFlow_MenuGetResult(BattleChampionshipMenu *menu) {
    if (WifiBattleMatchTask_IsEnded(menu->task)) {
        return menu->result;
    }
    return MENU_RESULT_NONE;
}

static void BattleChampionshipFlow_MenuTaskStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (menu->param.mode) {
    case MENU_MODE_TOP:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
        break;
    case MENU_MODE_REWARD:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskReward);
        break;
    case MENU_MODE_LIVE:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLive);
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

static void BattleChampionshipFlow_MenuTaskTop(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    s32 choice;

    switch (*seq) {
    case CHAMP_TOP_SHOW_PROMPT:
        BattleChampionshipFlow_MenuPrint(menu, BattleChampionshipFlow_Text_WelcomeBattleCompetition);
        *seq = CHAMP_TOP_WAIT_PROMPT;
        break;
    case CHAMP_TOP_WAIT_PROMPT:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = CHAMP_TOP_OPEN_MENU;
        }
        break;
    case CHAMP_TOP_OPEN_MENU:
        BattleChampionshipFlow_MenuOpenList(menu, LIST_TOP);
        *seq = CHAMP_TOP_WAIT_MENU;
        break;
    case CHAMP_TOP_WAIT_MENU:
        choice = BattleChampionshipFlow_MenuGetListChoice(menu);
        if (choice != -1) {
            switch (choice) {
            case CHAMP_TOP_CHOICE_WIFI:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifi);
                break;
            case CHAMP_TOP_CHOICE_LIVE:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLive);
                break;
            case CHAMP_TOP_CHOICE_INFO:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskIntro);
                break;
            case CHAMP_TOP_CHOICE_EXIT:
                BattleChampionshipFlow_MenuFinish(menu, MENU_RESULT_EXIT);
                break;
            }
            BattleChampionshipFlow_MenuCloseList(menu);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskIntro(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (*seq) {
    case CHAMP_INTRO_SHOW:
        BattleChampionshipFlow_MenuPrint(menu, BattleChampionshipFlow_Text_ThereTwoKindsBattle);
        *seq = CHAMP_INTRO_WAIT;
        break;
    case CHAMP_INTRO_WAIT:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskWifi(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    s32 choice;

    switch (*seq) {
    case CHAMP_WIFI_SHOW_PROMPT:
        BattleChampionshipFlow_MenuPrint(menu, BattleChampionshipFlow_Text_WillParticipateWiFi);
        *seq = CHAMP_WIFI_WAIT_PROMPT;
        break;
    case CHAMP_WIFI_WAIT_PROMPT:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = CHAMP_WIFI_OPEN_MENU;
        }
        break;
    case CHAMP_WIFI_OPEN_MENU:
        BattleChampionshipFlow_MenuOpenList(menu, LIST_WIFI);
        *seq = CHAMP_WIFI_WAIT_MENU;
        break;
    case CHAMP_WIFI_WAIT_MENU:
        choice = BattleChampionshipFlow_MenuGetListChoice(menu);
        if (choice != -1) {
            switch (choice) {
            case CHAMP_ENTRY_CHOICE_ENTER:
                if (!isWirelessEnabled()) {
                    WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskNoWireless);
                } else {
                    WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifiEntry);
                }
                break;
            case CHAMP_ENTRY_CHOICE_INFO:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifiInfo);
                break;
            case CHAMP_ENTRY_CHOICE_CARD:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifiCard);
                break;
            case CHAMP_ENTRY_CHOICE_BACK:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
                break;
            }
            BattleChampionshipFlow_MenuCloseList(menu);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskWifiInfo(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (*seq) {
    case CHAMP_WIFI_INFO_SHOW:
        BattleChampionshipFlow_MenuPrint(menu, BattleChampionshipFlow_Text_ParticipateWiFiCompetition);
        *seq = CHAMP_WIFI_INFO_WAIT;
        break;
    case CHAMP_WIFI_INFO_WAIT:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifi);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskWifiCard(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    DigitalCardCheckParam param;

    switch (*seq) {
    case CHAMP_WIFI_CARD_CREATE:
        MI_CpuFill8(&param, 0, sizeof(DigitalCardCheckParam));
        param.mode = DIGITAL_CARD_CHECK_WIFI;
        param.msgBox = menu->param.msgBox;
        param.font = menu->param.font;
        param.msgData = menu->param.msgData;
        param.printQueue = menu->param.printQueue;
        param.seq = menu->param.seq;
        param.net = menu->param.net;
        param.gameData = menu->param.gameData;
        menu->cardCheck = DigitalCardCheck_Create(&param, menu->heapId);
        (*seq)++;
        break;
    case CHAMP_WIFI_CARD_UPDATE:
        DigitalCardCheck_Update(menu->cardCheck);
        if (DigitalCardCheck_IsFinished(menu->cardCheck)) {
            (*seq)++;
        }
        break;
    case CHAMP_WIFI_CARD_DELETE:
        DigitalCardCheck_Delete(menu->cardCheck);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifi);
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskLiveCard(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    DigitalCardCheckParam param;

    switch (*seq) {
    case CHAMP_LIVE_CARD_CREATE:
        MI_CpuFill8(&param, 0, sizeof(DigitalCardCheckParam));
        param.mode = DIGITAL_CARD_CHECK_LIVE;
        param.msgBox = menu->param.msgBox;
        param.font = menu->param.font;
        param.msgData = menu->param.msgData;
        param.printQueue = menu->param.printQueue;
        param.seq = menu->param.seq;
        param.net = menu->param.net;
        param.gameData = menu->param.gameData;
        menu->cardCheck = DigitalCardCheck_Create(&param, menu->heapId);
        (*seq)++;
        break;
    case CHAMP_LIVE_CARD_UPDATE:
        DigitalCardCheck_Update(menu->cardCheck);
        if (DigitalCardCheck_IsFinished(menu->cardCheck)) {
            (*seq)++;
        }
        break;
    case CHAMP_LIVE_CARD_DELETE:
        DigitalCardCheck_Delete(menu->cardCheck);
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLive);
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskLive(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    s32 choice;

    switch (*seq) {
    case CHAMP_LIVE_MENU_SHOW_PROMPT:
        BattleChampionshipFlow_MenuPrint(menu, BattleChampionshipFlow_Text_WillParticipateLiveCompetition);
        *seq = CHAMP_LIVE_MENU_WAIT_PROMPT;
        break;
    case CHAMP_LIVE_MENU_WAIT_PROMPT:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = CHAMP_LIVE_MENU_OPEN_MENU;
        }
        break;
    case CHAMP_LIVE_MENU_OPEN_MENU:
        BattleChampionshipFlow_MenuOpenList(menu, LIST_LIVE_MENU);
        *seq = CHAMP_LIVE_MENU_WAIT_MENU;
        break;
    case CHAMP_LIVE_MENU_WAIT_MENU:
        choice = BattleChampionshipFlow_MenuGetListChoice(menu);
        if (choice != -1) {
            switch (choice) {
            case CHAMP_ENTRY_CHOICE_ENTER:
                if (!isWirelessEnabled()) {
                    WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskNoWireless);
                } else {
                    clearVSPlayer();
                    BattleChampionshipFlow_MenuFinish(menu, MENU_RESULT_LIVE);
                }
                break;
            case CHAMP_ENTRY_CHOICE_INFO:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLiveInfo);
                break;
            case CHAMP_ENTRY_CHOICE_CARD:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLiveCard);
                break;
            case CHAMP_ENTRY_CHOICE_BACK:
                WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
                break;
            }
            BattleChampionshipFlow_MenuCloseList(menu);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskLiveInfo(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (*seq) {
    case CHAMP_LIVE_INFO_SHOW:
        BattleChampionshipFlow_MenuPrint(menu, BattleChampionshipFlow_Text_CanCheckInformationRegarding);
        *seq = CHAMP_LIVE_INFO_WAIT;
        break;
    case CHAMP_LIVE_INFO_WAIT:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskLive);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskNoWireless(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;

    switch (*seq) {
    case CHAMP_NO_WIRELESS_SHOW:
        BattleChampionshipFlow_MenuPrintScript(menu, Global10520_Text_WirelessCommunicationsTurnedOff);
        *seq = CHAMP_NO_WIRELESS_WAIT;
        break;
    case CHAMP_NO_WIRELESS_WAIT:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
        }
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskWifiEntry(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    SaveControl *save = GameData_GetSaveControl(menu->param.gameData);
    RecordSave *record = getRecordBlkAddress(save);

    switch (*seq) {
    case CHAMP_WIFI_ENTRY_CHECK_RECORD:
        if (RecordSave_IsMatchInProgress(record)) {
            if (RecordSave_IsMacAddressSame(record)) {
                if (RecordSave_IsMatchTimedOut(record)) {
                    RecordAddOne(getTrainerCardInfoBlkAddress(save), 0x35);
                    *seq = CHAMP_WIFI_ENTRY_MARK_STARTED;
                } else {
                    *seq = CHAMP_WIFI_ENTRY_SHOW_INTERRUPTED;
                }
            } else {
                RecordSave_SetMatchInProgress(record);
                *seq = CHAMP_WIFI_ENTRY_SHOW_OTHER_CONSOLE;
            }
        } else {
            *seq = CHAMP_WIFI_ENTRY_MARK_STARTED;
        }
        break;
    case CHAMP_WIFI_ENTRY_MARK_STARTED:
        RecordSave_SetMatchInProgress(record);
        WifiBattleMatchTask_SetReturnSeq(task, CHAMP_WIFI_ENTRY_ENTER);
        *seq = CHAMP_WIFI_ENTRY_SHOW_SAVING;
        break;
    case CHAMP_WIFI_ENTRY_SHOW_OTHER_CONSOLE:
        BattleChampionshipFlow_MenuPrint(menu, BattleChampionshipFlow_Text_SinceSystemTurnedOff);
        *seq = CHAMP_WIFI_ENTRY_WAIT_OTHER_CONSOLE;
        break;
    case CHAMP_WIFI_ENTRY_WAIT_OTHER_CONSOLE:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            WifiBattleMatchTask_SetReturnSeq(task, CHAMP_WIFI_ENTRY_RETURN_MENU);
            *seq = CHAMP_WIFI_ENTRY_SHOW_SAVING;
        }
        break;
    case CHAMP_WIFI_ENTRY_SHOW_INTERRUPTED:
        BattleChampionshipFlow_MenuPrint(menu, BattleChampionshipFlow_Text_SinceSystemTurnedOff);
        *seq = CHAMP_WIFI_ENTRY_WAIT_INTERRUPTED;
        break;
    case CHAMP_WIFI_ENTRY_WAIT_INTERRUPTED:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = CHAMP_WIFI_ENTRY_RETURN_MENU;
        }
        break;
    case CHAMP_WIFI_ENTRY_SHOW_SAVING:
        WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->param.msgData,
                                    BattleChampionshipFlow_Text_SavingDontTurnOff, WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = CHAMP_WIFI_ENTRY_START_SAVE;
        break;
    case CHAMP_WIFI_ENTRY_START_SAVE:
        GCTX_HIDBlockSoftReset(0x40);
        GameData_StartSave(menu->param.gameData);
        *seq = CHAMP_WIFI_ENTRY_WAIT_SAVE;
        break;
    case CHAMP_WIFI_ENTRY_WAIT_SAVE:
        switch (GameData_UpdateSave(menu->param.gameData)) {
        case SAVE_RESULT_OK:
        case SAVE_RESULT_NG:
            WifiBattleMatchTask_ReturnToSeq(task);
            break;
        }
        break;
    case CHAMP_WIFI_ENTRY_ENTER:
        clearVSPlayer();
        BattleChampionshipFlow_MenuFinish(menu, MENU_RESULT_WIFI);
        break;
    case CHAMP_WIFI_ENTRY_RETURN_MENU:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskTop);
        break;
    }
}

static void BattleChampionshipFlow_MenuTaskReward(WifiBattleMatchTask *task, u32 *seq, void *work) {
    BattleChampionshipMenu *menu = work;
    RecordSave *record = getRecordBlkAddress(GameData_GetSaveControl(menu->param.gameData));
    StrBuf *text;
    BSubwayScoreData *score;
    u8 points;

    switch (*seq) {
    case CHAMP_REWARD_CHECK_POINTS:
        RecordSave_ClearMatchInProgress(record);
        if (GameData_GetCounter(menu->param.gameData) == 0) {
            *seq = CHAMP_REWARD_SHOW_SAVING;
        } else {
            *seq = CHAMP_REWARD_SHOW_INTRO;
        }
        break;
    case CHAMP_REWARD_SHOW_INTRO:
        WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->param.msgData,
                                    BattleChampionshipFlow_Text_RecognizeAchievementBattlePresent,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = CHAMP_REWARD_WAIT_INTRO;
        break;
    case CHAMP_REWARD_WAIT_INTRO:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            *seq = CHAMP_REWARD_SHOW_POINTS;
        }
        break;
    case CHAMP_REWARD_SHOW_POINTS:
        GFL_SndBGMPush();
        GFL_SndBGMPlay(SEQ_ME_BPGET, SND_CHANNEL_MASK_ALL);
        text = GFL_MsgDataLoadStrbufNew(menu->param.msgData, BattleChampionshipFlow_Text_ReceivedBp);
        copyVarForText(menu->wordSet, 0, GetGameDataPlayerInfo(menu->param.gameData));
        WordSetNumber(menu->wordSet, 1, GameData_GetCounter(menu->param.gameData), 2, 0, 1);
        GFL_WordSetFormatStrbuf(menu->wordSet, menu->strBuf, text);
        GFL_StrBufFree(text);
        WifiBattleMatchMsgBox_PrintStr(menu->param.msgBox, menu->strBuf, WIFIBATTLEMATCH_MSG_STREAM);
        *seq = CHAMP_REWARD_WAIT_FANFARE;
        break;
    case CHAMP_REWARD_WAIT_FANFARE:
        if (!GFL_SndBGMIsPlaying()) {
            GFL_SndBGMPop();
            WifiBattleMatchMsgBox_EnableKeyCursor(menu->param.msgBox);
            *seq = CHAMP_REWARD_ADD_POINTS;
        }
        break;
    case CHAMP_REWARD_ADD_POINTS:
        if (BattleChampionshipFlow_MenuIsPrinted(menu) == TRUE) {
            score = GameData_GetBSubwayScoreData(menu->param.gameData);
            points = GameData_GetCounter(menu->param.gameData);
            BSubwayScoreData_AddBattlePoints(score, points);
            GameData_ResetCounter(menu->param.gameData);
            if (points != 0) {
                RecordAdd(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(menu->param.gameData)), 0x21, points);
            }
            *seq = CHAMP_REWARD_SHOW_SAVING;
        }
        break;
    case CHAMP_REWARD_SHOW_SAVING:
        WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->param.msgData,
                                    BattleChampionshipFlow_Text_SavingDontTurnOff, WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = CHAMP_REWARD_START_SAVE;
        break;
    case CHAMP_REWARD_START_SAVE:
        GameData_StartSave(menu->param.gameData);
        *seq = CHAMP_REWARD_WAIT_SAVE;
        break;
    case CHAMP_REWARD_WAIT_SAVE:
        switch (GameData_UpdateSave(menu->param.gameData)) {
        case SAVE_RESULT_OK:
        case SAVE_RESULT_NG:
            GCTX_HIDUnblockSoftReset(0x40);
            *seq = CHAMP_REWARD_RETURN_MENU;
            break;
        }
        break;
    case CHAMP_REWARD_RETURN_MENU:
        WifiBattleMatchTask_SetFunc(task, BattleChampionshipFlow_MenuTaskWifi);
        break;
    }
}

static void BattleChampionshipFlow_MenuOpenList(BattleChampionshipMenu *menu, u32 kind) {
    u32 layout;
    TournamentBlock *tournament = getTournamentBlockAddress(GameData_GetSaveControl(menu->param.gameData));
    WifiBattleMatchMenuParam param;
    TournamentEntry *entry;
    u8 x, y, width, height;

    MI_CpuFill8(&param, 0, sizeof(WifiBattleMatchMenuParam));
    param.msgData = menu->param.msgData;
    param.font = menu->param.font;
    param.printQueue = menu->param.printQueue;
    switch (kind) {
    case LIST_TOP:
        param.msgIds[0] = 2;
        param.msgIds[1] = 3;
        param.msgIds[2] = 5;
        param.msgIds[3] = 6;
        param.count = 4;
        param.canCancel = 1;
        param.cancelValue = 3;
        layout = CHAMP_LAYOUT_CENTER;
        break;
    case LIST_WIFI:
        entry = TournamentBlock_GetEntry(tournament, 0);
        param.msgIds[0] = 7;
        param.msgIds[1] = 5;
        if (entry->unk15B == 1 || entry->unk15B == 2) {
            param.msgIds[2] = 13;
        } else {
            param.msgIds[2] = 4;
        }
        param.msgIds[3] = 6;
        param.count = 4;
        param.canCancel = 1;
        param.cancelValue = 3;
        layout = CHAMP_LAYOUT_CENTER;
        break;
    case LIST_LIVE_MENU:
        entry = TournamentBlock_GetEntry(tournament, 1);
        param.msgIds[0] = 7;
        param.msgIds[1] = 5;
        if (entry->unk15B == 1 || entry->unk15B == 2) {
            param.msgIds[2] = 13;
        } else {
            param.msgIds[2] = 4;
        }
        param.msgIds[3] = 6;
        param.count = 4;
        param.canCancel = 1;
        param.cancelValue = 3;
        layout = CHAMP_LAYOUT_CENTER;
        break;
    case LIST_LIVE:
        param.msgIds[0] = 11;
        param.msgIds[1] = 12;
        param.count = 2;
        param.canCancel = 1;
        param.cancelValue = 1;
        layout = CHAMP_LAYOUT_RIGHT;
        break;
    }
    param.bg = 0;
    param.palette = 15;
    param.framePalette = 13;
    param.frameChar = 1;
    switch (layout) {
    case CHAMP_LAYOUT_CENTER:
        width = 28;
        height = param.count * 2;
        x = 2;
        y = 9 - height / 2;
        break;
    case CHAMP_LAYOUT_RIGHT:
        width = 12;
        height = param.count * 2;
        x = 19;
        y = 17 - height;
        break;
    }
    menu->list = WifiBattleMatchMenu_Create(&param, x, y, width, height, menu->heapId);
    menu->listKind = kind;
}

static void BattleChampionshipFlow_MenuCloseList(BattleChampionshipMenu *menu) {
    WifiBattleMatchMenu_Free(menu->list);
}

static s32 BattleChampionshipFlow_MenuGetListChoice(BattleChampionshipMenu *menu) {
    return WifiBattleMatchMenu_Update(menu->list);
}

static void BattleChampionshipFlow_MenuPrint(BattleChampionshipMenu *menu, u32 msgId) {
    WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->param.msgData, msgId, WIFIBATTLEMATCH_MSG_STREAM_KEY);
}

static void BattleChampionshipFlow_MenuPrintScript(BattleChampionshipMenu *menu, u32 msgId) {
    WifiBattleMatchMsgBox_Print(menu->param.msgBox, menu->scriptMsgData, msgId, WIFIBATTLEMATCH_MSG_STREAM_KEY);
}

static BOOL BattleChampionshipFlow_MenuIsPrinted(BattleChampionshipMenu *menu) {
    return WifiBattleMatchMsgBox_IsDone(menu->param.msgBox);
}

static void BattleChampionshipFlow_MenuFinish(BattleChampionshipMenu *menu, u32 result) {
    menu->result = result;
    WifiBattleMatchTask_SetFunc(menu->task, BattleChampionshipFlow_MenuTaskEnd);
}
