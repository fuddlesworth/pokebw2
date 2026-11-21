#include "types.h"
#include "app/livebattlematch_flow.h"
#include "app/livebattlematch_irc.h"
#include "battle/btl_result.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms.h"
#include "system/wordset.h"

enum {
    LIVE_CHOICE_YES = 0,
    LIVE_CHOICE_NO = 1,
};

enum {
    LIVE_ACTIVE_CHOICE_BATTLE = 0,
    LIVE_ACTIVE_CHOICE_REPLAY = 1,
    LIVE_ACTIVE_CHOICE_RESULT = 2,
    LIVE_ACTIVE_CHOICE_QUIT = 3,
};

enum {
    LIVE_ACTIVE_NO_VIDEO_CHOICE_BATTLE = 0,
    LIVE_ACTIVE_NO_VIDEO_CHOICE_QUIT = 1,
};

enum {
    LIVE_FINISHED_CHOICE_REPLAY = 0,
    LIVE_FINISHED_CHOICE_RESULT = 1,
    LIVE_FINISHED_CHOICE_EXIT = 2,
};

enum {
    LIVE_FINISHED_NO_VIDEO_CHOICE_EXIT = 0,
};

enum {
    LIVE_ENTRY_CHECK_ENTRY = 0,
    LIVE_ENTRY_SHOW_CARD = 1,
    LIVE_ENTRY_WAIT_CARD = 2,
    LIVE_ENTRY_CHECK_BOX = 3,
    LIVE_ENTRY_SHOW_TEAM_UNLOCKED = 4,
    LIVE_ENTRY_SHOW_TEAM_LOCKED = 5,
    LIVE_ENTRY_OPEN_REPLACE_MENU = 6,
    LIVE_ENTRY_WAIT_REPLACE_MENU = 7,
    LIVE_ENTRY_WAIT_CARD_REPLACE = 8,
    LIVE_ENTRY_DELETE_CARD_REPLACE = 9,
    LIVE_ENTRY_WAIT_CARD_EXIT = 10,
    LIVE_ENTRY_DELETE_CARD_EXIT = 11,
    LIVE_ENTRY_SHOW_DELIVERY_INTRO = 12,
    LIVE_ENTRY_SHOW_DELIVERY_PROMPT = 13,
    LIVE_ENTRY_START_DELIVERY = 14,
    LIVE_ENTRY_WAIT_DELIVERY = 15,
    LIVE_ENTRY_CANCEL_DELIVERY = 16,
    LIVE_ENTRY_WAIT_CANCEL = 17,
    LIVE_ENTRY_SHOW_CANCELLED = 18,
    LIVE_ENTRY_OPEN_CANCEL_MENU = 19,
    LIVE_ENTRY_WAIT_CANCEL_MENU = 20,
    LIVE_ENTRY_CHECK_DELIVERY = 21,
    LIVE_ENTRY_SHOW_SAVING = 22,
    LIVE_ENTRY_STORE_ENTRY = 23,
    LIVE_ENTRY_WAIT_SAVE = 24,
    LIVE_ENTRY_SHOW_INVALID_DATA = 25,
    LIVE_ENTRY_SHOW_INCOMPATIBLE = 26,
    LIVE_ENTRY_SHOW_UNREGISTERED = 27,
    LIVE_ENTRY_SHOW_FINISHED = 28,
    LIVE_ENTRY_START_TEAM = 29,
    LIVE_ENTRY_EXIT = 30,
    LIVE_ENTRY_WAIT_MESSAGE = 31,
};

enum {
    LIVE_TEAM_SHOW_CARD_AND_TEAM = 0,
    LIVE_TEAM_WAIT_CARD_AND_TEAM = 1,
    LIVE_TEAM_SHOW_REGISTER_PROMPT = 2,
    LIVE_TEAM_OPEN_REGISTER_MENU = 3,
    LIVE_TEAM_WAIT_REGISTER_MENU = 4,
    LIVE_TEAM_SHOW_CANCEL_PROMPT = 5,
    LIVE_TEAM_OPEN_CANCEL_MENU = 6,
    LIVE_TEAM_WAIT_CANCEL_MENU = 7,
    LIVE_TEAM_HIDE_TEAM = 8,
    LIVE_TEAM_CHECK_TEAM = 9,
    LIVE_TEAM_SHOW_PROBLEM = 10,
    LIVE_TEAM_SHOW_NO_POKEMON = 11,
    LIVE_TEAM_HIDE_CARD = 12,
    LIVE_TEAM_STORE_TEAM = 13,
    LIVE_TEAM_SHOW_SAVING = 14,
    LIVE_TEAM_START_SAVE = 15,
    LIVE_TEAM_WAIT_SAVE = 16,
    LIVE_TEAM_SHOW_REGISTERED = 17,
    LIVE_TEAM_START_MENU = 18,
    LIVE_TEAM_HIDE_CARD_AND_TEAM = 19,
    LIVE_TEAM_EXIT = 20,
    LIVE_TEAM_WAIT_MESSAGE = 21,
    LIVE_TEAM_NEXT_PROBLEM = 22,
    LIVE_TEAM_SHOW_PROBLEMS_END = 23,
};

enum {
    LIVE_MENU_SHOW_CARD = 0,
    LIVE_MENU_WAIT_CARD = 1,
    LIVE_MENU_CHECK_DISCONNECT = 2,
    LIVE_MENU_SHOW_DISCONNECT = 3,
    LIVE_MENU_CHECK_MATCH_LIMIT = 4,
    LIVE_MENU_SHOW_PROMPT = 5,
    LIVE_MENU_OPEN_MENU = 6,
    LIVE_MENU_WAIT_MENU = 7,
    LIVE_MENU_SHOW_QUIT_PROMPT = 8,
    LIVE_MENU_OPEN_QUIT_MENU = 9,
    LIVE_MENU_WAIT_QUIT_MENU = 10,
    LIVE_MENU_SHOW_QUIT_CONFIRM = 11,
    LIVE_MENU_OPEN_QUIT_CONFIRM = 12,
    LIVE_MENU_WAIT_QUIT_CONFIRM = 13,
    LIVE_MENU_SHOW_SAVING = 14,
    LIVE_MENU_UNREGISTER = 15,
    LIVE_MENU_WAIT_SAVE = 16,
    LIVE_MENU_SHOW_UNREGISTERED = 17,
    LIVE_MENU_HIDE_CARD = 18,
    LIVE_MENU_CHECK_VIDEO = 19,
    LIVE_MENU_SHOW_VIDEO_PROMPT = 20,
    LIVE_MENU_SHOW_VIDEO_UNSUPPORTED = 21,
    LIVE_MENU_REPLAY = 22,
    LIVE_MENU_START_LINK = 23,
    LIVE_MENU_EXIT = 24,
    LIVE_MENU_WAIT_MESSAGE = 25,
    LIVE_MENU_SHOW_RESULT = 26,
};

enum {
    LIVE_LINK_SHOW_INTRO = 0,
    LIVE_LINK_SHOW_LINK_PROMPT = 1,
    LIVE_LINK_RESET_LINK = 2,
    LIVE_LINK_WAIT_LINK = 3,
    LIVE_LINK_CANCEL_LINK = 4,
    LIVE_LINK_WAIT_CANCEL = 5,
    LIVE_LINK_SHOW_CANCELLED = 6,
    LIVE_LINK_OPEN_CANCEL_MENU = 7,
    LIVE_LINK_WAIT_CANCEL_MENU = 8,
    LIVE_LINK_CANCEL_LOST_LINK = 9,
    LIVE_LINK_WAIT_LOST_LINK = 10,
    LIVE_LINK_SHOW_LOST_LINK = 11,
    LIVE_LINK_RETURN_AFTER_LOST_LINK = 12,
    LIVE_LINK_SEND_PLAYER_DATA = 13,
    LIVE_LINK_RECEIVE_PLAYER_DATA = 14,
    LIVE_LINK_CHECK_COMPETITION = 15,
    LIVE_LINK_SYNC_READY = 16,
    LIVE_LINK_WAIT_SYNC_READY = 17,
    LIVE_LINK_SEND_READY = 18,
    LIVE_LINK_RECEIVE_READY = 19,
    LIVE_LINK_CHECK_READY = 20,
    LIVE_LINK_RESET_WRONG_COMPETITION = 21,
    LIVE_LINK_WAIT_WRONG_COMPETITION = 22,
    LIVE_LINK_SHOW_WRONG_COMPETITION = 23,
    LIVE_LINK_RESET_REMATCH = 24,
    LIVE_LINK_WAIT_REMATCH = 25,
    LIVE_LINK_SHOW_REMATCH_DISALLOWED = 26,
    LIVE_LINK_SHOW_PARTNER_FOUND = 27,
    LIVE_LINK_WAIT_PARTNER_DELAY = 28,
    LIVE_LINK_SHOW_PARTNER_CARD = 29,
    LIVE_LINK_WAIT_PARTNER_CARD = 30,
    LIVE_LINK_WAIT_PARTNER_DISPLAY = 31,
    LIVE_LINK_SHOW_BATTLE_PROMPT = 32,
    LIVE_LINK_SHOW_STARTING = 33,
    LIVE_LINK_SYNC_START = 34,
    LIVE_LINK_WAIT_SYNC_START = 35,
    LIVE_LINK_SHOW_BATTLE_WAIT = 36,
    LIVE_LINK_WAIT_BATTLE_DELAY = 37,
    LIVE_LINK_START_BATTLE = 38,
    LIVE_LINK_RETURN_MENU = 39,
    LIVE_LINK_WAIT_MESSAGE = 40,
};

enum {
    LIVE_RECORD_RECORD_TRAINER = 0,
    LIVE_RECORD_RESET_LINK_END = 1,
    LIVE_RECORD_WAIT_LINK_END = 2,
    LIVE_RECORD_RECORD_RESULT = 3,
    LIVE_RECORD_SHOW_SAVING = 4,
    LIVE_RECORD_START_SAVE = 5,
    LIVE_RECORD_WAIT_SAVE = 6,
    LIVE_RECORD_EXIT = 7,
    LIVE_RECORD_WAIT_MESSAGE = 8,
};

enum {
    LIVE_QUIT_SHOW_CARD = 0,
    LIVE_QUIT_CHECK_MATCH_LIMIT = 1,
    LIVE_QUIT_SHOW_SAVING = 2,
    LIVE_QUIT_FINISH_ENTRY = 3,
    LIVE_QUIT_WAIT_SAVE = 4,
    LIVE_QUIT_SHOW_FINISHED = 5,
    LIVE_QUIT_SHOW_PROMPT = 6,
    LIVE_QUIT_OPEN_MENU = 7,
    LIVE_QUIT_WAIT_MENU = 8,
    LIVE_QUIT_SHOW_EXIT = 9,
    LIVE_QUIT_HIDE_CARD = 10,
    LIVE_QUIT_CHECK_VIDEO = 11,
    LIVE_QUIT_SHOW_VIDEO_PROMPT = 12,
    LIVE_QUIT_SHOW_VIDEO_UNSUPPORTED = 13,
    LIVE_QUIT_REPLAY = 14,
    LIVE_QUIT_EXIT = 15,
    LIVE_QUIT_RETURN_MENU = 16,
    LIVE_QUIT_WAIT_MESSAGE = 17,
    LIVE_QUIT_SHOW_RESULT = 18,
};

enum {
    LIVE_FLOW_MENU_YES_NO = 0,
    LIVE_FLOW_MENU_ACTIVE = 1,
    LIVE_FLOW_MENU_FINISHED = 2,
    LIVE_FLOW_MENU_YES_NO_DEFAULT_NO = 3,
};

enum {
    LIVE_FLOW_LAYOUT_CENTER = 0,
    LIVE_FLOW_LAYOUT_RIGHT = 1,
};

// A heap that the team is read into for a moment
#define HEAPID_LIVEBATTLEMATCH_TEAM HEAPID_TAIL(0x56)

// The steps of a flow that wait for a message to be printed
#define STEP_MESSAGE_QUEUE_NUM 4

struct LiveBattleMatchFlow {
    WifiBattleMatchTask *task;
    WifiBattleMatchMenu *menu;
    u32 menuKind;
    LiveBattleMatchIrc *irc;
    WifiBattleMatchCard *playerCard;
    WifiBattleMatchPartnerCard *partnerCard;
    WifiBattleMatchTeamList *teamList;
    WifiBattleMatchLinkIcon *linkIcon;
    MsgData *msgMain;
    MsgData *msgCard;
    Font *font;
    WordSet *wordSet;
    PokeParty *party;
    LiveMatchData *record;
    TournamentEntry *entry;
    // The competition data that the infrared delivery brings
    TournamentEntry delivered;
    u32 result;
    LiveBattleMatchFlowParam param;
    HeapID heapId;
    u32 frames;
    BOOL hasVideo;
    BOOL readySent;
    BOOL readyReceived;
    u32 pickFlags;
    u32 pickIndex;
};

static void LiveBattleMatchFlow_StepInit(WifiBattleMatchTask *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepEntry(WifiBattleMatchTask *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepTeam(WifiBattleMatchTask *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepMenu(WifiBattleMatchTask *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepLink(WifiBattleMatchTask *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepRecord(WifiBattleMatchTask *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_StepQuit(WifiBattleMatchTask *task, u32 *seq, void *work);
static void LiveBattleMatchFlow_MenuStart(LiveBattleMatchFlow *wk, u32 kind);
static void LiveBattleMatchFlow_MenuFree(LiveBattleMatchFlow *wk);
static s32 LiveBattleMatchFlow_MenuGetSelection(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PrintMessage(LiveBattleMatchFlow *wk, u32 msgId, u32 type);
static BOOL LiveBattleMatchFlow_IsMessageDone(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_Exit(LiveBattleMatchFlow *wk, u32 result);
static void LiveBattleMatchFlow_PlayerCardCreate(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PlayerCardCreateWait(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PlayerCardDelete(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_PlayerCardIsReady(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_PlayerCardIsDone(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PlayerCardShow(LiveBattleMatchFlow *wk, u32 page);
static void LiveBattleMatchFlow_PartnerCardCreate(LiveBattleMatchFlow *wk, WifiBattleMatchPlayer *data);
static void LiveBattleMatchFlow_PartnerCardDelete(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_PartnerCardIsDone(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_TeamListCreate(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_TeamListDelete(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_TeamListIsReady(LiveBattleMatchFlow *wk);
static BOOL LiveBattleMatchFlow_TeamListIsDone(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_LoadTeam(LiveBattleMatchFlow *wk, PokeParty *dest);
static void LiveBattleMatchFlow_FillPlayerData(WifiBattleMatchPlayer *data, LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_SetEntryField2(WifiBattleMatchPlayer *data, LiveBattleMatchFlow *wk);
static u32 LiveBattleMatchFlow_GetResultMessageId(LiveBattleMatchFlow *wk);
static void LiveBattleMatchFlow_PrintResult(LiveBattleMatchFlow *wk);

// Prints a message, and goes to the step that waits for it, after which the task goes to the step next
static inline void LiveBattleMatchFlow_PrintAndWait(LiveBattleMatchFlow *wk, WifiBattleMatchTask *task, u32 *seq,
                                                    u32 msgId, u32 type, u32 waitSeq, u32 next) {
    LiveBattleMatchFlow_PrintMessage(wk, msgId, type);
    *seq = waitSeq;
    WifiBattleMatchTask_SetReturnSeq(task, next);
}

LiveBattleMatchFlow *LiveBattleMatchFlow_Create(const LiveBattleMatchFlowParam *param, HeapID heapId) {
    LiveBattleMatchFlow *wk =
        GFL_HeapAllocate(heapId, sizeof(LiveBattleMatchFlow), FALSE, "livebattlematch_flow.c", 270);

    MI_CpuFill8(wk, 0, sizeof(LiveBattleMatchFlow));
    wk->param = *param;
    wk->heapId = heapId;
    wk->hasVideo = BattleRec_HasVideo();
    WifiBattleMatchRes_LoadBG(wk->param.seq, WIFIBATTLEMATCH_BG_CHAMPIONSHIP, heapId);
    wk->wordSet = GFL_WordSetSystemCreate(8, 0x40, heapId);
    wk->msgMain = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_LIVE_BATTLE_MATCH_FLOW, heapId);
    wk->msgCard = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WIFI_BATTLE_MATCH, heapId);
    wk->font = GFL_FontCreate(0x17, 3, 0, 0, heapId);
    wk->task = WifiBattleMatchTask_Create(wk, LiveBattleMatchFlow_StepInit, heapId);
    wk->irc = LiveBattleMatchIrc_Create(wk->param.gameData, heapId);
    wk->linkIcon =
        WifiBattleMatchLinkIcon_Create(WifiBattleMatchGraphic_GetClActUnit(wk->param.net), wk->param.seq, heapId);
    WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
    wk->entry = TournamentBlock_GetEntry(getTournamentBlockAddress(GameData_GetSaveControl(wk->param.gameData)), 1);
    wk->record = RecordSave_GetLiveMatch(getRecordBlkAddress(GameData_GetSaveControl(wk->param.gameData)));
    wk->party = PokeParty_Create(heapId);
    if (wk->param.mode == LIVE_FLOW_MODE_JOIN) {
        GCTX_HIDBlockSoftReset(8);
    }
    return wk;
}

void LiveBattleMatchFlow_Delete(LiveBattleMatchFlow *wk) {
    GFL_HeapFree(wk->party);
    if (wk->result == LIVE_FLOW_RESULT_MENU) {
        GCTX_HIDUnblockSoftReset(8);
    }
    LiveBattleMatchFlow_PlayerCardDelete(wk);
    LiveBattleMatchFlow_PartnerCardDelete(wk);
    WifiBattleMatchLinkIcon_Free(wk->linkIcon);
    LiveBattleMatchIrc_Delete(wk->irc);
    WifiBattleMatchTask_Free(wk->task);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgCard);
    GFL_MsgDataFree(wk->msgMain);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_HeapFree(wk);
}

void LiveBattleMatchFlow_Main(LiveBattleMatchFlow *wk) {
    WifiBattleMatchTask_Run(wk->task);
    LiveBattleMatchIrc_Main(wk->irc);
    WifiBattleMatchLinkIcon_Update(wk->linkIcon);
    if (wk->playerCard != NULL) {
        WifiBattleMatchCard_Flush(wk->playerCard, wk->param.printQueue);
    }
    if (wk->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Flush(wk->partnerCard, wk->param.printQueue);
    }
    if (wk->teamList != NULL) {
        WifiBattleMatchTeamList_Flush(wk->teamList, wk->param.printQueue);
    }
}

u32 LiveBattleMatchFlow_GetResult(LiveBattleMatchFlow *wk) {
    if (WifiBattleMatchTask_IsEnded(wk->task)) {
        return wk->result;
    }
    return LIVE_FLOW_RESULT_RUNNING;
}

// Picks the step to start with
static void LiveBattleMatchFlow_StepInit(WifiBattleMatchTask *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;

    switch (wk->param.mode) {
    case LIVE_FLOW_MODE_JOIN:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepEntry);
        break;
    case LIVE_FLOW_MODE_MENU:
        if (TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_REGISTRATION_STATE) == 3) {
            WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepQuit);
        } else {
            WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
        }
        break;
    case LIVE_FLOW_MODE_AFTER_MATCH:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepRecord);
        break;
    case LIVE_FLOW_MODE_QUIT:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepQuit);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
}

static void LiveBattleMatchFlow_StepEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

// Starts the flow of a player who has no entry yet, or one who has one that is half done
static void LiveBattleMatchFlow_StepEntry(WifiBattleMatchTask *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;
    u32 status;
    u32 pressed;

    switch (*seq) {
    case LIVE_ENTRY_CHECK_ENTRY:
        switch (TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_REGISTRATION_STATE)) {
        case 0:
        case 3:
        case 4:
        case 5:
            if (TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_CUP_ID) == 0) {
                *seq = LIVE_ENTRY_SHOW_DELIVERY_INTRO;
            } else {
                *seq = LIVE_ENTRY_SHOW_CARD;
            }
            break;
        case 1:
            WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepTeam);
            break;
        case 2:
            WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
            break;
        }
        break;
    case LIVE_ENTRY_SHOW_CARD:
        LiveBattleMatchFlow_PlayerCardCreate(wk);
        LiveBattleMatchFlow_PlayerCardShow(wk, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        *seq = LIVE_ENTRY_WAIT_CARD;
        break;
    case LIVE_ENTRY_WAIT_CARD:
        if (LiveBattleMatchFlow_PlayerCardIsReady(wk)) {
            *seq = LIVE_ENTRY_CHECK_BOX;
        }
        break;
    case LIVE_ENTRY_CHECK_BOX:
        if (BattleBoxSave_CheckFlags(getBattleBox(GameData_GetSaveControl(wk->param.gameData)), 2)) {
            *seq = LIVE_ENTRY_SHOW_TEAM_LOCKED;
        } else {
            *seq = LIVE_ENTRY_SHOW_TEAM_UNLOCKED;
        }
        break;
    case LIVE_ENTRY_SHOW_TEAM_UNLOCKED:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 0, TRUE, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_OPEN_REPLACE_MENU);
        break;
    case LIVE_ENTRY_SHOW_TEAM_LOCKED:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 1, TRUE, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_OPEN_REPLACE_MENU);
        break;
    case LIVE_ENTRY_OPEN_REPLACE_MENU:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_YES_NO);
        *seq = LIVE_ENTRY_WAIT_REPLACE_MENU;
        break;
    case LIVE_ENTRY_WAIT_REPLACE_MENU:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case LIVE_CHOICE_YES:
            *seq = LIVE_ENTRY_WAIT_CARD_REPLACE;
            break;
        case LIVE_CHOICE_NO:
            *seq = LIVE_ENTRY_WAIT_CARD_EXIT;
            break;
        }
        break;
    case LIVE_ENTRY_WAIT_CARD_REPLACE:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            *seq = LIVE_ENTRY_DELETE_CARD_REPLACE;
        }
        break;
    case LIVE_ENTRY_DELETE_CARD_REPLACE:
        LiveBattleMatchFlow_PlayerCardDelete(wk);
        *seq = LIVE_ENTRY_SHOW_DELIVERY_INTRO;
        break;
    case LIVE_ENTRY_WAIT_CARD_EXIT:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            *seq = LIVE_ENTRY_DELETE_CARD_EXIT;
        }
        break;
    case LIVE_ENTRY_DELETE_CARD_EXIT:
        LiveBattleMatchFlow_PlayerCardDelete(wk);
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_MENU);
        break;
    case LIVE_ENTRY_SHOW_DELIVERY_INTRO:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 2, TRUE, LIVE_ENTRY_WAIT_MESSAGE,
                                         LIVE_ENTRY_SHOW_DELIVERY_PROMPT);
        break;
    case LIVE_ENTRY_SHOW_DELIVERY_PROMPT:
        WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, TRUE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 32, FALSE, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_START_DELIVERY);
        break;
    case LIVE_ENTRY_START_DELIVERY:
        LiveBattleMatchIrc_StartDelivery(wk->irc, &wk->delivered);
        *seq = LIVE_ENTRY_WAIT_DELIVERY;
        break;
    case LIVE_ENTRY_WAIT_DELIVERY:
        status = LiveBattleMatchIrc_StepDelivery(wk->irc);
        if (status != LIVE_IR_DELIVERY_RESULT_WAIT) {
            WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
            if (status == LIVE_IR_DELIVERY_RESULT_RECEIVED) {
                *seq = LIVE_ENTRY_CHECK_DELIVERY;
            } else {
                *seq = LIVE_ENTRY_SHOW_INCOMPATIBLE;
            }
        }
        if (LiveBattleMatchIrc_CheckError(wk->irc) == TRUE) {
            WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
            *seq = LIVE_ENTRY_SHOW_DELIVERY_INTRO;
            break;
        }
        pressed = GCTX_HIDGetPressedKeys();
        if (pressed & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = LIVE_ENTRY_CANCEL_DELIVERY;
        }
        break;
    case LIVE_ENTRY_CANCEL_DELIVERY:
        LiveBattleMatchIrc_ResetDeliveryEnd(wk->irc);
        *seq = LIVE_ENTRY_WAIT_CANCEL;
        break;
    case LIVE_ENTRY_WAIT_CANCEL:
        if (LiveBattleMatchIrc_StepDeliveryEnd(wk->irc)) {
            *seq = LIVE_ENTRY_SHOW_CANCELLED;
        }
        break;
    case LIVE_ENTRY_SHOW_CANCELLED:
        WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 21, TRUE, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_OPEN_CANCEL_MENU);
        break;
    case LIVE_ENTRY_OPEN_CANCEL_MENU:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_YES_NO);
        *seq = LIVE_ENTRY_WAIT_CANCEL_MENU;
        break;
    case LIVE_ENTRY_WAIT_CANCEL_MENU:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case LIVE_CHOICE_YES:
            *seq = LIVE_ENTRY_EXIT;
            break;
        case LIVE_CHOICE_NO:
            *seq = LIVE_ENTRY_SHOW_DELIVERY_INTRO;
            break;
        }
        break;
    case LIVE_ENTRY_CHECK_DELIVERY:
        if (TournamentEntry_CheckCRC(&wk->delivered)) {
            if (TournamentEntry_GetField(&wk->delivered, TOURNAMENT_FIELD_VERSION_MASK) &
                LIVEBATTLEMATCH_VERSION_MASK) {
                if (TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_CUP_ID) ==
                    TournamentEntry_GetField(&wk->delivered, TOURNAMENT_FIELD_CUP_ID)) {
                    switch (TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_REGISTRATION_STATE)) {
                    case 4:
                        *seq = LIVE_ENTRY_SHOW_UNREGISTERED;
                        break;
                    case 3:
                        *seq = LIVE_ENTRY_SHOW_FINISHED;
                        break;
                    default:
                        *seq = LIVE_ENTRY_SHOW_SAVING;
                        break;
                    }
                } else {
                    *seq = LIVE_ENTRY_SHOW_SAVING;
                }
            } else {
                *seq = LIVE_ENTRY_SHOW_INCOMPATIBLE;
            }
        } else {
            *seq = LIVE_ENTRY_SHOW_INVALID_DATA;
        }
        break;
    case LIVE_ENTRY_SHOW_SAVING:
        clearVSPlayer();
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_STORE_ENTRY);
        break;
    case LIVE_ENTRY_STORE_ENTRY: {
        TournamentBlock *block = getTournamentBlockAddress(GameData_GetSaveControl(wk->param.gameData));
        TournamentTeam *records = TournamentBlock_GetTeam(block, 1);

        TournamentEntry_SetField(&wk->delivered, TOURNAMENT_FIELD_REGISTRATION_STATE, 1);
        LiveMatchData_Init(wk->record);
        TournamentTeam_Clear(records);
        TournamentBlock_SetEntry(block, 1, &wk->delivered);
        GameData_StartSave(wk->param.gameData);
        *seq = LIVE_ENTRY_WAIT_SAVE;
        break;
    }
    case LIVE_ENTRY_WAIT_SAVE:
        if (GameData_UpdateSave(wk->param.gameData) == SAVE_RESULT_OK) {
            *seq = LIVE_ENTRY_START_TEAM;
        }
        break;
    case LIVE_ENTRY_SHOW_INVALID_DATA:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 26, TRUE, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_EXIT);
        break;
    case LIVE_ENTRY_SHOW_INCOMPATIBLE:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 27, TRUE, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_EXIT);
        break;
    case LIVE_ENTRY_SHOW_UNREGISTERED:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 3, TRUE, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_EXIT);
        break;
    case LIVE_ENTRY_SHOW_FINISHED:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 4, TRUE, LIVE_ENTRY_WAIT_MESSAGE, LIVE_ENTRY_EXIT);
        break;
    case LIVE_ENTRY_START_TEAM:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepTeam);
        break;
    case LIVE_ENTRY_EXIT:
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_MENU);
        break;
    case LIVE_ENTRY_WAIT_MESSAGE:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

// Checks the team against the regulation
static void LiveBattleMatchFlow_StepTeam(WifiBattleMatchTask *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;

    switch (*seq) {
    case LIVE_TEAM_SHOW_CARD_AND_TEAM:
        LiveBattleMatchFlow_PlayerCardCreate(wk);
        LiveBattleMatchFlow_PlayerCardShow(wk, WIFIBATTLEMATCH_CARD_PAGE_EMPTY);
        LiveBattleMatchFlow_TeamListCreate(wk);
        *seq = LIVE_TEAM_WAIT_CARD_AND_TEAM;
        break;
    case LIVE_TEAM_WAIT_CARD_AND_TEAM: {
        BOOL ready = TRUE;

        ready &= LiveBattleMatchFlow_PlayerCardIsReady(wk);
        if (LiveBattleMatchFlow_TeamListIsReady(wk) & ready) {
            *seq = LIVE_TEAM_SHOW_REGISTER_PROMPT;
        }
        break;
    }
    case LIVE_TEAM_SHOW_REGISTER_PROMPT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 5, TRUE, LIVE_TEAM_WAIT_MESSAGE, LIVE_TEAM_OPEN_REGISTER_MENU);
        break;
    case LIVE_TEAM_OPEN_REGISTER_MENU:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_YES_NO);
        *seq = LIVE_TEAM_WAIT_REGISTER_MENU;
        break;
    case LIVE_TEAM_WAIT_REGISTER_MENU:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case LIVE_CHOICE_YES:
            *seq = LIVE_TEAM_HIDE_TEAM;
            break;
        case LIVE_CHOICE_NO:
            *seq = LIVE_TEAM_SHOW_CANCEL_PROMPT;
            break;
        }
        break;
    case LIVE_TEAM_SHOW_CANCEL_PROMPT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 8, TRUE, LIVE_TEAM_WAIT_MESSAGE, LIVE_TEAM_OPEN_CANCEL_MENU);
        break;
    case LIVE_TEAM_OPEN_CANCEL_MENU:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_YES_NO);
        *seq = LIVE_TEAM_WAIT_CANCEL_MENU;
        break;
    case LIVE_TEAM_WAIT_CANCEL_MENU:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case LIVE_CHOICE_YES:
            *seq = LIVE_TEAM_HIDE_CARD_AND_TEAM;
            break;
        case LIVE_CHOICE_NO:
            *seq = LIVE_TEAM_SHOW_REGISTER_PROMPT;
            break;
        }
        break;
    case LIVE_TEAM_HIDE_TEAM:
        if (LiveBattleMatchFlow_TeamListIsDone(wk)) {
            LiveBattleMatchFlow_TeamListDelete(wk);
            *seq = LIVE_TEAM_CHECK_TEAM;
        }
        break;
    case LIVE_TEAM_CHECK_TEAM: {
        Regulation *regulation = TournamentEntry_GetRegulation(wk->entry);
        u32 problem;

        LiveBattleMatchFlow_LoadTeam(wk, wk->party);
        if (howManyPokesAreAbleToFight(wk->party) == 0) {
            *seq = LIVE_TEAM_SHOW_NO_POKEMON;
            break;
        }
        problem = PokeRegulation_CheckParty(regulation, wk->party, &wk->pickFlags);
        if (PokeRegulation_PartyHasEgg(regulation, wk->party)) {
            wk->pickFlags |= 0x20;
            problem = 5;
        }
        if (problem == 1) {
            wk->pickFlags |= 0x100;
        }
        wk->pickIndex = 0;
        if (problem == 0) {
            *seq = LIVE_TEAM_STORE_TEAM;
            break;
        }
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 41, TRUE, LIVE_TEAM_WAIT_MESSAGE, LIVE_TEAM_SHOW_PROBLEM);
        break;
    }
    case LIVE_TEAM_SHOW_PROBLEM: {
        int index = wk->pickIndex;
        int mask = 1 << index;

        while (mask <= 0x100) {
            if (wk->pickFlags & mask) {
                break;
            }
            index = ++wk->pickIndex;
            mask = 1 << index;
        }
        if (mask <= 0x100) {
            LiveBattleMatchFlow_PrintAndWait(wk, task, seq, index + 42, TRUE, LIVE_TEAM_WAIT_MESSAGE,
                                             LIVE_TEAM_NEXT_PROBLEM);
        } else {
            *seq = LIVE_TEAM_NEXT_PROBLEM;
        }
        break;
    }
    case LIVE_TEAM_NEXT_PROBLEM: {
        int index = ++wk->pickIndex;
        int mask = 1 << index;

        while (mask <= 0x100) {
            if (wk->pickFlags & mask) {
                break;
            }
            index = ++wk->pickIndex;
            mask = 1 << index;
        }
        if (mask <= 0x100) {
            *seq = LIVE_TEAM_SHOW_PROBLEM;
        } else {
            *seq = LIVE_TEAM_SHOW_PROBLEMS_END;
        }
        break;
    }
    case LIVE_TEAM_SHOW_PROBLEMS_END:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 51, TRUE, LIVE_TEAM_WAIT_MESSAGE, LIVE_TEAM_HIDE_CARD);
        break;
    case LIVE_TEAM_SHOW_NO_POKEMON:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 37, TRUE, LIVE_TEAM_WAIT_MESSAGE, LIVE_TEAM_HIDE_CARD);
        break;
    case LIVE_TEAM_HIDE_CARD:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            LiveBattleMatchFlow_PlayerCardDelete(wk);
            *seq = LIVE_TEAM_EXIT;
        }
        break;
    case LIVE_TEAM_STORE_TEAM: {
        SaveControl *save = GameData_GetSaveControl(wk->param.gameData);
        BattleBoxSave *battleBox = getBattleBox(save);
        TournamentTeam *records = TournamentBlock_GetTeam(getTournamentBlockAddress(save), 1);
        PokeParty *party = convertBoxedPokeSetToParty(battleBox, HEAPID_LIVEBATTLEMATCH_TEAM);

        BattleBoxSave_SetFlags(battleBox, 2);
        TournamentEntry_SetField(wk->entry, TOURNAMENT_FIELD_REGISTRATION_STATE, 2);
        TournamentTeam_SetFromParty(records, party);
        GFL_HeapFree(party);
        *seq = LIVE_TEAM_SHOW_SAVING;
        break;
    }
    case LIVE_TEAM_SHOW_SAVING:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, LIVE_TEAM_WAIT_MESSAGE, LIVE_TEAM_START_SAVE);
        break;
    case LIVE_TEAM_START_SAVE:
        GameData_StartSave(wk->param.gameData);
        *seq = LIVE_TEAM_WAIT_SAVE;
        break;
    case LIVE_TEAM_WAIT_SAVE:
        if (GameData_UpdateSave(wk->param.gameData) == SAVE_RESULT_OK) {
            *seq = LIVE_TEAM_SHOW_REGISTERED;
        }
        break;
    case LIVE_TEAM_SHOW_REGISTERED:
        LiveBattleMatchFlow_PlayerCardShow(wk, WIFIBATTLEMATCH_CARD_PAGE_REGISTERED);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 11, TRUE, LIVE_TEAM_WAIT_MESSAGE, LIVE_TEAM_START_MENU);
        break;
    case LIVE_TEAM_START_MENU:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
        break;
    case LIVE_TEAM_HIDE_CARD_AND_TEAM: {
        BOOL done = TRUE;

        done &= LiveBattleMatchFlow_PlayerCardIsDone(wk);
        if (LiveBattleMatchFlow_TeamListIsDone(wk) & done) {
            LiveBattleMatchFlow_PlayerCardDelete(wk);
            LiveBattleMatchFlow_TeamListDelete(wk);
            *seq = LIVE_TEAM_EXIT;
        }
        break;
    }
    case LIVE_TEAM_EXIT:
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_MENU);
        break;
    case LIVE_TEAM_WAIT_MESSAGE:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

// The menu of a player who has an entry: to take part in a match, to see the card, to quit
static void LiveBattleMatchFlow_StepMenu(WifiBattleMatchTask *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;

    switch (*seq) {
    case LIVE_MENU_SHOW_CARD:
        LiveBattleMatchFlow_FillPlayerData(wk->param.playerData, wk);
        if (wk->playerCard != NULL) {
            *seq = LIVE_MENU_CHECK_MATCH_LIMIT;
            break;
        }
        LiveBattleMatchFlow_PlayerCardCreate(wk);
        LiveBattleMatchFlow_PlayerCardShow(wk, WIFIBATTLEMATCH_CARD_PAGE_REGISTERED);
        *seq = LIVE_MENU_WAIT_CARD;
        break;
    case LIVE_MENU_WAIT_CARD:
        if (LiveBattleMatchFlow_PlayerCardIsReady(wk)) {
            *seq = LIVE_MENU_CHECK_DISCONNECT;
        }
        break;
    case LIVE_MENU_CHECK_DISCONNECT:
        if (wk->param.matchResult->disconnected) {
            *seq = LIVE_MENU_SHOW_DISCONNECT;
        } else {
            *seq = LIVE_MENU_CHECK_MATCH_LIMIT;
        }
        break;
    case LIVE_MENU_SHOW_DISCONNECT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 39, TRUE, LIVE_MENU_WAIT_MESSAGE, LIVE_MENU_CHECK_MATCH_LIMIT);
        break;
    case LIVE_MENU_CHECK_MATCH_LIMIT: {
        Regulation *regulation = TournamentEntry_GetRegulation(wk->entry);
        u32 played = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_PLAYED);

        if (played >= Regulation_GetParam(regulation, REGULATION_PARAM_MAX_BATTLES)) {
            WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepQuit);
        } else {
            *seq = LIVE_MENU_SHOW_PROMPT;
        }
        break;
    }
    case LIVE_MENU_SHOW_PROMPT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 12, TRUE, LIVE_MENU_WAIT_MESSAGE, LIVE_MENU_OPEN_MENU);
        break;
    case LIVE_MENU_OPEN_MENU:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_ACTIVE);
        *seq = LIVE_MENU_WAIT_MENU;
        break;
    case LIVE_MENU_WAIT_MENU:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        if (wk->hasVideo && !wk->param.matchResult->disconnected) {
            switch (selection) {
            case LIVE_ACTIVE_CHOICE_BATTLE:
                *seq = LIVE_MENU_START_LINK;
                break;
            case LIVE_ACTIVE_CHOICE_REPLAY:
                *seq = LIVE_MENU_CHECK_VIDEO;
                break;
            case LIVE_ACTIVE_CHOICE_RESULT:
                *seq = LIVE_MENU_SHOW_RESULT;
                break;
            case LIVE_ACTIVE_CHOICE_QUIT:
                *seq = LIVE_MENU_SHOW_QUIT_PROMPT;
                break;
            }
        } else {
            switch (selection) {
            case LIVE_ACTIVE_NO_VIDEO_CHOICE_BATTLE:
                *seq = LIVE_MENU_START_LINK;
                break;
            case LIVE_ACTIVE_NO_VIDEO_CHOICE_QUIT:
                *seq = LIVE_MENU_SHOW_QUIT_PROMPT;
                break;
            }
        }
        break;
    case LIVE_MENU_SHOW_QUIT_PROMPT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 17, TRUE, LIVE_MENU_WAIT_MESSAGE, LIVE_MENU_OPEN_QUIT_MENU);
        break;
    case LIVE_MENU_OPEN_QUIT_MENU:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_YES_NO_DEFAULT_NO);
        *seq = LIVE_MENU_WAIT_QUIT_MENU;
        break;
    case LIVE_MENU_WAIT_QUIT_MENU:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case LIVE_CHOICE_YES:
            *seq = LIVE_MENU_SHOW_QUIT_CONFIRM;
            break;
        case LIVE_CHOICE_NO:
            *seq = LIVE_MENU_SHOW_PROMPT;
            break;
        }
        break;
    case LIVE_MENU_SHOW_QUIT_CONFIRM:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 18, TRUE, LIVE_MENU_WAIT_MESSAGE, LIVE_MENU_OPEN_QUIT_CONFIRM);
        break;
    case LIVE_MENU_OPEN_QUIT_CONFIRM:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_YES_NO_DEFAULT_NO);
        *seq = LIVE_MENU_WAIT_QUIT_CONFIRM;
        break;
    case LIVE_MENU_WAIT_QUIT_CONFIRM:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case LIVE_CHOICE_YES:
            *seq = LIVE_MENU_SHOW_SAVING;
            break;
        case LIVE_CHOICE_NO:
            *seq = LIVE_MENU_SHOW_PROMPT;
            break;
        }
        break;
    case LIVE_MENU_SHOW_SAVING:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, LIVE_MENU_WAIT_MESSAGE, LIVE_MENU_UNREGISTER);
        break;
    case LIVE_MENU_UNREGISTER:
        TournamentEntry_SetField(wk->entry, TOURNAMENT_FIELD_REGISTRATION_STATE, 4);
        BattleBoxSave_ClearFlags(getBattleBox(GameData_GetSaveControl(wk->param.gameData)), 2);
        GameData_StartSave(wk->param.gameData);
        *seq = LIVE_MENU_WAIT_SAVE;
        break;
    case LIVE_MENU_WAIT_SAVE:
        switch (GameData_UpdateSave(wk->param.gameData)) {
        case SAVE_RESULT_CONTINUE:
        case SAVE_RESULT_LAST:
            break;
        case SAVE_RESULT_OK:
        case SAVE_RESULT_NG:
            *seq = LIVE_MENU_SHOW_UNREGISTERED;
            break;
        }
        break;
    case LIVE_MENU_SHOW_UNREGISTERED:
        LiveBattleMatchFlow_PlayerCardShow(wk, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 19, TRUE, LIVE_MENU_WAIT_MESSAGE, LIVE_MENU_HIDE_CARD);
        break;
    case LIVE_MENU_HIDE_CARD:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            LiveBattleMatchFlow_PlayerCardDelete(wk);
            *seq = LIVE_MENU_EXIT;
        }
        break;
    case LIVE_MENU_CHECK_VIDEO:
        if (BattleRec_IsVersionSupported((u8)wk->param.partnerData->battleRecordVersion)) {
            *seq = LIVE_MENU_SHOW_VIDEO_PROMPT;
        } else {
            *seq = LIVE_MENU_SHOW_VIDEO_UNSUPPORTED;
        }
        break;
    case LIVE_MENU_SHOW_VIDEO_PROMPT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 16, TRUE, LIVE_MENU_WAIT_MESSAGE, LIVE_MENU_REPLAY);
        break;
    case LIVE_MENU_SHOW_VIDEO_UNSUPPORTED:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 34, TRUE, LIVE_MENU_WAIT_MESSAGE, LIVE_MENU_SHOW_PROMPT);
        break;
    case LIVE_MENU_REPLAY:
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_REPLAY);
        break;
    case LIVE_MENU_START_LINK:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepLink);
        break;
    case LIVE_MENU_EXIT:
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_MENU);
        break;
    case LIVE_MENU_WAIT_MESSAGE:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    case LIVE_MENU_SHOW_RESULT:
        LiveBattleMatchFlow_PrintResult(wk);
        *seq = LIVE_MENU_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, LIVE_MENU_SHOW_PROMPT);
        break;
    }
}

// Links with the other player over infrared, trades the players' data and teams and shows the players to each other
static void LiveBattleMatchFlow_StepLink(WifiBattleMatchTask *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;
    int linked;
    void *received;

    if (LiveBattleMatchIrc_CheckError(wk->irc) == TRUE) {
        wk->param.matchResult->disconnected = TRUE;
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
    }
    switch (*seq) {
    case LIVE_LINK_SHOW_INTRO:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 20, TRUE, LIVE_LINK_WAIT_MESSAGE, LIVE_LINK_SHOW_LINK_PROMPT);
        break;
    case LIVE_LINK_SHOW_LINK_PROMPT:
        LiveBattleMatchFlow_PrintMessage(wk, 33, FALSE);
        WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, TRUE);
        GFL_SndSEPlay(SEQ_SE_SYS_102);
        wk->readySent = FALSE;
        wk->readyReceived = FALSE;
        *seq = LIVE_LINK_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, LIVE_LINK_RESET_LINK);
        break;
    case LIVE_LINK_RESET_LINK:
        if (LiveBattleMatchIrc_ResetLink(wk->irc)) {
            *seq = LIVE_LINK_WAIT_LINK;
        }
        break;
    case LIVE_LINK_WAIT_LINK:
        linked = LiveBattleMatchIrc_StepLink(wk->irc);
        switch (linked) {
        case LIVE_IR_RESULT_CONNECTED:
            *seq = LIVE_LINK_SEND_PLAYER_DATA;
            break;
        case LIVE_IR_RESULT_LOST:
            *seq = LIVE_LINK_CANCEL_LOST_LINK;
            break;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *seq = LIVE_LINK_CANCEL_LINK;
        }
        break;
    case LIVE_LINK_CANCEL_LINK:
        LiveBattleMatchIrc_CancelLink(wk->irc);
        *seq = LIVE_LINK_WAIT_CANCEL;
        break;
    case LIVE_LINK_WAIT_CANCEL:
        if (LiveBattleMatchIrc_StepCancel(wk->irc)) {
            *seq = LIVE_LINK_SHOW_CANCELLED;
        }
        break;
    case LIVE_LINK_SHOW_CANCELLED:
        WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 21, TRUE, LIVE_LINK_WAIT_MESSAGE, LIVE_LINK_OPEN_CANCEL_MENU);
        break;
    case LIVE_LINK_OPEN_CANCEL_MENU:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_YES_NO);
        *seq = LIVE_LINK_WAIT_CANCEL_MENU;
        break;
    case LIVE_LINK_WAIT_CANCEL_MENU:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        switch (selection) {
        case LIVE_CHOICE_YES:
            WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
            break;
        case LIVE_CHOICE_NO:
            *seq = LIVE_LINK_SHOW_LINK_PROMPT;
            break;
        }
        break;
    case LIVE_LINK_CANCEL_LOST_LINK:
        WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
        LiveBattleMatchIrc_CancelLink(wk->irc);
        *seq = LIVE_LINK_WAIT_LOST_LINK;
        break;
    case LIVE_LINK_WAIT_LOST_LINK:
        if (LiveBattleMatchIrc_StepCancel(wk->irc)) {
            *seq = LIVE_LINK_SHOW_LOST_LINK;
        }
        break;
    case LIVE_LINK_SHOW_LOST_LINK:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 28, TRUE, LIVE_LINK_WAIT_MESSAGE,
                                         LIVE_LINK_RETURN_AFTER_LOST_LINK);
        break;
    case LIVE_LINK_RETURN_AFTER_LOST_LINK:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
        break;
    case LIVE_LINK_SEND_PLAYER_DATA:
        LiveBattleMatchFlow_SetEntryField2(wk->param.playerData, wk);
        if (LiveBattleMatchIrc_SendPlayerData(wk->irc, wk->param.playerData)) {
            *seq = LIVE_LINK_RECEIVE_PLAYER_DATA;
        }
        break;
    case LIVE_LINK_RECEIVE_PLAYER_DATA:
        if (LiveBattleMatchIrc_TakePlayerData(wk->irc, &received)) {
            MI_CpuCopy8(received, wk->param.partnerData, PokeParty_GetSaveDataSize() + sizeof(WifiBattleMatchPlayer));
            *seq = LIVE_LINK_CHECK_COMPETITION;
        }
        break;
    case LIVE_LINK_CHECK_COMPETITION:
        if (wk->param.playerData->entryField2 == wk->param.partnerData->entryField2) {
            u32 field12 = TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_ALLOW_REMATCH);
            BOOL known = LiveMatchData_HasOpponent(wk->record, wk->param.partnerData->mac);

            wk->readySent = (field12 != 0 || (field12 == 0 && !known));
            *seq = LIVE_LINK_SYNC_READY;
        } else {
            WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
            GFL_SndStop();
            *seq = LIVE_LINK_RESET_WRONG_COMPETITION;
        }
        break;
    case LIVE_LINK_SYNC_READY:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x48, 0x1f);
        *seq = LIVE_LINK_WAIT_SYNC_READY;
        break;
    case LIVE_LINK_WAIT_SYNC_READY:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x48, 0x1f)) {
            *seq = LIVE_LINK_SEND_READY;
        }
        break;
    case LIVE_LINK_SEND_READY:
        if (LiveBattleMatchIrc_SendReady(wk->irc, &wk->readySent)) {
            *seq = LIVE_LINK_RECEIVE_READY;
        }
        break;
    case LIVE_LINK_RECEIVE_READY:
        if (LiveBattleMatchIrc_TakeReady(wk->irc, &wk->readyReceived)) {
            *seq = LIVE_LINK_CHECK_READY;
        }
        break;
    case LIVE_LINK_CHECK_READY:
        WifiBattleMatchLinkIcon_SetShown(wk->linkIcon, FALSE);
        GFL_SndStop();
        if (wk->readyReceived != 0 && wk->readySent != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_74);
            *seq = LIVE_LINK_SHOW_PARTNER_FOUND;
        } else {
            *seq = LIVE_LINK_RESET_REMATCH;
        }
        break;
    case LIVE_LINK_RESET_WRONG_COMPETITION:
        if (LiveBattleMatchIrc_ResetEnd(wk->irc)) {
            *seq = LIVE_LINK_WAIT_WRONG_COMPETITION;
        }
        break;
    case LIVE_LINK_WAIT_WRONG_COMPETITION:
        if (LiveBattleMatchIrc_StepEnd(wk->irc)) {
            *seq = LIVE_LINK_SHOW_WRONG_COMPETITION;
        }
        break;
    case LIVE_LINK_SHOW_WRONG_COMPETITION:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 22, TRUE, LIVE_LINK_WAIT_MESSAGE, LIVE_LINK_RETURN_MENU);
        break;
    case LIVE_LINK_RESET_REMATCH:
        if (LiveBattleMatchIrc_ResetEnd(wk->irc)) {
            *seq = LIVE_LINK_WAIT_REMATCH;
        }
        break;
    case LIVE_LINK_WAIT_REMATCH:
        if (LiveBattleMatchIrc_StepEnd(wk->irc)) {
            *seq = LIVE_LINK_SHOW_REMATCH_DISALLOWED;
        }
        break;
    case LIVE_LINK_SHOW_REMATCH_DISALLOWED:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 36, TRUE, LIVE_LINK_WAIT_MESSAGE, LIVE_LINK_RETURN_MENU);
        break;
    case LIVE_LINK_SHOW_PARTNER_FOUND:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 23, TRUE, LIVE_LINK_WAIT_MESSAGE, LIVE_LINK_WAIT_PARTNER_DELAY);
        break;
    case LIVE_LINK_WAIT_PARTNER_DELAY:
        if (wk->frames++ > 120) {
            wk->frames = 0;
            GFL_BGSysSetBGEnabled(0, FALSE);
            *seq = LIVE_LINK_SHOW_PARTNER_CARD;
        }
        break;
    case LIVE_LINK_SHOW_PARTNER_CARD:
        LiveBattleMatchFlow_PartnerCardCreate(wk, wk->param.partnerData);
        *seq = LIVE_LINK_WAIT_PARTNER_CARD;
        break;
    case LIVE_LINK_WAIT_PARTNER_CARD:
        if (LiveBattleMatchFlow_PartnerCardIsDone(wk)) {
            *seq = LIVE_LINK_WAIT_PARTNER_DISPLAY;
        }
        break;
    case LIVE_LINK_WAIT_PARTNER_DISPLAY:
        if (wk->frames++ > 300) {
            wk->frames = 0;
            *seq = LIVE_LINK_SHOW_BATTLE_PROMPT;
        }
        break;
    case LIVE_LINK_SHOW_BATTLE_PROMPT:
        GFL_BGSysSetBGEnabled(0, TRUE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 24, TRUE, LIVE_LINK_WAIT_MESSAGE, LIVE_LINK_SHOW_STARTING);
        break;
    case LIVE_LINK_SHOW_STARTING:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 35, 2, LIVE_LINK_WAIT_MESSAGE, LIVE_LINK_SYNC_START);
        break;
    case LIVE_LINK_SYNC_START:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x47, 0x1f);
        *seq = LIVE_LINK_WAIT_SYNC_START;
        break;
    case LIVE_LINK_WAIT_SYNC_START:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x47, 0x1f)) {
            *seq = LIVE_LINK_SHOW_BATTLE_WAIT;
        }
        break;
    case LIVE_LINK_SHOW_BATTLE_WAIT:
        GFL_BGSysSetBGEnabled(0, TRUE);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 38, 2, LIVE_LINK_WAIT_MESSAGE, LIVE_LINK_WAIT_BATTLE_DELAY);
        break;
    case LIVE_LINK_WAIT_BATTLE_DELAY:
        if (wk->frames++ > 60) {
            Regulation *regulation = TournamentEntry_GetRegulation(wk->entry);

            wk->frames = 0;
            Regulation_GetParam(regulation, REGULATION_PARAM_SHOW_PARTNERS);
            *seq = LIVE_LINK_START_BATTLE;
        }
        break;
    case LIVE_LINK_START_BATTLE:
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_BATTLE);
        break;
    case LIVE_LINK_RETURN_MENU:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
        break;
    case LIVE_LINK_WAIT_MESSAGE:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

// Records how the match went, and shows the player the result
static void LiveBattleMatchFlow_StepRecord(WifiBattleMatchTask *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;

    if (LiveBattleMatchIrc_CheckError(wk->irc) == TRUE) {
        wk->param.matchResult->disconnected = TRUE;
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
    }
    switch (*seq) {
    case LIVE_RECORD_RECORD_TRAINER: {
        RivalDataSave *rival = getHollow_RivalData(GameData_GetSaveControl(wk->param.gameData));

        Misc_AddMetTrainer(rival, getIDAsUInt(&wk->param.partnerData->info));
        *seq = LIVE_RECORD_RESET_LINK_END;
        break;
    }
    case LIVE_RECORD_RESET_LINK_END:
        if (LiveBattleMatchIrc_ResetEnd(wk->irc)) {
            *seq = LIVE_RECORD_WAIT_LINK_END;
        }
        break;
    case LIVE_RECORD_WAIT_LINK_END:
        if (LiveBattleMatchIrc_StepEnd(wk->irc)) {
            *seq = LIVE_RECORD_RECORD_RESULT;
        }
        break;
    case LIVE_RECORD_RECORD_RESULT: {
        u8 won;
        u8 lost;
        u8 played = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_PLAYED);

        LiveMatchData_SetOpponentMacAddress(wk->record, played, wk->param.partnerData->mac);
        LiveMatchData_SetOpponentParam(wk->record, played, LIVEMATCH_OPPONENT_PARAM_ALIVE,
                                       wk->param.matchResult->aliveOther);
        LiveMatchData_SetOpponentParam(wk->record, played, LIVEMATCH_OPPONENT_PARAM_HP_PERCENT,
                                       wk->param.matchResult->otherHpPercent);
        won = 0;
        lost = 0;
        switch (wk->param.matchResult->result) {
        case BTL_RESULT_VICTORY:
        case BTL_RESULT_FLEE_FOE:
            won = 1;
            break;
        case BTL_RESULT_DEFEAT:
        case BTL_RESULT_FLEE:
            lost = 1;
            break;
        case BTL_RESULT_DRAW:
            break;
        default:
            GFL_ASSERT_MSG(FALSE, "\x83\x6f\x83\x67\x83\x8b\x8c\x8b\x89\xca\x95\x73\x90\xb3\x92\x6c %d\n",
                           wk->param.matchResult->result);
            break;
        }
        LiveMatchData_AddCount(wk->record, LIVEMATCH_COUNT_WON, won);
        LiveMatchData_AddCount(wk->record, LIVEMATCH_COUNT_LOST, lost);
        LiveMatchData_AddCount(wk->record, LIVEMATCH_COUNT_PLAYED, 1);
        LiveBattleMatchFlow_PlayerCardCreateWait(wk);
        *seq = LIVE_RECORD_SHOW_SAVING;
        break;
    }
    case LIVE_RECORD_SHOW_SAVING:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, LIVE_RECORD_WAIT_MESSAGE, LIVE_RECORD_START_SAVE);
        break;
    case LIVE_RECORD_START_SAVE:
        GameData_StartSave(wk->param.gameData);
        *seq = LIVE_RECORD_WAIT_SAVE;
        break;
    case LIVE_RECORD_WAIT_SAVE:
        if (GameData_UpdateSave(wk->param.gameData) == SAVE_RESULT_OK) {
            *seq = LIVE_RECORD_EXIT;
        }
        break;
    case LIVE_RECORD_EXIT:
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_RECORDED);
        break;
    case LIVE_RECORD_WAIT_MESSAGE:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

// The flow of a player whose entry has ended
static void LiveBattleMatchFlow_StepQuit(WifiBattleMatchTask *task, u32 *seq, void *work) {
    LiveBattleMatchFlow *wk = work;
    s32 selection;

    switch (*seq) {
    case LIVE_QUIT_SHOW_CARD:
        if (TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_REGISTRATION_STATE) == 3) {
            LiveBattleMatchFlow_PlayerCardCreate(wk);
            LiveBattleMatchFlow_PlayerCardShow(wk, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
            while (!LiveBattleMatchFlow_PlayerCardIsReady(wk)) {
            }
            *seq = LIVE_QUIT_SHOW_PROMPT;
        } else {
            LiveBattleMatchFlow_PlayerCardCreateWait(wk);
            *seq = LIVE_QUIT_CHECK_MATCH_LIMIT;
        }
        break;
    case LIVE_QUIT_CHECK_MATCH_LIMIT: {
        Regulation *regulation = TournamentEntry_GetRegulation(wk->entry);
        u32 played = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_PLAYED);

        if (played >= Regulation_GetParam(regulation, REGULATION_PARAM_MAX_BATTLES)) {
            *seq = LIVE_QUIT_SHOW_SAVING;
        } else {
            *seq = LIVE_QUIT_RETURN_MENU;
        }
        break;
    }
    case LIVE_QUIT_SHOW_SAVING:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 10, 2, LIVE_QUIT_WAIT_MESSAGE, LIVE_QUIT_FINISH_ENTRY);
        break;
    case LIVE_QUIT_FINISH_ENTRY:
        TournamentEntry_SetField(wk->entry, TOURNAMENT_FIELD_REGISTRATION_STATE, 3);
        BattleBoxSave_ClearFlags(getBattleBox(GameData_GetSaveControl(wk->param.gameData)), 2);
        GameData_StartSave(wk->param.gameData);
        *seq = LIVE_QUIT_WAIT_SAVE;
        break;
    case LIVE_QUIT_WAIT_SAVE:
        switch (GameData_UpdateSave(wk->param.gameData)) {
        case SAVE_RESULT_CONTINUE:
        case SAVE_RESULT_LAST:
            break;
        case SAVE_RESULT_OK:
        case SAVE_RESULT_NG:
            *seq = LIVE_QUIT_SHOW_FINISHED;
            break;
        }
        break;
    case LIVE_QUIT_SHOW_FINISHED:
        LiveBattleMatchFlow_PlayerCardShow(wk, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 25, TRUE, LIVE_QUIT_WAIT_MESSAGE, LIVE_QUIT_SHOW_PROMPT);
        break;
    case LIVE_QUIT_SHOW_PROMPT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 29, TRUE, LIVE_QUIT_WAIT_MESSAGE, LIVE_QUIT_OPEN_MENU);
        break;
    case LIVE_QUIT_OPEN_MENU:
        LiveBattleMatchFlow_MenuStart(wk, LIVE_FLOW_MENU_FINISHED);
        *seq = LIVE_QUIT_WAIT_MENU;
        break;
    case LIVE_QUIT_WAIT_MENU:
        selection = LiveBattleMatchFlow_MenuGetSelection(wk);
        if (selection == -1) {
            break;
        }
        LiveBattleMatchFlow_MenuFree(wk);
        if (wk->hasVideo && !wk->param.matchResult->disconnected) {
            switch (selection) {
            case LIVE_FINISHED_CHOICE_REPLAY:
                *seq = LIVE_QUIT_CHECK_VIDEO;
                break;
            case LIVE_FINISHED_CHOICE_RESULT:
                *seq = LIVE_QUIT_SHOW_RESULT;
                break;
            case LIVE_FINISHED_CHOICE_EXIT:
                *seq = LIVE_QUIT_SHOW_EXIT;
                break;
            }
        } else if (selection == LIVE_FINISHED_NO_VIDEO_CHOICE_EXIT) {
            *seq = LIVE_QUIT_SHOW_EXIT;
        }
        break;
    case LIVE_QUIT_SHOW_EXIT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 31, TRUE, LIVE_QUIT_WAIT_MESSAGE, LIVE_QUIT_HIDE_CARD);
        break;
    case LIVE_QUIT_HIDE_CARD:
        if (LiveBattleMatchFlow_PlayerCardIsDone(wk)) {
            LiveBattleMatchFlow_PlayerCardDelete(wk);
            *seq = LIVE_QUIT_EXIT;
        }
        break;
    case LIVE_QUIT_CHECK_VIDEO:
        if (BattleRec_IsVersionSupported((u8)wk->param.partnerData->battleRecordVersion)) {
            *seq = LIVE_QUIT_SHOW_VIDEO_PROMPT;
        } else {
            *seq = LIVE_QUIT_SHOW_VIDEO_UNSUPPORTED;
        }
        break;
    case LIVE_QUIT_SHOW_VIDEO_PROMPT:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 16, TRUE, LIVE_QUIT_WAIT_MESSAGE, LIVE_QUIT_REPLAY);
        break;
    case LIVE_QUIT_SHOW_VIDEO_UNSUPPORTED:
        LiveBattleMatchFlow_PrintAndWait(wk, task, seq, 34, TRUE, LIVE_QUIT_WAIT_MESSAGE, LIVE_QUIT_SHOW_PROMPT);
        break;
    case LIVE_QUIT_REPLAY:
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_REPLAY);
        break;
    case LIVE_QUIT_EXIT:
        LiveBattleMatchFlow_Exit(wk, LIVE_FLOW_RESULT_MENU);
        break;
    case LIVE_QUIT_RETURN_MENU:
        WifiBattleMatchTask_SetFunc(task, LiveBattleMatchFlow_StepMenu);
        break;
    case LIVE_QUIT_WAIT_MESSAGE:
        if (LiveBattleMatchFlow_IsMessageDone(wk)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    case LIVE_QUIT_SHOW_RESULT:
        LiveBattleMatchFlow_PrintResult(wk);
        *seq = LIVE_QUIT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, LIVE_QUIT_SHOW_PROMPT);
        break;
    }
}

// Shows a menu of kind 0 (yes or no at the right), 1 (the menu of a player with an entry), 2 (the menu of a player
// whose entry ended) or 3 (yes or no, which can be left)
static void LiveBattleMatchFlow_MenuStart(LiveBattleMatchFlow *wk, u32 kind) {
    WifiBattleMatchMenuParam param;
    u32 corner;
    u8 height;
    u8 x;
    u8 y;
    u8 width;

    MI_CpuFill8(&param, 0, sizeof(WifiBattleMatchMenuParam));
    param.msgData = wk->msgMain;
    param.font = wk->param.font;
    param.printQueue = wk->param.printQueue;
    switch (kind) {
    case LIVE_FLOW_MENU_YES_NO_DEFAULT_NO:
        param.cursorRow = 1;
    case LIVE_FLOW_MENU_YES_NO:
        param.msgIds[0] = 6;
        param.msgIds[1] = 7;
        param.count = 2;
        param.canCancel = 1;
        param.cancelValue = 1;
        corner = LIVE_FLOW_LAYOUT_RIGHT;
        break;
    case LIVE_FLOW_MENU_ACTIVE:
        param.canCancel = 0;
        if (wk->hasVideo && !wk->param.matchResult->disconnected) {
            param.msgIds[0] = 13;
            param.msgIds[1] = 14;
            param.msgIds[2] = 40;
            param.msgIds[3] = 15;
            param.count = 4;
        } else {
            param.msgIds[0] = 13;
            param.msgIds[1] = 15;
            param.count = 2;
        }
        corner = LIVE_FLOW_LAYOUT_CENTER;
        break;
    case LIVE_FLOW_MENU_FINISHED:
        param.canCancel = 0;
        if (wk->hasVideo && !wk->param.matchResult->disconnected) {
            param.msgIds[0] = 14;
            param.msgIds[1] = 40;
            param.msgIds[2] = 30;
            param.count = 3;
        } else {
            param.msgIds[0] = 30;
            param.count = 1;
        }
        corner = LIVE_FLOW_LAYOUT_CENTER;
        break;
    }
    param.bg = 0;
    param.palette = 15;
    param.framePalette = 13;
    param.frameChar = 1;
    switch (corner) {
    case LIVE_FLOW_LAYOUT_CENTER:
        width = 28;
        height = param.count * 2;
        x = 2;
        y = 9 - height / 2;
        break;
    case LIVE_FLOW_LAYOUT_RIGHT:
        width = 12;
        height = param.count * 2;
        x = 19;
        y = 17 - height;
        break;
    }
    wk->menu = WifiBattleMatchMenu_Create(&param, x, y, width, height, wk->heapId);
    wk->menuKind = kind;
}

static void LiveBattleMatchFlow_MenuFree(LiveBattleMatchFlow *wk) {
    WifiBattleMatchMenu_Free(wk->menu);
}

static s32 LiveBattleMatchFlow_MenuGetSelection(LiveBattleMatchFlow *wk) {
    return WifiBattleMatchMenu_Update(wk->menu);
}

static void LiveBattleMatchFlow_PrintMessage(LiveBattleMatchFlow *wk, u32 msgId, u32 type) {
    WifiBattleMatchMsgBox_Print(wk->param.msgBox, wk->msgMain, msgId, type);
}

static BOOL LiveBattleMatchFlow_IsMessageDone(LiveBattleMatchFlow *wk) {
    return WifiBattleMatchMsgBox_IsDone(wk->param.msgBox);
}

// Ends the flow with a result: 1 to 4
static void LiveBattleMatchFlow_Exit(LiveBattleMatchFlow *wk, u32 result) {
    wk->result = result;
    WifiBattleMatchTask_SetFunc(wk->task, LiveBattleMatchFlow_StepEnd);
}

// Three bytes as characters, the first at the top
static inline u32 LiveBattleMatchFlow_PackCode(u32 a, u32 b, u32 c) {
    return a << 24 | (b & 0xff) << 16 | (c & 0xff) << 8;
}

static void LiveBattleMatchFlow_PlayerCardCreate(LiveBattleMatchFlow *wk) {
    if (wk->playerCard == NULL) {
        SaveControl *save = GameData_GetSaveControl(wk->param.gameData);
        BattleBoxSave *battleBox = getBattleBox(save);
        Regulation *regulation = TournamentEntry_GetRegulation(wk->entry);
        TournamentTeam *records = TournamentBlock_GetTeam(getTournamentBlockAddress(save), 1);
        PlayerInfo *info = GetGameDataPlayerInfo(wk->param.gameData);
        ClActUnit *link = WifiBattleMatchGraphic_GetClActUnit(wk->param.net);
        WifiBattleMatchLiveCardInfo card;

        MI_CpuFill8(&card, 0, sizeof(WifiBattleMatchLiveCardInfo));
        card.wins = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_WON);
        card.losses = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_LOST);
        card.matchCount = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_PLAYED);
        card.maxBattles = Regulation_GetParam(regulation, REGULATION_PARAM_MAX_BATTLES);
        card.look = PlayerInfo_GetUnionView(info);
        MI_CpuCopy8(TournamentEntry_GetCardData(wk->entry), card.data, sizeof(card.data));
        card.startDate = LiveBattleMatchFlow_PackCode(TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_START_YEAR),
                                                      TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_START_MONTH),
                                                      TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_START_DAY));
        card.endDate = LiveBattleMatchFlow_PackCode(TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_END_YEAR),
                                                    TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_END_MONTH),
                                                    TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_END_DAY));
        card.variant = TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_VARIANT);
        wk->playerCard =
            WifiBattleMatchLiveCard_Create(&card, info, link, wk->param.seq, wk->param.font, wk->param.printQueue,
                                           wk->msgCard, wk->wordSet, records, 0, wk->heapId);
    }
}

static void LiveBattleMatchFlow_PlayerCardCreateWait(LiveBattleMatchFlow *wk) {
    LiveBattleMatchFlow_PlayerCardCreate(wk);
    while (!LiveBattleMatchFlow_PlayerCardIsReady(wk)) {
    }
}

static void LiveBattleMatchFlow_PlayerCardDelete(LiveBattleMatchFlow *wk) {
    if (wk->playerCard != NULL) {
        WifiBattleMatchLiveCard_Free(wk->playerCard);
        wk->playerCard = NULL;
    }
}

static BOOL LiveBattleMatchFlow_PlayerCardIsReady(LiveBattleMatchFlow *wk) {
    return WifiBattleMatchCard_SlideIn(wk->playerCard);
}

static BOOL LiveBattleMatchFlow_PlayerCardIsDone(LiveBattleMatchFlow *wk) {
    return WifiBattleMatchCard_SlideOut(wk->playerCard);
}

static void LiveBattleMatchFlow_PlayerCardShow(LiveBattleMatchFlow *wk, u32 page) {
    WifiBattleMatchLiveCard_ShowPage(wk->playerCard, page, wk->msgCard, wk->param.printQueue, wk->param.font,
                                     wk->heapId);
}

static void LiveBattleMatchFlow_PartnerCardCreate(LiveBattleMatchFlow *wk, WifiBattleMatchPlayer *data) {
    if (wk->partnerCard == NULL) {
        ClActUnit *link = WifiBattleMatchGraphic_GetClActUnit(wk->param.net);
        u32 field11 = TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_VARIANT);

        wk->partnerCard = WifiBattleMatchPartnerCard_Create(data, link, wk->param.seq, wk->param.font,
                                                            wk->param.printQueue, wk->msgCard, wk->wordSet,
                                                            WIFIBATTLEMATCH_PARTNER_CARD_LIVE, 0, field11, wk->heapId);
    }
}

static void LiveBattleMatchFlow_PartnerCardDelete(LiveBattleMatchFlow *wk) {
    if (wk->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Free(wk->partnerCard);
        wk->partnerCard = NULL;
    }
}

static BOOL LiveBattleMatchFlow_PartnerCardIsDone(LiveBattleMatchFlow *wk) {
    return WifiBattleMatchPartnerCard_SlideIn(wk->partnerCard);
}

static void LiveBattleMatchFlow_TeamListCreate(LiveBattleMatchFlow *wk) {
    if (wk->teamList == NULL) {
        ClActUnit *link = WifiBattleMatchGraphic_GetClActUnit(wk->param.net);
        BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(wk->param.gameData));

        wk->teamList = WifiBattleMatchTeamList_Create(battleBox, link, wk->param.seq, wk->font, wk->param.printQueue,
                                                      wk->msgCard, wk->heapId);
    }
}

static void LiveBattleMatchFlow_TeamListDelete(LiveBattleMatchFlow *wk) {
    if (wk->teamList != NULL) {
        WifiBattleMatchTeamList_Free(wk->teamList);
        wk->teamList = NULL;
    }
}

static BOOL LiveBattleMatchFlow_TeamListIsReady(LiveBattleMatchFlow *wk) {
    return WifiBattleMatchTeamList_SlideIn(wk->teamList);
}

static BOOL LiveBattleMatchFlow_TeamListIsDone(LiveBattleMatchFlow *wk) {
    return WifiBattleMatchTeamList_SlideOut(wk->teamList);
}

// Copies the team of the battle box into dest
static void LiveBattleMatchFlow_LoadTeam(LiveBattleMatchFlow *wk, PokeParty *dest) {
    PokeParty *party =
        convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(wk->param.gameData)), wk->heapId);

    MI_CpuCopy8(party, dest, PokeParty_GetSaveDataSize());
    GFL_HeapFree(party);
}

// Fills what is sent to the other player
static void LiveBattleMatchFlow_FillPlayerData(WifiBattleMatchPlayer *data, LiveBattleMatchFlow *wk) {
    GameData *gameData = wk->param.gameData;
    PokeParty *party;

    data->battleRecordVersion = 100;
    MI_CpuCopy8(GetGameDataPlayerInfo(gameData), data, PlayerInfo_GetSize());
    OS_GetMacAddress(data->mac);
    CGearPhrases_Get(CGearPhrases_GetBlock(GameData_GetSaveControl(gameData)), 0, &data->sentence);
    party = convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(gameData)), HEAPID_LIVEBATTLEMATCH_TEAM);
    MI_CpuCopy8(party, data->partyData, PokeParty_GetSaveDataSize());
    GFL_HeapFree(party);
    data->record1 = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_WON);
    data->record2 = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_LOST);
    data->record0 = LiveMatchData_GetCount(wk->record, LIVEMATCH_COUNT_PLAYED);
    data->entryField2 = TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_CUP_ID);
}

static void LiveBattleMatchFlow_SetEntryField2(WifiBattleMatchPlayer *data, LiveBattleMatchFlow *wk) {
    data->entryField2 = TournamentEntry_GetField(wk->entry, TOURNAMENT_FIELD_CUP_ID);
}

// The message for how the last match ended
static u32 LiveBattleMatchFlow_GetResultMessageId(LiveBattleMatchFlow *wk) {
    switch (wk->param.matchResult->result) {
    case BTL_RESULT_FLEE_FOE:
        return 56;
    case BTL_RESULT_VICTORY:
        return 52;
    case BTL_RESULT_FLEE:
        return 55;
    case BTL_RESULT_DEFEAT:
        return 53;
    default:
        GFL_ASSERT_MSG(FALSE, "\x83\x6f\x83\x67\x83\x8b\x8c\x8b\x89\xca\x95\x73\x90\xb3\x92\x6c %d\n",
                       wk->param.matchResult->result);
    case BTL_RESULT_DRAW:
        return 54;
    }
}

static void LiveBattleMatchFlow_PrintResult(LiveBattleMatchFlow *wk) {
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgMain, LiveBattleMatchFlow_GetResultMessageId(wk));
    StrBuf *text = GFL_StrBufCreate(GFL_StrBufGetCharCount(format) + 0x40, wk->heapId);
    WifiBattleMatchBattleResult *matchResult = wk->param.matchResult;
    PlayerInfo *info = GetGameDataPlayerInfo(wk->param.gameData);
    WifiBattleMatchPlayer *partnerData = wk->param.partnerData;

    copyVarForText(wk->wordSet, 0, info);
    copyVarForText(wk->wordSet, 1, &partnerData->info);
    WordSetNumber(wk->wordSet, 2, matchResult->aliveOwn, 1, 0, TRUE);
    WordSetNumber(wk->wordSet, 3, matchResult->aliveOther, 1, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, text, format);
    WifiBattleMatchMsgBox_PrintStr(wk->param.msgBox, text, TRUE);
    GFL_StrBufFree(text);
    GFL_StrBufFree(format);
}
