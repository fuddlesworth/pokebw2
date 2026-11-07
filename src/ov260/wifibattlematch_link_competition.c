// wifibattlematch_link_competition.c, in overlay 260 (a descriptive name: the ROM names only the next file,
// wifibattlematch_net.c): the Wi-Fi competition of the title screen's menu. The link checks the account on the
// Global Link server and the competition's period, updates the Digital Player ID, registers the Battle Box, finds an
// opponent, checks the team and the player's data that the other sends, and leaves overlay 290 to pick the teams
// and battle. Each step is a task of overlay 262 (wifibattlematch_util.c) that goes through its states with a
// message box and menus

#include "types.h"
#include "app/livebattlematch_flow.h"
#include "app/wifibattlematch_graphic.h"
#include "app/wifibattlematch_link.h"
#include "app/wifibattlematch_net.h"
#include "app/wifibattlematch_util.h"
#include "app/wifibattlematch_view.h"
#include "battle/btl_result.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
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
#include "gfl/str_sjis.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
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
    COMPETITION_PHASE_FADE_IN = 0,
    COMPETITION_PHASE_WAIT_FADE_IN = 1,
    COMPETITION_PHASE_RUN_TASK = 2,
    COMPETITION_PHASE_FADE_OUT = 3,
    COMPETITION_PHASE_WAIT_FADE_OUT = 4,
};

enum {
    COMPETITION_ACCOUNT_CHECK_GAME_SYNC_ID = 0,
    COMPETITION_ACCOUNT_SHOW_SEARCH_MESSAGE = 1,
    COMPETITION_ACCOUNT_REQUEST_ACCOUNT = 2,
    COMPETITION_ACCOUNT_WAIT_ACCOUNT = 3,
    COMPETITION_ACCOUNT_CHECK_REGISTRATION = 4,
    COMPETITION_ACCOUNT_SHOW_UNREGISTERED_MESSAGE = 5,
    COMPETITION_ACCOUNT_WAIT_UNREGISTERED_MESSAGE = 6,
    COMPETITION_ACCOUNT_WAIT_LOGIN_COOLDOWN = 7,
    COMPETITION_ACCOUNT_LOGIN = 8,
    COMPETITION_ACCOUNT_START_ENTRY = 9,
};

enum {
    COMPETITION_ENTRY_CHECK_RESET_REQUEST = 0,
    COMPETITION_ENTRY_RESET_ACCOUNT_ENTRY = 1,
    COMPETITION_ENTRY_WAIT_ACCOUNT_RESET = 2,
    COMPETITION_ENTRY_CHECK_LOCAL_REGISTRATION = 3,
    COMPETITION_ENTRY_CHECK_CLOSED_REGISTRATION = 4,
    COMPETITION_ENTRY_SHOW_ENDED_MESSAGE = 5,
    COMPETITION_ENTRY_SHOW_UNAVAILABLE_MESSAGE = 6,
    COMPETITION_ENTRY_UPDATE_SERVER_REGISTRATION = 7,
    COMPETITION_ENTRY_WAIT_REGISTRATION_UPDATE = 8,
    COMPETITION_ENTRY_RETURN_TO_UNAVAILABLE_MESSAGE = 9,
    COMPETITION_ENTRY_WAIT_PREVIOUS_WRITE = 10,
    COMPETITION_ENTRY_SHOW_DOWNLOAD_MESSAGE = 11,
    COMPETITION_ENTRY_START_DOWNLOAD = 12,
    COMPETITION_ENTRY_WAIT_DOWNLOAD = 13,
    COMPETITION_ENTRY_CHECK_SERVER_REGISTRATION = 14,
    COMPETITION_ENTRY_CHECK_STORED_COMPETITION = 15,
    COMPETITION_ENTRY_CHECK_PERIOD_BEFORE_DOWNLOAD = 16,
    COMPETITION_ENTRY_WAIT_PERIOD_BEFORE_DOWNLOAD = 17,
    COMPETITION_ENTRY_SHOW_NO_CURRENT_COMPETITION = 18,
    COMPETITION_ENTRY_CHECK_REPLACED_COMPETITION = 19,
    COMPETITION_ENTRY_CREATE_CARD = 20,
    COMPETITION_ENTRY_WAIT_CARD_SHOWN = 21,
    COMPETITION_ENTRY_CHECK_BATTLE_BOX = 22,
    COMPETITION_ENTRY_PROMPT_UPDATE_REGISTERED_BOX = 23,
    COMPETITION_ENTRY_CREATE_REGISTERED_UPDATE_MENU = 24,
    COMPETITION_ENTRY_WAIT_REGISTERED_UPDATE_MENU = 25,
    COMPETITION_ENTRY_EXPLAIN_REGISTERED_UPDATE_REQUIRED = 26,
    COMPETITION_ENTRY_CREATE_REGISTERED_QUIT_MENU = 27,
    COMPETITION_ENTRY_WAIT_REGISTERED_QUIT_MENU = 28,
    COMPETITION_ENTRY_PROMPT_UPDATE_ENTRY = 29,
    COMPETITION_ENTRY_CREATE_ENTRY_UPDATE_MENU = 30,
    COMPETITION_ENTRY_WAIT_ENTRY_UPDATE_MENU = 31,
    COMPETITION_ENTRY_EXPLAIN_ENTRY_UPDATE_REQUIRED = 32,
    COMPETITION_ENTRY_CREATE_ENTRY_QUIT_MENU = 33,
    COMPETITION_ENTRY_WAIT_ENTRY_QUIT_MENU = 34,
    COMPETITION_ENTRY_HIDE_OLD_CARD = 35,
    COMPETITION_ENTRY_DELETE_OLD_CARD = 36,
    COMPETITION_ENTRY_SHOW_UPLOAD_MESSAGE = 37,
    COMPETITION_ENTRY_UPLOAD_EMPTY_PARTY = 38,
    COMPETITION_ENTRY_WAIT_EMPTY_PARTY_UPLOAD = 39,
    COMPETITION_ENTRY_POST_ENTRY_CREATED = 40,
    COMPETITION_ENTRY_WAIT_ENTRY_CREATED = 41,
    COMPETITION_ENTRY_SHOW_ID_UPDATE_MESSAGE = 42,
    COMPETITION_ENTRY_SAVE_NEW_COMPETITION = 43,
    COMPETITION_ENTRY_WAIT_NEW_COMPETITION_SAVE = 44,
    COMPETITION_ENTRY_SHOW_SAVE_FAILED = 45,
    COMPETITION_ENTRY_SHOW_RECORD_DOWNLOAD_MESSAGE = 46,
    COMPETITION_ENTRY_DOWNLOAD_RECORD = 47,
    COMPETITION_ENTRY_WAIT_RECORD_DOWNLOAD = 48,
    COMPETITION_ENTRY_COMPARE_RECORD_RATING = 49,
    COMPETITION_ENTRY_SHOW_RECORD_UPDATE_MESSAGE = 50,
    COMPETITION_ENTRY_SAVE_UPDATED_RECORD = 51,
    COMPETITION_ENTRY_WAIT_UPDATED_RECORD_SAVE = 52,
    COMPETITION_ENTRY_START_TEAM_REGISTRATION = 53,
    COMPETITION_ENTRY_START_COMPETITION_MENU = 54,
    COMPETITION_ENTRY_END = 55,
    COMPETITION_ENTRY_WAIT_MESSAGE = 56,
};

enum {
    COMPETITION_REGISTER_CREATE_CARD_AND_TEAM = 0,
    COMPETITION_REGISTER_WAIT_CARD_AND_TEAM = 1,
    COMPETITION_REGISTER_PROMPT_REGISTER_TEAM = 2,
    COMPETITION_REGISTER_CREATE_REGISTER_MENU = 3,
    COMPETITION_REGISTER_WAIT_REGISTER_MENU = 4,
    COMPETITION_REGISTER_PROMPT_QUIT_REGISTRATION = 5,
    COMPETITION_REGISTER_CREATE_QUIT_MENU = 6,
    COMPETITION_REGISTER_WAIT_QUIT_MENU = 7,
    COMPETITION_REGISTER_CONFIRM_QUIT_COMPETITION = 8,
    COMPETITION_REGISTER_CREATE_CONFIRM_QUIT_MENU = 9,
    COMPETITION_REGISTER_WAIT_CONFIRM_QUIT_MENU = 10,
    COMPETITION_REGISTER_HIDE_CANCELLED_TEAM = 11,
    COMPETITION_REGISTER_CHECK_TEAM = 12,
    COMPETITION_REGISTER_SHOW_INVALID_TEAM = 13,
    COMPETITION_REGISTER_SHOW_EMPTY_TEAM = 14,
    COMPETITION_REGISTER_HIDE_CHECKED_TEAM = 15,
    COMPETITION_REGISTER_SHOW_CHECKING_MESSAGE = 16,
    COMPETITION_REGISTER_START_SERVER_CHECK = 17,
    COMPETITION_REGISTER_WAIT_SERVER_CHECK = 18,
    COMPETITION_REGISTER_SHOW_SERVER_REJECTION = 19,
    COMPETITION_REGISTER_UPLOAD_TEAM = 20,
    COMPETITION_REGISTER_WAIT_TEAM_UPLOAD = 21,
    COMPETITION_REGISTER_LOCK_TEAM = 22,
    COMPETITION_REGISTER_START_SAVE = 23,
    COMPETITION_REGISTER_WAIT_SAVE = 24,
    COMPETITION_REGISTER_POST_REGISTRATION = 25,
    COMPETITION_REGISTER_WAIT_REGISTRATION_POST = 26,
    COMPETITION_REGISTER_SHOW_COMPLETE_MESSAGE = 27,
    COMPETITION_REGISTER_START_COMPETITION_MENU = 28,
    COMPETITION_REGISTER_LOGOUT = 29,
    COMPETITION_REGISTER_WAIT_MESSAGE = 30,
};

enum {
    COMPETITION_MENU_FLOW_CHECK_CONNECTION = 0,
    COMPETITION_MENU_FLOW_CREATE_CARD = 1,
    COMPETITION_MENU_FLOW_WAIT_CARD_SHOWN = 2,
    COMPETITION_MENU_FLOW_SHOW_BATTLE_PROMPT = 3,
    COMPETITION_MENU_FLOW_CREATE_BATTLE_MENU = 4,
    COMPETITION_MENU_FLOW_WAIT_BATTLE_MENU = 5,
    COMPETITION_MENU_FLOW_START_CANCEL = 6,
    COMPETITION_MENU_FLOW_START_UNREGISTER = 7,
    COMPETITION_MENU_FLOW_WAIT_UNREGISTER = 8,
    COMPETITION_MENU_FLOW_START_LOGOUT = 9,
    COMPETITION_MENU_FLOW_SHOW_TEAM_CHECK_MESSAGE = 10,
    COMPETITION_MENU_FLOW_START_TEAM_CHECK = 11,
    COMPETITION_MENU_FLOW_WAIT_TEAM_CHECK = 12,
    COMPETITION_MENU_FLOW_START_MATCH = 13,
    COMPETITION_MENU_FLOW_SHOW_INVALID_TEAM = 14,
    COMPETITION_MENU_FLOW_LOGOUT_INVALID_TEAM = 15,
    COMPETITION_MENU_FLOW_WAIT_MESSAGE = 16,
};

enum {
    COMPETITION_MATCH_SHOW_PREPARATION_MESSAGE = 0,
    COMPETITION_MATCH_CHECK_PERIOD_CONNECTION = 1,
    COMPETITION_MATCH_WAIT_PERIOD = 2,
    COMPETITION_MATCH_LOGOUT = 3,
    COMPETITION_MATCH_DOWNLOAD_RECORD = 4,
    COMPETITION_MATCH_WAIT_RECORD = 5,
    COMPETITION_MATCH_DELAY_CARD_REFRESH = 6,
    COMPETITION_MATCH_HIDE_CARD = 7,
    COMPETITION_MATCH_CREATE_CARD = 8,
    COMPETITION_MATCH_WAIT_CARD_SHOWN = 9,
    COMPETITION_MATCH_SHOW_SEARCH_MESSAGE = 10,
    COMPETITION_MATCH_START_SEARCH = 11,
    COMPETITION_MATCH_WAIT_MATCH = 12,
    COMPETITION_MATCH_SEND_PLAYER_INFO = 13,
    COMPETITION_MATCH_RECEIVE_PLAYER_INFO = 14,
    COMPETITION_MATCH_CHECK_COMPETITION_ID = 15,
    COMPETITION_MATCH_CHECK_OPPONENT_TEAM = 16,
    COMPETITION_MATCH_DOWNLOAD_OPPONENT_TEAM = 17,
    COMPETITION_MATCH_WAIT_OPPONENT_TEAM = 18,
    COMPETITION_MATCH_SYNC_TEAM_DOWNLOAD = 19,
    COMPETITION_MATCH_WAIT_TEAM_DOWNLOAD_SYNC = 20,
    COMPETITION_MATCH_VERIFY_OPPONENT_TEAM = 21,
    COMPETITION_MATCH_SEND_VALIDATION_RESULT = 22,
    COMPETITION_MATCH_RECEIVE_VALIDATION_RESULT = 23,
    COMPETITION_MATCH_CHECK_VALIDATION_RESULTS = 24,
    COMPETITION_MATCH_START_NAME_CHECK = 25,
    COMPETITION_MATCH_WAIT_NAME_CHECK = 26,
    COMPETITION_MATCH_SYNC_VALIDATED_PLAYERS = 27,
    COMPETITION_MATCH_WAIT_VALIDATED_PLAYERS_SYNC = 28,
    COMPETITION_MATCH_SYNC_MATCH_READY = 29,
    COMPETITION_MATCH_WAIT_MATCH_READY_SYNC = 30,
    COMPETITION_MATCH_SHOW_OPPONENT_FOUND = 31,
    COMPETITION_MATCH_WAIT_OPPONENT_FOUND_DELAY = 32,
    COMPETITION_MATCH_WAIT_PARTNER_CARD = 33,
    COMPETITION_MATCH_WAIT_PARTNER_CARD_DELAY = 34,
    COMPETITION_MATCH_START_SESSION_REPORT = 35,
    COMPETITION_MATCH_WAIT_SESSION_REPORT = 36,
    COMPETITION_MATCH_SHOW_TEAM_SELECTION_MESSAGE = 37,
    COMPETITION_MATCH_WAIT_TEAM_SELECTION_DELAY = 38,
    COMPETITION_MATCH_CANCEL_SESSION = 39,
    COMPETITION_MATCH_PROMPT_QUIT = 40,
    COMPETITION_MATCH_CREATE_QUIT_MENU = 41,
    COMPETITION_MATCH_WAIT_QUIT_MENU = 42,
    COMPETITION_MATCH_PROMPT_CONTINUE = 43,
    COMPETITION_MATCH_CREATE_CONTINUE_MENU = 44,
    COMPETITION_MATCH_WAIT_CONTINUE_MENU = 45,
    COMPETITION_MATCH_DELAY_PERIOD_RECHECK = 46,
    COMPETITION_MATCH_CHECK_PERIOD_RECONNECTION = 47,
    COMPETITION_MATCH_WAIT_PERIOD_RECHECK = 48,
    COMPETITION_MATCH_SEND_CANCEL_COMMAND = 49,
    COMPETITION_MATCH_CANCEL_AND_RETRY = 50,
    COMPETITION_MATCH_START_DISCONNECT = 51,
    COMPETITION_MATCH_WAIT_DISCONNECT = 52,
    COMPETITION_MATCH_WAIT_MESSAGE = 53,
};

enum {
    COMPETITION_RESULT_CREATE_REPORT_HEAP = 0,
    COMPETITION_RESULT_SHOW_REPORT_MESSAGE = 1,
    COMPETITION_RESULT_START_REPORT = 2,
    COMPETITION_RESULT_WAIT_REPORT = 3,
    COMPETITION_RESULT_FINISH_REPORT = 4,
    COMPETITION_RESULT_RECORD_MET_TRAINER = 5,
    COMPETITION_RESULT_WAIT_DISCONNECT = 6,
    COMPETITION_RESULT_UPLOAD_RESULTS = 7,
    COMPETITION_RESULT_WAIT_RESULTS_UPLOAD = 8,
    COMPETITION_RESULT_SHOW_RESULTS_SENT = 9,
    COMPETITION_RESULT_SHOW_RESULT_SCREEN = 10,
    COMPETITION_RESULT_WAIT_MESSAGE = 13,
};

enum {
    COMPETITION_AFTER_BATTLE_SHOW_SAVE_MESSAGE = 0,
    COMPETITION_AFTER_BATTLE_START_SAVE = 1,
    COMPETITION_AFTER_BATTLE_WAIT_SAVE = 2,
    COMPETITION_AFTER_BATTLE_CHECK_PERIOD_CONNECTION = 3,
    COMPETITION_AFTER_BATTLE_WAIT_PERIOD = 4,
    COMPETITION_AFTER_BATTLE_WAIT_MESSAGE = 5,
};

enum {
    COMPETITION_CONTINUE_CHECK_CONNECTION = 0,
    COMPETITION_CONTINUE_SHOW_BATTLE_PROMPT = 1,
    COMPETITION_CONTINUE_CREATE_MENU = 2,
    COMPETITION_CONTINUE_WAIT_MENU = 3,
    COMPETITION_CONTINUE_START_UNREGISTER = 4,
    COMPETITION_CONTINUE_WAIT_UNREGISTER = 5,
    COMPETITION_CONTINUE_LOGOUT = 6,
    COMPETITION_CONTINUE_WAIT_MESSAGE = 7,
};

enum {
    COMPETITION_CANCEL_PROMPT_QUIT = 0,
    COMPETITION_CANCEL_CREATE_MENU = 1,
    COMPETITION_CANCEL_WAIT_MENU = 2,
    COMPETITION_CANCEL_WAIT_MESSAGE = 3,
};

enum {
    COMPETITION_LOGOUT_SHOW_SAVE_MESSAGE = 0,
    COMPETITION_LOGOUT_START_SAVE = 1,
    COMPETITION_LOGOUT_WAIT_SAVE = 2,
    COMPETITION_LOGOUT_SHOW_UPLOAD_MESSAGE = 3,
    COMPETITION_LOGOUT_UPLOAD_LOGOUT = 4,
    COMPETITION_LOGOUT_WAIT_LOGOUT_UPLOAD = 5,
    COMPETITION_LOGOUT_END = 6,
    COMPETITION_LOGOUT_WAIT_MESSAGE = 7,
};

enum {
    COMPETITION_QUIT_CHECK_PERIOD_CONNECTION = 0,
    COMPETITION_QUIT_WAIT_PERIOD = 1,
    COMPETITION_QUIT_LOGOUT = 2,
    COMPETITION_QUIT_WAIT_MESSAGE = 3,
};

enum {
    COMPETITION_PERIOD_DELAY_DATE_CHECK = 0,
    COMPETITION_PERIOD_CHECK_DATES = 1,
    COMPETITION_PERIOD_SHOW_NOT_STARTED = 2,
    COMPETITION_PERIOD_SHOW_ENDED = 3,
    COMPETITION_PERIOD_CHECK_REGISTRATION = 4,
    COMPETITION_PERIOD_SHOW_SAVE_MESSAGE = 5,
    COMPETITION_PERIOD_UNLOCK_TEAM_AND_SAVE = 6,
    COMPETITION_PERIOD_WAIT_SAVE = 7,
    COMPETITION_PERIOD_CHECK_TEAM_WAS_LOCKED = 8,
    COMPETITION_PERIOD_SHOW_TEAM_UNLOCKED = 9,
    COMPETITION_PERIOD_SHOW_UPLOAD_MESSAGE = 10,
    COMPETITION_PERIOD_POST_PERIOD_ENDED = 11,
    COMPETITION_PERIOD_WAIT_PERIOD_POST = 12,
    COMPETITION_PERIOD_END = 13,
    COMPETITION_PERIOD_WAIT_MESSAGE = 14,
};

enum {
    COMPETITION_UNREGISTER_CHECK_REGISTRATION = 0,
    COMPETITION_UNREGISTER_SHOW_NOT_REGISTERED = 1,
    COMPETITION_UNREGISTER_PROMPT_UNREGISTER = 2,
    COMPETITION_UNREGISTER_CREATE_UNREGISTER_MENU = 3,
    COMPETITION_UNREGISTER_WAIT_UNREGISTER_MENU = 4,
    COMPETITION_UNREGISTER_PROMPT_CONFIRM_UNREGISTER = 5,
    COMPETITION_UNREGISTER_CREATE_CONFIRM_MENU = 6,
    COMPETITION_UNREGISTER_WAIT_CONFIRM_MENU = 7,
    COMPETITION_UNREGISTER_SHOW_SAVE_MESSAGE = 8,
    COMPETITION_UNREGISTER_UNLOCK_TEAM_AND_SAVE = 9,
    COMPETITION_UNREGISTER_WAIT_SAVE = 10,
    COMPETITION_UNREGISTER_SHOW_TEAM_UNLOCKED = 11,
    COMPETITION_UNREGISTER_SHOW_UPLOAD_MESSAGE = 12,
    COMPETITION_UNREGISTER_POST_UNREGISTER = 13,
    COMPETITION_UNREGISTER_WAIT_UNREGISTER_POST = 14,
    COMPETITION_UNREGISTER_END = 15,
    COMPETITION_UNREGISTER_WAIT_MESSAGE = 16,
};

enum {
    COMPETITION_TEAM_CHECK_DELAY = 0,
    COMPETITION_TEAM_CHECK_START_CHECK = 1,
    COMPETITION_TEAM_CHECK_WAIT_CHECK = 2,
    COMPETITION_TEAM_CHECK_COPY_SIGNATURE = 3,
    COMPETITION_TEAM_CHECK_END = 4,
};

enum {
    COMPETITION_SEARCH_CANCEL_IDLE = 0,
    COMPETITION_SEARCH_CANCEL_SHOW_QUIT_PROMPT = 1,
    COMPETITION_SEARCH_CANCEL_WAIT_QUIT_PROMPT = 2,
    COMPETITION_SEARCH_CANCEL_CREATE_MENU = 3,
    COMPETITION_SEARCH_CANCEL_WAIT_MENU = 4,
    COMPETITION_SEARCH_CANCEL_SHOW_DISCONNECT_MESSAGE = 5,
    COMPETITION_SEARCH_CANCEL_DISCONNECT = 6,
    COMPETITION_SEARCH_CANCEL_CONFIRMED = 7,
};

enum {
    COMPETITION_MENU_SELECTION_BATTLE = 0,
    COMPETITION_MENU_SELECTION_UNREGISTER = 1,
    COMPETITION_MENU_SELECTION_QUIT = 2,
};

#define HEAPID_WIFIBATTLEMATCH 0x56

// The menus
enum {
    COMPETITION_MENU_YES_NO,
    COMPETITION_MENU_JOIN_INFO_CANCEL,
    COMPETITION_MENU_REGISTER,
    COMPETITION_MENU_BATTLE,
    COMPETITION_MENU_BATTLE_AGAIN,
    COMPETITION_MENU_YES_NO_NO_FIRST,
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

// What a request to the server's account takes
typedef struct {
    u8 unk0;
    u8 registrationState;
    u32 profileId;
} WifiBattleMatchLinkRequest;

// The work of the Wi-Fi competition
typedef struct {
    WifiBattleMatchGraphic *graphic;
    WifiBattleMatchRes *res;
    Font *font;
    Font *smallFont;
    PrintQueue *printQueue;
    MsgData *msgData;
    WordSet *wordSet;
    WifiBattleMatchCard *card;
    WifiBattleMatchPartnerCard *partnerCard;
    WifiBattleMatchTeamList *teamList;
    WifiBattleMatchLinkIcon *linkIcon;
    WifiBattleMatchMsgBox *msgBox;
    WifiBattleMatchMenu *menu;
    WifiBattleMatchNet *net;
    WifiBattleMatchTask *task;
    WifiBattleMatchTask *subTask;
    u32 subTaskResult;
    WifiBattleMatchNetCheck signature;
    u32 timer;
    u32 opponentValidationErrors;
    u32 peerValidationErrors;
    u32 cancelSeq;
    u32 idleTimer;
    u32 exchangeResult;
    u32 exchangeDone;
    PokeParty *party;
    BOOL hadExchangeResult;
    WifiBattleMatchLinkParam *param;
    TournamentEntry *entry;
    TournamentEntry serverEntry;
    BOOL hadBattleBox;
} WifiBattleMatchLinkCompetition;

// The three bytes of a date in the tournament block, the first in the high byte
static inline u32 WifiBattleMatchLink_PackBytes(u32 high, u32 middle, u32 low) {
    return (high << 24) | ((middle & 0xff) << 16) | ((low & 0xff) << 8);
}

static BOOL WifiBattleMatchLink_CompetitionInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_CompetitionExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_CompetitionMain(GameProc *proc, u32 *state, void *param, void *work);
static void WifiBattleMatchLink_CompetitionTaskStart(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskAccount(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskEntry(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskRegister(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskMenu(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskMatch(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskResult(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskContinue(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskCancel(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskLogout(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskQuit(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskRelogin(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskCheckPeriod(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskUnregister(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionTaskSignTeam(WifiBattleMatchTask *task, u32 *seq, void *work);
static void WifiBattleMatchLink_CompetitionCreateCard(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionDeleteCard(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsCardShown(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsCardHidden(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionShowCard(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionSetCardPage(WifiBattleMatchLinkCompetition *link, u32 page);
static void WifiBattleMatchLink_CompetitionCreatePartnerCard(WifiBattleMatchLinkCompetition *link,
                                                             WifiBattleMatchPlayer *partner);
static void WifiBattleMatchLink_CompetitionDeletePartnerCard(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsPartnerCardDone(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionCreateTeamList(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionDeleteTeamList(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsTeamListShown(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionIsTeamListHidden(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionCreateMenu(WifiBattleMatchLinkCompetition *link, u32 kind);
static void WifiBattleMatchLink_CompetitionDeleteMenu(WifiBattleMatchLinkCompetition *link);
static s32 WifiBattleMatchLink_CompetitionGetMenuSelection(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionStartSubTask(WifiBattleMatchLinkCompetition *link,
                                                        WifiBattleMatchTaskFunc func);
static u32 WifiBattleMatchLink_CompetitionUpdateSubTask(WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionFillBattleBoxParty(WifiBattleMatchPlayer *player,
                                                              WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionFillPlayer(WifiBattleMatchPlayer *player,
                                                      WifiBattleMatchLinkCompetition *link);
static void WifiBattleMatchLink_CompetitionCopySignature(WifiBattleMatchPlayer *player,
                                                         WifiBattleMatchNetCheck *signature);
static BOOL WifiBattleMatchLink_CompetitionCheckSignature(WifiBattleMatchPlayer *player, PokeParty *party,
                                                          HeapID heapId);
static BOOL WifiBattleMatchLink_CompetitionCompareParties(PokeParty *party, PokeParty *otherParty);
static u32 WifiBattleMatchLink_CompetitionUpdateCancel(WifiBattleMatchLinkCompetition *link, BOOL canCancel,
                                                       BOOL canDisconnect);
static void WifiBattleMatchLink_CompetitionResetCancel(WifiBattleMatchLinkCompetition *link);
static BOOL WifiBattleMatchLink_CompetitionUpdateRecord(WifiBattleMatchLinkCompetition *link,
                                                        WifiBattleMatchLinkRanking *ranking);

const GameProcFunctions WIFIBATTLEMATCH_LINK_COMPETITION_PROC_FUNCTIONS = {
    WifiBattleMatchLink_CompetitionInit,
    WifiBattleMatchLink_CompetitionMain,
    WifiBattleMatchLink_CompetitionExit,
};

static BOOL WifiBattleMatchLink_CompetitionInit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkParam *linkParam = param;
    WifiBattleMatchLinkCompetition *link;

    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK, 0x30000);
    link = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchLinkCompetition), HEAPID_WIFIBATTLEMATCH_LINK);
    MI_CpuFill8(link, 0, sizeof(WifiBattleMatchLinkCompetition));
    link->param = linkParam;
    link->entry =
        TournamentBlock_GetEntry(getTournamentBlockAddress(GameData_GetSaveControl(linkParam->param->gameData)), 0);
    link->graphic = WifiBattleMatchGraphic_Create(1, HEAPID_WIFIBATTLEMATCH_LINK);
    link->res = WifiBattleMatchRes_Create(WifiBattleMatchGraphic_GetClActUnit(link->graphic),
                                          WIFIBATTLEMATCH_BG_WIFI_COMPETITION, HEAPID_WIFIBATTLEMATCH_LINK);
    link->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, HEAPID_WIFIBATTLEMATCH_LINK);
    link->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, 0, HEAPID_WIFIBATTLEMATCH_LINK);
    link->printQueue = PrintQueue_CreateDefault(HEAPID_WIFIBATTLEMATCH_LINK);
    link->msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_FISHING, HEAPID_WIFIBATTLEMATCH_LINK);
    link->wordSet = GFL_WordSetSystemCreate(8, 0x40, HEAPID_WIFIBATTLEMATCH_LINK);
    link->net = WifiBattleMatchNet_Create(link->param->netData, linkParam->param->gameData, linkParam->loginBuffer,
                                          HEAPID_WIFIBATTLEMATCH_LINK);
    link->msgBox =
        WifiBattleMatchMsgBox_Create(0, 15, 14, 10, link->printQueue, link->font, HEAPID_WIFIBATTLEMATCH_LINK);
    link->task =
        WifiBattleMatchTask_Create(link, WifiBattleMatchLink_CompetitionTaskStart, HEAPID_WIFIBATTLEMATCH_LINK);
    link->subTask = WifiBattleMatchTask_Create(link, NULL, HEAPID_WIFIBATTLEMATCH_LINK);
    link->linkIcon = WifiBattleMatchLinkIcon_Create(WifiBattleMatchGraphic_GetClActUnit(link->graphic), link->res,
                                                    HEAPID_WIFIBATTLEMATCH_LINK);
    WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
    link->party = PokeParty_Create(HEAPID_WIFIBATTLEMATCH_LINK);
    WifiBattleMatchNet_ShowIcon(link->net, TRUE);
    return TRUE;
}

static BOOL WifiBattleMatchLink_CompetitionExit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkCompetition *link = work;

    GFL_HeapFree(link->party);
    WifiBattleMatchNet_ShowIcon(link->net, FALSE);
    WifiBattleMatchLinkIcon_Free(link->linkIcon);
    WifiBattleMatchTask_Free(link->subTask);
    WifiBattleMatchTask_Free(link->task);
    WifiBattleMatchMsgBox_Free(link->msgBox);
    if (link->net != NULL) {
        WifiBattleMatchNet_Delete(link->net);
    }
    WifiBattleMatchLink_CompetitionDeleteTeamList(link);
    WifiBattleMatchLink_CompetitionDeletePartnerCard(link);
    WifiBattleMatchLink_CompetitionDeleteCard(link);
    WifiBattleMatchLink_CompetitionDeleteMenu(link);
    GFL_WordSetSystemFree(link->wordSet);
    GFL_MsgDataFree(link->msgData);
    PrintQueue_Free(link->printQueue);
    GFL_FontFree(link->smallFont);
    GFL_FontFree(link->font);
    WifiBattleMatchRes_Free(link->res);
    WifiBattleMatchGraphic_Free(link->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH_LINK);
    GFL_OvlUnload(OVERLAY_ID(189));
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

static BOOL WifiBattleMatchLink_CompetitionMain(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkCompetition *link = work;

    switch (*state) {
    case COMPETITION_PHASE_FADE_IN:
        GFL_FadeSet((FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK), 16, 0, 0);
        *state = COMPETITION_PHASE_WAIT_FADE_IN;
        break;
    case COMPETITION_PHASE_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            *state = COMPETITION_PHASE_RUN_TASK;
        }
        break;
    case COMPETITION_PHASE_RUN_TASK:
        WifiBattleMatchTask_Run(link->task);
        if (WifiBattleMatchTask_IsEnded(link->task)) {
            *state = COMPETITION_PHASE_FADE_OUT;
        }
        break;
    case COMPETITION_PHASE_FADE_OUT:
        GFL_FadeSet((FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK), 0, 16, 0);
        *state = COMPETITION_PHASE_WAIT_FADE_OUT;
        break;
    case COMPETITION_PHASE_WAIT_FADE_OUT:
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
    WifiBattleMatchNet_Nop(link->net);
    if (link->card != NULL) {
        WifiBattleMatchCard_Flush(link->card, link->printQueue);
    }
    if (link->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Flush(link->partnerCard, link->printQueue);
    }
    if (link->teamList != NULL) {
        WifiBattleMatchTeamList_Flush(link->teamList, link->printQueue);
    }
    return FALSE;
}

static void WifiBattleMatchLink_CompetitionTaskStart(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;

    switch (link->param->result) {
    case WBM_LINK_RESULT_INITIAL:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskAccount);
        return;
    case WBM_LINK_RESULT_CANCELLED:
        WifiBattleMatchNet_CancelMatch(link->net);
        // fall through
    case WBM_LINK_RESULT_BATTLE_FINISHED:
        WifiBattleMatchLink_CompetitionShowCard(link);
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskResult);
        return;
    case WBM_LINK_RESULT_RESULT_SHOWN:
        WifiBattleMatchLink_CompetitionShowCard(link);
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskAfterBattle);
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskAccount(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    u32 ready;

    switch (*seq) {
    case COMPETITION_ACCOUNT_CHECK_GAME_SYNC_ID:
        if (DreamWorldSave_GetIDShown(getDreamWorldStuffAddress(GameData_GetSaveControl(param->param->gameData)))) {
            *seq = COMPETITION_ACCOUNT_SHOW_SEARCH_MESSAGE;
        } else {
            *seq = COMPETITION_ACCOUNT_SHOW_UNREGISTERED_MESSAGE;
        }
        break;
    case COMPETITION_ACCOUNT_SHOW_SEARCH_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingCompetitionData,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = COMPETITION_ACCOUNT_REQUEST_ACCOUNT;
        break;
    case COMPETITION_ACCOUNT_REQUEST_ACCOUNT:
        MI_CpuFill8(param->account, 0, sizeof(WifiBattleMatchLinkAccount));
        WifiBattleMatchNet_StartAccountRequest(link->net, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = COMPETITION_ACCOUNT_WAIT_ACCOUNT;
        break;
    case COMPETITION_ACCOUNT_WAIT_ACCOUNT:
        ready = WifiBattleMatchNet_AccountRequestMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            WifiBattleMatchNet_GetAccountValues(link->net, link->param->account);
            *seq = COMPETITION_ACCOUNT_CHECK_REGISTRATION;
            break;
        }
        if (ready == WBM_NET_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ACCOUNT_CHECK_REGISTRATION:
        if (param->account->registered != 0) {
            *seq = COMPETITION_ACCOUNT_WAIT_LOGIN_COOLDOWN;
        } else {
            *seq = COMPETITION_ACCOUNT_SHOW_UNREGISTERED_MESSAGE;
        }
        break;
    case COMPETITION_ACCOUNT_SHOW_UNREGISTERED_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_GameSyncIdHas,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_ACCOUNT_WAIT_UNREGISTERED_MESSAGE;
        break;
    case COMPETITION_ACCOUNT_WAIT_UNREGISTERED_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskEnd);
        }
        break;
    case COMPETITION_ACCOUNT_WAIT_LOGIN_COOLDOWN:
        if (*link->param->recordRequestCooldown == 0) {
            WifiBattleMatchNet_ResetLogin(link->net);
            *seq = COMPETITION_ACCOUNT_LOGIN;
        }
        break;
    case COMPETITION_ACCOUNT_LOGIN:
        ready = WifiBattleMatchNet_LoginMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 1800;
            *seq = COMPETITION_ACCOUNT_START_ENTRY;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ACCOUNT_START_ENTRY:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskEntry);
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskEntry(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    TournamentEntry *entry = link->entry;
    u32 ready;
    s32 selected;
    u32 state;
    u8 accountState;

    switch (*seq) {
    case COMPETITION_ENTRY_CHECK_RESET_REQUEST:
        if (param->account->unk4 == 1) {
            *seq = COMPETITION_ENTRY_RESET_ACCOUNT_ENTRY;
        } else {
            *seq = COMPETITION_ENTRY_CHECK_LOCAL_REGISTRATION;
        }
        break;
    case COMPETITION_ENTRY_RESET_ACCOUNT_ENTRY: {
        WifiBattleMatchLinkRequest request;

        MI_CpuFill8(&request, 0, sizeof(request));
        request.registrationState = WBM_REGISTRATION_NONE;
        request.unk0 = link->param->account->unk4;
        request.profileId = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->registrationState = WBM_REGISTRATION_NONE;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = COMPETITION_ENTRY_WAIT_ACCOUNT_RESET;
        break;
    }
    case COMPETITION_ENTRY_WAIT_ACCOUNT_RESET:
        ready = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *seq = COMPETITION_ENTRY_SHOW_DOWNLOAD_MESSAGE;
        } else if (ready == WBM_NET_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ENTRY_CHECK_LOCAL_REGISTRATION:
        if (link->param->account->registrationState == TournamentEntry_GetField(entry, TOURNAMENT_FIELD_UNK15B)) {
            *seq = COMPETITION_ENTRY_CHECK_CLOSED_REGISTRATION;
        } else if (TournamentEntry_GetField(entry, TOURNAMENT_FIELD_UNK15B) == WBM_REGISTRATION_WITHDRAWN ||
                   TournamentEntry_GetField(entry, TOURNAMENT_FIELD_UNK15B) == 5) {
            *seq = COMPETITION_ENTRY_UPDATE_SERVER_REGISTRATION;
        } else {
            *seq = COMPETITION_ENTRY_CHECK_CLOSED_REGISTRATION;
        }
        break;
    case COMPETITION_ENTRY_CHECK_CLOSED_REGISTRATION:
        accountState = param->account->registrationState;
        if (accountState == WBM_REGISTRATION_PERIOD_ENDED) {
            *seq = COMPETITION_ENTRY_SHOW_ENDED_MESSAGE;
        } else if (accountState == WBM_REGISTRATION_WITHDRAWN || accountState == 5) {
            *seq = COMPETITION_ENTRY_SHOW_UNAVAILABLE_MESSAGE;
        } else {
            *seq = COMPETITION_ENTRY_SHOW_DOWNLOAD_MESSAGE;
        }
        break;
    case COMPETITION_ENTRY_SHOW_ENDED_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_NoCompetitionsHaveBeen,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_SHOW_RECORD_DOWNLOAD_MESSAGE);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_SHOW_UNAVAILABLE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_NoCompetitionsHaveBeen,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_SHOW_RECORD_DOWNLOAD_MESSAGE);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_UPDATE_SERVER_REGISTRATION: {
        WifiBattleMatchLinkRequest request;

        MI_CpuFill8(&request, 0, sizeof(request));
        request.registrationState = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_UNK15B);
        request.profileId = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->registrationState = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_UNK15B);
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = COMPETITION_ENTRY_WAIT_REGISTRATION_UPDATE;
        break;
    }
    case COMPETITION_ENTRY_WAIT_REGISTRATION_UPDATE:
        ready = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *seq = COMPETITION_ENTRY_RETURN_TO_UNAVAILABLE_MESSAGE;
        } else if (ready == WBM_NET_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ENTRY_RETURN_TO_UNAVAILABLE_MESSAGE:
        *seq = COMPETITION_ENTRY_SHOW_UNAVAILABLE_MESSAGE;
        break;
    case COMPETITION_ENTRY_WAIT_PREVIOUS_WRITE:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 1800;
            *seq = COMPETITION_ENTRY_SHOW_UNAVAILABLE_MESSAGE;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ENTRY_SHOW_DOWNLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CheckingCompetitionDetails,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_START_DOWNLOAD);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_START_DOWNLOAD:
        WifiBattleMatchNet_InitDownload(link->net, param->account->profileId);
        *seq = COMPETITION_ENTRY_WAIT_DOWNLOAD;
        break;
    case COMPETITION_ENTRY_WAIT_DOWNLOAD:
        ready = WifiBattleMatchNet_DownloadMain(link->net);
        if (ready == WBM_DOWNLOAD_SUCCESS) {
            WifiBattleMatchNet_GetDownload(link->net, &link->serverEntry);
            if (TournamentEntry_CheckCRC(&link->serverEntry)) {
                *seq = COMPETITION_ENTRY_CHECK_SERVER_REGISTRATION;
            } else {
                *seq = COMPETITION_ENTRY_CHECK_STORED_COMPETITION;
            }
        } else if (ready == WBM_DOWNLOAD_NO_FILE) {
            *seq = COMPETITION_ENTRY_CHECK_STORED_COMPETITION;
        }
        if (ready == WBM_DOWNLOAD_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ENTRY_CHECK_STORED_COMPETITION:
        if (TournamentEntry_GetField(entry, TOURNAMENT_FIELD_CUP_ID) == 0) {
            *seq = COMPETITION_ENTRY_SHOW_NO_CURRENT_COMPETITION;
        } else {
            *seq = COMPETITION_ENTRY_CHECK_PERIOD_BEFORE_DOWNLOAD;
        }
        break;
    case COMPETITION_ENTRY_CHECK_PERIOD_BEFORE_DOWNLOAD:
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case WBM_RESULT_OK:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = COMPETITION_ENTRY_WAIT_PERIOD_BEFORE_DOWNLOAD;
            break;
        }
        break;
    case COMPETITION_ENTRY_WAIT_PERIOD_BEFORE_DOWNLOAD:
        ready = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ready != SUB_TASK_RUNNING) {
            *seq = COMPETITION_ENTRY_SHOW_RECORD_DOWNLOAD_MESSAGE;
        }
        if (ready == WBM_PERIOD_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ENTRY_SHOW_NO_CURRENT_COMPETITION:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereNoCurrentCompetitions,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_END);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_CHECK_SERVER_REGISTRATION:
        if (param->account->registrationState == WBM_REGISTRATION_NONE) {
            *seq = COMPETITION_ENTRY_CHECK_REPLACED_COMPETITION;
        } else if (param->account->registrationState == WBM_REGISTRATION_TEAM_REQUIRED) {
            *seq = COMPETITION_ENTRY_START_TEAM_REGISTRATION;
        } else if (param->account->registrationState == WBM_REGISTRATION_COMPLETE) {
            *seq = COMPETITION_ENTRY_START_COMPETITION_MENU;
        } else {
            *seq = COMPETITION_ENTRY_END;
        }
        break;
    case COMPETITION_ENTRY_CHECK_REPLACED_COMPETITION:
        if (TournamentEntry_GetField(entry, TOURNAMENT_FIELD_CUP_ID) == 0) {
            *seq = COMPETITION_ENTRY_SHOW_ID_UPDATE_MESSAGE;
        } else {
            *seq = COMPETITION_ENTRY_CREATE_CARD;
        }
        break;
    case COMPETITION_ENTRY_CREATE_CARD:
        WifiBattleMatchLink_CompetitionCreateCard(link);
        if (BattleBoxSave_CheckFlags(getBattleBox(GameData_GetSaveControl(param->param->gameData)), 1)) {
            WifiBattleMatchLink_CompetitionSetCardPage(link, WIFIBATTLEMATCH_CARD_PAGE_REGISTERED);
        } else {
            WifiBattleMatchLink_CompetitionSetCardPage(link, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        }
        *seq = COMPETITION_ENTRY_WAIT_CARD_SHOWN;
        break;
    case COMPETITION_ENTRY_WAIT_CARD_SHOWN:
        if (WifiBattleMatchLink_CompetitionIsCardShown(link)) {
            *seq = COMPETITION_ENTRY_CHECK_BATTLE_BOX;
        }
        break;
    case COMPETITION_ENTRY_CHECK_BATTLE_BOX:
        if (BattleBoxSave_CheckFlags(getBattleBox(GameData_GetSaveControl(param->param->gameData)), 1)) {
            *seq = COMPETITION_ENTRY_PROMPT_UPDATE_REGISTERED_BOX;
        } else {
            *seq = COMPETITION_ENTRY_PROMPT_UPDATE_ENTRY;
        }
        break;
    case COMPETITION_ENTRY_PROMPT_UPDATE_REGISTERED_BOX:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ParticipateWiFiCompetition_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_CREATE_REGISTERED_UPDATE_MENU);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_CREATE_REGISTERED_UPDATE_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_ENTRY_WAIT_REGISTERED_UPDATE_MENU;
        break;
    case COMPETITION_ENTRY_WAIT_REGISTERED_UPDATE_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_ENTRY_HIDE_OLD_CARD;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_ENTRY_EXPLAIN_REGISTERED_UPDATE_REQUIRED;
            break;
        }
        break;
    case COMPETITION_ENTRY_EXPLAIN_REGISTERED_UPDATE_REQUIRED:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CannotParticipateWithoutUpdating,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_CREATE_REGISTERED_QUIT_MENU);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_CREATE_REGISTERED_QUIT_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_ENTRY_WAIT_REGISTERED_QUIT_MENU;
        break;
    case COMPETITION_ENTRY_WAIT_REGISTERED_QUIT_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_ENTRY_END;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_ENTRY_PROMPT_UPDATE_REGISTERED_BOX;
            break;
        }
        break;
    case COMPETITION_ENTRY_PROMPT_UPDATE_ENTRY:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ParticipateWiFiCompetition,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_CREATE_ENTRY_UPDATE_MENU);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_CREATE_ENTRY_UPDATE_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_ENTRY_WAIT_ENTRY_UPDATE_MENU;
        break;
    case COMPETITION_ENTRY_WAIT_ENTRY_UPDATE_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_ENTRY_HIDE_OLD_CARD;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_ENTRY_EXPLAIN_ENTRY_UPDATE_REQUIRED;
            break;
        }
        break;
    case COMPETITION_ENTRY_EXPLAIN_ENTRY_UPDATE_REQUIRED:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CannotParticipateWithoutUpdating,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_CREATE_ENTRY_QUIT_MENU);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_CREATE_ENTRY_QUIT_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_ENTRY_WAIT_ENTRY_QUIT_MENU;
        break;
    case COMPETITION_ENTRY_WAIT_ENTRY_QUIT_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_ENTRY_END;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_ENTRY_PROMPT_UPDATE_ENTRY;
            break;
        }
        break;
    case COMPETITION_ENTRY_HIDE_OLD_CARD:
        if (WifiBattleMatchLink_CompetitionIsCardHidden(link)) {
            *seq = COMPETITION_ENTRY_DELETE_OLD_CARD;
        }
        break;
    case COMPETITION_ENTRY_DELETE_OLD_CARD:
        WifiBattleMatchLink_CompetitionDeleteCard(link);
        *seq = COMPETITION_ENTRY_SHOW_ID_UPDATE_MESSAGE;
        break;
    case COMPETITION_ENTRY_SHOW_UPLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_UPLOAD_EMPTY_PARTY);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_UPLOAD_EMPTY_PARTY:
        if (*link->param->recordRequestCooldown == 0) {
            MI_CpuFill8(&param->ranking->partyData, 0, sizeof(param->ranking->partyData));
            WifiBattleMatchNet_StartWrite(link->net, WBM_WRITE_WIFICUP, &link->param->ranking->partyData);
            *seq = COMPETITION_ENTRY_WAIT_EMPTY_PARTY_UPLOAD;
        }
        break;
    case COMPETITION_ENTRY_WAIT_EMPTY_PARTY_UPLOAD:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 1800;
            *seq = COMPETITION_ENTRY_POST_ENTRY_CREATED;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ENTRY_POST_ENTRY_CREATED: {
        WifiBattleMatchLinkRequest request;

        MI_CpuFill8(&request, 0, sizeof(request));
        request.unk0 = 0;
        request.registrationState = WBM_REGISTRATION_TEAM_REQUIRED;
        request.profileId = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->registrationState = WBM_REGISTRATION_TEAM_REQUIRED;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = COMPETITION_ENTRY_WAIT_ENTRY_CREATED;
        break;
    }
    case COMPETITION_ENTRY_WAIT_ENTRY_CREATED:
        ready = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *seq = COMPETITION_ENTRY_START_TEAM_REGISTRATION;
        } else if (ready == WBM_NET_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ENTRY_SHOW_ID_UPDATE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PlayerIdWillUpdated,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_SAVE_NEW_COMPETITION);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_SAVE_NEW_COMPETITION: {
        SaveControl *save = GameData_GetSaveControl(param->param->gameData);
        BattleBoxSave *battleBox = getBattleBox(save);
        TournamentTeam *previous = TournamentBlock_GetTeam(getTournamentBlockAddress(save), 0);

        MI_CpuCopy8(&link->serverEntry, link->entry, sizeof(link->serverEntry));
        BattleBoxSave_ClearFlags(battleBox, 1);
        TournamentEntry_SetField(link->entry, TOURNAMENT_FIELD_UNK15B, WBM_REGISTRATION_TEAM_REQUIRED);
        RndMatchData_SetParam(param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_RATING, 0);
        RndMatchData_SetParam(param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_WIN, 0);
        RndMatchData_SetParam(param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_LOSE, 0);
        TournamentTeam_Clear(previous);
        GameData_StartSave(param->param->gameData);
        *seq = COMPETITION_ENTRY_WAIT_NEW_COMPETITION_SAVE;
        break;
    }
    case COMPETITION_ENTRY_WAIT_NEW_COMPETITION_SAVE:
        state = GameData_UpdateSave(param->param->gameData);
        if (state == SAVE_RESULT_OK) {
            *seq = COMPETITION_ENTRY_SHOW_UPLOAD_MESSAGE;
        } else if (state == SAVE_RESULT_NG) {
            *seq = COMPETITION_ENTRY_SHOW_SAVE_FAILED;
        }
        break;
    case COMPETITION_ENTRY_SHOW_SAVE_FAILED:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_UnableUpdateDigitalPlayer,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_END);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_SHOW_RECORD_DOWNLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataDontTurn,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_DOWNLOAD_RECORD);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_DOWNLOAD_RECORD:
        if (*link->param->recordRequestCooldown == 0) {
            WifiBattleMatchNet_StartRead(link->net, -1, WBM_READ_WIFICUP, param->ranking);
            *seq = COMPETITION_ENTRY_WAIT_RECORD_DOWNLOAD;
        }
        break;
    case COMPETITION_ENTRY_WAIT_RECORD_DOWNLOAD:
        ready = WifiBattleMatchNet_ReadMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 1800;
            *seq = COMPETITION_ENTRY_COMPARE_RECORD_RATING;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_ENTRY_COMPARE_RECORD_RATING:
        if (link->param->ranking->rating !=
            RndMatchData_GetParam(param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_RATING)) {
            *seq = COMPETITION_ENTRY_SHOW_RECORD_UPDATE_MESSAGE;
        } else {
            *seq = COMPETITION_ENTRY_END;
        }
        break;
    case COMPETITION_ENTRY_SHOW_RECORD_UPDATE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PlayerIdWillUpdated,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_ENTRY_SAVE_UPDATED_RECORD);
        *seq = COMPETITION_ENTRY_WAIT_MESSAGE;
        break;
    case COMPETITION_ENTRY_SAVE_UPDATED_RECORD:
        WifiBattleMatchLink_CompetitionUpdateRecord(link, param->ranking);
        GameData_StartSave(param->param->gameData);
        *seq = COMPETITION_ENTRY_WAIT_UPDATED_RECORD_SAVE;
        break;
    case COMPETITION_ENTRY_WAIT_UPDATED_RECORD_SAVE:
        if (GameData_UpdateSave(param->param->gameData) == SAVE_RESULT_OK) {
            *seq = COMPETITION_ENTRY_END;
        }
        break;
    case COMPETITION_ENTRY_START_TEAM_REGISTRATION:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRegister);
        break;
    case COMPETITION_ENTRY_START_COMPETITION_MENU:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
        break;
    case COMPETITION_ENTRY_END:
        param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskEnd);
        break;
    case COMPETITION_ENTRY_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskRegister(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchPlayer *player;
    Regulation *regulation;
    PokeParty *party;
    SaveControl *save;
    BattleBoxSave *battleBox;
    TournamentTeam *previous;
    BOOL ready;
    u32 invalid;
    BOOL otherInvalid;
    u32 invalidFlags;
    s32 selected;
    u32 ret;
    int count;
    int i;
    int zeroCount;

    switch (*seq) {
    case COMPETITION_REGISTER_CREATE_CARD_AND_TEAM:
        WifiBattleMatchLink_CompetitionCreateCard(link);
        WifiBattleMatchLink_CompetitionSetCardPage(link, WIFIBATTLEMATCH_CARD_PAGE_EMPTY);
        WifiBattleMatchLink_CompetitionCreateTeamList(link);
        *seq = COMPETITION_REGISTER_WAIT_CARD_AND_TEAM;
        break;
    case COMPETITION_REGISTER_WAIT_CARD_AND_TEAM:
        ready = TRUE;
        ready &= WifiBattleMatchLink_CompetitionIsCardShown(link);
        if (WifiBattleMatchLink_CompetitionIsTeamListShown(link) & ready) {
            *seq = COMPETITION_REGISTER_PROMPT_REGISTER_TEAM;
        }
        break;
    case COMPETITION_REGISTER_PROMPT_REGISTER_TEAM:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleBoxWillRegistered,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_REGISTER_CREATE_REGISTER_MENU);
        *seq = COMPETITION_REGISTER_WAIT_MESSAGE;
        break;
    case COMPETITION_REGISTER_CREATE_REGISTER_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_REGISTER);
        *seq = COMPETITION_REGISTER_WAIT_REGISTER_MENU;
        break;
    case COMPETITION_REGISTER_WAIT_REGISTER_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_REGISTER_CHECK_TEAM;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_REGISTER_PROMPT_QUIT_REGISTRATION;
            break;
        }
        break;
    case COMPETITION_REGISTER_PROMPT_QUIT_REGISTRATION:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitRegistration,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_REGISTER_CREATE_QUIT_MENU);
        *seq = COMPETITION_REGISTER_WAIT_MESSAGE;
        break;
    case COMPETITION_REGISTER_CREATE_QUIT_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_REGISTER_WAIT_QUIT_MENU;
        break;
    case COMPETITION_REGISTER_WAIT_QUIT_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_REGISTER_CONFIRM_QUIT_COMPETITION;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_REGISTER_PROMPT_REGISTER_TEAM;
            break;
        }
        break;
    case COMPETITION_REGISTER_CONFIRM_QUIT_COMPETITION:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitCompetitionRegistration,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_REGISTER_CREATE_CONFIRM_QUIT_MENU);
        *seq = COMPETITION_REGISTER_WAIT_MESSAGE;
        break;
    case COMPETITION_REGISTER_CREATE_CONFIRM_QUIT_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_REGISTER_WAIT_CONFIRM_QUIT_MENU;
        break;
    case COMPETITION_REGISTER_WAIT_CONFIRM_QUIT_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_REGISTER_HIDE_CANCELLED_TEAM;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_REGISTER_PROMPT_REGISTER_TEAM;
            break;
        }
        break;
    case COMPETITION_REGISTER_HIDE_CANCELLED_TEAM:
        if (WifiBattleMatchLink_CompetitionIsTeamListHidden(link)) {
            WifiBattleMatchLink_CompetitionDeleteTeamList(link);
            *seq = COMPETITION_REGISTER_LOGOUT;
        }
        break;
    case COMPETITION_REGISTER_CHECK_TEAM:
        player = param->player;
        WifiBattleMatchLink_CompetitionFillBattleBoxParty(player, link);
        if (howManyPokesAreAbleToFight((PokeParty *)player->partyData) == 0) {
            *seq = COMPETITION_REGISTER_SHOW_EMPTY_TEAM;
            break;
        }
        invalidFlags = 0;
        regulation = TournamentEntry_GetRegulation(link->entry);
        invalid = PokeRegulation_CheckParty(regulation, (PokeParty *)player->partyData, &invalidFlags);
        otherInvalid = PokeRegulation_PartyHasEgg(regulation, (PokeParty *)player->partyData);
        if (invalid == 0 && otherInvalid == 0) {
            *seq = COMPETITION_REGISTER_HIDE_CHECKED_TEAM;
        } else {
            *seq = COMPETITION_REGISTER_SHOW_INVALID_TEAM;
        }
        break;
    case COMPETITION_REGISTER_SHOW_INVALID_TEAM:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ConditionsTournamentHaveNot,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_REGISTER_LOGOUT);
        *seq = COMPETITION_REGISTER_WAIT_MESSAGE;
        break;
    case COMPETITION_REGISTER_SHOW_EMPTY_TEAM:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereNoPokemonBattle,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_REGISTER_LOGOUT);
        *seq = COMPETITION_REGISTER_WAIT_MESSAGE;
        break;
    case COMPETITION_REGISTER_HIDE_CHECKED_TEAM:
        if (WifiBattleMatchLink_CompetitionIsTeamListHidden(link)) {
            WifiBattleMatchLink_CompetitionDeleteTeamList(link);
            *seq = COMPETITION_REGISTER_SHOW_CHECKING_MESSAGE;
        }
        break;
    case COMPETITION_REGISTER_SHOW_CHECKING_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_REGISTER_START_SERVER_CHECK);
        *seq = COMPETITION_REGISTER_WAIT_MESSAGE;
        break;
    case COMPETITION_REGISTER_START_SERVER_CHECK:
        WifiBattleMatchNet_StartPokemonCheck(link->net, param->player->partyData, WBM_POKEMON_CHECK_PARTY);
        MI_CpuFill8(&link->signature, 0, sizeof(link->signature));
        *seq = COMPETITION_REGISTER_WAIT_SERVER_CHECK;
        break;
    case COMPETITION_REGISTER_WAIT_SERVER_CHECK:
        ret = WifiBattleMatchNet_PokemonCheckMain(link->net, &link->signature);
        if (ret == WBM_NET_SUCCESS) {
            count = PokeParty_GetPkmCount((PokeParty *)param->player->partyData);
            zeroCount = 0;
            for (i = 0; i < count; i++) {
                if (link->signature.result[i] == WBM_POKEMON_VALIDATION_OK) {
                    zeroCount++;
                }
            }
            if (link->signature.status == WBM_POKEMON_VALIDATION_OK && zeroCount == count) {
                *seq = COMPETITION_REGISTER_UPLOAD_TEAM;
            } else {
                *seq = COMPETITION_REGISTER_SHOW_SERVER_REJECTION;
            }
        } else if (ret == WBM_NET_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            *seq = COMPETITION_REGISTER_CREATE_CARD_AND_TEAM;
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_REGISTER_SHOW_SERVER_REJECTION:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereProblemPokemonThey,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_REGISTER_LOGOUT);
        *seq = COMPETITION_REGISTER_WAIT_MESSAGE;
        break;
    case COMPETITION_REGISTER_UPLOAD_TEAM:
        if (*link->param->recordRequestCooldown == 0) {
            WifiBattleMatchNet_StartWrite(link->net, WBM_WRITE_WIFICUP, param->player->partyData);
            *seq = COMPETITION_REGISTER_WAIT_TEAM_UPLOAD;
        }
        break;
    case COMPETITION_REGISTER_WAIT_TEAM_UPLOAD:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 1800;
            *seq = COMPETITION_REGISTER_LOCK_TEAM;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            *seq = COMPETITION_REGISTER_CREATE_CARD_AND_TEAM;
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_REGISTER_LOCK_TEAM:
        save = GameData_GetSaveControl(param->param->gameData);
        battleBox = getBattleBox(save);
        previous = TournamentBlock_GetTeam(getTournamentBlockAddress(save), 0);
        party = convertBoxedPokeSetToParty(battleBox, HEAPID_TAIL(HEAPID_WIFIBATTLEMATCH));
        BattleBoxSave_SetFlags(battleBox, 1);
        TournamentEntry_SetField(link->entry, TOURNAMENT_FIELD_UNK15B, WBM_REGISTRATION_COMPLETE);
        TournamentTeam_SetFromParty(previous, party);
        GFL_HeapFree(party);
        *seq = COMPETITION_REGISTER_START_SAVE;
        break;
    case COMPETITION_REGISTER_START_SAVE:
        GameData_StartSave(param->param->gameData);
        *seq = COMPETITION_REGISTER_WAIT_SAVE;
        break;
    case COMPETITION_REGISTER_WAIT_SAVE:
        ret = GameData_UpdateSave(param->param->gameData);
        if (ret == SAVE_RESULT_OK) {
            *seq = COMPETITION_REGISTER_POST_REGISTRATION;
        } else if (ret == SAVE_RESULT_NG) {
            *seq = COMPETITION_REGISTER_LOGOUT;
        }
        break;
    case COMPETITION_REGISTER_POST_REGISTRATION: {
        WifiBattleMatchLinkRequest request;

        MI_CpuFill8(&request, 0, sizeof(request));
        request.registrationState = WBM_REGISTRATION_COMPLETE;
        request.profileId = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->registrationState = WBM_REGISTRATION_COMPLETE;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = COMPETITION_REGISTER_WAIT_REGISTRATION_POST;
        break;
    }
    case COMPETITION_REGISTER_WAIT_REGISTRATION_POST:
        ret = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ret == WBM_NET_SUCCESS) {
            *seq = COMPETITION_REGISTER_SHOW_COMPLETE_MESSAGE;
        } else if (ret == WBM_NET_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            *seq = COMPETITION_REGISTER_CREATE_CARD_AND_TEAM;
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_REGISTER_SHOW_COMPLETE_MESSAGE:
        WifiBattleMatchLink_CompetitionSetCardPage(link, WIFIBATTLEMATCH_CARD_PAGE_REGISTERED);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CompetitionRegistrationComplete,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_REGISTER_START_COMPETITION_MENU);
        *seq = COMPETITION_REGISTER_WAIT_MESSAGE;
        break;
    case COMPETITION_REGISTER_START_COMPETITION_MENU:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
        break;
    case COMPETITION_REGISTER_LOGOUT:
        param->action = WIFIBATTLEMATCH_LINK_ACTION_LOGOUT;
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case COMPETITION_REGISTER_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskMenu(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    u32 ret;
    s32 selected;

    switch (*seq) {
    case COMPETITION_MENU_FLOW_CHECK_CONNECTION:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_OK:
            *seq = COMPETITION_MENU_FLOW_CREATE_CARD;
            break;
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MENU_FLOW_CREATE_CARD:
        if (link->card == NULL) {
            WifiBattleMatchLink_CompetitionCreateCard(link);
            WifiBattleMatchLink_CompetitionSetCardPage(link, WIFIBATTLEMATCH_CARD_PAGE_REGISTERED);
            *seq = COMPETITION_MENU_FLOW_WAIT_CARD_SHOWN;
        } else {
            *seq = COMPETITION_MENU_FLOW_SHOW_BATTLE_PROMPT;
        }
        break;
    case COMPETITION_MENU_FLOW_WAIT_CARD_SHOWN:
        if (WifiBattleMatchLink_CompetitionIsCardShown(link)) {
            *seq = COMPETITION_MENU_FLOW_SHOW_BATTLE_PROMPT;
        }
        break;
    case COMPETITION_MENU_FLOW_SHOW_BATTLE_PROMPT:
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PokemonCompetitionOverNintendo,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MENU_FLOW_CREATE_BATTLE_MENU);
        *seq = COMPETITION_MENU_FLOW_WAIT_MESSAGE;
        break;
    case COMPETITION_MENU_FLOW_CREATE_BATTLE_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_BATTLE);
        *seq = COMPETITION_MENU_FLOW_WAIT_BATTLE_MENU;
        break;
    case COMPETITION_MENU_FLOW_WAIT_BATTLE_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case COMPETITION_MENU_SELECTION_BATTLE:
            *seq = COMPETITION_MENU_FLOW_SHOW_TEAM_CHECK_MESSAGE;
            break;
        case COMPETITION_MENU_SELECTION_UNREGISTER:
            *seq = COMPETITION_MENU_FLOW_START_UNREGISTER;
            break;
        case COMPETITION_MENU_SELECTION_QUIT:
            *seq = COMPETITION_MENU_FLOW_START_CANCEL;
            break;
        }
        break;
    case COMPETITION_MENU_FLOW_START_CANCEL:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskCancel);
        break;
    case COMPETITION_MENU_FLOW_START_UNREGISTER:
        WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskUnregister);
        *seq = COMPETITION_MENU_FLOW_WAIT_UNREGISTER;
        break;
    case COMPETITION_MENU_FLOW_WAIT_UNREGISTER:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == WBM_UNREGISTER_RESULT_NOT_CHANGED || ret == WBM_UNREGISTER_RESULT_NOT_REGISTERED) {
            *seq = COMPETITION_MENU_FLOW_CREATE_CARD;
        } else if (ret == WBM_UNREGISTER_RESULT_DONE) {
            *seq = COMPETITION_MENU_FLOW_START_LOGOUT;
        } else if (ret == WBM_UNREGISTER_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MENU_FLOW_START_LOGOUT:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case COMPETITION_MENU_FLOW_SHOW_TEAM_CHECK_MESSAGE:
        WifiBattleMatchLink_CompetitionFillBattleBoxParty(param->player, link);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MENU_FLOW_START_TEAM_CHECK);
        *seq = COMPETITION_MENU_FLOW_WAIT_MESSAGE;
        break;
    case COMPETITION_MENU_FLOW_START_TEAM_CHECK:
        WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskSignTeam);
        *seq = COMPETITION_MENU_FLOW_WAIT_TEAM_CHECK;
        break;
    case COMPETITION_MENU_FLOW_WAIT_TEAM_CHECK:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == WBM_TEAM_CHECK_RESULT_VALID) {
            *seq = COMPETITION_MENU_FLOW_START_MATCH;
        } else if (ret == WBM_TEAM_CHECK_RESULT_INVALID) {
            *seq = COMPETITION_MENU_FLOW_START_MATCH;
        } else if (ret == WBM_TEAM_CHECK_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MENU_FLOW_START_MATCH:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMatch);
        break;
    case COMPETITION_MENU_FLOW_SHOW_INVALID_TEAM:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ThereProblemPokemonThey_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_MENU_FLOW_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MENU_FLOW_LOGOUT_INVALID_TEAM);
        break;
    case COMPETITION_MENU_FLOW_LOGOUT_INVALID_TEAM:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case COMPETITION_MENU_FLOW_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskMatch(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchNetKeys condition;
    void *received;
    u32 flags;
    BOOL opponentNameRejected;
    char ownName[8];
    char otherName[8];
    WifiBattleMatchPlayer *player;
    WifiBattleMatchPlayer *otherPlayer;
    PokeParty *party;
    TournamentEntry *entry;
    Regulation *regulation;
    BOOL countIdle = FALSE;
    BOOL canCancel = FALSE;
    BOOL canDisconnect;
    BOOL timedOut;
    BOOL noTerminator;
    u32 cancel;
    u32 fixCount;
    u32 ret;
    s32 selected;
    int i;

    if ((s32)*seq >= COMPETITION_MATCH_WAIT_MATCH && (s32)*seq < COMPETITION_MATCH_SYNC_VALIDATED_PLAYERS) {
        canCancel = TRUE;
    }
    canDisconnect =
        ((s32)*seq < COMPETITION_MATCH_START_NAME_CHECK || (s32)*seq > COMPETITION_MATCH_WAIT_NAME_CHECK) &&
        ((s32)*seq < COMPETITION_MATCH_DOWNLOAD_OPPONENT_TEAM || (s32)*seq > COMPETITION_MATCH_WAIT_OPPONENT_TEAM);
    cancel = WifiBattleMatchLink_CompetitionUpdateCancel(link, canCancel, canDisconnect);
    if (cancel == CANCEL_CONFIRMED) {
        *seq = COMPETITION_MATCH_PROMPT_QUIT;
        return;
    }
    switch (*seq) {
    case COMPETITION_MATCH_SHOW_PREPARATION_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_ReceivingDataPleaseWait,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MATCH_CHECK_PERIOD_CONNECTION);
        *seq = COMPETITION_MATCH_WAIT_MESSAGE;
        break;
    case COMPETITION_MATCH_CHECK_PERIOD_CONNECTION:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case WBM_RESULT_OK:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = COMPETITION_MATCH_WAIT_PERIOD;
            break;
        }
        break;
    case COMPETITION_MATCH_WAIT_PERIOD:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == WBM_PERIOD_RESULT_ACTIVE) {
            *seq = COMPETITION_MATCH_DOWNLOAD_RECORD;
        } else if (ret == WBM_PERIOD_RESULT_EXPIRED || ret == WBM_PERIOD_RESULT_NOT_STARTED) {
            *seq = COMPETITION_MATCH_LOGOUT;
        } else if (ret == WBM_PERIOD_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MATCH_LOGOUT:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case COMPETITION_MATCH_DOWNLOAD_RECORD:
        if (*link->param->recordRequestCooldown != 0) {
            break;
        }
        WifiBattleMatchNet_StartRead(link->net, -1, WBM_READ_WIFICUP, link->param->ranking);
        *seq = COMPETITION_MATCH_WAIT_RECORD;
        break;
    case COMPETITION_MATCH_WAIT_RECORD:
        ret = WifiBattleMatchNet_ReadMain(link->net);
        if (ret == WBM_NET_SUCCESS) {
            player = param->player;
            *link->param->recordRequestCooldown = 0;
            WifiBattleMatchLink_CompetitionFillPlayer(player, link);
            *link->param->recordSaveIndex = link->param->ranking->recordSaveIndex;
            if (WifiBattleMatchLink_CompetitionUpdateRecord(link, link->param->ranking)) {
                *seq = COMPETITION_MATCH_DELAY_CARD_REFRESH;
            } else {
                *seq = COMPETITION_MATCH_SHOW_SEARCH_MESSAGE;
            }
        }
        if (ret == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MATCH_DELAY_CARD_REFRESH:
        *seq = COMPETITION_MATCH_HIDE_CARD;
        break;
    case COMPETITION_MATCH_HIDE_CARD:
        if (WifiBattleMatchLink_CompetitionIsCardHidden(link)) {
            WifiBattleMatchLink_CompetitionDeleteCard(link);
            *seq = COMPETITION_MATCH_CREATE_CARD;
        }
        break;
    case COMPETITION_MATCH_CREATE_CARD:
        WifiBattleMatchLink_CompetitionCreateCard(link);
        *seq = COMPETITION_MATCH_WAIT_CARD_SHOWN;
        break;
    case COMPETITION_MATCH_WAIT_CARD_SHOWN:
        if (WifiBattleMatchLink_CompetitionIsCardShown(link)) {
            *seq = COMPETITION_MATCH_SHOW_SEARCH_MESSAGE;
        }
        break;
    case COMPETITION_MATCH_SHOW_SEARCH_MESSAGE:
        link->cancelSeq = COMPETITION_SEARCH_CANCEL_IDLE;
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE);
        *seq = COMPETITION_MATCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MATCH_START_SEARCH);
        break;
    case COMPETITION_MATCH_START_SEARCH:
        entry = link->entry;
        MI_CpuFill8(&condition, 0, sizeof(condition));
        condition.rating = link->param->ranking->rating;
        condition.disconnects = link->param->ranking->disconnectsCounter;
        condition.cup = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_CUP_ID);
        condition.matches = link->param->ranking->wins + link->param->ranking->losses;
        WifiBattleMatchNet_StartMatch(link->net, WBM_MATCH_MODE_COMPETITION, 0, param->param->battleType, &condition);
        link->opponentValidationErrors = 0;
        link->peerValidationErrors = 0;
        *seq = COMPETITION_MATCH_WAIT_MATCH;
        break;
    case COMPETITION_MATCH_WAIT_MATCH:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        ret = WifiBattleMatchNet_MatchMain(link->net);
        if (ret == WBM_NET_SUCCESS) {
            *seq = COMPETITION_MATCH_SEND_PLAYER_INFO;
        } else if (ret == WBM_NET_FAILED) {
            *seq = COMPETITION_MATCH_START_SEARCH;
        }
        break;
    case COMPETITION_MATCH_SEND_PLAYER_INFO:
        if (cancel == CANCEL_NONE) {
            if (WifiBattleMatchNet_SendPlayerInfo(link->net, param->player)) {
                *seq = COMPETITION_MATCH_RECEIVE_PLAYER_INFO;
                link->idleTimer = 0;
            }
        } else {
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
            case WBM_RESULT_DISCONNECTED:
            case WBM_RESULT_TIMEOUT:
                link->cancelSeq = COMPETITION_SEARCH_CANCEL_IDLE;
                WifiBattleMatchLink_CompetitionDeleteMenu(link);
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
                WifiBattleMatchLink_CompetitionResetCancel(link);
                break;
            case WBM_RESULT_FAILED:
                WifiBattleMatchLink_CompetitionDeleteMenu(link);
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
                break;
            }
        }
        break;
    case COMPETITION_MATCH_RECEIVE_PLAYER_INFO:
        if (WifiBattleMatchNet_TakePlayerInfo(link->net, &received)) {
            MI_CpuCopy8(received, param->otherPlayer, PokeParty_GetSaveDataSize() + sizeof(WifiBattleMatchPlayer));
            otherPlayer = param->otherPlayer;
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
            *seq = COMPETITION_MATCH_CHECK_COMPETITION_ID;
        }
        countIdle = TRUE;
        break;
    case COMPETITION_MATCH_CHECK_COMPETITION_ID:
        player = param->player;
        StrUnicodeToSjis(player->info.name, ownName, 8);
        party = (PokeParty *)player->partyData;
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            PokeParty_GetPkm(party, i);
        }
        otherPlayer = param->otherPlayer;
        StrUnicodeToSjis(otherPlayer->info.name, otherName, 8);
        party = (PokeParty *)otherPlayer->partyData;
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            PokeParty_GetPkm(party, i);
        }
        if (param->player->entryField2 == param->otherPlayer->entryField2 &&
            param->player->checksum == param->otherPlayer->checksum) {
            *seq = COMPETITION_MATCH_CHECK_OPPONENT_TEAM;
        } else {
            *seq = COMPETITION_MATCH_START_DISCONNECT;
        }
        break;
    case COMPETITION_MATCH_CHECK_OPPONENT_TEAM:
        flags = 0;
        regulation = TournamentEntry_GetRegulation(link->entry);
        if (PokeRegulation_CheckParty(regulation, (PokeParty *)param->otherPlayer->partyData, &flags) == 0) {
            *seq = COMPETITION_MATCH_DOWNLOAD_OPPONENT_TEAM;
        } else {
            link->opponentValidationErrors++;
            *seq = COMPETITION_MATCH_SEND_VALIDATION_RESULT;
        }
        break;
    case COMPETITION_MATCH_DOWNLOAD_OPPONENT_TEAM:
        if (*link->param->recordRequestCooldown == 0) {
            WifiBattleMatchNet_StartRead(link->net, link->param->otherPlayer->recordId, WBM_READ_WIFICUP_PARTY,
                                         link->party);
            *seq = COMPETITION_MATCH_WAIT_OPPONENT_TEAM;
        }
        break;
    case COMPETITION_MATCH_WAIT_OPPONENT_TEAM:
        ret = WifiBattleMatchNet_ReadMain(link->net);
        if (ret == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 3600;
            *seq = COMPETITION_MATCH_SYNC_TEAM_DOWNLOAD;
        }
        if (ret == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MATCH_SYNC_TEAM_DOWNLOAD:
        WifiBattleMatchNet_StartSync(link->net, WBM_SYNC_TEAM_DOWNLOADED);
        *seq = COMPETITION_MATCH_WAIT_TEAM_DOWNLOAD_SYNC;
        break;
    case COMPETITION_MATCH_WAIT_TEAM_DOWNLOAD_SYNC:
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = COMPETITION_MATCH_VERIFY_OPPONENT_TEAM;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MATCH_VERIFY_OPPONENT_TEAM:
        if (!WifiBattleMatchLink_CompetitionCompareParties(link->party,
                                                           (PokeParty *)link->param->otherPlayer->partyData)) {
            link->opponentValidationErrors++;
        }
        if (!WifiBattleMatchLink_CompetitionCheckSignature(link->param->otherPlayer,
                                                           (PokeParty *)link->param->otherPlayer->partyData,
                                                           HEAPID_WIFIBATTLEMATCH_LINK)) {
            link->opponentValidationErrors++;
        }
        *seq = COMPETITION_MATCH_SEND_VALIDATION_RESULT;
        break;
    case COMPETITION_MATCH_SEND_VALIDATION_RESULT:
        if (cancel == CANCEL_NONE) {
            if (WifiBattleMatchNet_SendValidationErrors(link->net, &link->opponentValidationErrors)) {
                *seq = COMPETITION_MATCH_RECEIVE_VALIDATION_RESULT;
                link->idleTimer = 0;
            }
        } else {
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
            case WBM_RESULT_DISCONNECTED:
            case WBM_RESULT_TIMEOUT:
                link->cancelSeq = COMPETITION_SEARCH_CANCEL_IDLE;
                WifiBattleMatchLink_CompetitionDeleteMenu(link);
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
                WifiBattleMatchLink_CompetitionResetCancel(link);
                break;
            case WBM_RESULT_FAILED:
                WifiBattleMatchLink_CompetitionDeleteMenu(link);
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
                break;
            }
        }
        break;
    case COMPETITION_MATCH_RECEIVE_VALIDATION_RESULT:
        if (WifiBattleMatchNet_TakeValidationErrors(link->net, &link->peerValidationErrors)) {
            *seq = COMPETITION_MATCH_CHECK_VALIDATION_RESULTS;
        } else {
            countIdle = TRUE;
        }
        break;
    case COMPETITION_MATCH_CHECK_VALIDATION_RESULTS:
        if (link->peerValidationErrors == 0 && link->opponentValidationErrors == 0) {
            *seq = COMPETITION_MATCH_START_NAME_CHECK;
        } else {
            *seq = COMPETITION_MATCH_START_DISCONNECT;
        }
        break;
    case COMPETITION_MATCH_START_NAME_CHECK:
        WifiBattleMatchNet_StartNameCheck(link->net, &param->otherPlayer->info, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = COMPETITION_MATCH_WAIT_NAME_CHECK;
        break;
    case COMPETITION_MATCH_WAIT_NAME_CHECK:
        if (WifiBattleMatchNet_NameCheckMain(link->net, &opponentNameRejected)) {
            *link->param->opponentNameRejected = opponentNameRejected;
            *seq = COMPETITION_MATCH_SYNC_VALIDATED_PLAYERS;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MATCH_SYNC_VALIDATED_PLAYERS:
        if (cancel == CANCEL_NONE) {
            WifiBattleMatchNet_StartSync(link->net, WBM_SYNC_VALIDATED_PLAYERS);
            link->idleTimer = 0;
            *seq = COMPETITION_MATCH_WAIT_VALIDATED_PLAYERS_SYNC;
        }
        break;
    case COMPETITION_MATCH_WAIT_VALIDATED_PLAYERS_SYNC:
        if (!WifiBattleMatchNet_TakeCancelSearch(link->net) && WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = COMPETITION_MATCH_SYNC_MATCH_READY;
        }
        countIdle = TRUE;
        break;
    case COMPETITION_MATCH_SYNC_MATCH_READY:
        WifiBattleMatchNet_StartSync(link->net, WBM_SYNC_MATCH_READY);
        *seq = COMPETITION_MATCH_WAIT_MATCH_READY_SYNC;
        link->idleTimer = 0;
        break;
    case COMPETITION_MATCH_WAIT_MATCH_READY_SYNC:
        if (WifiBattleMatchNet_CheckSync(link->net)) {
            *seq = COMPETITION_MATCH_SHOW_OPPONENT_FOUND;
        }
        countIdle = TRUE;
        break;
    case COMPETITION_MATCH_SHOW_OPPONENT_FOUND:
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_74);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_OpponentHasBeenFound_2,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_MATCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MATCH_WAIT_OPPONENT_FOUND_DELAY);
        link->timer = 0;
        break;
    case COMPETITION_MATCH_WAIT_OPPONENT_FOUND_DELAY:
        if (link->timer++ > 120) {
            link->timer = 0;
            GFL_BGSysSetBGEnabled(0, FALSE);
            WifiBattleMatchLink_CompetitionCreatePartnerCard(link, param->otherPlayer);
            *seq = COMPETITION_MATCH_WAIT_PARTNER_CARD;
        }
        break;
    case COMPETITION_MATCH_WAIT_PARTNER_CARD:
        if (WifiBattleMatchLink_CompetitionIsPartnerCardDone(link)) {
            *seq = COMPETITION_MATCH_WAIT_PARTNER_CARD_DELAY;
        }
        break;
    case COMPETITION_MATCH_WAIT_PARTNER_CARD_DELAY:
        if (link->timer++ != 0) {
            link->timer = 0;
            *seq = COMPETITION_MATCH_START_SESSION_REPORT;
        }
        break;
    case COMPETITION_MATCH_START_SESSION_REPORT:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK_DWC, 0x60000);
        DWCRapCommon_SetSubHeapEx(HEAPID_DEVICE_ALLOC, 0x60000, 1000, HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        DWCRapCommon_SetSubHeapOwner(HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        WifiBattleMatchNet_StartReport(link->net, WBM_REPORT_MODE_START, WBM_MATCH_MODE_COMPETITION, 0, NULL, 0);
        link->exchangeResult = 0;
        *seq = COMPETITION_MATCH_WAIT_SESSION_REPORT;
        link->timer = 0;
        link->exchangeDone = WBM_NET_RUNNING;
        break;
    case COMPETITION_MATCH_WAIT_SESSION_REPORT:
        if (link->exchangeDone == WBM_NET_RUNNING) {
            link->exchangeDone = WifiBattleMatchNet_ReportMain(link->net, &link->exchangeResult);
        }
        if (link->exchangeDone == WBM_NET_SUCCESS) {
            *seq = COMPETITION_MATCH_SHOW_TEAM_SELECTION_MESSAGE;
        }
        if (link->exchangeDone != WBM_NET_RUNNING) {
            switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
            case WBM_RESULT_DISCONNECTED:
                WifiBattleMatchNet_CancelMatch(link->net);
                // fall through
            case WBM_RESULT_FAILED:
            case WBM_RESULT_TIMEOUT:
                DWCRapCommon_EndSubHeap();
                WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
                WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
                break;
            case WBM_RESULT_OK:
                if (link->exchangeDone == WBM_NET_FAILED) {
                    if (link->timer++ > 3600) {
                        GFL_ASSERT(0);
                    }
                }
                break;
            }
        }
        break;
    case COMPETITION_MATCH_SHOW_TEAM_SELECTION_MESSAGE:
        DWCRapCommon_EndSubHeap();
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PleaseChoosePokemon,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = COMPETITION_MATCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MATCH_WAIT_TEAM_SELECTION_DELAY);
        break;
    case COMPETITION_MATCH_WAIT_TEAM_SELECTION_DELAY:
        if (link->timer++ > 60) {
            link->timer = 0;
            param->action = WIFIBATTLEMATCH_LINK_ACTION_TEAM_LIST;
            WifiBattleMatchTask_End(task);
        }
        break;
    case COMPETITION_MATCH_CANCEL_SESSION:
        WifiBattleMatchNet_CancelMatch(link->net);
        WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
        param->result = WBM_LINK_RESULT_CANCELLED;
        link->hadExchangeResult = TRUE;
        if (link->exchangeResult != 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskResult);
        } else {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
        }
        break;
    case COMPETITION_MATCH_PROMPT_QUIT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitCompetition,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_MATCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MATCH_CREATE_QUIT_MENU);
        break;
    case COMPETITION_MATCH_CREATE_QUIT_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_MATCH_WAIT_QUIT_MENU;
        break;
    case COMPETITION_MATCH_WAIT_QUIT_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskQuit);
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_MATCH_PROMPT_CONTINUE;
            break;
        }
        break;
    case COMPETITION_MATCH_PROMPT_CONTINUE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WantContinueBattling,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_MATCH_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_MATCH_CREATE_CONTINUE_MENU);
        break;
    case COMPETITION_MATCH_CREATE_CONTINUE_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_MATCH_WAIT_CONTINUE_MENU;
        break;
    case COMPETITION_MATCH_WAIT_CONTINUE_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_MATCH_DELAY_PERIOD_RECHECK;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_MATCH_PROMPT_QUIT;
            break;
        }
        break;
    case COMPETITION_MATCH_DELAY_PERIOD_RECHECK:
        *seq = COMPETITION_MATCH_CHECK_PERIOD_RECONNECTION;
        break;
    case COMPETITION_MATCH_CHECK_PERIOD_RECONNECTION:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case WBM_RESULT_OK:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = COMPETITION_MATCH_WAIT_PERIOD_RECHECK;
            break;
        }
        break;
    case COMPETITION_MATCH_WAIT_PERIOD_RECHECK:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == WBM_PERIOD_RESULT_ACTIVE) {
            *seq = COMPETITION_MATCH_SHOW_SEARCH_MESSAGE;
        } else if (ret == WBM_PERIOD_RESULT_EXPIRED || ret == WBM_PERIOD_RESULT_NOT_STARTED) {
            *seq = COMPETITION_MATCH_LOGOUT;
        } else if (ret == WBM_PERIOD_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMenu);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_MATCH_SEND_CANCEL_COMMAND:
        if (WifiBattleMatchNet_SendCancelSearch(link->net)) {
            *seq = COMPETITION_MATCH_CANCEL_AND_RETRY;
        }
        break;
    case COMPETITION_MATCH_CANCEL_AND_RETRY:
        WifiBattleMatchNet_CancelMatch(link->net);
        *seq = COMPETITION_MATCH_SHOW_SEARCH_MESSAGE;
        break;
    case COMPETITION_MATCH_START_DISCONNECT:
        WifiBattleMatchNet_ResetEnd(link->net);
        *seq = COMPETITION_MATCH_WAIT_DISCONNECT;
        link->idleTimer = 0;
        break;
    case COMPETITION_MATCH_WAIT_DISCONNECT:
        countIdle = TRUE;
        if (WifiBattleMatchNet_EndMain(link->net)) {
            *seq = COMPETITION_MATCH_START_SEARCH;
        }
        break;
    case COMPETITION_MATCH_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
    if (countIdle) {
        timedOut = FALSE;
        if (link->idleTimer++ > 1200) {
            link->idleTimer = 0;
            timedOut = TRUE;
        }
        if (WifiBattleMatchNet_TakeCancelSearch(link->net)) {
            timedOut = TRUE;
        }
        if (timedOut) {
            WifiBattleMatchNet_CancelMatch(link->net);
            if (!GFL_NetErrCheck() && GFL_NetIsInitialized()) {
                DWC_CloseAllConnectionsHard();
            }
            *seq = COMPETITION_MATCH_SHOW_SEARCH_MESSAGE;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            WifiBattleMatchLink_CompetitionResetCancel(link);
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchLinkIcon_SetShown(link->linkIcon, FALSE);
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
    }
}

static void WifiBattleMatchLink_CompetitionTaskResult(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkReportBuf reportBuf;
    RivalDataSave *rivalData;
    u32 ready;

    switch (*seq) {
    case COMPETITION_RESULT_CREATE_REPORT_HEAP:
        if (param->result == WBM_LINK_RESULT_CANCELLED && link->hadExchangeResult == 0) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskAfterBattle);
            return;
        }
        link->hadExchangeResult = 0;
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_LINK_DWC, 0x60000);
        DWCRapCommon_SetSubHeapEx(HEAPID_DEVICE_ALLOC, 0x60000, 1000, HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        DWCRapCommon_SetSubHeapOwner(HEAPID_WIFIBATTLEMATCH_LINK_DWC);
        *seq = COMPETITION_RESULT_SHOW_REPORT_MESSAGE;
        break;
    case COMPETITION_RESULT_SHOW_REPORT_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = COMPETITION_RESULT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_RESULT_START_REPORT);
        break;
    case COMPETITION_RESULT_START_REPORT:
        WifiBattleMatchNet_StartReport(link->net, WBM_REPORT_MODE_RESULT, WBM_MATCH_MODE_COMPETITION,
                                       param->param->battleType, param->battleResult,
                                       param->result == WBM_LINK_RESULT_CANCELLED);
        link->exchangeDone = WBM_NET_RUNNING;
        link->exchangeResult = 0;
        *seq = COMPETITION_RESULT_WAIT_REPORT;
        break;
    case COMPETITION_RESULT_WAIT_REPORT:
        if (link->exchangeDone == WBM_NET_RUNNING) {
            link->exchangeDone = WifiBattleMatchNet_ReportMain(link->net, &link->exchangeResult);
            if (link->exchangeDone != WBM_NET_RUNNING && link->exchangeResult != 0) {
                WifiBattleMatch_RecordResult(param->param->gameData, param->battleResult->result);
            }
        }
        if (link->exchangeDone == WBM_NET_SUCCESS) {
            param->battleResult->otherCommandCheckError = WifiBattleMatchNet_GetPeerCommandCheckError(link->net);
            *seq = COMPETITION_RESULT_FINISH_REPORT;
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
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            DWCRapCommon_EndSubHeap();
            return;
        }
        break;
    case COMPETITION_RESULT_FINISH_REPORT:
        DWCRapCommon_EndSubHeap();
        if (param->result == WBM_LINK_RESULT_CANCELLED) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskAfterBattle);
            return;
        }
        *seq = COMPETITION_RESULT_RECORD_MET_TRAINER;
        break;
    case COMPETITION_RESULT_RECORD_MET_TRAINER:
        rivalData = getHollow_RivalData(GameData_GetSaveControl(param->param->gameData));
        Misc_AddMetTrainer(rivalData, getIDAsUInt(&param->otherPlayer->info));
        WifiBattleMatchNet_ResetEnd(link->net);
        *seq = COMPETITION_RESULT_WAIT_DISCONNECT;
        break;
    case COMPETITION_RESULT_WAIT_DISCONNECT:
        if (WifiBattleMatchNet_EndMain(link->net)) {
            *seq = COMPETITION_RESULT_UPLOAD_RESULTS;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            *seq = COMPETITION_RESULT_UPLOAD_RESULTS;
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            return;
        }
        break;
    case COMPETITION_RESULT_UPLOAD_RESULTS:
        if (*link->param->recordRequestCooldown != 0) {
            break;
        }
        MI_CpuFill8(&reportBuf, 0, sizeof(reportBuf));
        reportBuf.report = *link->param->report;
        reportBuf.recordSaveIndex = *link->param->recordSaveIndex;
        WifiBattleMatchNet_StartWrite(link->net, WBM_WRITE_RECORD_DATA, &reportBuf);
        *seq = COMPETITION_RESULT_WAIT_RESULTS_UPLOAD;
        break;
    case COMPETITION_RESULT_WAIT_RESULTS_UPLOAD:
        ready = WifiBattleMatchNet_WriteMain(link->net);
        if (ready == WBM_NET_SUCCESS) {
            *link->param->recordRequestCooldown = 1800;
            *seq = COMPETITION_RESULT_SHOW_RESULTS_SENT;
        }
        if (ready == WBM_NET_RUNNING) {
            break;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            return;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            return;
        }
        break;
    case COMPETITION_RESULT_SHOW_RESULTS_SENT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleResultsWereSent,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_RESULT_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_RESULT_SHOW_RESULT_SCREEN);
        break;
    case COMPETITION_RESULT_SHOW_RESULT_SCREEN:
        param->action = WIFIBATTLEMATCH_LINK_ACTION_RESULT;
        WifiBattleMatchTask_End(task);
        break;
    case COMPETITION_RESULT_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskAfterBattle(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    u32 ret;

    switch (*seq) {
    case COMPETITION_AFTER_BATTLE_SHOW_SAVE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = COMPETITION_AFTER_BATTLE_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_AFTER_BATTLE_START_SAVE);
        break;
    case COMPETITION_AFTER_BATTLE_START_SAVE:
        if (param->result != WBM_LINK_RESULT_CANCELLED) {
            WifiBattleMatch_RecordBattleTime(param->param->gameData, param->battleResult->result);
        }
        WifiBattleMatchLink_CompetitionUpdateRecord(link, link->param->ranking);
        GameData_StartSave(param->param->gameData);
        *seq = COMPETITION_AFTER_BATTLE_WAIT_SAVE;
        break;
    case COMPETITION_AFTER_BATTLE_WAIT_SAVE:
        ret = GameData_UpdateSave(param->param->gameData);
        if (ret == SAVE_RESULT_OK) {
            *seq = COMPETITION_AFTER_BATTLE_CHECK_PERIOD_CONNECTION;
        } else if (ret == SAVE_RESULT_NG) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        }
        break;
    case COMPETITION_AFTER_BATTLE_CHECK_PERIOD_CONNECTION:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case WBM_RESULT_OK:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = COMPETITION_AFTER_BATTLE_WAIT_PERIOD;
            break;
        }
        break;
    case COMPETITION_AFTER_BATTLE_WAIT_PERIOD:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == WBM_PERIOD_RESULT_ACTIVE) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
        } else if (ret == WBM_PERIOD_RESULT_EXPIRED || ret == WBM_PERIOD_RESULT_NOT_STARTED) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        } else if (ret == WBM_PERIOD_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_AFTER_BATTLE_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskContinue(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    u32 ret;
    s32 selected;

    switch (*seq) {
    case COMPETITION_CONTINUE_CHECK_CONNECTION:
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, TRUE)) {
        case WBM_RESULT_OK:
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_TIMEOUT:
            *seq = COMPETITION_CONTINUE_SHOW_BATTLE_PROMPT;
            break;
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_CONTINUE_SHOW_BATTLE_PROMPT:
        GFL_BGSysSetBGEnabled(0, TRUE);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_PokemonWiFiCompetition,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_CONTINUE_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_CONTINUE_CREATE_MENU);
        break;
    case COMPETITION_CONTINUE_CREATE_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_BATTLE_AGAIN);
        *seq = COMPETITION_CONTINUE_WAIT_MENU;
        break;
    case COMPETITION_CONTINUE_WAIT_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case COMPETITION_MENU_SELECTION_BATTLE:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskMatch);
            break;
        case COMPETITION_MENU_SELECTION_UNREGISTER:
            *seq = COMPETITION_CONTINUE_START_UNREGISTER;
            break;
        case COMPETITION_MENU_SELECTION_QUIT:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskCancel);
            break;
        }
        break;
    case COMPETITION_CONTINUE_START_UNREGISTER:
        WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskUnregister);
        *seq = COMPETITION_CONTINUE_WAIT_UNREGISTER;
        break;
    case COMPETITION_CONTINUE_WAIT_UNREGISTER:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret == WBM_UNREGISTER_RESULT_NOT_CHANGED || ret == WBM_UNREGISTER_RESULT_NOT_REGISTERED) {
            *seq = COMPETITION_CONTINUE_SHOW_BATTLE_PROMPT;
        } else if (ret == WBM_UNREGISTER_RESULT_DONE) {
            *seq = COMPETITION_CONTINUE_LOGOUT;
        }
        switch (WifiBattleMatchNet_CheckError(link->net, FALSE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_CONTINUE_LOGOUT:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case COMPETITION_CONTINUE_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskCancel(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    s32 selected;

    switch (*seq) {
    case COMPETITION_CANCEL_PROMPT_QUIT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitCompetition,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_CANCEL_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_CANCEL_CREATE_MENU);
        break;
    case COMPETITION_CANCEL_CREATE_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        *seq = COMPETITION_CANCEL_WAIT_MENU;
        break;
    case COMPETITION_CANCEL_WAIT_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskQuit);
            break;
        case WBM_MENU_NO:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskContinue);
            break;
        }
        break;
    case COMPETITION_CANCEL_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskLogout(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;

    switch (*seq) {
    case COMPETITION_LOGOUT_SHOW_SAVE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_LOGOUT_START_SAVE);
        *seq = COMPETITION_LOGOUT_WAIT_MESSAGE;
        break;
    case COMPETITION_LOGOUT_START_SAVE:
        GameData_StartSave(param->param->gameData);
        *seq = COMPETITION_LOGOUT_WAIT_SAVE;
        break;
    case COMPETITION_LOGOUT_WAIT_SAVE:
        if (GameData_UpdateSave(param->param->gameData) == SAVE_RESULT_OK) {
            *seq = COMPETITION_LOGOUT_SHOW_UPLOAD_MESSAGE;
        }
        break;
    case COMPETITION_LOGOUT_SHOW_UPLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_LOGOUT_UPLOAD_LOGOUT);
        *seq = COMPETITION_LOGOUT_WAIT_MESSAGE;
        break;
    case COMPETITION_LOGOUT_UPLOAD_LOGOUT:
        if (*link->param->recordRequestCooldown == 0) {
            WifiBattleMatchNet_StartWrite(link->net, WBM_WRITE_PROFILE, NULL);
            *seq = COMPETITION_LOGOUT_WAIT_LOGOUT_UPLOAD;
        }
        break;
    case COMPETITION_LOGOUT_WAIT_LOGOUT_UPLOAD:
        if (WifiBattleMatchNet_WriteMain(link->net)) {
            *link->param->recordRequestCooldown = 1800;
            *seq = COMPETITION_LOGOUT_END;
            WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE);
        }
        break;
    case COMPETITION_LOGOUT_END:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskEnd);
        break;
    case COMPETITION_LOGOUT_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskQuit(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    u32 ret;

    switch (*seq) {
    case COMPETITION_QUIT_CHECK_PERIOD_CONNECTION:
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        case WBM_RESULT_OK:
            WifiBattleMatchLink_CompetitionStartSubTask(link, WifiBattleMatchLink_CompetitionTaskCheckPeriod);
            *seq = COMPETITION_QUIT_WAIT_PERIOD;
            break;
        }
        break;
    case COMPETITION_QUIT_WAIT_PERIOD:
        ret = WifiBattleMatchLink_CompetitionUpdateSubTask(link);
        if (ret != SUB_TASK_RUNNING) {
            *seq = COMPETITION_QUIT_LOGOUT;
        }
        if (ret == WBM_PERIOD_RESULT_SERVER_ERROR) {
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
        }
        switch (WifiBattleMatchNet_CheckError(link->net, TRUE, FALSE)) {
        case WBM_RESULT_DISCONNECTED:
        case WBM_RESULT_FAILED:
            WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskRelogin);
            break;
        }
        break;
    case COMPETITION_QUIT_LOGOUT:
        WifiBattleMatchTask_SetFunc(task, WifiBattleMatchLink_CompetitionTaskLogout);
        break;
    case COMPETITION_QUIT_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskEnd(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchTask_End(task);
}

static void WifiBattleMatchLink_CompetitionTaskRelogin(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;

    link->param->action = WIFIBATTLEMATCH_LINK_ACTION_RELOGIN;
    if (link->net != NULL) {
        WifiBattleMatchNet_Delete(link->net);
        link->net = NULL;
    }
    GFL_NetErrShutdown();
    WifiBattleMatchTask_End(task);
}

static void WifiBattleMatchLink_CompetitionTaskCheckPeriod(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    TournamentEntry *entry;
    RTCDate startDate;
    RTCDate endDate;
    RTCDate date;
    RTCTime time;
    BOOL valid;
    s32 startDay;
    s32 endDay;
    s32 today;
    u8 accountState;
    u32 ret;
    WifiBattleMatchLinkRequest request;

    switch (*seq) {
    case COMPETITION_PERIOD_DELAY_DATE_CHECK:
        *seq = COMPETITION_PERIOD_CHECK_DATES;
        break;
    case COMPETITION_PERIOD_CHECK_DATES:
        entry = link->entry;
        if (!GFL_NetIsInitialized()) {
            GFL_NetErrAbort();
        }
        valid = DWC_GetDateTime(&date, &time);
        if (valid) {
            RTC_ConvertSecondToDateTime(&date, &time, RTC_ConvertDateTimeToSecond(&date, &time));
        }
        startDate.year = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_YEAR);
        startDate.month = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_MONTH);
        startDate.day = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_DAY);
        startDate.week = RTC_WEEK_SUNDAY;
        endDate.year = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_YEAR);
        endDate.month = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_MONTH);
        endDate.day = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_DAY);
        endDate.week = RTC_WEEK_SUNDAY;
        startDay = RTC_ConvertDateToDay(&startDate);
        endDay = RTC_ConvertDateToDay(&endDate);
        today = RTC_ConvertDateToDay(&date);
        if (valid && today < startDay) {
            link->subTaskResult = WBM_PERIOD_RESULT_NOT_STARTED;
            *seq = COMPETITION_PERIOD_SHOW_NOT_STARTED;
            return;
        }
        if (valid && startDay <= today && today <= endDay) {
            link->subTaskResult = WBM_PERIOD_RESULT_ACTIVE;
            *seq = COMPETITION_PERIOD_END;
            return;
        }
        link->subTaskResult = WBM_PERIOD_RESULT_EXPIRED;
        *seq = COMPETITION_PERIOD_SHOW_ENDED;
        break;
    case COMPETITION_PERIOD_SHOW_NOT_STARTED:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CompetitionYouveRegisteredNot,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_PERIOD_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_PERIOD_END);
        break;
    case COMPETITION_PERIOD_SHOW_ENDED:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CompetitionTimeHasRun,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_PERIOD_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_PERIOD_CHECK_REGISTRATION);
        break;
    case COMPETITION_PERIOD_CHECK_REGISTRATION:
        accountState = param->account->registrationState;
        if (accountState == WBM_REGISTRATION_TEAM_REQUIRED || accountState == WBM_REGISTRATION_COMPLETE) {
            *seq = COMPETITION_PERIOD_SHOW_SAVE_MESSAGE;
        } else {
            *seq = COMPETITION_PERIOD_END;
        }
        break;
    case COMPETITION_PERIOD_SHOW_SAVE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = COMPETITION_PERIOD_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_PERIOD_UNLOCK_TEAM_AND_SAVE);
        break;
    case COMPETITION_PERIOD_UNLOCK_TEAM_AND_SAVE: {
        BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(param->param->gameData));

        link->hadBattleBox = BattleBoxSave_CheckFlags(battleBox, 1);
        BattleBoxSave_ClearFlags(battleBox, 1);
        TournamentEntry_SetField(link->entry, TOURNAMENT_FIELD_UNK15B, WBM_REGISTRATION_PERIOD_ENDED);
        GameData_StartSave(param->param->gameData);
        *seq = COMPETITION_PERIOD_WAIT_SAVE;
        break;
    }
    case COMPETITION_PERIOD_WAIT_SAVE:
        if (GameData_UpdateSave(param->param->gameData) == SAVE_RESULT_OK) {
            *seq = COMPETITION_PERIOD_CHECK_TEAM_WAS_LOCKED;
        }
        break;
    case COMPETITION_PERIOD_CHECK_TEAM_WAS_LOCKED:
        getBattleBox(GameData_GetSaveControl(param->param->gameData));
        if (link->hadBattleBox != 0) {
            *seq = COMPETITION_PERIOD_SHOW_TEAM_UNLOCKED;
        } else {
            *seq = COMPETITION_PERIOD_SHOW_UPLOAD_MESSAGE;
        }
        break;
    case COMPETITION_PERIOD_SHOW_TEAM_UNLOCKED:
        WifiBattleMatchLink_CompetitionSetCardPage(link, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleBoxHasBeen,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_PERIOD_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_PERIOD_SHOW_UPLOAD_MESSAGE);
        break;
    case COMPETITION_PERIOD_SHOW_UPLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = COMPETITION_PERIOD_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_PERIOD_POST_PERIOD_ENDED);
        break;
    case COMPETITION_PERIOD_POST_PERIOD_ENDED:
        MI_CpuFill8(&request, 0, sizeof(request));
        request.registrationState = WBM_REGISTRATION_PERIOD_ENDED;
        request.profileId = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->registrationState = WBM_REGISTRATION_PERIOD_ENDED;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = COMPETITION_PERIOD_WAIT_PERIOD_POST;
        break;
    case COMPETITION_PERIOD_WAIT_PERIOD_POST:
        ret = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ret == WBM_NET_SUCCESS) {
            *seq = COMPETITION_PERIOD_END;
        } else if (ret == WBM_NET_SERVER_ERROR) {
            link->subTaskResult = WBM_PERIOD_RESULT_SERVER_ERROR;
            *seq = COMPETITION_PERIOD_END;
        }
        break;
    case COMPETITION_PERIOD_END:
        WifiBattleMatchTask_End(task);
        break;
    case COMPETITION_PERIOD_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskUnregister(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchLinkParam *param = link->param;
    WifiBattleMatchLinkRequest request;
    u8 accountState;
    u32 ret;
    s32 selected;

    switch (*seq) {
    case COMPETITION_UNREGISTER_CHECK_REGISTRATION:
        accountState = param->account->registrationState;
        if (accountState == WBM_REGISTRATION_TEAM_REQUIRED || accountState == WBM_REGISTRATION_COMPLETE) {
            link->subTaskResult = WBM_UNREGISTER_RESULT_NOT_CHANGED;
            *seq = COMPETITION_UNREGISTER_PROMPT_UNREGISTER;
        } else {
            link->subTaskResult = WBM_UNREGISTER_RESULT_NOT_REGISTERED;
            *seq = COMPETITION_UNREGISTER_SHOW_NOT_REGISTERED;
        }
        break;
    case COMPETITION_UNREGISTER_SHOW_NOT_REGISTERED:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_CurrentlyHaveRegisteredNo,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_UNREGISTER_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_UNREGISTER_END);
        break;
    case COMPETITION_UNREGISTER_PROMPT_UNREGISTER:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_OnceUnregisteredWillNot,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_UNREGISTER_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_UNREGISTER_CREATE_UNREGISTER_MENU);
        break;
    case COMPETITION_UNREGISTER_CREATE_UNREGISTER_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO_NO_FIRST);
        *seq = COMPETITION_UNREGISTER_WAIT_UNREGISTER_MENU;
        break;
    case COMPETITION_UNREGISTER_WAIT_UNREGISTER_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_UNREGISTER_PROMPT_CONFIRM_UNREGISTER;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_UNREGISTER_END;
            break;
        }
        break;
    case COMPETITION_UNREGISTER_PROMPT_CONFIRM_UNREGISTER:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_WillUnregisteredSureAbout,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_UNREGISTER_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_UNREGISTER_CREATE_CONFIRM_MENU);
        break;
    case COMPETITION_UNREGISTER_CREATE_CONFIRM_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO_NO_FIRST);
        *seq = COMPETITION_UNREGISTER_WAIT_CONFIRM_MENU;
        break;
    case COMPETITION_UNREGISTER_WAIT_CONFIRM_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            *seq = COMPETITION_UNREGISTER_SHOW_SAVE_MESSAGE;
            break;
        case WBM_MENU_NO:
            *seq = COMPETITION_UNREGISTER_END;
            break;
        }
        break;
    case COMPETITION_UNREGISTER_SHOW_SAVE_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SavingDontTurnOff,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = COMPETITION_UNREGISTER_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_UNREGISTER_UNLOCK_TEAM_AND_SAVE);
        break;
    case COMPETITION_UNREGISTER_UNLOCK_TEAM_AND_SAVE:
        BattleBoxSave_ClearFlags(getBattleBox(GameData_GetSaveControl(param->param->gameData)), 1);
        TournamentEntry_SetField(link->entry, TOURNAMENT_FIELD_UNK15B, WBM_REGISTRATION_WITHDRAWN);
        GameData_StartSave(param->param->gameData);
        *seq = COMPETITION_UNREGISTER_WAIT_SAVE;
        break;
    case COMPETITION_UNREGISTER_WAIT_SAVE:
        ret = GameData_UpdateSave(param->param->gameData);
        if (ret == SAVE_RESULT_OK) {
            link->subTaskResult = WBM_UNREGISTER_RESULT_DONE;
            *seq = COMPETITION_UNREGISTER_SHOW_TEAM_UNLOCKED;
        } else if (ret == SAVE_RESULT_NG) {
            *seq = COMPETITION_UNREGISTER_END;
        }
        break;
    case COMPETITION_UNREGISTER_SHOW_TEAM_UNLOCKED:
        WifiBattleMatchLink_CompetitionSetCardPage(link, WIFIBATTLEMATCH_CARD_PAGE_UNREGISTERED);
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_BattleBoxHasBeen,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        *seq = COMPETITION_UNREGISTER_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_UNREGISTER_SHOW_UPLOAD_MESSAGE);
        break;
    case COMPETITION_UNREGISTER_SHOW_UPLOAD_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SendingDataDontTurn_2,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        *seq = COMPETITION_UNREGISTER_WAIT_MESSAGE;
        WifiBattleMatchTask_SetReturnSeq(task, COMPETITION_UNREGISTER_POST_UNREGISTER);
        break;
    case COMPETITION_UNREGISTER_POST_UNREGISTER:
        MI_CpuFill8(&request, 0, sizeof(request));
        request.registrationState = WBM_REGISTRATION_WITHDRAWN;
        request.profileId = WifiBattleMatchNet_GetGSID(link->net);
        link->param->account->registrationState = WBM_REGISTRATION_WITHDRAWN;
        WifiBattleMatchNet_StartPostRequest(link->net, &request, HEAPID_WIFIBATTLEMATCH_LINK);
        *seq = COMPETITION_UNREGISTER_WAIT_UNREGISTER_POST;
        break;
    case COMPETITION_UNREGISTER_WAIT_UNREGISTER_POST:
        ret = WifiBattleMatchNet_PostRequestMain(link->net);
        if (ret == WBM_NET_SUCCESS) {
            *seq = COMPETITION_UNREGISTER_END;
        } else if (ret == WBM_NET_SERVER_ERROR) {
            link->subTaskResult = WBM_UNREGISTER_RESULT_SERVER_ERROR;
            *seq = COMPETITION_UNREGISTER_END;
        }
        break;
    case COMPETITION_UNREGISTER_END:
        WifiBattleMatchTask_End(task);
        break;
    case COMPETITION_UNREGISTER_WAIT_MESSAGE:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            WifiBattleMatchTask_ReturnToSeq(task);
        }
        break;
    }
}

static void WifiBattleMatchLink_CompetitionTaskSignTeam(WifiBattleMatchTask *task, u32 *seq, void *work) {
    WifiBattleMatchLinkCompetition *link = work;
    WifiBattleMatchPlayer *player = link->param->player;
    u32 ret;

    switch (*seq) {
    case COMPETITION_TEAM_CHECK_DELAY:
        *seq = COMPETITION_TEAM_CHECK_START_CHECK;
        break;
    case COMPETITION_TEAM_CHECK_START_CHECK:
        MI_CpuFill8(&link->signature, 0, sizeof(link->signature));
        WifiBattleMatchNet_StartPokemonCheck(link->net, player->partyData, WBM_POKEMON_CHECK_PARTY);
        *seq = COMPETITION_TEAM_CHECK_WAIT_CHECK;
        break;
    case COMPETITION_TEAM_CHECK_WAIT_CHECK:
        ret = WifiBattleMatchNet_PokemonCheckMain(link->net, &link->signature);
        if (ret == WBM_NET_SUCCESS) {
            *seq = COMPETITION_TEAM_CHECK_COPY_SIGNATURE;
        } else if (ret == WBM_NET_FAILED) {
            link->subTaskResult = WBM_TEAM_CHECK_RESULT_REQUEST_FAILED;
            *seq = COMPETITION_TEAM_CHECK_END;
        } else if (ret == WBM_NET_SERVER_ERROR) {
            link->subTaskResult = WBM_TEAM_CHECK_RESULT_SERVER_ERROR;
            *seq = COMPETITION_TEAM_CHECK_END;
        }
        break;
    case COMPETITION_TEAM_CHECK_COPY_SIGNATURE:
        if (link->signature.status == WBM_POKEMON_VALIDATION_OK) {
            WifiBattleMatchLink_CompetitionCopySignature(player, &link->signature);
            link->subTaskResult = WBM_TEAM_CHECK_RESULT_VALID;
            *seq = COMPETITION_TEAM_CHECK_END;
        } else {
            link->subTaskResult = WBM_TEAM_CHECK_RESULT_INVALID;
            *seq = COMPETITION_TEAM_CHECK_END;
        }
        break;
    case COMPETITION_TEAM_CHECK_END:
        WifiBattleMatchTask_End(task);
        break;
    }
}

static void WifiBattleMatchLink_CompetitionCreateCard(WifiBattleMatchLinkCompetition *link) {
    WifiBattleMatchWifiCardInfo info;
    PlayerInfo *playerInfo;
    ClActUnit *unit;
    TournamentTeam *previous;
    TournamentEntry *entry;
    u32 wins;

    if (link->card != NULL) {
        return;
    }
    entry = link->entry;
    playerInfo = GetGameDataPlayerInfo(link->param->param->gameData);
    unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
    previous =
        TournamentBlock_GetTeam(getTournamentBlockAddress(GameData_GetSaveControl(link->param->param->gameData)), 0);
    MI_CpuFill8(&info, 0, sizeof(info));
    MI_CpuCopy8(TournamentEntry_GetCardData(entry), &info, 0x92);
    info.startDate = WifiBattleMatchLink_PackBytes(TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_YEAR),
                                                   TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_MONTH),
                                                   TournamentEntry_GetField(entry, TOURNAMENT_FIELD_START_DAY));
    info.endDate = WifiBattleMatchLink_PackBytes(TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_YEAR),
                                                 TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_MONTH),
                                                 TournamentEntry_GetField(entry, TOURNAMENT_FIELD_END_DAY));
    info.look = PlayerInfo_GetUnionView(playerInfo);
    info.rating = RndMatchData_GetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_RATING);
    wins = RndMatchData_GetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_WIN);
    info.matchCount =
        wins + RndMatchData_GetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_LOSE);
    info.variant = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_VARIANT);
    if (info.matchCount == 0 && info.rating == 0) {
        info.rating = 1500;
    }
    link->card =
        WifiBattleMatchWifiCard_Create(&info, 0, playerInfo, unit, link->res, link->font, link->printQueue,
                                       link->msgData, link->wordSet, previous, FALSE, HEAPID_WIFIBATTLEMATCH_LINK);
}

static void WifiBattleMatchLink_CompetitionDeleteCard(WifiBattleMatchLinkCompetition *link) {
    if (link->card != NULL) {
        WifiBattleMatchWifiCard_Free(link->card);
        link->card = NULL;
    }
}

static BOOL WifiBattleMatchLink_CompetitionIsCardShown(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchCard_SlideIn(link->card);
}

static BOOL WifiBattleMatchLink_CompetitionIsCardHidden(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchCard_SlideOut(link->card);
}

static void WifiBattleMatchLink_CompetitionShowCard(WifiBattleMatchLinkCompetition *link) {
    WifiBattleMatchLink_CompetitionCreateCard(link);
    while (!WifiBattleMatchLink_CompetitionIsCardShown(link)) {
    }
}

static void WifiBattleMatchLink_CompetitionSetCardPage(WifiBattleMatchLinkCompetition *link, u32 page) {
    if (link->card != NULL) {
        WifiBattleMatchWifiCard_ShowPage(link->card, page, link->msgData, link->printQueue, link->font,
                                         HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_CompetitionCreatePartnerCard(WifiBattleMatchLinkCompetition *link,
                                                             WifiBattleMatchPlayer *partner) {
    ClActUnit *unit;

    if (link->partnerCard == NULL) {
        unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
        link->partnerCard = WifiBattleMatchPartnerCard_Create(
            partner, unit, link->res, link->font, link->printQueue, link->msgData, link->wordSet,
            WIFIBATTLEMATCH_PARTNER_CARD_WIFI_COMPETITION, 1,
            TournamentEntry_GetField(link->entry, TOURNAMENT_FIELD_VARIANT), HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_CompetitionDeletePartnerCard(WifiBattleMatchLinkCompetition *link) {
    if (link->partnerCard != NULL) {
        WifiBattleMatchPartnerCard_Free(link->partnerCard);
        link->partnerCard = NULL;
    }
}

static BOOL WifiBattleMatchLink_CompetitionIsPartnerCardDone(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchPartnerCard_SlideIn(link->partnerCard);
}

static void WifiBattleMatchLink_CompetitionCreateTeamList(WifiBattleMatchLinkCompetition *link) {
    ClActUnit *unit;

    if (link->teamList == NULL) {
        unit = WifiBattleMatchGraphic_GetClActUnit(link->graphic);
        link->teamList = WifiBattleMatchTeamList_Create(
            getBattleBox(GameData_GetSaveControl(link->param->param->gameData)), unit, link->res, link->smallFont,
            link->printQueue, link->msgData, HEAPID_WIFIBATTLEMATCH_LINK);
    }
}

static void WifiBattleMatchLink_CompetitionDeleteTeamList(WifiBattleMatchLinkCompetition *link) {
    if (link->teamList != NULL) {
        WifiBattleMatchTeamList_Free(link->teamList);
        link->teamList = NULL;
    }
}

static BOOL WifiBattleMatchLink_CompetitionIsTeamListShown(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchTeamList_SlideIn(link->teamList);
}

static BOOL WifiBattleMatchLink_CompetitionIsTeamListHidden(WifiBattleMatchLinkCompetition *link) {
    return WifiBattleMatchTeamList_SlideOut(link->teamList);
}

static void WifiBattleMatchLink_CompetitionCreateMenu(WifiBattleMatchLinkCompetition *link, u32 kind) {
    WifiBattleMatchMenuParam menuParam;

    if (link->menu != NULL) {
        return;
    }
    MI_CpuFill8(&menuParam, 0, sizeof(menuParam));
    menuParam.msgData = link->msgData;
    menuParam.font = link->font;
    menuParam.printQueue = link->printQueue;
    menuParam.bg = 0;
    menuParam.palette = 15;
    menuParam.framePalette = 13;
    menuParam.frameChar = 1;
    switch (kind) {
    case COMPETITION_MENU_YES_NO_NO_FIRST:
        menuParam.cursorRow = 1;
        // fall through
    case COMPETITION_MENU_YES_NO:
        menuParam.msgIds[0] = EventFishing_Text_Yes_2;
        menuParam.msgIds[1] = EventFishing_Text_No_2;
        menuParam.count = 2;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 1;
        link->menu = WifiBattleMatchMenu_CreateBottomRight(&menuParam, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    case COMPETITION_MENU_JOIN_INFO_CANCEL:
        menuParam.msgIds[0] = EventFishing_Text_Join;
        menuParam.msgIds[1] = EventFishing_Text_Info;
        menuParam.msgIds[2] = EventFishing_Text_Cancel;
        menuParam.count = 3;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 2;
        link->menu = WifiBattleMatchMenu_CreateBottomRight(&menuParam, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    case COMPETITION_MENU_REGISTER:
        menuParam.msgIds[0] = EventFishing_Text_Yes_3;
        menuParam.msgIds[1] = EventFishing_Text_No_3;
        menuParam.count = 2;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 1;
        link->menu = WifiBattleMatchMenu_CreateBottomRight(&menuParam, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    case COMPETITION_MENU_BATTLE:
        menuParam.msgIds[0] = EventFishing_Text_Battle;
        menuParam.msgIds[1] = EventFishing_Text_CancelParticipation;
        menuParam.msgIds[2] = EventFishing_Text_Cancel_2;
        menuParam.count = 3;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 2;
        link->menu = WifiBattleMatchMenu_Create(&menuParam, 3, 6, 26, 6, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    case COMPETITION_MENU_BATTLE_AGAIN:
        menuParam.msgIds[0] = EventFishing_Text_Battle_2;
        menuParam.msgIds[1] = EventFishing_Text_CancelParticipation_2;
        menuParam.msgIds[2] = EventFishing_Text_Cancel_3;
        menuParam.count = 3;
        menuParam.canCancel = TRUE;
        menuParam.cancelValue = 2;
        link->menu = WifiBattleMatchMenu_Create(&menuParam, 3, 6, 26, 6, HEAPID_WIFIBATTLEMATCH_LINK);
        break;
    }
}

static void WifiBattleMatchLink_CompetitionDeleteMenu(WifiBattleMatchLinkCompetition *link) {
    if (link->menu != NULL) {
        WifiBattleMatchMenu_Free(link->menu);
        link->menu = NULL;
    }
}

static s32 WifiBattleMatchLink_CompetitionGetMenuSelection(WifiBattleMatchLinkCompetition *link) {
    if (link->menu != NULL) {
        return WifiBattleMatchMenu_Update(link->menu);
    }
    return -1;
}

static void WifiBattleMatchLink_CompetitionStartSubTask(WifiBattleMatchLinkCompetition *link,
                                                        WifiBattleMatchTaskFunc func) {
    WifiBattleMatchTask_SetFunc(link->subTask, func);
    link->subTaskResult = SUB_TASK_RUNNING;
}

static u32 WifiBattleMatchLink_CompetitionUpdateSubTask(WifiBattleMatchLinkCompetition *link) {
    WifiBattleMatchTask_Run(link->subTask);
    if (WifiBattleMatchTask_IsEnded(link->subTask)) {
        return link->subTaskResult;
    }
    return SUB_TASK_RUNNING;
}

static void WifiBattleMatchLink_CompetitionFillBattleBoxParty(WifiBattleMatchPlayer *player,
                                                              WifiBattleMatchLinkCompetition *link) {
    PokeParty *party;

    MI_CpuFill8(player, 0, sizeof(WifiBattleMatchPlayer));
    party = convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(link->param->param->gameData)),
                                       HEAPID_WIFIBATTLEMATCH_LINK);
    PokeParty_Init((PokeParty *)player->partyData);
    MI_CpuCopy8(party, player->partyData, PokeParty_GetSaveDataSize());
    GFL_HeapFree(party);
    player->battleRecordVersion = 100;
}

static void WifiBattleMatchLink_CompetitionFillPlayer(WifiBattleMatchPlayer *player,
                                                      WifiBattleMatchLinkCompetition *link) {
    SaveControl *save = GameData_GetSaveControl(link->param->param->gameData);
    WifiBattleMatchLinkParam *param;
    TournamentEntry *entry;

    PlayerInfo_Copy(GetGameDataPlayerInfo(link->param->param->gameData), &player->info);
    CGearPhrases_Get(CGearPhrases_GetBlock(save), 0, &player->sentence);
    param = link->param;
    player->record1 = param->ranking->wins;
    player->record2 = param->ranking->losses;
    player->rating = param->ranking->rating;
    player->recordId = WifiBattleMatchNet_GetRecordId(link->net);
    player->gsid = WifiList_GetMyGSID(GameData_GetWifiList(link->param->param->gameData));
    entry = link->entry;
    player->entryField2 = TournamentEntry_GetField(entry, TOURNAMENT_FIELD_CUP_ID);
    player->checksum = CRC16_350bytes(entry);
}

static void WifiBattleMatchLink_CompetitionCopySignature(WifiBattleMatchPlayer *player,
                                                         WifiBattleMatchNetCheck *signature) {
    MI_CpuCopy8(signature->signature, player->signature, sizeof(player->signature));
}

static BOOL WifiBattleMatchLink_CompetitionCheckSignature(WifiBattleMatchPlayer *player, PokeParty *party,
                                                          HeapID heapId) {
    void *array;
    int i;
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

static BOOL WifiBattleMatchLink_CompetitionCompareParties(PokeParty *party, PokeParty *otherParty) {
    u32 count = PokeParty_GetPkmCount(party);
    u32 i;

    if (count == PokeParty_GetPkmCount(otherParty)) {
        for (i = 0; i < count; i++) {
            PartyPkm *pkm = PokeParty_GetPkm(party, i);
            PartyPkm *otherPkm = PokeParty_GetPkm(otherParty, i);

            if (GFL_STD_MemCmp(PokeParty_GetBoxPkm(pkm), PokeParty_GetBoxPkm(otherPkm), PML_GetPkmRawSize()) != 0) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

static u32 WifiBattleMatchLink_CompetitionUpdateCancel(WifiBattleMatchLinkCompetition *link, BOOL canCancel,
                                                       BOOL canDisconnect) {
    s32 selected;

    switch (link->cancelSeq) {
    case COMPETITION_SEARCH_CANCEL_IDLE:
        if (canCancel && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            link->cancelSeq++;
        }
        break;
    case COMPETITION_SEARCH_CANCEL_SHOW_QUIT_PROMPT:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_QuitSearchingSomeoneBattle,
                                    WIFIBATTLEMATCH_MSG_STREAM_KEY);
        link->cancelSeq++;
        break;
    case COMPETITION_SEARCH_CANCEL_WAIT_QUIT_PROMPT:
        if (WifiBattleMatchMsgBox_IsDone(link->msgBox)) {
            link->cancelSeq++;
        }
        break;
    case COMPETITION_SEARCH_CANCEL_CREATE_MENU:
        WifiBattleMatchLink_CompetitionCreateMenu(link, COMPETITION_MENU_YES_NO);
        link->cancelSeq++;
        break;
    case COMPETITION_SEARCH_CANCEL_WAIT_MENU:
        selected = WifiBattleMatchLink_CompetitionGetMenuSelection(link);
        if (selected == WBM_MENU_NO_SELECTION) {
            break;
        }
        WifiBattleMatchLink_CompetitionDeleteMenu(link);
        switch (selected) {
        case WBM_MENU_YES:
            link->cancelSeq = COMPETITION_SEARCH_CANCEL_SHOW_DISCONNECT_MESSAGE;
            break;
        case WBM_MENU_NO:
            WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_SearchingOpponent,
                                        WIFIBATTLEMATCH_MSG_QUEUE);
            link->cancelSeq = COMPETITION_SEARCH_CANCEL_IDLE;
            return CANCEL_DECLINED;
            break;
        }
        break;
    case COMPETITION_SEARCH_CANCEL_SHOW_DISCONNECT_MESSAGE:
        WifiBattleMatchMsgBox_Print(link->msgBox, link->msgData, EventFishing_Text_AwaitingCommunicationPleaseStand,
                                    WIFIBATTLEMATCH_MSG_QUEUE_WAIT_ICON);
        link->cancelSeq++;
        break;
    case COMPETITION_SEARCH_CANCEL_DISCONNECT:
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
    case COMPETITION_SEARCH_CANCEL_CONFIRMED:
        link->cancelSeq = COMPETITION_SEARCH_CANCEL_IDLE;
        return CANCEL_CONFIRMED;
    }
    return link->cancelSeq == COMPETITION_SEARCH_CANCEL_IDLE ? CANCEL_NONE : CANCEL_BUSY;
}

static void WifiBattleMatchLink_CompetitionResetCancel(WifiBattleMatchLinkCompetition *link) {
    link->cancelSeq = COMPETITION_SEARCH_CANCEL_IDLE;
    WifiBattleMatchLink_CompetitionDeleteMenu(link);
}

static BOOL WifiBattleMatchLink_CompetitionUpdateRecord(WifiBattleMatchLinkCompetition *link,
                                                        WifiBattleMatchLinkRanking *ranking) {
    BOOL changed = FALSE;

    if (ranking->rating !=
            RndMatchData_GetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_RATING) ||
        ranking->wins != RndMatchData_GetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_WIN) ||
        ranking->losses !=
            RndMatchData_GetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_LOSE)) {
        changed = TRUE;
    }
    RndMatchData_SetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_RATING, ranking->rating);
    RndMatchData_SetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_WIN, ranking->wins);
    RndMatchData_SetParam(link->param->record, RNDMATCH_RECORD_COMPETITION, RNDMATCH_FIELD_LOSE, ranking->losses);
    return changed;
}
