// wifibattlematch_link_random.c, in overlay 260 (a descriptive name: the ROM names only the next file,
// wifibattlematch_net.c): the random matchups of the Wi-Fi battle competitions, which the field starts. The player
// picks the free matchups or the ones by rating, and the link finds an opponent through the Global Link server,
// checks the team and the player's data that the other sends, and leaves overlay 290 to pick the teams and battle.
// Each step is a task of overlay 262 (wifibattlematch_util.c) that goes through its states with a message box and
// menus. After the battle the link comes back to send the result

#include "types.h"
#include "app/wifibattlematch_graphic.h"
#include "app/wifibattlematch_link.h"
#include "app/wifibattlematch_net.h"
#include "app/wifibattlematch_util.h"
#include "app/wifibattlematch_view.h"
#include "battle/btl_result.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "dwc/dwc.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/nhttp_rap_evilcheck.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms.h"
#include "system/pms_data.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/event_fishing.h"

enum {
    RANDOM_LINK_PHASE_FADE_IN = 0,
    RANDOM_LINK_PHASE_WAIT_FADE_IN = 1,
    RANDOM_LINK_PHASE_RUN_TASK = 2,
    RANDOM_LINK_PHASE_FADE_OUT = 3,
    RANDOM_LINK_PHASE_WAIT_FADE_OUT = 4,
};

enum {
    RANDOM_SELECT_MODE_HIDE_LINK_ICON = 0,
    RANDOM_SELECT_MODE_PROMPT_MODE = 1,
    RANDOM_SELECT_MODE_CREATE_MODE_MENU = 2,
    RANDOM_SELECT_MODE_WAIT_MODE_MENU = 3,
    RANDOM_SELECT_MODE_PROMPT_QUIT = 4,
    RANDOM_SELECT_MODE_CREATE_QUIT_MENU = 5,
    RANDOM_SELECT_MODE_WAIT_QUIT_MENU = 6,
    RANDOM_SELECT_MODE_WAIT_MESSAGE = 7,
};

enum {
    RATING_START_CHECK_GAME_SYNC_ID = 0,
    RATING_START_SHOW_RATING_MODE = 1,
    RATING_START_SHOW_DOWNLOAD_MESSAGE = 2,
    RATING_START_REQUEST_ACCOUNT = 3,
    RATING_START_WAIT_ACCOUNT = 4,
    RATING_START_CHECK_REGISTRATION = 5,
    RATING_START_SHOW_UNREGISTERED_MESSAGE = 6,
    RATING_START_START_TEAM_CHECK = 9,
    RATING_START_WAIT_TEAM_CHECK = 10,
    RATING_START_SHOW_INVALID_TEAM = 11,
    RATING_START_END = 12,
    RATING_START_CONNECT = 13,
    RATING_START_WAIT_MESSAGE = 14,
};

enum {
    RATING_CONNECT_SHOW_DOWNLOAD_MESSAGE = 0,
    RATING_CONNECT_DOWNLOAD_RATING = 1,
    RATING_CONNECT_WAIT_RATING = 2,
    RATING_CONNECT_CHECK_RECORD_ID = 3,
    RATING_CONNECT_WAIT_RECORD_CREATION = 4,
    RATING_CONNECT_CHECK_CARD_REFRESH = 5,
    RATING_CONNECT_DELAY_CARD_REFRESH = 6,
    RATING_CONNECT_HIDE_OLD_CARD = 7,
    RATING_CONNECT_CREATE_CARD = 8,
    RATING_CONNECT_WAIT_CARD_SHOWN = 9,
    RATING_CONNECT_SEARCH = 10,
    RATING_CONNECT_WAIT_MESSAGE = 11,
};

enum {
    RATING_SEARCH_SHOW_SEARCH_MESSAGE = 0,
    RATING_SEARCH_START_MATCH = 1,
    RATING_SEARCH_WAIT_MATCH = 2,
    RATING_SEARCH_SEND_PLAYER_INFO = 3,
    RATING_SEARCH_RECEIVE_PLAYER_INFO = 4,
    RATING_SEARCH_CHECK_SIGNATURE = 5,
    RATING_SEARCH_START_NAME_CHECK = 6,
    RATING_SEARCH_WAIT_NAME_CHECK = 7,
    RATING_SEARCH_SYNC_VALIDATED_PLAYERS = 8,
    RATING_SEARCH_WAIT_VALIDATED_PLAYERS_SYNC = 9,
    RATING_SEARCH_SYNC_MATCH_READY = 10,
    RATING_SEARCH_WAIT_MATCH_READY_SYNC = 11,
    RATING_SEARCH_SHOW_OPPONENT_FOUND = 12,
    RATING_SEARCH_WAIT_OPPONENT_FOUND_DELAY = 13,
    RATING_SEARCH_WAIT_PARTNER_CARD = 14,
    RATING_SEARCH_WAIT_PARTNER_CARD_DELAY = 15,
    RATING_SEARCH_START_SESSION_REPORT = 16,
    RATING_SEARCH_WAIT_SESSION_REPORT = 17,
    RATING_SEARCH_END_REPORT_HEAP = 18,
    RATING_SEARCH_SHOW_TEAM_SELECTION_MESSAGE = 19,
    RATING_SEARCH_WAIT_TEAM_SELECTION_DELAY = 20,
    RATING_SEARCH_CANCEL_SESSION = 21,
    RATING_SEARCH_PROMPT_QUIT = 22,
    RATING_SEARCH_CREATE_QUIT_MENU = 23,
    RATING_SEARCH_WAIT_QUIT_MENU = 24,
    RATING_SEARCH_PROMPT_CONTINUE = 25,
    RATING_SEARCH_CREATE_CONTINUE_MENU = 26,
    RATING_SEARCH_WAIT_CONTINUE_MENU = 27,
    RATING_SEARCH_SEND_CANCEL_COMMAND = 28,
    RATING_SEARCH_CANCEL_AND_RETRY = 29,
    RATING_SEARCH_WAIT_MESSAGE = 30,
};

enum {
    RATING_RESULT_CREATE_REPORT_HEAP = 0,
    RATING_RESULT_SHOW_REPORT_MESSAGE = 1,
    RATING_RESULT_START_REPORT = 2,
    RATING_RESULT_WAIT_REPORT = 3,
    RATING_RESULT_FINISH_REPORT = 4,
    RATING_RESULT_SEND_JOIN_AVENUE_PERSON = 5,
    RATING_RESULT_RECEIVE_JOIN_AVENUE_PERSON = 6,
    RATING_RESULT_RECORD_MET_TRAINER = 7,
    RATING_RESULT_WAIT_DISCONNECT = 8,
    RATING_RESULT_WAIT_WRITE_COOLDOWN = 9,
    RATING_RESULT_UPLOAD_RESULT = 10,
    RATING_RESULT_WAIT_RESULT_UPLOAD = 11,
    RATING_RESULT_SHOW_RESULTS_SENT = 12,
    RATING_RESULT_SHOW_SAVE_MESSAGE = 13,
    RATING_RESULT_START_SAVE = 14,
    RATING_RESULT_WAIT_SAVE = 15,
    RATING_RESULT_WAIT_MESSAGE = 16,
};

enum {
    RATING_AFTER_BATTLE_CHECK_CONNECTION = 0,
    RATING_AFTER_BATTLE_CONTINUE = 1,
    RATING_AFTER_BATTLE_WAIT_MESSAGE = 2,
};

enum {
    RATING_CONTINUE_CHECK_CONNECTION = 0,
    RATING_CONTINUE_PROMPT_CONTINUE = 1,
    RATING_CONTINUE_CREATE_MENU = 2,
    RATING_CONTINUE_WAIT_MENU = 3,
    RATING_CONTINUE_WAIT_MESSAGE = 4,
};

enum {
    RATING_QUIT_PROMPT_QUIT = 0,
    RATING_QUIT_CREATE_MENU = 1,
    RATING_QUIT_WAIT_MENU = 2,
    RATING_QUIT_WAIT_MESSAGE = 3,
};

enum {
    RATING_QUIT_SAVE_SHOW_SAVE_MESSAGE = 0,
    RATING_QUIT_SAVE_START_SAVE = 1,
    RATING_QUIT_SAVE_WAIT_SAVE = 2,
    RATING_QUIT_SAVE_SHOW_UPLOAD_MESSAGE = 3,
    RATING_QUIT_SAVE_UPLOAD_LOGOUT = 4,
    RATING_QUIT_SAVE_WAIT_LOGOUT_UPLOAD = 5,
    RATING_QUIT_SAVE_END = 6,
    RATING_QUIT_SAVE_WAIT_MESSAGE = 7,
};

enum {
    FREE_START_SHOW_MODE_AND_CREATE_CARD = 0,
    FREE_START_WAIT_CARD_SHOWN = 1,
    FREE_START_WAIT_MODE_MESSAGE = 2,
    FREE_START_SHOW_DOWNLOAD_MESSAGE = 3,
    FREE_START_START_TEAM_CHECK = 4,
    FREE_START_WAIT_TEAM_CHECK = 5,
    FREE_START_SHOW_INVALID_TEAM = 6,
    FREE_START_WAIT_INVALID_TEAM_MESSAGE = 7,
};

enum {
    FREE_SEARCH_UPDATE_PLAYER_RECORD = 0,
    FREE_SEARCH_SHOW_SEARCH_MESSAGE = 1,
    FREE_SEARCH_START_MATCH = 2,
    FREE_SEARCH_WAIT_MATCH = 3,
    FREE_SEARCH_SYNC_CONNECTED_PLAYERS = 6,
    FREE_SEARCH_WAIT_CONNECTED_PLAYERS_SYNC = 7,
    FREE_SEARCH_SEND_PLAYER_INFO = 8,
    FREE_SEARCH_RECEIVE_PLAYER_INFO = 9,
    FREE_SEARCH_CHECK_SIGNATURE = 10,
    FREE_SEARCH_START_NAME_CHECK = 11,
    FREE_SEARCH_WAIT_NAME_CHECK = 12,
    FREE_SEARCH_SYNC_VALIDATED_PLAYERS = 13,
    FREE_SEARCH_WAIT_VALIDATED_PLAYERS_SYNC = 14,
    FREE_SEARCH_SYNC_MATCH_READY = 15,
    FREE_SEARCH_WAIT_MATCH_READY_SYNC = 16,
    FREE_SEARCH_SHOW_OPPONENT_FOUND = 17,
    FREE_SEARCH_WAIT_OPPONENT_FOUND_DELAY = 18,
    FREE_SEARCH_WAIT_PARTNER_CARD = 19,
    FREE_SEARCH_WAIT_PARTNER_CARD_DELAY = 20,
    FREE_SEARCH_SHOW_TEAM_SELECTION_MESSAGE = 21,
    FREE_SEARCH_WAIT_TEAM_SELECTION_DELAY = 22,
    FREE_SEARCH_SEND_CANCEL_COMMAND = 23,
    FREE_SEARCH_CANCEL_AND_RETRY = 24,
    FREE_SEARCH_WAIT_MESSAGE = 25,
};

enum {
    FREE_RESULT_CHECK_CANCELLATION = 0,
    FREE_RESULT_SEND_COMMAND_CHECK_RESULT = 1,
    FREE_RESULT_RECEIVE_COMMAND_CHECK_RESULT = 2,
    FREE_RESULT_SEND_JOIN_AVENUE_PERSON = 3,
    FREE_RESULT_RECEIVE_JOIN_AVENUE_PERSON = 4,
    FREE_RESULT_SHOW_EXCHANGE_MESSAGE = 5,
    FREE_RESULT_RECORD_MET_TRAINER = 6,
    FREE_RESULT_WAIT_DISCONNECT = 7,
    FREE_RESULT_WAIT_DISCONNECT_DELAY = 8,
    FREE_RESULT_DELAY_SAVE = 9,
    FREE_RESULT_SHOW_SAVE_MESSAGE = 10,
    FREE_RESULT_START_SAVE = 11,
    FREE_RESULT_WAIT_SAVE = 12,
    FREE_RESULT_WAIT_MESSAGE = 13,
};

enum {
    FREE_AFTER_BATTLE_FIRST_DELAY_FRAME = 0,
    FREE_AFTER_BATTLE_SECOND_DELAY_FRAME = 1,
    FREE_AFTER_BATTLE_CONTINUE = 2,
    FREE_AFTER_BATTLE_WAIT_MESSAGE = 3,
};

enum {
    FREE_CONTINUE_PROMPT_CONTINUE = 0,
    FREE_CONTINUE_CREATE_MENU = 1,
    FREE_CONTINUE_WAIT_MENU = 2,
    FREE_CONTINUE_WAIT_MESSAGE = 3,
};

enum {
    FREE_QUIT_PROMPT_QUIT = 0,
    FREE_QUIT_CREATE_MENU = 1,
    FREE_QUIT_WAIT_MENU = 2,
    FREE_QUIT_SHOW_DISCONNECT_MESSAGE = 3,
    FREE_QUIT_END = 4,
    FREE_QUIT_WAIT_MESSAGE = 5,
};

enum {
    RANDOM_LINK_END_RELEASE_NETWORK = 0,
};

enum {
    RANDOM_TEAM_CHECK_CLEAR_SIGNATURE = 0,
    RANDOM_TEAM_CHECK_START_CHECK = 1,
    RANDOM_TEAM_CHECK_WAIT_CHECK = 2,
    RANDOM_TEAM_CHECK_COPY_SIGNATURE = 3,
    RANDOM_TEAM_CHECK_END = 4,
};

enum {
    RANDOM_SEARCH_CANCEL_IDLE = 0,
    RANDOM_SEARCH_CANCEL_SHOW_QUIT_PROMPT = 1,
    RANDOM_SEARCH_CANCEL_WAIT_QUIT_PROMPT = 2,
    RANDOM_SEARCH_CANCEL_CREATE_MENU = 3,
    RANDOM_SEARCH_CANCEL_WAIT_MENU = 4,
    RANDOM_SEARCH_CANCEL_SHOW_DISCONNECT_MESSAGE = 5,
    RANDOM_SEARCH_CANCEL_DISCONNECT = 6,
    RANDOM_SEARCH_CANCEL_CONFIRMED = 7,
};

enum {
    RANDOM_MODE_SELECTION_FREE = 0,
    RANDOM_MODE_SELECTION_RATING = 1,
    RANDOM_MODE_SELECTION_QUIT = 2,
};

// The modes of the random matchups (WifiBattleMatchLinkParam.mode); the 2 is overlay 290's own
enum {
    LINK_MODE_RATING,
    LINK_MODE_FREE,
    LINK_MODE_QUIT = 3,
};

// The menus
enum {
    MENU_YES_NO,
    MENU_MODE,
};

// What the check of a B press during the search says
enum {
    CANCEL_NONE,
    CANCEL_BUSY,
    CANCEL_DECLINED,
    CANCEL_CONFIRMED,
};

// What a sub task has left while it runs
#define SUB_TASK_RUNNING 0xffff

// The work of the random matchups
typedef struct {
    WifiBattleMatchGraphic *graphic;
    WifiBattleMatchRes *res;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData;
    WordSet *wordSet;
    WifiBattleMatchCard *card;
    WifiBattleMatchPartnerCard *partnerCard;
    WifiBattleMatchLinkIcon *linkIcon;
    WifiBattleMatchMsgBox *msgBox;
    WifiBattleMatchMenu *menu;
    u32 menuKind;
    WifiBattleMatchNet *net;
    WifiBattleMatchTask *task;
    WifiBattleMatchTask *subTask;
    u32 subTaskResult;
    WifiBattleMatchLinkParam *param;
    WifiBattleMatchNetCheck signature;
    u32 timer;
    u32 cancelSeq;
    u32 idleTimer;
    u32 unkEC;
    BOOL recordChanged;
    u32 exchangeResult;
    BOOL hadExchangeResult;
    u32 exchangeDone;
} WifiBattleMatchLinkRandom;

static BOOL WifiBattleMatchLink_RandomInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_RandomExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_RandomMain(GameProc *proc, u32 *state, void *param, void *work);
static void WifiBattleMatchLink_TaskStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskSelectMode(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingConnect(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingSearch(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingResult(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingContinue(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingQuitConfirm(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRatingQuitSave(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeSearch(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeResult(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeContinue(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskFreeQuitConfirm(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskRelogin(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_TaskSignTeam(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CreateRankCard(WifiBattleMatchLinkRandom *link, u32 kind);
static void WifiBattleMatchLink_DeleteRankCard(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_IsRankCardShown(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_IsRankCardHidden(WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_ShowRankCard(WifiBattleMatchLinkRandom *link, u32 kind);
static void WifiBattleMatchLink_HideRankCard(WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_CreatePartnerCard(WifiBattleMatchLinkRandom *link, WifiBattleMatchPlayer *partner);
static void WifiBattleMatchLink_DeletePartnerCard(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_IsPartnerCardDone(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_UpdateRecord(RndMatchData *results, u32 battleType, WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_AddMatchResult(RndMatchData *results, u32 battleType, u32 matchResult);
static void WifiBattleMatchLink_CreateMenu(WifiBattleMatchLinkRandom *link, u32 kind);
static void WifiBattleMatchLink_DeleteMenu(WifiBattleMatchLinkRandom *link);
static s32 WifiBattleMatchLink_GetMenuSelection(WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_FillCondition(WifiBattleMatchNetKeys *condition, WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_FillOwnPlayer(WifiBattleMatchPlayer *player, WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_UpdatePlayerRecord(WifiBattleMatchPlayer *player, WifiBattleMatchLinkRandom *link);
static void WifiBattleMatchLink_StartSubTask(WifiBattleMatchLinkRandom *link, WifiBattleMatchTaskFunc func);
static u32 WifiBattleMatchLink_UpdateSubTask(WifiBattleMatchLinkRandom *link);
static BOOL WifiBattleMatchLink_CheckSignature(WifiBattleMatchPlayer *player, HeapID heapId);
static u32 WifiBattleMatchLink_UpdateCancel(WifiBattleMatchLinkRandom *link, BOOL canCancel, BOOL canDisconnect);
static void WifiBattleMatchLink_ResetCancel(WifiBattleMatchLinkRandom *link);

const GameProcFunctions WIFIBATTLEMATCH_LINK_RANDOM_PROC_FUNCTIONS = {
    WifiBattleMatchLink_RandomInit,
    WifiBattleMatchLink_RandomMain,
    WifiBattleMatchLink_RandomExit,
};

static BOOL WifiBattleMatchLink_RandomInit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkParam *linkParam = param;
    WifiBattleMatchLinkRandom *link;

    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK, 0x35000);
    link = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchLinkRandom), HEAPID_WIFIBATTLEMATCH_LINK);
    MI_CpuFill8(link, 0, sizeof(WifiBattleMatchLinkRandom));
    link->param = linkParam;
    link->graphic = WifiBattleMatchGraphic_Create(1, HEAPID_WIFIBATTLEMATCH_LINK);
    link->res = WifiBattleMatchRes_Create(WifiBattleMatchGraphic_GetClActUnit(link->graphic), WIFIBATTLEMATCH_BG_RANDOM,
                                          HEAPID_WIFIBATTLEMATCH_LINK);
    WifiBattleMatchLink_FillOwnPlayer(linkParam->player, link);
    link->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, HEAPID_WIFIBATTLEMATCH_LINK);
    link->printQueue = PrintQueue_CreateDefault(HEAPID_WIFIBATTLEMATCH_LINK);
    link->msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING, HEAPID_WIFIBATTLEMATCH_LINK);
    link->wordSet = GFL_WordSetSystemCreate(8, 0x40, HEAPID_WIFIBATTLEMATCH_LINK);
    link->net = WifiBattleMatchNet_Create(link->param->netData, linkParam->param->gameData, linkParam->loginBuffer,
                                          HEAPID_WIFIBATTLEMATCH_LINK);
    link->msgBox =
        WifiBattleMatchMsgBox_Create(0, 15, 14, 10, link->printQueue, link->font, HEAPID_WIFIBATTLEMATCH_LINK);
    link->task = WifiBattleMatchTask_Create(link, WifiBattleMatchLink_TaskStart, HEAPID_WIFIBATTLEMATCH_LINK);
    link->subTask = WifiBattleMatchTask_Create(link, NULL, HEAPID_WIFIBATTLEMATCH_LINK);
    link->linkIcon = WifiBattleMatchLinkIcon_Create(WifiBattleMatchGraphic_GetClActUnit(link->graphic), link->res,
                                                    HEAPID_WIFIBATTLEMATCH_LINK);
    WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
    if (linkParam->result != WBM_LINK_RESULT_INITIAL) {
        if (linkParam->mode == LINK_MODE_RATING) {
            WifiBattleMatchLink_ShowRankCard(link, 0);
        } else if (linkParam->mode == LINK_MODE_FREE) {
            if (linkParam->result == WBM_LINK_RESULT_BATTLE_FINISHED) {
                WifiBattleMatchLink_AddMatchResult(linkParam->record, linkParam->param->battleType,
                                                   linkParam->battleResult->result);
            }
            WifiBattleMatchLink_ShowRankCard(link, 1);
        }
    }
    WifiBattleMatchNet_ShowIcon(link->net, TRUE);
    if (linkParam->result == WBM_LINK_RESULT_BATTLE_FINISHED) {
        WifiBattleMatchNet_SetOpponentNameRejected(link->net, *link->param->opponentNameRejected);
    }
    return TRUE;
}

static BOOL WifiBattleMatchLink_RandomExit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    WifiBattleMatchNet_ShowIcon(link->net, FALSE);
    WifiBattleMatchLinkIcon_Free(link->linkIcon);
    WifiBattleMatchTask_Free(link->subTask);
    WifiBattleMatchTask_Free(link->task);
    WifiBattleMatchMsgBox_Free(link->msgBox);
    if (link->net != NULL) {
        WifiBattleMatchNet_Delete(link->net);
    }
    WifiBattleMatchLink_DeletePartnerCard(link);
    WifiBattleMatchLink_DeleteRankCard(link);
    WifiBattleMatchLink_DeleteMenu(link);
    GFL_WordSetSystemFree(link->wordSet);
    GFL_MsgDataFree(link->msgData);
    PrintQueue_Free(link->printQueue);
    GFL_FontFree(link->font);
    WifiBattleMatchRes_Free(link->res);
    WifiBattleMatchGraphic_Free(link->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH_LINK);
    GFL_OvlUnload(OVERLAY_ID(189));
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

static BOOL WifiBattleMatchLink_RandomMain(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    switch (*state) {
    case RANDOM_LINK_PHASE_FADE_IN:
        GFL_FadeSet((FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK), 16, 0, 0);
        *state = RANDOM_LINK_PHASE_WAIT_FADE_IN;
        break;
    case RANDOM_LINK_PHASE_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            *state = RANDOM_LINK_PHASE_RUN_TASK;
        }
        break;
    case RANDOM_LINK_PHASE_RUN_TASK:
        WifiBattleMatchTask_Run(link->task);
        if (WifiBattleMatchTask_IsEnded(link->task)) {
            *state = RANDOM_LINK_PHASE_FADE_OUT;
        }
        if (link->net != NULL) {
            WifiBattleMatchNet_Nop(link->net);
        }
        break;
    case RANDOM_LINK_PHASE_FADE_OUT:
        GFL_FadeSet((FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK), 0, 16, 0);
        *state = RANDOM_LINK_PHASE_WAIT_FADE_OUT;
        break;
    case RANDOM_LINK_PHASE_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            return TRUE;
        }
        break;
    }
    WifiBattleMatchGraphic_Update(link->graphic);
    PrintQueue_Main(link->printQueue);
    WifiBattleMatchMsgBox_Update(link->msgBox);
    WifiBattleMatchLinkIcon_Update(link->linkIcon);
    WifiBattleMatchRes_Update(link->res);
    if (link->card != NULL) {
        WifiBattleMatchCard_Flush(link->card, link->printQueue);
    }
    if (link->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Flush(link->partnerCard, link->printQueue);
    }
    return FALSE;
}

static void WifiBattleMatchLink_TaskStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;

    switch (param->result) {
    case WBM_LINK_RESULT_INITIAL:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
        return;
    case WBM_LINK_RESULT_CANCELLED:
        WifiBattleMatchNet_CancelMatch(link->net);
        // fall through
    case WBM_LINK_RESULT_BATTLE_FINISHED:
        if (param->mode == LINK_MODE_RATING) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingResult);
            return;
        } else if (param->mode == LINK_MODE_FREE) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeResult);
            return;
        }
        break;
    case WBM_LINK_RESULT_RESULT_SHOWN:
        if (param->mode == LINK_MODE_RATING) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingAfterBattle);
            return;
        } else if (param->mode == LINK_MODE_FREE) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeAfterBattle);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskSelectMode(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    s32 selected;

    switch (*seq) {
    case RANDOM_SELECT_MODE_HIDE_LINK_ICON:
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        *seq = RANDOM_SELECT_MODE_PROMPT_MODE;
        break;
    case RANDOM_SELECT_MODE_PROMPT_MODE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WhichModeWillChallenge,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RANDOM_SELECT_MODE_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RANDOM_SELECT_MODE_CREATE_MODE_MENU);
        break;
    case RANDOM_SELECT_MODE_CREATE_MODE_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_MODE);
        *seq = RANDOM_SELECT_MODE_WAIT_MODE_MENU;
        break;
    case RANDOM_SELECT_MODE_WAIT_MODE_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case RANDOM_MODE_SELECTION_FREE:
            param->mode = LINK_MODE_FREE;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeStart);
            break;
        case RANDOM_MODE_SELECTION_RATING:
            param->mode = LINK_MODE_RATING;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingStart);
            break;
        case RANDOM_MODE_SELECTION_QUIT:
            param->mode = LINK_MODE_QUIT;
            *seq = RANDOM_SELECT_MODE_PROMPT_QUIT;
            break;
        }
        break;
    case RANDOM_SELECT_MODE_PROMPT_QUIT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillQuitRandomMatchup,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RANDOM_SELECT_MODE_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RANDOM_SELECT_MODE_CREATE_QUIT_MENU);
        break;
    case RANDOM_SELECT_MODE_CREATE_QUIT_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = RANDOM_SELECT_MODE_WAIT_QUIT_MENU;
        break;
    case RANDOM_SELECT_MODE_WAIT_QUIT_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
            break;
        case WBM_MENU_NO:
            *seq = RANDOM_SELECT_MODE_PROMPT_MODE;
            break;
        }
        break;
    case RANDOM_SELECT_MODE_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;

    switch (*seq) {
    case RATING_START_CHECK_GAME_SYNC_ID:
        if (DreamWorldSave_GetIDShown(getDreamWorldStuffAddress(GameData_GetSaveControl(param->param->gameData)))) {
            *seq = RATING_START_SHOW_RATING_MODE;
        } else {
            *seq = RATING_START_SHOW_UNREGISTERED_MESSAGE;
        }
        break;
    case RATING_START_SHOW_RATING_MODE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_RandomMatchupRatingMode_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_START_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_START_SHOW_DOWNLOAD_MESSAGE);
        break;
    case RATING_START_SHOW_DOWNLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = RATING_START_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_START_REQUEST_ACCOUNT);
        break;
    case RATING_START_REQUEST_ACCOUNT:
        MI_CpuFill8(param->account, 0, sizeof(WifiBattleMatchLinkAccount));
        WifiBattleMatchNet_StartAccountRequest(link->net, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = RATING_START_WAIT_ACCOUNT;
        break;
    case RATING_START_WAIT_ACCOUNT:
        switch (WifiBattleMatchNet_AccountRequestMain(link->net)) {
        case WBM_NET_RUNNING:
            break;
        case WBM_NET_SUCCESS:
            WifiBattleMatchNet_GetAccountValues(link->net, link->param->account);
            *seq = RATING_START_CHECK_REGISTRATION;
            break;
        case WBM_NET_SERVER_ERROR:
            MI_CpuFill8(link->param->account, 0, sizeof(WifiBattleMatchLinkAccount));
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        case WBM_NET_FAILED:
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
            case WBM_RESULT_FAILED:
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
                break;
            case WBM_RESULT_DISCONNECTED:
                WifiBattleMatchLink_HideRankCard(link);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
                break;
            }
            break;
        }
        break;
    case RATING_START_CHECK_REGISTRATION:
        if (param->account->registered != 0) {
            *seq = RATING_START_START_TEAM_CHECK;
        } else {
            *seq = RATING_START_SHOW_UNREGISTERED_MESSAGE;
        }
        break;
    case RATING_START_SHOW_UNREGISTERED_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_GameSyncIdHas,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_START_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_START_END);
        break;
    case RATING_START_START_TEAM_CHECK:
        WifiBattleMatchLink_StartSubTask(link, WifiBattleMatchLink_TaskSignTeam);
        *seq = RATING_START_WAIT_TEAM_CHECK;
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case RATING_START_WAIT_TEAM_CHECK: {
        u32 signResult = WifiBattleMatchLink_UpdateSubTask(link);

        if (signResult == WBM_TEAM_CHECK_RESULT_VALID) {
            *seq = RATING_START_CONNECT;
        } else if (signResult == WBM_TEAM_CHECK_RESULT_INVALID) {
            *seq = RATING_START_SHOW_INVALID_TEAM;
        } else if (signResult == WBM_TEAM_CHECK_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
        }
        if (signResult == SUB_TASK_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    }
    case RATING_START_SHOW_INVALID_TEAM:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereProblemPokemonThey_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_START_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_START_END);
        break;
    case RATING_START_END:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
        break;
    case RATING_START_CONNECT:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingConnect);
        break;
    case RATING_START_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingConnect(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    BOOL ready;

    switch (*seq) {
    case RATING_CONNECT_SHOW_DOWNLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, RATING_CONNECT_DOWNLOAD_RATING);
        *seq = RATING_CONNECT_WAIT_MESSAGE;
        break;
    case RATING_CONNECT_DOWNLOAD_RATING:
        if (*link->param->recordRequestCooldown != 0) {
            break;
        }
        WifiBattleMatchNet_StartRead(link->net, -1, WBM_READ_ARENA, param->ratings);
        *seq = RATING_CONNECT_WAIT_RATING;
        break;
    case RATING_CONNECT_WAIT_RATING:
        ready = WifiBattleMatchNet_ReadMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 1800;
            link->recordChanged = WifiBattleMatchLink_UpdateRecord(param->record, param->param->battleType, link);
            WifiBattleMatchLink_UpdatePlayerRecord(param->player, link);
            *link->param->recordSaveIndex = link->param->ratings->recordSaveIndex;
            *seq = RATING_CONNECT_CHECK_RECORD_ID;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            *link->param->recordRequestCooldown = 1800;
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            *link->param->recordRequestCooldown = 1800;
            break;
        }
        break;
    case RATING_CONNECT_CHECK_RECORD_ID:
        if (WifiBattleMatchNet_GetRecordId(link->net) == 0) {
            WifiBattleMatchNet_StartCreateRecord(link->net);
            *seq = RATING_CONNECT_WAIT_RECORD_CREATION;
        } else {
            *seq = RATING_CONNECT_CHECK_CARD_REFRESH;
        }
        break;
    case RATING_CONNECT_WAIT_RECORD_CREATION: {
        u32 done = WifiBattleMatchNet_CreateRecordMain(link->net);

        if (done == WBM_NET_SUCCESS) {
            *seq = RATING_CONNECT_DOWNLOAD_RATING;
        }
        if (done == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            *link->param->recordRequestCooldown = 1800;
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            *link->param->recordRequestCooldown = 1800;
            break;
        }
        break;
    }
    case RATING_CONNECT_CHECK_CARD_REFRESH:
        if (link->card == NULL) {
            *seq = RATING_CONNECT_CREATE_CARD;
        } else if (link->recordChanged) {
            *seq = RATING_CONNECT_DELAY_CARD_REFRESH;
        } else {
            *seq = RATING_CONNECT_SEARCH;
        }
        link->recordChanged = FALSE;
        break;
    case RATING_CONNECT_DELAY_CARD_REFRESH:
        *seq = RATING_CONNECT_HIDE_OLD_CARD;
        break;
    case RATING_CONNECT_HIDE_OLD_CARD:
        if (WifiBattleMatchLink_IsRankCardHidden(link)) {
            WifiBattleMatchLink_DeleteRankCard(link);
            *seq = RATING_CONNECT_CREATE_CARD;
        }
        break;
    case RATING_CONNECT_CREATE_CARD:
        WifiBattleMatchLink_CreateRankCard(link, 0);
        *seq = RATING_CONNECT_WAIT_CARD_SHOWN;
        break;
    case RATING_CONNECT_WAIT_CARD_SHOWN:
        if (WifiBattleMatchLink_IsRankCardShown(link)) {
            *seq = RATING_CONNECT_SEARCH;
        }
        break;
    case RATING_CONNECT_SEARCH:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingSearch);
        break;
    case RATING_CONNECT_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static inline u32 WifiBattleMatchLinkPlayer_GetSize(void) {
    return PokeParty_GetSaveDataSize() + sizeof(WifiBattleMatchPlayer);
}

static void WifiBattleMatchLink_TaskRatingSearch(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchPlayer *otherPlayer;
    void *received;
    WifiBattleMatchNetKeys condition;
    BOOL countIdle = FALSE;
    BOOL canCancel = FALSE;
    BOOL canDisconnect;
    BOOL timedOut;
    BOOL noTerminator;
    BOOL opponentNameRejected;
    u32 cancel;
    u32 ret;
    u32 fixCount;
    s32 selected;
    int i;

    if ((s32)*seq >= RATING_SEARCH_WAIT_MATCH && (s32)*seq < RATING_SEARCH_SYNC_VALIDATED_PLAYERS) {
        canCancel = TRUE;
    }
    canDisconnect = (s32)*seq < RATING_SEARCH_START_NAME_CHECK || (s32)*seq > RATING_SEARCH_WAIT_NAME_CHECK;
    cancel = WifiBattleMatchLink_UpdateCancel(link, canCancel, canDisconnect);
    if (cancel == CANCEL_CONFIRMED) {
        *seq = RATING_SEARCH_PROMPT_QUIT;
        return;
    }
    switch (*seq) {
    case RATING_SEARCH_SHOW_SEARCH_MESSAGE:
        link->cancelSeq = RANDOM_SEARCH_CANCEL_IDLE;
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent,
                                    WIFIBATTLEMATCH_MSG_QUEUE);
        *seq = RATING_SEARCH_START_MATCH;
        break;
    case RATING_SEARCH_START_MATCH:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        default:
            WifiBattleMatchLink_FillCondition(&condition, link);
            WifiBattleMatchNet_StartMatch(link->net, WBM_MATCH_MODE_RATING, 1, param->param->battleType, &condition);
            *seq = RATING_SEARCH_WAIT_MATCH;
            break;
        }
        break;
    case RATING_SEARCH_WAIT_MATCH:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            WifiBattleMatchLink_ResetCancel(link);
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        default:
            ret = WifiBattleMatchNet_MatchMain(link->net);
            if (ret == WBM_NET_SUCCESS) {
                link->idleTimer = 0;
                *seq = RATING_SEARCH_SEND_PLAYER_INFO;
            } else if (ret == WBM_NET_FAILED) {
                *seq = RATING_SEARCH_START_MATCH;
            }
            break;
        }
        break;
    case RATING_SEARCH_SEND_PLAYER_INFO:
        if (cancel != CANCEL_NONE) {
            break;
        }
        if (WifiBattleMatchNet_SendPlayerInfo(link->net, param->player)) {
            link->idleTimer = 0;
            *seq = RATING_SEARCH_RECEIVE_PLAYER_INFO;
        }
        countIdle = TRUE;
        break;
    case RATING_SEARCH_RECEIVE_PLAYER_INFO:
        if (WifiBattleMatchNet_TakePlayerInfo(link->net, &received)) {
            otherPlayer = param->otherPlayer;
            MI_CpuCopy8(received, otherPlayer, WifiBattleMatchLinkPlayer_GetSize());
            fixCount = 0;
            noTerminator = TRUE;
            for (i = 0; i < 8; i++) {
                if (otherPlayer->info.name[i] == GFL_StrBufGetTerminator()) {
                    noTerminator = FALSE;
                }
            }
            if (noTerminator) {
                MsgData *msgData;
                StrBuf *name;

                fixCount++;
                msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING,
                                             HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK));
                name = GFL_MsgDataLoadStrbufNew(msgData, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK));
                GFL_StrBufStoreString(name, otherPlayer->info.name, 8);
                GFL_StrBufFree(name);
                GFL_MsgDataFree(msgData);
            }
            if (otherPlayer->info.gender != GENDER_MALE && otherPlayer->info.gender != GENDER_FEMALE) {
                otherPlayer->info.gender = GENDER_MALE;
                fixCount++;
            }
            if (otherPlayer->info.unionView >= 16) {
                otherPlayer->info.unionView = otherPlayer->info.gender == GENDER_MALE ? 0 : 8;
                fixCount++;
            }
            if (!Country_IsValidPlace(otherPlayer->info.country, otherPlayer->info.province,
                                      otherPlayer->info.region)) {
                otherPlayer->info.country = 0;
                otherPlayer->info.province = 0;
                fixCount++;
            }
            if (!PMSData_IsValid(&otherPlayer->sentence, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK))) {
                PMSData_Init(&otherPlayer->sentence, 1);
                fixCount++;
            }
            if (fixCount == 0) {
                getPalPadFriendListAddress(GameData_GetSaveControl(param->param->gameData));
            }
            link->idleTimer = 0;
            *seq = RATING_SEARCH_CHECK_SIGNATURE;
        }
        countIdle = TRUE;
        break;
    case RATING_SEARCH_CHECK_SIGNATURE:
        if (WifiBattleMatchLink_CheckSignature(param->otherPlayer, HEAPID_WIFIBATTLEMATCH_LINK)) {
            *seq = RATING_SEARCH_START_NAME_CHECK;
        } else {
            WifiBattleMatchNet_CancelMatch(link->net);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingSearch);
        }
        break;
    case RATING_SEARCH_START_NAME_CHECK:
        WifiBattleMatchNet_StartNameCheck(link->net, &param->otherPlayer->info, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = RATING_SEARCH_WAIT_NAME_CHECK;
        break;
    case RATING_SEARCH_WAIT_NAME_CHECK:
        if (WifiBattleMatchNet_NameCheckMain(link->net, &opponentNameRejected)) {
            *link->param->opponentNameRejected = opponentNameRejected;
            *seq = RATING_SEARCH_SYNC_VALIDATED_PLAYERS;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
            WifiBattleMatchLink_ResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case RATING_SEARCH_SYNC_VALIDATED_PLAYERS:
        if (cancel != CANCEL_NONE) {
            break;
        }
        WifiBattleMatchNet_StartSync(link->net, WBM_SYNC_VALIDATED_PLAYERS);
        link->idleTimer = 0;
        *seq = RATING_SEARCH_WAIT_VALIDATED_PLAYERS_SYNC;
        break;
    case RATING_SEARCH_WAIT_VALIDATED_PLAYERS_SYNC:
        if (!WifiBattleMatchNet_TakeCancelSearch(link->net) && WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = RATING_SEARCH_SYNC_MATCH_READY;
        }
        countIdle = TRUE;
        break;
    case RATING_SEARCH_SYNC_MATCH_READY:
        WifiBattleMatchNet_StartSync(link->net, WBM_SYNC_MATCH_READY);
        link->idleTimer = 0;
        *seq = RATING_SEARCH_WAIT_MATCH_READY_SYNC;
        break;
    case RATING_SEARCH_WAIT_MATCH_READY_SYNC:
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = RATING_SEARCH_SHOW_OPPONENT_FOUND;
        }
        countIdle = TRUE;
        break;
    case RATING_SEARCH_SHOW_OPPONENT_FOUND:
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_74);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_OpponentHasBeenFound,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_SEARCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_SEARCH_WAIT_OPPONENT_FOUND_DELAY);
        break;
    case RATING_SEARCH_WAIT_OPPONENT_FOUND_DELAY:
        if (link->timer++ > 120) {
            link->timer = 0;
            WifiBattleMatchLink_CreatePartnerCard(link, param->otherPlayer);
            GFL_BGSysSetBGEnabled(0, FALSE);
            *seq = RATING_SEARCH_WAIT_PARTNER_CARD;
        }
        break;
    case RATING_SEARCH_WAIT_PARTNER_CARD:
        if (WifiBattleMatchLink_IsPartnerCardDone(link)) {
            *seq = RATING_SEARCH_WAIT_PARTNER_CARD_DELAY;
        }
        break;
    case RATING_SEARCH_WAIT_PARTNER_CARD_DELAY:
        if (link->timer++ != 0) {
            link->timer = 0;
            *seq = RATING_SEARCH_START_SESSION_REPORT;
        }
        break;
    case RATING_SEARCH_START_SESSION_REPORT:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK_DWC, 0x60000);
        DWCRapCommon_SetSubHeapEx(HEAPID_DEVICE_ALLOC, 0x60000, 1000, HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        DWCRapCommon_SetSubHeapOwner(HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        WifiBattleMatchNet_StartReport(link->net, WBM_REPORT_MODE_START, WBM_MATCH_MODE_RATING, 0, NULL, 0);
        link->exchangeDone = WBM_NET_RUNNING;
        link->exchangeResult = 0;
        *seq = RATING_SEARCH_WAIT_SESSION_REPORT;
        break;
    case RATING_SEARCH_WAIT_SESSION_REPORT:
        if (link->exchangeDone == WBM_NET_RUNNING) {
            link->exchangeDone = WifiBattleMatchNet_ReportMain(link->net, &link->exchangeResult);
        }
        if (link->exchangeDone == WBM_NET_SUCCESS) {
            *seq = RATING_SEARCH_END_REPORT_HEAP;
        }
        if (link->exchangeDone != WBM_NET_RUNNING) {
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
            case WBM_RESULT_DISCONNECTED:
                WifiBattleMatchNet_CancelMatch(link->net);
                // fall through
            case WBM_RESULT_FAILED:
            case WBM_RESULT_TIMEOUT:
                DWCRapCommon_EndSubHeap();
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
                break;
            }
        }
        break;
    case RATING_SEARCH_END_REPORT_HEAP:
        DWCRapCommon_EndSubHeap();
        *seq = RATING_SEARCH_SHOW_TEAM_SELECTION_MESSAGE;
        break;
    case RATING_SEARCH_SHOW_TEAM_SELECTION_MESSAGE:
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PleaseChoosePokemon,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = RATING_SEARCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_SEARCH_WAIT_TEAM_SELECTION_DELAY);
        break;
    case RATING_SEARCH_WAIT_TEAM_SELECTION_DELAY:
        if (link->timer++ > 60) {
            link->timer = 0;
            param->action = WIFIBATTLEMATCH_LINK_ACTION_TEAM_LIST;
            WifiBattleMatchTask_End(task);
        }
        break;
    case RATING_SEARCH_CANCEL_SESSION:
        WifiBattleMatchNet_CancelMatch(link->net);
        param->result = WBM_LINK_RESULT_CANCELLED;
        if (link->exchangeResult != 0) {
            link->hadExchangeResult = TRUE;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingResult);
        } else {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingAfterBattle);
        }
        break;
    case RATING_SEARCH_PROMPT_QUIT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillQuitRandomMatchup_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_SEARCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_SEARCH_CREATE_QUIT_MENU);
        break;
    case RATING_SEARCH_CREATE_QUIT_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = RATING_SEARCH_WAIT_QUIT_MENU;
        break;
    case RATING_SEARCH_WAIT_QUIT_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingQuitSave);
            break;
        case WBM_MENU_NO:
            *seq = RATING_SEARCH_PROMPT_CONTINUE;
            break;
        }
        break;
    case RATING_SEARCH_PROMPT_CONTINUE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WantContinueBattling,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_SEARCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_SEARCH_CREATE_CONTINUE_MENU);
        break;
    case RATING_SEARCH_CREATE_CONTINUE_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = RATING_SEARCH_WAIT_CONTINUE_MENU;
        break;
    case RATING_SEARCH_WAIT_CONTINUE_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = RATING_SEARCH_SHOW_SEARCH_MESSAGE;
            break;
        case WBM_MENU_NO:
            *seq = RATING_SEARCH_PROMPT_QUIT;
            break;
        }
        break;
    case RATING_SEARCH_SEND_CANCEL_COMMAND:
        if (WifiBattleMatchNet_SendCancelSearch(link->net)) {
            *seq = RATING_SEARCH_CANCEL_AND_RETRY;
        }
        break;
    case RATING_SEARCH_CANCEL_AND_RETRY:
        WifiBattleMatchNet_CancelMatch(link->net);
        *seq = RATING_SEARCH_SHOW_SEARCH_MESSAGE;
        break;
    case RATING_SEARCH_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
    if (countIdle) {
        timedOut = FALSE;
        if (link->idleTimer++ > 2400) {
            link->idleTimer = 0;
            timedOut = TRUE;
        }
        if (WifiBattleMatchNet_TakeCancelSearch(link->net)) {
            timedOut = TRUE;
        }
        if (timedOut) {
            *seq = RATING_SEARCH_SEND_CANCEL_COMMAND;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
            WifiBattleMatchLink_ResetCancel(link);
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
    }
}

static void WifiBattleMatchLink_TaskRatingResult(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchPlayer *otherPlayer;
    WifiBattleMatchLinkReportBuf reportBuf;
    RivalDataSave *rivalData;
    u32 ready;

    switch (*seq) {
    case RATING_RESULT_CREATE_REPORT_HEAP:
        if (param->result == WBM_LINK_RESULT_CANCELLED && link->hadExchangeResult == 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingAfterBattle);
            return;
        }
        link->hadExchangeResult = 0;
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK_DWC, 0x60000);
        DWCRapCommon_SetSubHeapEx(HEAPID_DEVICE_ALLOC, 0x60000, 1000, HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        DWCRapCommon_SetSubHeapOwner(HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        *seq = RATING_RESULT_SHOW_REPORT_MESSAGE;
        break;
    case RATING_RESULT_SHOW_REPORT_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = RATING_RESULT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_RESULT_START_REPORT);
        break;
    case RATING_RESULT_START_REPORT:
        WifiBattleMatchNet_StartReport(link->net, WBM_REPORT_MODE_RESULT, WBM_MATCH_MODE_RATING,
                                       param->param->battleType, param->battleResult,
                                       param->result == WBM_LINK_RESULT_CANCELLED);
        link->exchangeDone = WBM_NET_RUNNING;
        link->exchangeResult = 0;
        *seq = RATING_RESULT_WAIT_REPORT;
        break;
    case RATING_RESULT_WAIT_REPORT:
        if (link->exchangeDone == WBM_NET_RUNNING) {
            link->exchangeDone = WifiBattleMatchNet_ReportMain(link->net, &link->exchangeResult);
            if (link->exchangeDone != WBM_NET_RUNNING && link->exchangeResult != 0) {
                WifiBattleMatch_RecordResult(param->param->gameData, param->battleResult->result);
            }
        }
        if (link->exchangeDone == WBM_NET_SUCCESS) {
            param->battleResult->otherCommandCheckError = WifiBattleMatchNet_GetPeerCommandCheckError(link->net);
            *seq = RATING_RESULT_FINISH_REPORT;
        }
        if (link->exchangeDone == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchNet_CancelMatch(link->net);
            // fall through
        case WBM_RESULT_FAILED:
        case WBM_RESULT_TIMEOUT:
            DWCRapCommon_EndSubHeap();
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case RATING_RESULT_FINISH_REPORT:
        DWCRapCommon_EndSubHeap();
        if (param->result == WBM_LINK_RESULT_CANCELLED) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingAfterBattle);
            return;
        }
        *seq = RATING_RESULT_SEND_JOIN_AVENUE_PERSON;
        break;
    case RATING_RESULT_SEND_JOIN_AVENUE_PERSON:
        otherPlayer = param->otherPlayer;
        if (PlayerInfo_GetVersion(&otherPlayer->info) == VERSION_WHITE2 ||
            PlayerInfo_GetVersion(&otherPlayer->info) == VERSION_BLACK2) {
            if (WifiBattleMatchNet_SendPerson(link->net)) {
                *seq = RATING_RESULT_RECEIVE_JOIN_AVENUE_PERSON;
            }
        } else {
            *seq = RATING_RESULT_RECORD_MET_TRAINER;
        }
        break;
    case RATING_RESULT_RECEIVE_JOIN_AVENUE_PERSON:
        if (WifiBattleMatchNet_TakePerson(link->net)) {
            *seq = RATING_RESULT_RECORD_MET_TRAINER;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchNet_CancelMatch(link->net);
            // fall through
        case WBM_RESULT_FAILED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case RATING_RESULT_RECORD_MET_TRAINER:
        rivalData = getHollow_RivalData(GameData_GetSaveControl(param->param->gameData));
        Misc_AddMetTrainer(rivalData, getIDAsUInt(&param->otherPlayer->info));
        *seq = RATING_RESULT_WAIT_DISCONNECT;
        WifiBattleMatchNet_ResetEnd(link->net);
        break;
    case RATING_RESULT_WAIT_DISCONNECT:
        if (WifiBattleMatchNet_EndMain(link->net)) {
            if (*link->param->recordRequestCooldown == 0) {
                *seq = RATING_RESULT_UPLOAD_RESULT;
            } else {
                WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WaitingServerDontTurn,
                                            WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
                *seq = RATING_RESULT_WAIT_MESSAGE;
                WifiBattleMatchTask_SetReturnSeq(task, RATING_RESULT_WAIT_WRITE_COOLDOWN);
            }
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            *seq = RATING_RESULT_WAIT_WRITE_COOLDOWN;
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case RATING_RESULT_WAIT_WRITE_COOLDOWN:
        if (*link->param->recordRequestCooldown == 0) {
            *seq = RATING_RESULT_UPLOAD_RESULT;
        }
        break;
    case RATING_RESULT_UPLOAD_RESULT:
        MI_CpuFill8(&reportBuf, 0, sizeof(reportBuf));
        reportBuf.report = *link->param->report;
        reportBuf.recordSaveIndex = *link->param->recordSaveIndex;
        WifiBattleMatchNet_StartWrite(link->net, WBM_WRITE_RECORD_DATA, &reportBuf);
        *seq = RATING_RESULT_WAIT_RESULT_UPLOAD;
        break;
    case RATING_RESULT_WAIT_RESULT_UPLOAD:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 1800;
            *seq = RATING_RESULT_SHOW_RESULTS_SENT;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            *link->param->recordRequestCooldown = 1800;
            return;
        case WBM_RESULT_DISCONNECTED:
            *link->param->recordRequestCooldown = 1800;
            *seq = RATING_RESULT_SHOW_RESULTS_SENT;
            return;
        }
        break;
    case RATING_RESULT_SHOW_RESULTS_SENT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleResultsWereSent,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_RESULT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_RESULT_SHOW_SAVE_MESSAGE);
        break;
    case RATING_RESULT_SHOW_SAVE_MESSAGE:
        WifiBattleMatch_RecordBattleTime(param->param->gameData, param->battleResult->result);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = RATING_RESULT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_RESULT_START_SAVE);
        break;
    case RATING_RESULT_START_SAVE:
        GameData_StartSave(param->param->gameData);
        *seq = RATING_RESULT_WAIT_SAVE;
        break;
    case RATING_RESULT_WAIT_SAVE:
        switch (GameData_UpdateSave(param->param->gameData)) {
        case SAVE_RESULT_CONTINUE:
        case SAVE_RESULT_LAST:
            break;
        case SAVE_RESULT_OK:
        case SAVE_RESULT_NG:
            param->action = WIFIBATTLEMATCH_LINK_ACTION_RESULT;
            WifiBattleMatchTask_End(task);
            break;
        }
        break;
    case RATING_RESULT_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    switch (*seq) {
    case RATING_AFTER_BATTLE_CHECK_CONNECTION:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_OK:
            *seq = RATING_AFTER_BATTLE_CONTINUE;
            break;
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case RATING_AFTER_BATTLE_CONTINUE:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
        break;
    case RATING_AFTER_BATTLE_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingContinue(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    s32 selected;

    switch (*seq) {
    case RATING_CONTINUE_CHECK_CONNECTION:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_OK:
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            *seq = RATING_CONTINUE_PROMPT_CONTINUE;
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case RATING_CONTINUE_PROMPT_CONTINUE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WantContinueBattling,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_CONTINUE_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_CONTINUE_CREATE_MENU);
        break;
    case RATING_CONTINUE_CREATE_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = RATING_CONTINUE_WAIT_MENU;
        break;
    case RATING_CONTINUE_WAIT_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingConnect);
            break;
        case WBM_MENU_NO:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingQuitConfirm);
            break;
        }
        break;
    case RATING_CONTINUE_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingQuitConfirm(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    s32 selected;

    switch (*seq) {
    case RATING_QUIT_PROMPT_QUIT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillQuitRandomMatchup_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = RATING_QUIT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_QUIT_CREATE_MENU);
        break;
    case RATING_QUIT_CREATE_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = RATING_QUIT_WAIT_MENU;
        break;
    case RATING_QUIT_WAIT_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            WifiBattleMatchNet_CancelMatch(link->net);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingQuitSave);
            break;
        case WBM_MENU_NO:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRatingContinue);
            break;
        }
        break;
    case RATING_QUIT_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRatingQuitSave(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;

    switch (*seq) {
    case RATING_QUIT_SAVE_SHOW_SAVE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, RATING_QUIT_SAVE_START_SAVE);
        *seq = RATING_QUIT_SAVE_WAIT_MESSAGE;
        break;
    case RATING_QUIT_SAVE_START_SAVE:
        GameData_StartSave(param->param->gameData);
        *seq = RATING_QUIT_SAVE_WAIT_SAVE;
        break;
    case RATING_QUIT_SAVE_WAIT_SAVE:
        if (GameData_UpdateSave(param->param->gameData) == SAVE_RESULT_OK) {
            *seq = RATING_QUIT_SAVE_SHOW_UPLOAD_MESSAGE;
        }
        break;
    case RATING_QUIT_SAVE_SHOW_UPLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = RATING_QUIT_SAVE_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, RATING_QUIT_SAVE_UPLOAD_LOGOUT);
        break;
    case RATING_QUIT_SAVE_UPLOAD_LOGOUT:
        if (*link->param->recordRequestCooldown == 0) {
            WifiBattleMatchNet_StartWrite(link->net, WBM_WRITE_PROFILE, NULL);
            *seq = RATING_QUIT_SAVE_WAIT_LOGOUT_UPLOAD;
        }
        break;
    case RATING_QUIT_SAVE_WAIT_LOGOUT_UPLOAD:
        if (WifiBattleMatchNet_WriteMain(link->net)) {
            *link->param->recordRequestCooldown = 1800;
            *seq = RATING_QUIT_SAVE_END;
            WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE);
        }
        break;
    case RATING_QUIT_SAVE_END:
        WifiBattleMatchNet_CancelMatch(link->net);
        param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
        break;
    case RATING_QUIT_SAVE_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    u32 signResult;

    switch (*seq) {
    case FREE_START_SHOW_MODE_AND_CREATE_CARD:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_RandomMatchupFreeMode_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchLink_CreateRankCard(link, 1);
        *seq = FREE_START_WAIT_CARD_SHOWN;
        break;
    case FREE_START_WAIT_CARD_SHOWN:
        if (WifiBattleMatchLink_IsRankCardShown(link)) {
            *seq = FREE_START_WAIT_MODE_MESSAGE;
        }
        break;
    case FREE_START_WAIT_MODE_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            *seq = FREE_START_SHOW_DOWNLOAD_MESSAGE;
        }
        break;
    case FREE_START_SHOW_DOWNLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = FREE_START_START_TEAM_CHECK;
        break;
    case FREE_START_START_TEAM_CHECK:
        WifiBattleMatchLink_StartSubTask(link, WifiBattleMatchLink_TaskSignTeam);
        *seq = FREE_START_WAIT_TEAM_CHECK;
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case FREE_START_WAIT_TEAM_CHECK:
        signResult = WifiBattleMatchLink_UpdateSubTask(link);
        if (signResult == WBM_TEAM_CHECK_RESULT_VALID) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeSearch);
        } else if (signResult == WBM_TEAM_CHECK_RESULT_INVALID) {
            *seq = FREE_START_SHOW_INVALID_TEAM;
        } else if (signResult == WBM_TEAM_CHECK_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
        }
        if (signResult == SUB_TASK_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLink_HideRankCard(link);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case FREE_START_SHOW_INVALID_TEAM:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereProblemPokemonThey_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = FREE_START_WAIT_INVALID_TEAM_MESSAGE;
        break;
    case FREE_START_WAIT_INVALID_TEAM_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeSearch(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchPlayer *otherPlayer;
    void *received;
    WifiBattleMatchNetKeys condition;
    BOOL countIdle = FALSE;
    BOOL canCancel = FALSE;
    BOOL canDisconnect;
    BOOL timedOut;
    BOOL noTerminator;
    BOOL opponentNameRejected;
    u32 cancel;
    u32 ret;
    int i;

    if ((s32)*seq >= FREE_SEARCH_WAIT_MATCH && (s32)*seq < FREE_SEARCH_SYNC_VALIDATED_PLAYERS) {
        canCancel = TRUE;
    }
    canDisconnect = (s32)*seq < FREE_SEARCH_START_NAME_CHECK || (s32)*seq > FREE_SEARCH_WAIT_NAME_CHECK;
    cancel = WifiBattleMatchLink_UpdateCancel(link, canCancel, canDisconnect);
    if (cancel == CANCEL_CONFIRMED) {
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeQuitConfirm);
        return;
    }
    switch (*seq) {
    case FREE_SEARCH_UPDATE_PLAYER_RECORD:
        WifiBattleMatchLink_UpdatePlayerRecord(param->player, link);
        *seq = FREE_SEARCH_SHOW_SEARCH_MESSAGE;
        break;
    case FREE_SEARCH_SHOW_SEARCH_MESSAGE:
        link->cancelSeq = RANDOM_SEARCH_CANCEL_IDLE;
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent,
                                    WIFIBATTLEMATCH_MSG_QUEUE);
        *seq = FREE_SEARCH_START_MATCH;
        break;
    case FREE_SEARCH_START_MATCH:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        default:
            WifiBattleMatchLink_FillCondition(&condition, link);
            WifiBattleMatchNet_StartMatch(link->net, WBM_MATCH_MODE_FREE, 0, param->param->battleType, &condition);
            *seq = FREE_SEARCH_WAIT_MATCH;
            break;
        }
        break;
    case FREE_SEARCH_WAIT_MATCH:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskSelectMode);
            WifiBattleMatchLink_ResetCancel(link);
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        default:
            ret = WifiBattleMatchNet_MatchMain(link->net);
            if (ret == WBM_NET_SUCCESS) {
                link->idleTimer = 0;
                *seq = FREE_SEARCH_SYNC_CONNECTED_PLAYERS;
            } else if (ret == WBM_NET_FAILED) {
                *seq = FREE_SEARCH_START_MATCH;
            }
            break;
        }
        break;
    case FREE_SEARCH_SYNC_CONNECTED_PLAYERS:
        WifiBattleMatchNet_StartSync(link->net, WBM_SYNC_CONNECTED_PLAYERS);
        *seq = FREE_SEARCH_WAIT_CONNECTED_PLAYERS_SYNC;
        link->idleTimer = 0;
        break;
    case FREE_SEARCH_WAIT_CONNECTED_PLAYERS_SYNC:
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            link->idleTimer = 0;
            *seq = FREE_SEARCH_SEND_PLAYER_INFO;
        }
        countIdle = TRUE;
        break;
    case FREE_SEARCH_SEND_PLAYER_INFO:
        if (cancel != CANCEL_NONE) {
            break;
        }
        if (WifiBattleMatchNet_SendPlayerInfo(link->net, param->player)) {
            link->idleTimer = 0;
            *seq = FREE_SEARCH_RECEIVE_PLAYER_INFO;
        }
        countIdle = TRUE;
        break;
    case FREE_SEARCH_RECEIVE_PLAYER_INFO:
        if (WifiBattleMatchNet_TakePlayerInfo(link->net, &received)) {
            otherPlayer = param->otherPlayer;
            MI_CpuCopy8(received, otherPlayer, WifiBattleMatchLinkPlayer_GetSize());
            noTerminator = TRUE;
            for (i = 0; i < 8; i++) {
                if (otherPlayer->info.name[i] == GFL_StrBufGetTerminator()) {
                    noTerminator = FALSE;
                }
            }
            if (noTerminator) {
                MsgData *msgData;
                StrBuf *name;

                msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING,
                                             HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK));
                name = GFL_MsgDataLoadStrbufNew(msgData, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK));
                GFL_StrBufStoreString(name, otherPlayer->info.name, 8);
                GFL_StrBufFree(name);
                GFL_MsgDataFree(msgData);
            }
            if (otherPlayer->info.gender != GENDER_MALE && otherPlayer->info.gender != GENDER_FEMALE) {
                otherPlayer->info.gender = GENDER_MALE;
            }
            if (otherPlayer->info.unionView >= 16) {
                otherPlayer->info.unionView = otherPlayer->info.gender == GENDER_MALE ? 0 : 8;
            }
            if (!Country_IsValidPlace(otherPlayer->info.country, otherPlayer->info.province,
                                      otherPlayer->info.region)) {
                otherPlayer->info.country = 0;
                otherPlayer->info.province = 0;
            }
            if (!PMSData_IsValid(&otherPlayer->sentence, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH_LINK))) {
                PMSData_Init(&otherPlayer->sentence, 1);
            }
            link->idleTimer = 0;
            *seq = FREE_SEARCH_CHECK_SIGNATURE;
        }
        countIdle = TRUE;
        break;
    case FREE_SEARCH_CHECK_SIGNATURE:
        if (WifiBattleMatchLink_CheckSignature(param->otherPlayer, HEAPID_WIFIBATTLEMATCH_LINK)) {
            *seq = FREE_SEARCH_START_NAME_CHECK;
        } else {
            WifiBattleMatchNet_CancelMatch(link->net);
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeSearch);
        }
        break;
    case FREE_SEARCH_START_NAME_CHECK:
        WifiBattleMatchNet_StartNameCheck(link->net, &param->otherPlayer->info, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = FREE_SEARCH_WAIT_NAME_CHECK;
        break;
    case FREE_SEARCH_WAIT_NAME_CHECK:
        if (WifiBattleMatchNet_NameCheckMain(link->net, &opponentNameRejected)) {
            *link->param->opponentNameRejected = opponentNameRejected;
            *seq = FREE_SEARCH_SYNC_VALIDATED_PLAYERS;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeContinue);
            WifiBattleMatchLink_ResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
        break;
    case FREE_SEARCH_SYNC_VALIDATED_PLAYERS:
        if (cancel != CANCEL_NONE) {
            break;
        }
        WifiBattleMatchNet_StartSync(link->net, WBM_SYNC_VALIDATED_PLAYERS);
        link->idleTimer = 0;
        *seq = FREE_SEARCH_WAIT_VALIDATED_PLAYERS_SYNC;
        break;
    case FREE_SEARCH_WAIT_VALIDATED_PLAYERS_SYNC:
        if (!WifiBattleMatchNet_TakeCancelSearch(link->net) && WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = FREE_SEARCH_SYNC_MATCH_READY;
        }
        countIdle = TRUE;
        break;
    case FREE_SEARCH_SYNC_MATCH_READY:
        WifiBattleMatchNet_StartSync(link->net, WBM_SYNC_MATCH_READY);
        link->idleTimer = 0;
        *seq = FREE_SEARCH_WAIT_MATCH_READY_SYNC;
        break;
    case FREE_SEARCH_WAIT_MATCH_READY_SYNC:
        countIdle = TRUE;
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = FREE_SEARCH_SHOW_OPPONENT_FOUND;
        }
        break;
    case FREE_SEARCH_SHOW_OPPONENT_FOUND:
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_74);
        WifiBattleMatchLink_DeleteMenu(link);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_OpponentHasBeenFound,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = FREE_SEARCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, FREE_SEARCH_WAIT_OPPONENT_FOUND_DELAY);
        break;
    case FREE_SEARCH_WAIT_OPPONENT_FOUND_DELAY:
        if (link->timer++ > 120) {
            link->timer = 0;
            WifiBattleMatchLink_CreatePartnerCard(link, param->otherPlayer);
            GFL_BGSysSetBGEnabled(0, FALSE);
            *seq = FREE_SEARCH_WAIT_PARTNER_CARD;
        }
        break;
    case FREE_SEARCH_WAIT_PARTNER_CARD:
        if (WifiBattleMatchLink_IsPartnerCardDone(link)) {
            *seq = FREE_SEARCH_WAIT_PARTNER_CARD_DELAY;
        }
        break;
    case FREE_SEARCH_WAIT_PARTNER_CARD_DELAY:
        if (link->timer++ > 300) {
            link->timer = 0;
            *seq = FREE_SEARCH_SHOW_TEAM_SELECTION_MESSAGE;
        }
        break;
    case FREE_SEARCH_SHOW_TEAM_SELECTION_MESSAGE:
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PleaseChoosePokemon,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = FREE_SEARCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, FREE_SEARCH_WAIT_TEAM_SELECTION_DELAY);
        break;
    case FREE_SEARCH_WAIT_TEAM_SELECTION_DELAY:
        if (link->timer++ > 60) {
            link->timer = 0;
            param->action = WIFIBATTLEMATCH_LINK_ACTION_TEAM_LIST;
            WifiBattleMatchTask_End(task);
        }
        break;
    case FREE_SEARCH_SEND_CANCEL_COMMAND:
        if (WifiBattleMatchNet_SendCancelSearch(link->net)) {
            *seq = FREE_SEARCH_CANCEL_AND_RETRY;
        }
        break;
    case FREE_SEARCH_CANCEL_AND_RETRY:
        WifiBattleMatchNet_CancelMatch(link->net);
        *seq = FREE_SEARCH_SHOW_SEARCH_MESSAGE;
        break;
    case FREE_SEARCH_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
    if (countIdle) {
        timedOut = FALSE;
        if (link->idleTimer++ > 2400) {
            link->idleTimer = 0;
            timedOut = TRUE;
        }
        if (WifiBattleMatchNet_TakeCancelSearch(link->net)) {
            timedOut = TRUE;
        }
        if (timedOut) {
            *seq = FREE_SEARCH_SEND_CANCEL_COMMAND;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeContinue);
            WifiBattleMatchLink_ResetCancel(link);
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            break;
        }
    }
}

static void WifiBattleMatchLink_TaskFreeResult(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchPlayer *otherPlayer;
    RivalDataSave *rivalData;

    switch (*seq) {
    case FREE_RESULT_CHECK_CANCELLATION:
        if (param->result == WBM_LINK_RESULT_CANCELLED) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeAfterBattle);
            return;
        }
        *seq = FREE_RESULT_SHOW_EXCHANGE_MESSAGE;
        break;
    case FREE_RESULT_SHOW_EXCHANGE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_AwaitingCommunicationPleaseStand,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = FREE_RESULT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, FREE_RESULT_SEND_COMMAND_CHECK_RESULT);
        link->timer = 0;
        break;
    case FREE_RESULT_SEND_COMMAND_CHECK_RESULT:
        if (WifiBattleMatchNet_SendCommandCheckError(link->net, &param->battleResult->ownCommandCheckError)) {
            *seq = FREE_RESULT_RECEIVE_COMMAND_CHECK_RESULT;
        }
        break;
    case FREE_RESULT_RECEIVE_COMMAND_CHECK_RESULT:
        if (WifiBattleMatchNet_TakeCommandCheckError(link->net, &param->battleResult->otherCommandCheckError)) {
            *seq = FREE_RESULT_SEND_JOIN_AVENUE_PERSON;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            link->timer = 0;
            *seq = FREE_RESULT_DELAY_SAVE;
            return;
        case WBM_RESULT_FAILED:
            link->timer = 0;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        }
        break;
    case FREE_RESULT_SEND_JOIN_AVENUE_PERSON:
        otherPlayer = param->otherPlayer;
        if (PlayerInfo_GetVersion(&otherPlayer->info) == VERSION_WHITE2 ||
            PlayerInfo_GetVersion(&otherPlayer->info) == VERSION_BLACK2) {
            if (WifiBattleMatchNet_SendPerson(link->net)) {
                *seq = FREE_RESULT_RECEIVE_JOIN_AVENUE_PERSON;
            }
        } else {
            *seq = FREE_RESULT_RECORD_MET_TRAINER;
        }
        break;
    case FREE_RESULT_RECEIVE_JOIN_AVENUE_PERSON:
        if (WifiBattleMatchNet_TakePerson(link->net)) {
            *seq = FREE_RESULT_RECORD_MET_TRAINER;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            link->timer = 0;
            *seq = FREE_RESULT_DELAY_SAVE;
            return;
        case WBM_RESULT_FAILED:
            link->timer = 0;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        }
        break;
    case FREE_RESULT_RECORD_MET_TRAINER:
        rivalData = getHollow_RivalData(GameData_GetSaveControl(param->param->gameData));
        Misc_AddMetTrainer(rivalData, getIDAsUInt(&param->otherPlayer->info));
        WifiBattleMatch_RecordResult(param->param->gameData, param->battleResult->result);
        WifiBattleMatchNet_ResetEnd(link->net);
        *seq = FREE_RESULT_WAIT_DISCONNECT;
        break;
    case FREE_RESULT_WAIT_DISCONNECT:
        link->timer++;
        if (WifiBattleMatchNet_EndMain(link->net)) {
            *seq = FREE_RESULT_WAIT_DISCONNECT_DELAY;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            link->timer = 0;
            *seq = FREE_RESULT_DELAY_SAVE;
            return;
        case WBM_RESULT_FAILED:
            link->timer = 0;
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskRelogin);
            return;
        }
        break;
    case FREE_RESULT_WAIT_DISCONNECT_DELAY:
        if (link->timer++ > 30) {
            link->timer = 0;
            *seq = FREE_RESULT_DELAY_SAVE;
        }
        break;
    case FREE_RESULT_DELAY_SAVE:
        *seq = FREE_RESULT_SHOW_SAVE_MESSAGE;
        break;
    case FREE_RESULT_SHOW_SAVE_MESSAGE:
        WifiBattleMatch_RecordBattleTime(param->param->gameData, param->battleResult->result);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = FREE_RESULT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, FREE_RESULT_START_SAVE);
        break;
    case FREE_RESULT_START_SAVE:
        GameData_StartSave(param->param->gameData);
        *seq = FREE_RESULT_WAIT_SAVE;
        break;
    case FREE_RESULT_WAIT_SAVE:
        switch (GameData_UpdateSave(param->param->gameData)) {
        case SAVE_RESULT_CONTINUE:
        case SAVE_RESULT_LAST:
            break;
        case SAVE_RESULT_OK:
        case SAVE_RESULT_NG:
            param->action = WIFIBATTLEMATCH_LINK_ACTION_RESULT;
            WifiBattleMatchTask_End(task);
            break;
        }
        break;
    case FREE_RESULT_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    switch (*seq) {
    case FREE_AFTER_BATTLE_FIRST_DELAY_FRAME:
        *seq = FREE_AFTER_BATTLE_SECOND_DELAY_FRAME;
        break;
    case FREE_AFTER_BATTLE_SECOND_DELAY_FRAME:
        *seq = FREE_AFTER_BATTLE_CONTINUE;
        break;
    case FREE_AFTER_BATTLE_CONTINUE:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeContinue);
        break;
    case FREE_AFTER_BATTLE_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeContinue(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    s32 selected;

    switch (*seq) {
    case FREE_CONTINUE_PROMPT_CONTINUE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WantContinueBattling,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = FREE_CONTINUE_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, FREE_CONTINUE_CREATE_MENU);
        break;
    case FREE_CONTINUE_CREATE_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = FREE_CONTINUE_WAIT_MENU;
        break;
    case FREE_CONTINUE_WAIT_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeSearch);
            break;
        case WBM_MENU_NO:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeQuitConfirm);
            break;
        }
        break;
    case FREE_CONTINUE_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskFreeQuitConfirm(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    s32 selected;

    switch (*seq) {
    case FREE_QUIT_PROMPT_QUIT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillQuitRandomMatchup_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = FREE_QUIT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, FREE_QUIT_CREATE_MENU);
        break;
    case FREE_QUIT_CREATE_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        *seq = FREE_QUIT_WAIT_MENU;
        break;
    case FREE_QUIT_WAIT_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
            *seq = FREE_QUIT_SHOW_DISCONNECT_MESSAGE;
            break;
        case WBM_MENU_NO:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskFreeContinue);
            break;
        }
        break;
    case FREE_QUIT_SHOW_DISCONNECT_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_AwaitingCommunicationPleaseStand,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = FREE_QUIT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, FREE_QUIT_END);
        break;
    case FREE_QUIT_END:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_TaskEnd);
        break;
    case FREE_QUIT_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_TaskRelogin(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    link->param->action = WIFIBATTLEMATCH_LINK_ACTION_RELOGIN;
    if (link->net != NULL) {
        WifiBattleMatchNet_Delete(link->net);
        link->net = NULL;
    }
    GFL_NetErrShutdown();
    WifiBattleMatchTask_End(task);
}

static void WifiBattleMatchLink_TaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;

    if (*seq == RANDOM_LINK_END_RELEASE_NETWORK) {
        if (link->net != NULL) {
            WifiBattleMatchNet_Delete(link->net);
            link->net = NULL;
        }
        WifiBattleMatchTask_End(task);
    }
}

static void WifiBattleMatchLink_TaskSignTeam(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkRandom *link = work;
    WifiBattleMatchPlayer *player = link->param->player;
    u32 ret;

    switch (*seq) {
    case RANDOM_TEAM_CHECK_CLEAR_SIGNATURE:
        link->timer = 0;
        MI_CpuFill8(&link->signature, 0, sizeof(link->signature));
        (*seq)++;
        break;
    case RANDOM_TEAM_CHECK_START_CHECK:
        WifiBattleMatchNet_StartPokemonCheck(link->net, player->partyData, WBM_POKEMON_CHECK_PARTY);
        (*seq)++;
        break;
    case RANDOM_TEAM_CHECK_WAIT_CHECK:
        ret = WifiBattleMatchNet_PokemonCheckMain(link->net, &link->signature);
        if (ret == WBM_NET_SUCCESS) {
            *seq = RANDOM_TEAM_CHECK_COPY_SIGNATURE;
        } else if (ret == WBM_NET_FAILED) {
            link->subTaskResult = WBM_TEAM_CHECK_RESULT_REQUEST_FAILED;
            *seq = RANDOM_TEAM_CHECK_END;
        } else if (ret == WBM_NET_SERVER_ERROR) {
            link->subTaskResult = WBM_TEAM_CHECK_RESULT_SERVER_ERROR;
            *seq = RANDOM_TEAM_CHECK_END;
        }
        break;
    case RANDOM_TEAM_CHECK_COPY_SIGNATURE:
        if (link->signature.status == WBM_POKEMON_VALIDATION_OK) {
            MI_CpuCopy8(link->signature.signature, player->signature, sizeof(player->signature));
            link->subTaskResult = WBM_TEAM_CHECK_RESULT_VALID;
            *seq = RANDOM_TEAM_CHECK_END;
        } else {
            link->subTaskResult = WBM_TEAM_CHECK_RESULT_INVALID;
            *seq = RANDOM_TEAM_CHECK_END;
        }
        break;
    case RANDOM_TEAM_CHECK_END:
        WifiBattleMatchTask_End(task);
        break;
    }
}

static void WifiBattleMatchLink_CreateRankCard(WifiBattleMatchLinkRandom *link, u32 kind) {
    GameData *gameData;
    PlayerInfo *playerInfo;
    ClActUnit *unit;
    WifiBattleMatchRankInfo info;
    u32 titleMsg;

    if (link->card != NULL) {
        return;
    }
    gameData = link->param->param->gameData;
    playerInfo = GetGameDataPlayerInfo(gameData);
    unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
    MI_CpuFill8(&info, 0, sizeof(info));
    if (kind == 0) {
        titleMsg = 1;
        info.titleMsg = link->param->param->battleType;
        info.rating = RndMatchData_GetParam(
            link->param->record, link->param->param->battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_RATING);
        info.wins = RndMatchData_GetParam(link->param->record, link->param->param->battleType + RNDMATCH_RECORD_RATING,
                                          RNDMATCH_FIELD_WIN);
        info.losses = RndMatchData_GetParam(
            link->param->record, link->param->param->battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_LOSE);
        info.matchCount = info.wins + info.losses;
        if (info.rating == 0 && info.matchCount == 0) {
            info.rating = 1500;
        }
    } else if (kind == 1) {
        titleMsg = 0;
        info.titleMsg = link->param->param->battleType;
        info.rating = 0;
        info.wins = RndMatchData_GetParam(link->param->record, link->param->param->battleType, RNDMATCH_FIELD_WIN);
        info.losses = RndMatchData_GetParam(link->param->record, link->param->param->battleType, RNDMATCH_FIELD_LOSE);
        info.matchCount = info.wins + info.losses;
    }
    info.look = PlayerInfo_GetUnionView(playerInfo);
    link->card =
        WifiBattleMatchRankCard_Create(&info, titleMsg, playerInfo, unit, link->res, link->font, link->printQueue,
                                       link->msgData, link->wordSet, HEAPID_WIFIBATTLEMATCH_LINK);
}

static void WifiBattleMatchLink_DeleteRankCard(WifiBattleMatchLinkRandom *link) {
    if (link->card != NULL) {
        WifiBattleMatchRankCard_Free(link->card);
        link->card = NULL;
    }
}

static BOOL WifiBattleMatchLink_IsRankCardShown(WifiBattleMatchLinkRandom *link) {
    return WifiBattleMatchCard_SlideIn(link->card);
}

static BOOL WifiBattleMatchLink_IsRankCardHidden(WifiBattleMatchLinkRandom *link) {
    return WifiBattleMatchCard_SlideOut(link->card);
}

static void WifiBattleMatchLink_ShowRankCard(WifiBattleMatchLinkRandom *link, u32 kind) {
    WifiBattleMatchLink_CreateRankCard(link, kind);
    while (!WifiBattleMatchLink_IsRankCardShown(link)) {
    }
}

static void WifiBattleMatchLink_HideRankCard(WifiBattleMatchLinkRandom *link) {
    if (link->card != NULL) {
        while (!WifiBattleMatchLink_IsRankCardHidden(link)) {
        }
        WifiBattleMatchLink_DeleteRankCard(link);
    }
}

static void WifiBattleMatchLink_CreatePartnerCard(WifiBattleMatchLinkRandom *link, WifiBattleMatchPlayer *partner) {
    ClActUnit *unit;
    u32 kind;
    u32 shown;

    if (link->partnerCard == NULL) {
        unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
        if (link->param->mode == LINK_MODE_RATING) {
            shown = 1;
            kind = WIFIBATTLEMATCH_PARTNER_CARD_RATING;
        } else if (link->param->mode == LINK_MODE_FREE) {
            shown = 0;
            kind = WIFIBATTLEMATCH_PARTNER_CARD_FREE;
        }
        link->partnerCard =
            WifiBattleMatchPartnerCard_Create(partner, unit, link->res, link->font, link->printQueue, link->msgData,
                                              link->wordSet, kind, shown, 0, HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_DeletePartnerCard(WifiBattleMatchLinkRandom *link) {
    if (link->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Free(link->partnerCard);
        link->partnerCard = NULL;
    }
}

static BOOL WifiBattleMatchLink_IsPartnerCardDone(WifiBattleMatchLinkRandom *link) {
    return WifiBattleMatchPartnerCard_SlideIn(link->partnerCard);
}

static BOOL WifiBattleMatchLink_UpdateRecord(RndMatchData *results, u32 battleType, WifiBattleMatchLinkRandom *link) {
    u32 rating;
    u32 wins;
    u32 losses;
    BOOL changed;

    switch (battleType) {
    case WBM_RULE_SINGLE:
        rating = link->param->ratings->rating[WBM_RATING_INDEX_SINGLE];
        wins = link->param->ratings->results[WBM_RATING_INDEX_SINGLE].wins;
        losses = link->param->ratings->results[WBM_RATING_INDEX_SINGLE].losses;
        break;
    case WBM_RULE_DOUBLE:
        rating = link->param->ratings->rating[WBM_RATING_INDEX_DOUBLE];
        wins = link->param->ratings->results[WBM_RATING_INDEX_DOUBLE].wins;
        losses = link->param->ratings->results[WBM_RATING_INDEX_DOUBLE].losses;
        break;
    case WBM_RULE_TRIPLE:
        rating = link->param->ratings->rating[WBM_RATING_INDEX_TRIPLE];
        wins = link->param->ratings->results[WBM_RATING_INDEX_TRIPLE].wins;
        losses = link->param->ratings->results[WBM_RATING_INDEX_TRIPLE].losses;
        break;
    case WBM_RULE_ROTATION:
        rating = link->param->ratings->rating[WBM_RATING_INDEX_ROTATION];
        wins = link->param->ratings->results[WBM_RATING_INDEX_ROTATION].wins;
        losses = link->param->ratings->results[WBM_RATING_INDEX_ROTATION].losses;
        break;
    case WBM_RULE_SHOOTER:
        rating = link->param->ratings->rating[WBM_RATING_INDEX_SHOOTER];
        wins = link->param->ratings->results[WBM_RATING_INDEX_SHOOTER].wins;
        losses = link->param->ratings->results[WBM_RATING_INDEX_SHOOTER].losses;
        break;
    }
    if (rating != RndMatchData_GetParam(results, battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_RATING) ||
        wins != RndMatchData_GetParam(results, battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_WIN) ||
        losses != RndMatchData_GetParam(results, battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_LOSE)) {
        changed = TRUE;
    } else {
        changed = FALSE;
    }
    RndMatchData_SetParam(results, battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_RATING, rating);
    RndMatchData_SetParam(results, battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_WIN, wins);
    RndMatchData_SetParam(results, battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_LOSE, losses);
    return changed;
}

static void WifiBattleMatchLink_AddMatchResult(RndMatchData *results, u32 battleType, u32 matchResult) {
    switch (matchResult) {
    case BTL_RESULT_VICTORY:
    case BTL_RESULT_FLEE_FOE:
        RndMatchData_IncParam(results, battleType, RNDMATCH_FIELD_WIN);
        break;
    case BTL_RESULT_DEFEAT:
    case BTL_RESULT_FLEE:
        RndMatchData_IncParam(results, battleType, RNDMATCH_FIELD_LOSE);
        break;
    }
}

static void WifiBattleMatchLink_CreateMenu(WifiBattleMatchLinkRandom *link, u32 kind) {
    WifiBattleMatchMenuParam menuParam;

    if (link->menu == NULL) {
        MI_CpuFill8(&menuParam, 0, sizeof(menuParam));
        menuParam.msgData = link->msgData;
        menuParam.font = link->font;
        menuParam.printQueue = link->printQueue;
        menuParam.bg = 0;
        menuParam.palette = 15;
        menuParam.framePalette = 13;
        menuParam.frameChar = 1;
        switch (kind) {
        case MENU_YES_NO:
            menuParam.msgIds[0] = EventFishing_Text_Yes;
            menuParam.msgIds[1] = EventFishing_Text_No;
            menuParam.count = 2;
            menuParam.canCancel = TRUE;
            menuParam.cancelValue = 1;
            break;
        case MENU_MODE:
            menuParam.msgIds[0] = EventFishing_Text_Free;
            menuParam.msgIds[1] = EventFishing_Text_Rating_2;
            menuParam.msgIds[2] = EventFishing_Text_Quit;
            menuParam.count = 3;
            menuParam.canCancel = TRUE;
            menuParam.cancelValue = 2;
            break;
        }
        link->menuKind = kind;
        link->menu = WifiBattleMatchMenu_CreateBottomRight(&menuParam, HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_DeleteMenu(WifiBattleMatchLinkRandom *link) {
    if (link->menu != NULL) {
        WifiBattleMatchMenu_Free(link->menu);
        link->menu = NULL;
    }
}

static s32 WifiBattleMatchLink_GetMenuSelection(WifiBattleMatchLinkRandom *link) {
    if (link->menu != NULL) {
        return WifiBattleMatchMenu_Update(link->menu);
    }
    return -1;
}

static void WifiBattleMatchLink_FillCondition(WifiBattleMatchNetKeys *condition, WifiBattleMatchLinkRandom *link) {
    WifiBattleMatchLinkParam *param;

    MI_CpuFill8(condition, 0, sizeof(WifiBattleMatchNetKeys));
    param = link->param;
    condition->disconnects = param->ratings->disconnectsCounter;
    condition->cup = 0;
    condition->matches = param->ratings->results[WBM_RATING_INDEX_SINGLE].wins +
                         param->ratings->results[WBM_RATING_INDEX_SINGLE].losses +
                         param->ratings->results[WBM_RATING_INDEX_DOUBLE].wins +
                         param->ratings->results[WBM_RATING_INDEX_DOUBLE].losses +
                         param->ratings->results[WBM_RATING_INDEX_TRIPLE].wins +
                         param->ratings->results[WBM_RATING_INDEX_TRIPLE].losses +
                         param->ratings->results[WBM_RATING_INDEX_ROTATION].wins +
                         param->ratings->results[WBM_RATING_INDEX_ROTATION].losses +
                         param->ratings->results[WBM_RATING_INDEX_SHOOTER].wins +
                         param->ratings->results[WBM_RATING_INDEX_SHOOTER].losses;
    switch (param->param->battleType) {
    case WBM_RULE_SINGLE:
        condition->rating = param->ratings->rating[WBM_RATING_INDEX_SINGLE];
        break;
    case WBM_RULE_DOUBLE:
        condition->rating = param->ratings->rating[WBM_RATING_INDEX_DOUBLE];
        break;
    case WBM_RULE_TRIPLE:
        condition->rating = param->ratings->rating[WBM_RATING_INDEX_TRIPLE];
        break;
    case WBM_RULE_ROTATION:
        condition->rating = param->ratings->rating[WBM_RATING_INDEX_ROTATION];
        break;
    case WBM_RULE_SHOOTER:
        condition->rating = param->ratings->rating[WBM_RATING_INDEX_SHOOTER];
        break;
    }
}

static void WifiBattleMatchLink_FillOwnPlayer(WifiBattleMatchPlayer *player, WifiBattleMatchLinkRandom *link) {
    GameData *gameData = link->param->param->gameData;
    SaveControl *save = GameData_GetSaveControl(gameData);
    PlayerInfo *info;

    player->battleRecordVersion = 100;
    info = GetGameDataPlayerInfo(gameData);
    MI_CpuCopy8(info, player, PlayerInfo_GetSize());
    CGearPhrases_Get(CGearPhrases_GetBlock(save), 0, &player->sentence);
    if (link->param->mode == LINK_MODE_RATING) {
        player->record1 = RndMatchData_GetParam(
            link->param->record, link->param->param->battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_WIN);
        player->record2 = RndMatchData_GetParam(
            link->param->record, link->param->param->battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_LOSE);
    } else {
        player->record1 =
            RndMatchData_GetParam(link->param->record, link->param->param->battleType, RNDMATCH_FIELD_WIN);
        player->record2 =
            RndMatchData_GetParam(link->param->record, link->param->param->battleType, RNDMATCH_FIELD_LOSE);
    }
    player->gsid = WifiList_GetMyGSID(GameData_GetWifiList(gameData));
}

static void WifiBattleMatchLink_UpdatePlayerRecord(WifiBattleMatchPlayer *player, WifiBattleMatchLinkRandom *link) {
    if (link->param->mode == LINK_MODE_RATING) {
        player->record1 = RndMatchData_GetParam(
            link->param->record, link->param->param->battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_WIN);
        player->record2 = RndMatchData_GetParam(
            link->param->record, link->param->param->battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_LOSE);
        player->rating = RndMatchData_GetParam(
            link->param->record, link->param->param->battleType + RNDMATCH_RECORD_RATING, RNDMATCH_FIELD_RATING);
    } else {
        player->record1 =
            RndMatchData_GetParam(link->param->record, link->param->param->battleType, RNDMATCH_FIELD_WIN);
        player->record2 =
            RndMatchData_GetParam(link->param->record, link->param->param->battleType, RNDMATCH_FIELD_LOSE);
    }
}

static void WifiBattleMatchLink_StartSubTask(WifiBattleMatchLinkRandom *link, WifiBattleMatchTaskFunc func) {
    WifiBattleMatchTask_SetFunc(link->subTask, func);
    link->subTaskResult = SUB_TASK_RUNNING;
}

static u32 WifiBattleMatchLink_UpdateSubTask(WifiBattleMatchLinkRandom *link) {
    WifiBattleMatchTask_Run(link->subTask);
    if (WifiBattleMatchTask_IsEnded(link->subTask)) {
        return link->subTaskResult;
    }
    return SUB_TASK_RUNNING;
}

static BOOL WifiBattleMatchLink_CheckSignature(WifiBattleMatchPlayer *player, HeapID heapId) {
    int i;
    PokeParty *party = (PokeParty *)player->partyData;
    void *array;
    BOOL result;

    array = NHttpRapEvilCheck_AllocArray(PokeParty_GetPkmRawSize(), PokeParty_GetPkmCount(party), HEAPID_TAIL(heapId));
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        NHttpRapEvilCheck_SetElement(array, pkm, PokeParty_GetPkmRawSize(), i);
    }
    result = NHttpRapEvilCheck_Sign(array, PokeParty_GetPkmRawSize(), PokeParty_GetPkmCount(party), player->signature,
                                    HEAPID_TAIL(heapId));
    NHttpRapEvilCheck_FreeArray(array);
    return result;
}

static u32 WifiBattleMatchLink_UpdateCancel(WifiBattleMatchLinkRandom *link, BOOL canCancel, BOOL canDisconnect) {
    s32 selected;

    switch (link->cancelSeq) {
    case RANDOM_SEARCH_CANCEL_IDLE:
        if (canCancel && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            link->cancelSeq++;
            break;
        }
        return CANCEL_NONE;
    case RANDOM_SEARCH_CANCEL_SHOW_QUIT_PROMPT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitSearchingSomeoneBattle,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        link->cancelSeq++;
        break;
    case RANDOM_SEARCH_CANCEL_WAIT_QUIT_PROMPT:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            link->cancelSeq++;
        }
        break;
    case RANDOM_SEARCH_CANCEL_CREATE_MENU:
        WifiBattleMatchLink_CreateMenu(link, MENU_YES_NO);
        link->cancelSeq++;
        break;
    case RANDOM_SEARCH_CANCEL_WAIT_MENU:
        selected = WifiBattleMatchLink_GetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_DeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            link->cancelSeq = RANDOM_SEARCH_CANCEL_SHOW_DISCONNECT_MESSAGE;
            break;
        case WBM_MENU_NO:
            WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent,
                                        WIFIBATTLEMATCH_MSG_QUEUE);
            link->cancelSeq = RANDOM_SEARCH_CANCEL_IDLE;
            return CANCEL_DECLINED;
            break;
        }
        break;
    case RANDOM_SEARCH_CANCEL_SHOW_DISCONNECT_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_AwaitingCommunicationPleaseStand,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        link->cancelSeq++;
        break;
    case RANDOM_SEARCH_CANCEL_DISCONNECT:
        if (canDisconnect) {
            WifiBattleMatchNet_SendCancelSearch(link->net);
            link->cancelSeq++;
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchNet_CancelMatch(link->net);
            if (!GFL_NetErrCheck() && GFL_NetIsInitialized()) {
                DWC_CloseAllConnectionsHard();
            }
        }
        break;
    case RANDOM_SEARCH_CANCEL_CONFIRMED:
        link->cancelSeq = RANDOM_SEARCH_CANCEL_IDLE;
        return CANCEL_CONFIRMED;
    }
    return CANCEL_BUSY;
}

static void WifiBattleMatchLink_ResetCancel(WifiBattleMatchLinkRandom *link) {
    link->cancelSeq = RANDOM_SEARCH_CANCEL_IDLE;
    WifiBattleMatchLink_DeleteMenu(link);
}
