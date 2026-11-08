#include "types.h"
#include "app/wifi_bsubway.h"
#include "battle/battle_rec_tool.h"
#include "battle/btl_result.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "constants/pokemon.h"
#include "constants/version.h"
#include "constants/zones.h"
#include "field/bsubway_scr.h"
#include "field/comm_entry_menu.h"
#include "field/event_wifi_bsubway.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_gimmick_bsubway.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/player_state.h"
#include "field/scrcmd_bsubway.h"
#include "field/scrcmd_msg.h"
#include "field/zone.h"
#include "gfl/backup_system.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_system.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/battle_rec.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/event_work.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/iss_switch_sys.h"
#include "system/iss_sys.h"
#include "system/vm.h"

// The script plugin of the Battle Subway (plugin 1), commands from 1000. Command 1003 does most of the work, with its
// own command IDs: from 300 they need the subway's work

enum {
    BSUBWAY_TOOL_GET_ZONE_MODE = 0,
    BSUBWAY_TOOL_RESET_SYSTEM = 1,
    BSUBWAY_TOOL_CLEAR_SAVED_PLAY = 2,
    BSUBWAY_TOOL_GET_PLAY_SAVED = 3,
    BSUBWAY_TOOL_SET_RETURN_WARP = 4,
    BSUBWAY_TOOL_CLEAR_ON_TRAIN = 5,
    BSUBWAY_TOOL_GET_MODE_STREAK = 6,
    BSUBWAY_TOOL_RESET_SAVED_PLAY = 7,
    BSUBWAY_TOOL_GET_SAVED_CAR_NUMBER = 8,
    BSUBWAY_TOOL_GET_CURRENT_CAR_NUMBER = 11,
    BSUBWAY_TOOL_GET_PLAYER_OBJ_CODE = 12,
    BSUBWAY_TOOL_SET_ACTOR_HIDDEN = 13,
    BSUBWAY_TOOL_GET_MODE_CLEARED = 14,
    BSUBWAY_TOOL_SET_MODE_CLEARED = 15,
    BSUBWAY_TOOL_GET_PARTNER_INTRO_SEEN = 16,
    BSUBWAY_TOOL_SET_PARTNER_INTRO_SEEN = 17,
    BSUBWAY_TOOL_START_TRAIN_EFFECT = 18,
    BSUBWAY_TOOL_SET_TRAIN_ANIMATION = 19,
    BSUBWAY_TOOL_SET_TRAIN_HIDDEN = 20,
    BSUBWAY_TOOL_GET_SAVED_MODE = 21,
    BSUBWAY_TOOL_IS_MULTI_MODE = 22,
    BSUBWAY_TOOL_SET_ACTOR_HEIGHT = 23,
    BSUBWAY_TOOL_ENABLE_ACTOR_HEIGHT_CALC = 24,
    BSUBWAY_TOOL_HAS_ACTOR = 25,
    BSUBWAY_TOOL_GET_PASSENGER_MESSAGE = 26,
    BSUBWAY_TOOL_GET_WIFI_RANK = 27,
    BSUBWAY_TOOL_PROMOTE_WIFI_RANK = 28,
    BSUBWAY_TOOL_CHECK_WIFI_DEMOTION = 29,
    BSUBWAY_TOOL_GET_MODE_STAGE = 30,
    BSUBWAY_TOOL_GET_PARTNER_OBJ_CODE = 31,
    BSUBWAY_TOOL_HAS_BATTLE_BOX = 32,
    BSUBWAY_TOOL_START_FIXED_TRAIN_EFFECT = 34,
    BSUBWAY_TOOL_SET_ACTOR_FACING = 35,
    BSUBWAY_TOOL_DISABLE_ACTOR_MOVEMENT = 36,
    BSUBWAY_TOOL_SHOW_WIFI_LEADER_MESSAGE = 37,
    BSUBWAY_TOOL_HAS_MODE_STAGE = 38,
    BSUBWAY_TOOL_HAS_PASSENGER_GIFT = 39,
    BSUBWAY_TOOL_GET_PASSENGER_GIFT_PROMPT = 40,
    BSUBWAY_TOOL_GET_PASSENGER_GIFT_MESSAGE = 41,
    BSUBWAY_TOOL_GET_PASSENGER_GIFT_ITEM = 42,
    BSUBWAY_TOOL_GET_WIFI_LEADER_COUNT = 43,
    BSUBWAY_TOOL_OPEN_YES_NO_MENU = 44,
    BSUBWAY_TOOL_WARP_TO_ZONE_MODE = 45,
    BSUBWAY_TOOL_SET_WIFI_RESULTS_PENDING = 100,
    BSUBWAY_TOOL_GET_WIFI_RESULTS_PENDING = 101,
    BSUBWAY_TOOL_UPLOAD_WIFI_RESULTS = 103,
    BSUBWAY_TOOL_DOWNLOAD_WIFI_TRAIN = 104,
    BSUBWAY_TOOL_DOWNLOAD_WIFI_LEADERS = 105,
    BSUBWAY_TOOL_GET_DOWNLOADED_TRAIN_RANK = 106,
    BSUBWAY_TOOL_HAS_WIFI_TRAIN = 107,
    BSUBWAY_TOOL_SET_WIFI_RANKED_CHALLENGE = 108,
    BSUBWAY_TOOL_GET_WIFI_RANKED_CHALLENGE = 109,
    BSUBWAY_TOOL_HAS_WIFI_LEADERS = 110,
    BSUBWAY_TOOL_SHOW_RECORDS = 111,
    BSUBWAY_TOOL_GET_WIFI_LEADER_RANK = 112,
    BSUBWAY_TOOL_GET_WIFI_LEADER_ROOM = 113,
    BSUBWAY_TOOL_SELECT_MEMBERS = 300,
    BSUBWAY_TOOL_SET_SELECTED_MEMBERS = 301,
    BSUBWAY_TOOL_IS_WIN_MAX = 303,
    BSUBWAY_TOOL_SET_LOSS_SCORE = 304,
    BSUBWAY_TOOL_SET_CLEAR_SCORE = 305,
    BSUBWAY_TOOL_SAVE_REST_PLAY = 306,
    BSUBWAY_TOOL_CREATE_ROUND_TRAINERS = 307,
    BSUBWAY_TOOL_GET_TRAINER_OBJ_CODE = 308,
    BSUBWAY_TOOL_START_BATTLE = 309,
    BSUBWAY_TOOL_GET_CURRENT_MODE = 310,
    BSUBWAY_TOOL_SET_FIXED_REWARD = 311,
    BSUBWAY_TOOL_GET_FIXED_REWARD = 312,
    BSUBWAY_TOOL_ADD_BATTLE_POINTS = 313,
    BSUBWAY_TOOL_SET_PARTNER_INDEX = 314,
    BSUBWAY_TOOL_GET_PARTNER_INDEX = 315,
    BSUBWAY_TOOL_DRAW_TRAINERS = 316,
    BSUBWAY_TOOL_GET_MEMBER_SLOT = 317,
    BSUBWAY_TOOL_CLEAR_RECEIVE_BUFFER = 319,
    BSUBWAY_TOOL_SHOW_TRAINER_MESSAGE_TOP_RIGHT = 320,
    BSUBWAY_TOOL_SAVE_MEMBERS = 321,
    BSUBWAY_TOOL_WARP_TO_SAVED_MODE = 322,
    BSUBWAY_TOOL_LOAD_MEMBERS = 323,
    BSUBWAY_TOOL_SET_USE_BATTLE_BOX = 324,
    BSUBWAY_TOOL_CHANGE_COMM_MULTI_MODE = 325,
    BSUBWAY_TOOL_IS_SUPER_MODE = 326,
    BSUBWAY_TOOL_PLACE_PASSENGERS = 329,
    BSUBWAY_TOOL_SET_RETURN_SYNC_REQUIRED = 330,
    BSUBWAY_TOOL_GET_RETURN_SYNC_REQUIRED = 331,
    BSUBWAY_TOOL_IS_BATTLE_WON = 332,
    BSUBWAY_TOOL_CLEAR_BATTLE_STATE = 333,
    BSUBWAY_TOOL_SHOW_CAR_SWITCHES = 334,
    BSUBWAY_TOOL_CREATE_PARTNERS = 335,
    BSUBWAY_TOOL_GET_MEMBER_SPECIES = 336,
    BSUBWAY_TOOL_SET_USE_EXISTING_NET = 337,
    BSUBWAY_TOOL_GET_USE_EXISTING_NET = 338,
    BSUBWAY_TOOL_CAN_PARTY_ENTER = 339,
    BSUBWAY_TOOL_LOAD_BATTLE_BOX_COPY = 340,
    BSUBWAY_TOOL_GET_REGULATION = 341,
    BSUBWAY_TOOL_GET_MEMBER_COUNT = 342,
    BSUBWAY_TOOL_CHECK_VIDEO_SLOT = 343,
    BSUBWAY_TOOL_HAS_VIDEO_SLOT = 344,
    BSUBWAY_TOOL_STORE_BATTLE_VIDEO = 345,
    BSUBWAY_TOOL_SAVE_BATTLE_VIDEO = 346,
    BSUBWAY_TOOL_FREE_BATTLE_SETUP = 347,
    BSUBWAY_TOOL_SHOW_VIDEO_SAVE_SCREEN = 348,
    BSUBWAY_TOOL_RESET_MODE_STAGE = 349,
    BSUBWAY_TOOL_STOP_TRAIN_SHAKE = 350,
    BSUBWAY_TOOL_GET_CURRENT_TRAINER = 351,
    BSUBWAY_TOOL_SHOW_TRAINER_MESSAGE_BOTTOM_RIGHT = 352,
    BSUBWAY_TOOL_ADD_WIFI_BATTLE_RECORD = 353,
    BSUBWAY_TOOL_SET_VIDEO_SAVE_DISABLED = 354,
    BSUBWAY_TOOL_GET_VIDEO_SAVE_DISABLED = 355,
    BSUBWAY_TOOL_GET_USE_BATTLE_BOX = 356,
    BSUBWAY_TOOL_ADVANCE_ROUND = 357,
    BSUBWAY_TOOL_GET_WIN_COUNT = 358,
    BSUBWAY_TOOL_START_MODE_STAGE = 359,
    BSUBWAY_TOOL_START_COMM = 400,
    BSUBWAY_TOOL_END_COMM = 401,
    BSUBWAY_TOOL_WAIT_COMM_TIMING = 402,
    BSUBWAY_TOOL_OPEN_PARENT_ENTRY_MENU = 403,
    BSUBWAY_TOOL_OPEN_CHILD_ENTRY_MENU = 404,
    BSUBWAY_TOOL_SEND_COMM_DATA = 405,
    BSUBWAY_TOOL_WAIT_COMM_DATA = 406,
    BSUBWAY_TOOL_GET_NET_ID = 407,
    BSUBWAY_TOOL_START_IRC_MATCH = 408,
    BSUBWAY_TOOL_GET_IRC_MATCH_RESULT = 409,
    BSUBWAY_TOOL_ADD_MET_PARTNER = 410,
    BSUBWAY_TOOL_GET_NET_PLAYER_0_OBJ_CODE = 411,
    BSUBWAY_TOOL_GET_NET_PLAYER_1_OBJ_CODE = 412,
    BSUBWAY_TOOL_MARK_NET_ERROR_SHOWN = 413,
    BSUBWAY_TOOL_SET_NET_ERROR_PENDING = 414,
    BSUBWAY_TOOL_HAS_NET_ERROR = 415,
    BSUBWAY_TOOL_CAN_RECORD_BATTLE_VERSION = 416,
};

enum {
    BSUBWAY_RULE_SINGLE = 20,
    BSUBWAY_RULE_DOUBLE = 21,
    BSUBWAY_RULE_MULTI = 22,
};

enum {
    BSUBWAY_VIDEO_UNCHECKED = 0,
    BSUBWAY_VIDEO_EMPTY = 1,
    BSUBWAY_VIDEO_PRESENT = 2,
};

enum {
    BSUBWAY_RANK_GET = 0,
    BSUBWAY_RANK_PROMOTE = 3,
    BSUBWAY_RANK_CHECK_DEMOTION = 4,
};

enum {
    BSUBWAY_ENTRY_DONE = 0,
    BSUBWAY_ENTRY_CANCELLED = 1,
    BSUBWAY_ENTRY_ERROR = 2,
};

enum {
    BSUBWAY_IRC_MATCH_READY = 0,
    BSUBWAY_IRC_MATCH_QUIT = 1,
    BSUBWAY_IRC_MATCH_BACK = 2,
};

#define BSUBWAY_WORK_COMMAND_MIN 300
#define BSUBWAY_NO_MODE_CLEAR_FLAG 11
#define BSUBWAY_PASSENGER_END 0xffff

// Set while the player is on the subway
#define FLAG_BSUBWAY_ON_TRAIN 0x965
#define BSUBWAY_COUNT_MAX 9999

#define OVERLAY_EVENT_WIFI_BSUBWAY OVERLAY_ID(9)

typedef struct {
    s16 x;
    s16 unk2;
    s16 z;
    s16 unk6;
} BSubwayActorPos;

typedef struct {
    u16 objCode;
    u16 stage;
    s16 x;
    u16 unk6;
    s16 z;
    u16 messageId;
    u16 afterGiftMessageId;
    u16 itemId;
} BSubwayActor;

// The message window event of command 44
typedef struct {
    FieldScriptEnv *env;
    u16 *var;
    ListMenuUI *window;
    u32 cursorRow;
} BSubwayMsgEvent;

static const u16 sVideoModesByPlayMode[] = { BATTLE_REC_MODE_BATTLE_SUBWAY_SINGLE, BATTLE_REC_MODE_BATTLE_SUBWAY_DOUBLE,
                                             BATTLE_REC_MODE_BATTLE_SUBWAY_MULTI,  BATTLE_REC_MODE_BATTLE_SUBWAY_MULTI,
                                             BATTLE_REC_MODE_BATTLE_SUBWAY_SINGLE, BATTLE_REC_MODE_BATTLE_SUBWAY_SINGLE,
                                             BATTLE_REC_MODE_BATTLE_SUBWAY_DOUBLE, BATTLE_REC_MODE_BATTLE_SUBWAY_MULTI,
                                             BATTLE_REC_MODE_BATTLE_SUBWAY_MULTI };
static const u16 sModeClearFlags[] = { BSUBWAY_SCORE_FLAG_SINGLE_CLEARED,
                                       BSUBWAY_SCORE_FLAG_DOUBLE_CLEARED,
                                       BSUBWAY_SCORE_FLAG_MULTI_CLEARED,
                                       BSUBWAY_SCORE_FLAG_MULTI_CLEARED,
                                       BSUBWAY_NO_MODE_CLEAR_FLAG,
                                       BSUBWAY_SCORE_FLAG_SUPER_SINGLE_CLEARED,
                                       BSUBWAY_SCORE_FLAG_SUPER_DOUBLE_CLEARED,
                                       BSUBWAY_SCORE_FLAG_SUPER_MULTI_CLEARED,
                                       BSUBWAY_SCORE_FLAG_SUPER_MULTI_CLEARED };
static const u16 sMaleLeaderObjCodes[] = { 44, 11, 12, 48, 72, 28, 13, 71, 30, 32 };
static const u16 sFemaleLeaderObjCodes[] = { 33, 15, 16, 49, 53, 17, 31, 33, 45, 31 };
static const VecFx32 sTrainEffectPositions[] = {
    { 0x2d0000, -0x20000, 0xb6000 },
    { 0x2f0000, -0x30000, 0x97000 },
};
static const u32 sTrainModelsByPlayMode[] = { 0, 2, 4, 4, 6, 1, 3, 5, 5 };
static const BSubwayActorPos sActorPositions[] = {
    { 70, 0, 16, 0 }, { 61, 0, 14, 0 }, { 47, 0, 16, 0 }, { 37, 0, 14, 0 }, { 26, 0, 16, 0 },
};
// The zones to warp to for each play mode
static const u32 sWarpZones[] = { ZONE_GEAR_STATION_2, ZONE_GEAR_STATION_4, ZONE_GEAR_STATION_6,
                                  ZONE_GEAR_STATION_6, ZONE_GEAR_STATION_8, ZONE_GEAR_STATION_3,
                                  ZONE_GEAR_STATION_5, ZONE_GEAR_STATION_7, ZONE_GEAR_STATION_7 };
static const u32 sStages[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 24, 32, 46, 146 };
static const VecFx32 sWarpPositions[] = {
    { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 },
    { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 },
    { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 }, { 0xb8000, 0, 0xf8000 },
};
// The actors on each stage of the subway, until objCode 0xffff
static const BSubwayActor sActors[] = {
    { 11, 1, 71, 0, 17, 32, 0, 0 },     { 11, 4, 62, 0, 14, 33, 0, 0 },       { 15, 1, 66, 0, 14, 34, 0, 0 },
    { 15, 4, 50, 0, 16, 35, 0, 0 },     { 12, 1, 23, 0, 16, 36, 0, 0 },       { 12, 4, 41, 0, 14, 37, 0, 0 },
    { 16, 1, 35, 0, 16, 38, 0, 0 },     { 16, 4, 40, 0, 14, 39, 0, 0 },       { 48, 1, 61, 0, 13, 40, 0, 0 },
    { 48, 2, 31, 0, 14, 41, 0, 0 },     { 48, 4, 58, 0, 15, 42, 0, 0 },       { 48, 9, 22, 0, 13, 43, 0, 0 },
    { 48, 15, 42, 0, 17, 44, 0, 0 },    { 49, 2, 67, 0, 17, 45, 0, 0 },       { 49, 4, 12, 0, 16, 46, 0, 0 },
    { 49, 8, 38, 0, 14, 47, 0, 0 },     { 49, 12, 28, 0, 16, 48, 0, 0 },      { 72, 2, 44, 0, 15, 49, 0, 0 },
    { 72, 3, 19, 0, 16, 50, 0, 0 },     { 72, 5, 50, 0, 16, 51, 0, 0 },       { 72, 11, 68, 0, 14, 52, 0, 0 },
    { 28, 1, 43, 0, 14, 53, 0, 0 },     { 28, 2, 22, 0, 14, 54, 0, 0 },       { 28, 3, 58, 0, 15, 55, 0, 0 },
    { 28, 5, 61, 0, 16, 56, 0, 0 },     { 28, 6, 21, 0, 14, 57, 58, 51 },     { 28, 8, 20, 0, 16, 59, 0, 0 },
    { 28, 12, 67, 0, 14, 60, 0, 0 },    { 53, 3, 67, 0, 15, 61, 0, 0 },       { 53, 4, 82, 0, 17, 62, 0, 0 },
    { 53, 6, 60, 0, 16, 63, 0, 0 },     { 53, 11, 34, 0, 15, 64, 0, 0 },      { 68, 3, 45, 0, 14, 65, 0, 0 },
    { 68, 5, 12, 0, 15, 66, 0, 0 },     { 68, 6, 78, 0, 15, 67, 0, 0 },       { 68, 7, 19, 0, 16, 68, 0, 0 },
    { 68, 8, 62, 0, 14, 69, 0, 0 },     { 68, 15, 61, 0, 17, 70, 0, 0 },      { 68, 18, 67, 0, 15, 71, 0, 0 },
    { 13, 4, 32, 0, 15, 72, 0, 0 },     { 17, 1, 55, 0, 17, 73, 0, 0 },       { 71, 17, 59, 0, 15, 74, 0, 0 },
    { 30, 9, 69, 0, 17, 75, 0, 0 },     { 30, 10, 81, 0, 14, 76, 0, 0 },      { 30, 12, 49, 0, 14, 77, 0, 0 },
    { 30, 14, 63, 0, 16, 78, 0, 0 },    { 30, 21, 70, 0, 17, 79, 0, 0 },      { 31, 7, 67, 0, 17, 80, 81, 50 },
    { 31, 9, 45, 0, 15, 82, 0, 0 },     { 31, 11, 23, 0, 16, 83, 0, 0 },      { 31, 16, 17, 0, 16, 84, 0, 0 },
    { 31, 20, 45, 0, 15, 85, 86, 207 }, { 61, 1, 83, 0, 17, 87, 0, 0 },       { 61, 2, 50, 0, 16, 88, 0, 0 },
    { 61, 5, 43, 0, 13, 89, 0, 0 },     { 69, 3, 39, 0, 16, 90, 91, 51 },     { 69, 5, 35, 0, 16, 92, 0, 0 },
    { 69, 7, 58, 0, 15, 93, 0, 0 },     { 69, 22, 50, 0, 14, 94, 0, 0 },      { 32, 7, 39, 0, 14, 95, 0, 0 },
    { 32, 9, 62, 0, 16, 96, 0, 0 },     { 32, 11, 53, 0, 14, 97, 0, 0 },      { 32, 13, 59, 0, 15, 98, 0, 0 },
    { 32, 15, 55, 0, 14, 99, 0, 0 },    { 33, 8, 57, 0, 16, 100, 0, 0 },      { 33, 10, 39, 0, 13, 101, 0, 0 },
    { 33, 13, 60, 0, 15, 102, 0, 0 },   { 33, 16, 68, 0, 15, 103, 0, 0 },     { 33, 19, 64, 0, 14, 104, 0, 0 },
    { 45, 3, 25, 0, 14, 105, 0, 0 },    { 45, 5, 82, 0, 15, 106, 0, 0 },      { 45, 8, 50, 0, 15, 107, 0, 0 },
    { 44, 3, 26, 0, 14, 108, 0, 0 },    { 44, 6, 44, 0, 14, 109, 0, 0 },      { 44, 7, 48, 0, 16, 110, 0, 0 },
    { 44, 10, 58, 0, 16, 111, 0, 0 },   { 52, 6, 66, 0, 14, 112, 0, 0 },      { 52, 7, 55, 0, 13, 113, 0, 0 },
    { 52, 10, 20, 0, 16, 114, 0, 0 },   { 52, 18, 28, 0, 14, 115, 116, 206 }, { BSUBWAY_PASSENGER_END },
};

static BOOL ScriptNative_BSubwaySendWait(VM *vm, void *env);
static BOOL ScriptNative_BSubwayTimingWait(VM *vm, void *env);
static BOOL ScriptNative_BSubwayParentEntryMenuWait(VM *vm, void *env);
static BOOL ScriptNative_BSubwayChildEntryMenuWait(VM *vm, void *env);
static BOOL ScriptNative_BSubwayRecvWait(VM *vm, void *env);
static GameEvent *EventBSubwayCommEnd_Create(GameSystem *gsys, BSubwayScrWork *bsw);
static GameEvent *EventBSubwayMsgWindow_Create(GameSystem *gsys, FieldScriptEnv *env, u16 *var, u32 cursorRow);
static void BSubway_PlacePassengers(BSubwayScrWork *bsw, GameData *gameData, MMSys *mmSys, Field *field);
static u16 BSubway_GetPassengerMessageId(FieldActor *actor);
static BOOL BSubwayCmd_HasPassengerGift(BSubwayScrWork *bsw, u16 index);
static void BSubwayCmd_GetPassengerGiftInfo(BSubwayScrWork *bsw, u16 index, u16 *beforeMessage, u16 *afterMessage,
                                            u16 *giftItem);
static BOOL ScriptNative_BSubwayBattleRecSaveWait(VM *vm, void *env);
static u32 BSubway_GetModeRegulation(u32 mode);
static u32 BSubway_GetZonePlayMode(Field *field);
static u16 BSubway_GetPlayerObjCode(PlayerInfo *info);

BOOL Scrcmd_Bsubway_CreateWork(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    u16 resume = VM_Read16(vm);

    BsubwayScr_CreateWork(gsys, resume, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Bsubway_ClearWork(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);

    BsubwayScr_ClearWorkPtr(ScriptWork_GetGameSystem(work));
    return FALSE;
}

BOOL Scrcmd_Bsubway_FreeWork(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);

    BsubwayScr_DeleteWork(gsys, GameData_GetBSubwayScrWork(GSYS_GetGameData(gsys)));
    return FALSE;
}

BOOL Scrcmd_Bsubway_Tool(VM *vm, FieldScriptEnv *env) {
    PartyPkm *pkm;
    BOOL result = FALSE;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    GameData *gameData = GSYS_GetGameData(gsys);
    SaveControl *save = GameData_GetSaveControl(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BSubwayScrWork *bsw = GameData_GetBSubwayScrWork(gameData);
    BSubwayPlayData *playData = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY);
    u32 mode = BSubwayPlayData_GetData(playData, BSUBWAY_PLAY_MODE, NULL);
    BSubwayScoreData *score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    void *data3A = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_3A);
    Field *field = GSYS_GetField(gsys);
    u16 cmd = VM_Read16(vm);
    u16 operand0 = ScriptReadAny(vm, env);
    u16 operand1 = ScriptReadAny(vm, env);
    u16 retWkId = VM_Read16(vm);
    u16 *retWk = ScriptWork_GetWkAddr(work, gameData, retWkId);
    FieldActor *actor;
    EventWifiBSubwayArgs args;

    if (bsw == NULL && cmd >= BSUBWAY_WORK_COMMAND_MIN) {
        return result;
    }
    switch (cmd) {
    case BSUBWAY_TOOL_GET_ZONE_MODE:
        *retWk = BSubway_GetZonePlayMode(field);
        break;
    case BSUBWAY_TOOL_RESET_SYSTEM:
        OS_ResetSystem(0);
        break;
    case BSUBWAY_TOOL_CLEAR_SAVED_PLAY:
        BSubwayPlayData_Init(playData);
        break;
    case BSUBWAY_TOOL_GET_PLAY_SAVED:
        *retWk = BSubwayPlayData_GetSaveFlag(playData);
        break;
    case BSUBWAY_TOOL_SET_RETURN_WARP: {
        VecFx32 pos;
        ZoneSpawnInfo spawn;
        FieldPlayer *player = Field_GetPlayer(field);
        u32 dir = FieldPlayer_GetFaceDir(player);
        int warpDir;

        FieldPlayer_GetWPos(player, &pos);
        switch (dir) {
        case DIR_UP:
            warpDir = WARP_DIR_UP;
            break;
        case DIR_DOWN:
            warpDir = WARP_DIR_DOWN;
            break;
        case DIR_LEFT:
            warpDir = WARP_DIR_LEFT;
            break;
        case DIR_RIGHT:
            warpDir = WARP_DIR_RIGHT;
            break;
        default:
            warpDir = 0;
            break;
        }
        CreateZoneChangeData(&spawn, Field_GetPlayerStateZoneID(field), warpDir, pos.x, pos.y, pos.z);
        GameData_SetNextZone(gameData, &spawn);
        EventWork_FlagSet(eventWork, FLAG_BSUBWAY_ON_TRAIN);
        break;
    }
    case BSUBWAY_TOOL_CLEAR_ON_TRAIN:
        EventWork_FlagReset(eventWork, FLAG_BSUBWAY_ON_TRAIN);
        break;
    case BSUBWAY_TOOL_GET_MODE_STREAK:
        *retWk = BSubwayScoreData_GetStreak(score, operand0);
        if (*retWk > BSUBWAY_COUNT_MAX) {
            *retWk = BSUBWAY_COUNT_MAX;
        }
        break;
    case BSUBWAY_TOOL_RESET_SAVED_PLAY:
        *retWk = BsubwayScr_ResetSavedPlay(gsys);
        break;
    case BSUBWAY_TOOL_GET_PLAYER_OBJ_CODE:
        *retWk = FieldPlayer_GetObjCodeByForme(FieldPlayer_GetSex(Field_GetPlayer(field)), 0);
        break;
    case BSUBWAY_TOOL_SET_ACTOR_HIDDEN:
        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        if (actor != NULL) {
            SetActorHidden(actor, operand1);
        }
        break;
    case BSUBWAY_TOOL_GET_SAVED_CAR_NUMBER:
        *retWk = BSubwayPlayData_GetRound(playData) + 1;
        break;
    case BSUBWAY_TOOL_GET_CURRENT_CAR_NUMBER:
        *retWk = BSubwayPlayData_GetRound(bsw->playData);
        *retWk = *retWk + 1;
        break;
    case BSUBWAY_TOOL_GET_MODE_CLEARED:
        if (sModeClearFlags[operand0] == BSUBWAY_NO_MODE_CLEAR_FLAG) {
            *retWk = TRUE;
        } else {
            *retWk = BSubwayScoreData_SetFlag(score, sModeClearFlags[operand0], BSUBWAY_FLAG_GET);
        }
        break;
    case BSUBWAY_TOOL_SET_MODE_CLEARED:
        if (sModeClearFlags[operand0] != BSUBWAY_NO_MODE_CLEAR_FLAG) {
            BSubwayScoreData_SetFlag(score, sModeClearFlags[operand0], BSUBWAY_FLAG_SET);
        }
        break;
    case BSUBWAY_TOOL_GET_PARTNER_INTRO_SEEN:
        *retWk = BSubwayScoreData_SetFlag(score, BSUBWAY_SCORE_FLAG_PARTNER_INTRO_SEEN, BSUBWAY_FLAG_GET);
        break;
    case BSUBWAY_TOOL_SET_PARTNER_INTRO_SEEN:
        BSubwayScoreData_SetFlag(score, BSUBWAY_SCORE_FLAG_PARTNER_INTRO_SEEN, BSUBWAY_FLAG_SET);
        break;
    case BSUBWAY_TOOL_START_TRAIN_EFFECT:
        FieldGimmickBSubway_StartTrainEffect(field, sTrainModelsByPlayMode[operand0], &sTrainEffectPositions[operand1]);
        break;
    case BSUBWAY_TOOL_SET_TRAIN_ANIMATION:
        FldeffBtrain_SetAnime(FieldGimmickBSubway_GetTrainEffect(field), operand0);
        break;
    case BSUBWAY_TOOL_SET_TRAIN_HIDDEN:
        FldeffBtrain_SetHidden(FieldGimmickBSubway_GetTrainEffect(field), operand0);
        break;
    case BSUBWAY_TOOL_GET_SAVED_MODE:
        *retWk = BSubwayPlayData_GetData(playData, BSUBWAY_PLAY_MODE, NULL);
        break;
    case BSUBWAY_TOOL_IS_MULTI_MODE:
        *retWk = FALSE;
        switch (operand0) {
        case BSUBWAY_MODE_MULTI:
        case BSUBWAY_MODE_COMM_MULTI:
        case BSUBWAY_MODE_SUPER_MULTI:
        case BSUBWAY_MODE_SUPER_COMM_MULTI:
            *retWk = TRUE;
            break;
        }
        break;
    case BSUBWAY_TOOL_SET_ACTOR_HEIGHT: {
        fx32 y = operand1 << 16;
        VecFx32 pos;

        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        if (actor != NULL) {
            CopyActorWPos(actor, &pos);
            pos.y = y;
            SetActorWPosAll(actor, &pos, GetActorFaceDir(actor));
            FldAct_SetHeightCalcDisabled(actor, TRUE);
        }
        break;
    }
    case BSUBWAY_TOOL_ENABLE_ACTOR_HEIGHT_CALC:
        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        if (actor != NULL) {
            FldAct_SetHeightCalcDisabled(actor, FALSE);
        }
        break;
    case BSUBWAY_TOOL_HAS_ACTOR:
        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        *retWk = FALSE;
        if (actor != NULL) {
            *retWk = TRUE;
        }
        break;
    case BSUBWAY_TOOL_GET_PASSENGER_MESSAGE:
        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        *retWk = 0;
        if (actor != NULL) {
            *retWk = BSubway_GetPassengerMessageId(actor);
        }
        break;
    case BSUBWAY_TOOL_GET_WIFI_RANK:
        *retWk = BsubwayScr_UpdateStageRank(score, gsys, BSUBWAY_RANK_GET);
        if ((s16)*retWk <= 0) {
            *retWk = 1;
        }
        break;
    case BSUBWAY_TOOL_PROMOTE_WIFI_RANK:
        *retWk = BsubwayScr_UpdateStageRank(score, gsys, BSUBWAY_RANK_PROMOTE);
        break;
    case BSUBWAY_TOOL_CHECK_WIFI_DEMOTION:
        *retWk = BsubwayScr_UpdateStageRank(score, gsys, BSUBWAY_RANK_CHECK_DEMOTION);
        break;
    case BSUBWAY_TOOL_GET_MODE_STAGE:
        *retWk = BSubwayScoreData_GetStage(score, operand0);
        break;
    case BSUBWAY_TOOL_GET_PARTNER_OBJ_CODE:
        *retWk = FieldPlayer_GetObjCodeByExState(
            getTrainerGender(&GameData_GetPlayerState(gameData)->playerInfo) == GENDER_MALE ? 1 : 0, 0);
        break;
    case BSUBWAY_TOOL_HAS_BATTLE_BOX: {
        BattleBoxSave *battleBox = getBattleBox(save);

        *retWk = FALSE;
        if (BattleBoxSave_HasPkm(battleBox) == TRUE) {
            *retWk = TRUE;
        }
        break;
    }
    case 33:
        *retWk = 0;
        break;
    case BSUBWAY_TOOL_START_FIXED_TRAIN_EFFECT:
        FieldGimmickBSubway_StartTrainEffect(field, 7, sTrainEffectPositions);
        break;
    case BSUBWAY_TOOL_SET_ACTOR_FACING:
        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        if (actor != NULL) {
            CheckSetActorFaceDir(actor, operand1);
        }
        break;
    case BSUBWAY_TOOL_DISABLE_ACTOR_MOVEMENT:
        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        if (actor != NULL) {
            DisableActorMovement(actor);
        }
        break;
    case BSUBWAY_TOOL_SHOW_WIFI_LEADER_MESSAGE:
        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        ScriptWork_CallEvent(work, EventBSubwayLeaderMsg_Create(gsys, GetActorUserParam(actor, 0), operand0));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_HAS_MODE_STAGE:
        *retWk = BSubwayScoreData_HasStage(score, operand0);
        break;
    case BSUBWAY_TOOL_HAS_PASSENGER_GIFT: {
        u16 index;

        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        index = GetActorUserParam(actor, 0);
        *retWk = FALSE;
        if (BSubwayCmd_HasPassengerGift(bsw, index) == TRUE) {
            *retWk = TRUE;
        }
        break;
    }
    case BSUBWAY_TOOL_GET_PASSENGER_GIFT_PROMPT: {
        u16 beforeMessage, afterMessage, giftItem;

        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        BSubwayCmd_GetPassengerGiftInfo(bsw, GetActorUserParam(actor, 0), &beforeMessage, &afterMessage, &giftItem);
        *retWk = beforeMessage;
        break;
    }
    case BSUBWAY_TOOL_GET_PASSENGER_GIFT_MESSAGE: {
        u16 beforeMessage, afterMessage, giftItem;

        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        BSubwayCmd_GetPassengerGiftInfo(bsw, GetActorUserParam(actor, 0), &beforeMessage, &afterMessage, &giftItem);
        *retWk = afterMessage;
        break;
    }
    case BSUBWAY_TOOL_GET_PASSENGER_GIFT_ITEM: {
        u16 beforeMessage, afterMessage, giftItem;

        actor = FindFieldActor(Field_GetActorSystem(field), operand0);
        BSubwayCmd_GetPassengerGiftInfo(bsw, GetActorUserParam(actor, 0), &beforeMessage, &afterMessage, &giftItem);
        *retWk = giftItem;
        break;
    }
    case BSUBWAY_TOOL_GET_WIFI_LEADER_COUNT: {
        BSubwayWifiLeader *list = BSubwayWifiData_AllocLeaders(data3A, HEAPID_GAMEEVENT);

        *retWk = BSubwayWifiLeader_CountList(list);
        GFL_HeapFree(list);
        break;
    }
    case BSUBWAY_TOOL_OPEN_YES_NO_MENU: {
        BOOL cursorRow = FALSE;

        if (operand0 == TRUE) {
            cursorRow = TRUE;
        }
        ScriptWork_CallEvent(work, EventBSubwayMsgWindow_Create(gsys, env, retWk, cursorRow));
        result = TRUE;
        break;
    }
    case BSUBWAY_TOOL_WARP_TO_ZONE_MODE: {
        ZoneSpawnInfo spawn;
        u16 index = BSubway_GetZonePlayMode(field);
        const VecFx32 *pos = &sWarpPositions[index];

        CreateZoneChangeData(&spawn, sWarpZones[index], WARP_DIR_RIGHT, pos->x, pos->y, pos->z);
        GameData_SetNextZone(gameData, &spawn);
        EventWork_FlagSet(eventWork, FLAG_BSUBWAY_ON_TRAIN);
        break;
    }
    case BSUBWAY_TOOL_SET_WIFI_RESULTS_PENDING:
        if (operand0 == 0) {
            BSubwayScoreData_SetFlag(score, BSUBWAY_SCORE_FLAG_WIFI_RESULTS_PENDING, BSUBWAY_FLAG_RESET);
        } else {
            BSubwayScoreData_SetFlag(score, BSUBWAY_SCORE_FLAG_WIFI_RESULTS_PENDING, BSUBWAY_FLAG_SET);
        }
        break;
    case BSUBWAY_TOOL_GET_WIFI_RESULTS_PENDING:
        *retWk = BSubwayScoreData_SetFlag(score, BSUBWAY_SCORE_FLAG_WIFI_RESULTS_PENDING, BSUBWAY_FLAG_GET);
        break;
    case BSUBWAY_TOOL_UPLOAD_WIFI_RESULTS:
        args.mode = WIFI_BSUBWAY_MODE_UPLOAD;
        args.result = retWk;
        ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_WIFI_BSUBWAY,
                                                                   EventWifiBSubway_CreateFromArgs, &args));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_DOWNLOAD_WIFI_TRAIN:
        result = TRUE;
        args.mode = WIFI_BSUBWAY_MODE_DOWNLOAD_TRAIN;
        args.result = retWk;
        ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_WIFI_BSUBWAY,
                                                                   EventWifiBSubway_CreateFromArgs, &args));
        break;
    case BSUBWAY_TOOL_DOWNLOAD_WIFI_LEADERS:
        args.mode = WIFI_BSUBWAY_MODE_DOWNLOAD_LEADERS;
        args.result = retWk;
        ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_EVENT_WIFI_BSUBWAY,
                                                                   EventWifiBSubway_CreateFromArgs, &args));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_GET_DOWNLOADED_TRAIN_RANK:
        *retWk = BSubwayWifiData_GetPlayerRank(data3A);
        break;
    case BSUBWAY_TOOL_HAS_WIFI_TRAIN:
        *retWk = BSubwayWifiData_HasPlayers(data3A);
        break;
    case BSUBWAY_TOOL_SET_WIFI_RANKED_CHALLENGE: {
        u8 value = operand0;

        BSubwayPlayData_SetData(playData, BSUBWAY_PLAY_WIFI_RANKED_CHALLENGE, &value);
        break;
    }
    case BSUBWAY_TOOL_GET_WIFI_RANKED_CHALLENGE:
        *retWk = BSubwayPlayData_GetData(playData, BSUBWAY_PLAY_WIFI_RANKED_CHALLENGE, NULL);
        break;
    case BSUBWAY_TOOL_HAS_WIFI_LEADERS:
        *retWk = BSubwayWifiData_HasLeaders(data3A);
        break;
    case BSUBWAY_TOOL_SHOW_RECORDS:
        ScriptWork_CallEvent(work, EventBSubwayRecords_Create(gsys));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_GET_WIFI_LEADER_RANK:
        *retWk = BSubwayWifiData_GetLeaderRank(data3A);
        break;
    case BSUBWAY_TOOL_GET_WIFI_LEADER_ROOM:
        *retWk = BSubwayWifiData_GetLeaderRoom(data3A);
        break;
    case 200:
        *retWk = 0;
        break;
    case 203:
        *retWk = 0;
        break;
    case BSUBWAY_TOOL_WARP_TO_SAVED_MODE:
        if (mode < BSUBWAY_SCORE_MODE_MAX) {
            ZoneSpawnInfo spawn;
            const VecFx32 *pos = &sWarpPositions[mode];

            CreateZoneChangeData(&spawn, sWarpZones[mode], WARP_DIR_RIGHT, pos->x, pos->y, pos->z);
            GameData_SetNextZone(gameData, &spawn);
            EventWork_FlagSet(eventWork, FLAG_BSUBWAY_ON_TRAIN);
        }
        break;
    case BSUBWAY_TOOL_SELECT_MEMBERS: {
        u8 value = BSubwayPlayData_GetData(playData, BSUBWAY_PLAY_USE_BATTLE_BOX, NULL);

        ScriptWork_CallEvent(work, EventBSubwayPokeSelect_Create(bsw, gsys, value));
        result = TRUE;
        break;
    }
    case BSUBWAY_TOOL_SET_SELECTED_MEMBERS:
        *retWk = BsubwayScr_SetSelectedMembers(bsw, gsys);
        break;
    case 302:
        *retWk = 0;
        break;
    case BSUBWAY_TOOL_IS_WIN_MAX:
        if (BsubwayScr_IsWinMax(bsw) == TRUE) {
            *retWk = 1;
        } else {
            *retWk = 0;
        }
        break;
    case BSUBWAY_TOOL_SET_LOSS_SCORE:
        BsubwayScr_SetLoseScore(gsys, bsw);
        break;
    case BSUBWAY_TOOL_SET_CLEAR_SCORE:
        BsubwayScr_SetClearScore(bsw, gsys);
        break;
    case BSUBWAY_TOOL_SAVE_REST_PLAY:
        BsubwayScr_SaveRestPlayData(bsw);
        break;
    case BSUBWAY_TOOL_CREATE_ROUND_TRAINERS:
        BsubwayScr_CreateRoundTrainers(bsw);
        break;
    case BSUBWAY_TOOL_GET_TRAINER_OBJ_CODE:
        *retWk = BsubwayScr_GetTrainerObjCode(bsw, operand0);
        break;
    case BSUBWAY_TOOL_START_BATTLE:
        ScriptWork_CallEvent(work, EventBSubwayBattle_Create(bsw, gsys, field));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_GET_CURRENT_MODE:
        *retWk = bsw->playMode;
        break;
    case BSUBWAY_TOOL_SET_FIXED_REWARD:
        bsw->fixedReward = operand0;
        break;
    case BSUBWAY_TOOL_GET_FIXED_REWARD:
        *retWk = bsw->fixedReward;
        break;
    case BSUBWAY_TOOL_ADD_BATTLE_POINTS:
        *retWk = BsubwayScr_AddBattlePoints(bsw);
        break;
    case BSUBWAY_TOOL_SET_PARTNER_INDEX:
        bsw->partnerIndex = operand0;
        break;
    case BSUBWAY_TOOL_GET_PARTNER_INDEX:
        *retWk = bsw->partnerIndex;
        break;
    case BSUBWAY_TOOL_DRAW_TRAINERS:
        BsubwayScr_DrawTrainerNos(bsw);
        break;
    case BSUBWAY_TOOL_GET_MEMBER_SLOT:
        *retWk = bsw->memberSlots[operand0];
        break;
    case BSUBWAY_TOOL_CLEAR_RECEIVE_BUFFER:
        bsw->recvCount = 0;
        MI_CpuFill8(bsw->recvBuf, 0, sizeof(bsw->recvBuf));
        break;
    case BSUBWAY_TOOL_SHOW_TRAINER_MESSAGE_TOP_RIGHT:
        ScriptWork_CallEvent(work, EventBSubwayTrainerMsg_Create(bsw, gsys, operand0, operand1, MSG_WIN_POS_TOP_RIGHT));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_SAVE_MEMBERS:
        BsubwayScr_SavePlayDataMembers(bsw);
        break;
    case BSUBWAY_TOOL_LOAD_MEMBERS:
        BsubwayScr_LoadMembers(bsw, gsys);
        break;
    case BSUBWAY_TOOL_SET_USE_BATTLE_BOX: {
        u8 value = operand0;

        BSubwayPlayData_SetData(playData, BSUBWAY_PLAY_USE_BATTLE_BOX, &value);
        break;
    }
    case BSUBWAY_TOOL_CHANGE_COMM_MULTI_MODE:
        BsubwayScr_ChangeCommMultiMode(bsw);
        break;
    case BSUBWAY_TOOL_IS_SUPER_MODE:
        if (mode == BSUBWAY_MODE_SUPER_SINGLE || mode == BSUBWAY_MODE_SUPER_DOUBLE ||
            mode == BSUBWAY_MODE_SUPER_MULTI || mode == BSUBWAY_MODE_SUPER_COMM_MULTI) {
            *retWk = 1;
        } else {
            *retWk = 0;
        }
        break;
    case BSUBWAY_TOOL_PLACE_PASSENGERS:
        BSubway_PlacePassengers(bsw, gameData, Field_GetActorSystem(field), field);
        break;
    case BSUBWAY_TOOL_SET_RETURN_SYNC_REQUIRED:
        bsw->returnSyncRequired = operand0;
        break;
    case BSUBWAY_TOOL_GET_RETURN_SYNC_REQUIRED:
        *retWk = bsw->returnSyncRequired;
        break;
    case BSUBWAY_TOOL_IS_BATTLE_WON:
        if (bsw->btlSetup == NULL) {
            *retWk = 0;
        } else if (bsw->btlSetup->result == BTL_RESULT_VICTORY) {
            *retWk = 1;
        } else {
            *retWk = 0;
        }
        break;
    case BSUBWAY_TOOL_CLEAR_BATTLE_STATE:
        BsubwayScr_ClearStateBits(bsw, gsys);
        break;
    // Fades in the switches for the train's cars
    case BSUBWAY_TOOL_SHOW_CAR_SWITCHES: {
        ISSSwitchIndex i = 0;
        u16 count = BSubwayPlayData_GetRound(playData);
        ISSSwitchSys *switchSys;

        if (count != 0) {
            switchSys = ISS_GetSwitchSys(GameSystem_GetISS(gsys));
            count = i + count + 1;
            for (; i < count; i++) {
                if (i < ISS_SWITCH_COUNT && i != 0) {
                    ISSSwitchSys_ReqSwitchFadeIn(switchSys, i);
                }
            }
        }
        break;
    }
    case BSUBWAY_TOOL_CREATE_PARTNERS:
        BsubwayScr_CreatePartners(
            bsw, getTrainerGender(&GameData_GetPlayerState(gameData)->playerInfo) == GENDER_MALE ? 1 : 0);
        break;
    case BSUBWAY_TOOL_GET_MEMBER_SPECIES:
        *retWk = bsw->memberSpecies[operand0];
        break;
    case BSUBWAY_TOOL_SET_USE_EXISTING_NET:
        bsw->useExistingNet = operand0;
        break;
    case BSUBWAY_TOOL_GET_USE_EXISTING_NET:
        *retWk = bsw->useExistingNet;
        break;
    case BSUBWAY_TOOL_CAN_PARTY_ENTER: {
        PokeParty *party = NULL;
        u32 useBattleBox;
        Regulation *regulation;
        PokeParty *checked;
        u32 check;

        *retWk = 0;
        useBattleBox = BSubwayPlayData_GetData(playData, BSUBWAY_PLAY_USE_BATTLE_BOX, NULL);
        regulation = PokeRegulation_LoadNew(BSubway_GetModeRegulation(mode), HEAPID_GAMEEVENT);
        if (useBattleBox == TRUE) {
            party = convertBoxedPokeSetToParty(getBattleBox(save), HEAPID_GAMEEVENT);
            checked = party;
        } else {
            checked = GameData_GetParty(gameData);
        }
        check = PokeRegulation_CheckPartyCanEnter(regulation, checked);
        GFL_HeapFree(regulation);
        if (party != NULL) {
            GFL_HeapFree(party);
        }
        if (check <= REGULATION_RESULT_LEVEL_TOTAL) {
            *retWk = 1;
        }
        break;
    }
    case BSUBWAY_TOOL_LOAD_BATTLE_BOX_COPY:
        BsubwayScr_LoadPartyCopy(bsw);
        break;
    case BSUBWAY_TOOL_GET_REGULATION:
        *retWk = BSubway_GetModeRegulation(mode);
        break;
    case BSUBWAY_TOOL_GET_MEMBER_COUNT:
        *retWk = BsubwayScr_GetMemberNum(mode);
        break;
    case BSUBWAY_TOOL_CHECK_VIDEO_SLOT: {
        u32 buffer;

        if (bsw->videoState == BSUBWAY_VIDEO_UNCHECKED) {
            bsw->videoState = BSUBWAY_VIDEO_EMPTY;
            if (BattleRec_IsSlotValid(save, 4, &buffer, 0) == 1) {
                bsw->videoState = BSUBWAY_VIDEO_PRESENT;
            }
        }
        break;
    }
    case BSUBWAY_TOOL_HAS_VIDEO_SLOT:
        if (bsw->videoState == BSUBWAY_VIDEO_EMPTY) {
            *retWk = 0;
        } else {
            *retWk = 1;
        }
        break;
    case BSUBWAY_TOOL_STORE_BATTLE_VIDEO:
        BattleRec_LoadToolOverlay();
        BattleRecTool_StoreSetup(bsw->btlSetup);
        BattleRec_UnloadToolOverlay();
        break;
    case BSUBWAY_TOOL_SAVE_BATTLE_VIDEO:
        bsw->videoSeq = 0;
        bsw->videoSaveSeq = 0;
        bsw->videoCounter = operand0;
        VM_SetNativeCallback(vm, ScriptNative_BSubwayBattleRecSaveWait);
        bsw->videoState = BSUBWAY_VIDEO_PRESENT;
        result = TRUE;
        break;
    case BSUBWAY_TOOL_FREE_BATTLE_SETUP:
        if (bsw->btlSetup != NULL) {
            freeVSPlayerBlkClearPtr();
            BtlSetup_Free(bsw->btlSetup);
            bsw->btlSetup = NULL;
        }
        break;
    case BSUBWAY_TOOL_SHOW_VIDEO_SAVE_SCREEN:
        MI_CpuFill8(&bsw->ov306Param, 0, sizeof(bsw->ov306Param));
        bsw->ov306Param.gameData = gameData;
        bsw->ov306Param.compatible = 1;
        bsw->ov306Param.comm = 0;
        bsw->ov306Param.recorded = 1;
        bsw->ov306Param.mode = sVideoModesByPlayMode[mode];
        bsw->ov306Param.counter = BsubwayScr_GetWinCount(bsw);
        if (bsw->ov306Param.counter > BSUBWAY_COUNT_MAX) {
            bsw->ov306Param.counter = BSUBWAY_COUNT_MAX;
        }
        ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_ID(306),
                                                                                     &BtlRecSelProcFunctions,
                                                                                     &bsw->ov306Param, NULL, NULL));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_RESET_MODE_STAGE:
        BSubwayPlayData_ResetRound(playData);
        BSubwayScoreData_ResetStage(score, mode);
        break;
    case BSUBWAY_TOOL_STOP_TRAIN_SHAKE:
        FieldGimmickBSubway_StopShake(field);
        break;
    case BSUBWAY_TOOL_GET_CURRENT_TRAINER: {
        u16 index = BSubwayPlayData_GetRound(playData);

        switch (mode) {
        case BSUBWAY_MODE_MULTI:
        case BSUBWAY_MODE_COMM_MULTI:
        case BSUBWAY_MODE_SUPER_MULTI:
        case BSUBWAY_MODE_SUPER_COMM_MULTI:
            *retWk = bsw->trainerNos[operand0 + index * 2];
            break;
        default:
            *retWk = bsw->trainerNos[index];
            break;
        }
        break;
    }
    case BSUBWAY_TOOL_SHOW_TRAINER_MESSAGE_BOTTOM_RIGHT:
        ScriptWork_CallEvent(work,
                             EventBSubwayTrainerMsg_Create(bsw, gsys, operand0, operand1, MSG_WIN_POS_BOTTOM_RIGHT));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_ADD_WIFI_BATTLE_RECORD:
        if (bsw->btlSetup != NULL) {
            u8 a = bsw->btlSetup->turnCount;
            u8 b = bsw->btlSetup->playerFaintedCount;
            int count;
            PokeParty *party = bsw->btlSetup->party[0];
            int i;
            u32 hp;
            u32 hpSum;
            u32 maxHpSum;

            count = PokeParty_GetPkmCount(party);
            hpSum = 0;
            maxHpSum = 0;
            for (i = 0; i < count; i++) {
                pkm = PokeParty_GetPkm(party, i);
                hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
                maxHpSum += PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
                hpSum += hp;
            }
            BSubwayPlayData_AddWifiRecord(playData, b, a, maxHpSum - hpSum);
        }
        break;
    case BSUBWAY_TOOL_SET_VIDEO_SAVE_DISABLED:
        bsw->videoSaveDisabled = operand0;
        break;
    case BSUBWAY_TOOL_GET_VIDEO_SAVE_DISABLED:
        *retWk = bsw->videoSaveDisabled;
        break;
    case BSUBWAY_TOOL_GET_USE_BATTLE_BOX:
        *retWk = BSubwayPlayData_GetData(playData, BSUBWAY_PLAY_USE_BATTLE_BOX, NULL);
        break;
    case BSUBWAY_TOOL_ADVANCE_ROUND:
        BSubwayPlayData_IncRound(playData);
        BsubwayScr_IncWinCount(bsw);
        *retWk = BsubwayScr_GetWinCount(bsw);
        if (*retWk > BSUBWAY_COUNT_MAX) {
            *retWk = BSUBWAY_COUNT_MAX;
        }
        break;
    case BSUBWAY_TOOL_GET_WIN_COUNT:
        *retWk = BsubwayScr_GetWinCount(bsw);
        if (*retWk > BSUBWAY_COUNT_MAX) {
            *retWk = BSUBWAY_COUNT_MAX;
        }
        break;
    case BSUBWAY_TOOL_START_MODE_STAGE:
        if (BSubwayScoreData_HasStage(score, mode) == 0) {
            BSubwayScoreData_StartStage(score, mode);
            BSubwayPlayData_ResetRound(playData);
            BsubwayScr_ClearWinCount(bsw);
        }
        break;
    case BSUBWAY_TOOL_START_COMM:
        if (bsw->useExistingNet == TRUE) {
            BSubwayComm_AddCommands(bsw);
        } else {
            BSubwayComm_InitNet(bsw);
        }
        break;
    case BSUBWAY_TOOL_END_COMM:
        ScriptWork_CallEvent(work, EventBSubwayCommEnd_Create(gsys, bsw));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_WAIT_COMM_TIMING:
        bsw->timing = operand0;
        bsw->resultVarId = retWkId;
        BSubwayComm_SendTiming(bsw->timing);
        VM_SetNativeCallback(vm, ScriptNative_BSubwayTimingWait);
        result = TRUE;
        break;
    case BSUBWAY_TOOL_OPEN_PARENT_ENTRY_MENU:
        bsw->resultVar = retWk;
        bsw->entryMenu = CommEntryMenu_Create(GetGameDataPlayerInfo(gameData), field, 2, 2, HEAPID_GAMEEVENT,
                                              COMM_ENTRY_MENU_LEADER, COMM_ENTRY_MENU_BATTLE_SUBWAY, NULL);
        VM_SetNativeCallback(vm, ScriptNative_BSubwayParentEntryMenuWait);
        result = TRUE;
        break;
    case BSUBWAY_TOOL_OPEN_CHILD_ENTRY_MENU:
        bsw->resultVar = retWk;
        bsw->entryMenu = CommEntryMenu_Create(GetGameDataPlayerInfo(gameData), field, 2, 2, HEAPID_GAMEEVENT,
                                              COMM_ENTRY_MENU_SELECT_LEADER, COMM_ENTRY_MENU_BATTLE_SUBWAY, NULL);
        result = TRUE;
        VM_SetNativeCallback(vm, ScriptNative_BSubwayChildEntryMenuWait);
        break;
    case BSUBWAY_TOOL_SEND_COMM_DATA:
        BSubwayComm_SetSendData(bsw, operand0, operand1);
        VM_SetNativeCallback(vm, ScriptNative_BSubwaySendWait);
        result = TRUE;
        break;
    case BSUBWAY_TOOL_WAIT_COMM_DATA:
        BSubwayComm_SetRecvMode(bsw, operand0);
        bsw->resultVarId = retWkId;
        VM_SetNativeCallback(vm, ScriptNative_BSubwayRecvWait);
        result = TRUE;
        break;
    case BSUBWAY_TOOL_GET_NET_ID:
        *retWk = NetSystem_GetNetId();
        break;
    case BSUBWAY_TOOL_START_IRC_MATCH:
        MI_CpuFill8(&bsw->ircMatchParam, 0, sizeof(bsw->ircMatchParam));
        bsw->ircMatchParam.gameData = gameData;
        bsw->ircMatchParam.result = IRCBATTLEMATCH_MODE_BSUBWAY;
        ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(
                                       gsys, field, OVERLAY_IRCBATTLEMATCH, &IRCBATTLEMATCH_PROC_FUNCTIONS,
                                       &bsw->ircMatchParam, NULL, NULL));
        result = TRUE;
        break;
    case BSUBWAY_TOOL_GET_IRC_MATCH_RESULT:
        switch (bsw->ircMatchParam.result) {
        case IRCBATTLEMATCH_MODE_BSUBWAY:
            *retWk = BSUBWAY_IRC_MATCH_READY;
            break;
        case IRCBATTLEMATCH_RESULT_QUIT:
            *retWk = BSUBWAY_IRC_MATCH_QUIT;
            break;
        case IRCBATTLEMATCH_RESULT_BACK:
            *retWk = BSUBWAY_IRC_MATCH_BACK;
            break;
        default:
            *retWk = BSUBWAY_IRC_MATCH_QUIT;
            break;
        }
        break;
    case BSUBWAY_TOOL_ADD_MET_PARTNER:
        Misc_AddMetTrainer(getHollow_RivalData(save), bsw->partner.id);
        break;
    case BSUBWAY_TOOL_GET_NET_PLAYER_0_OBJ_CODE:
        *retWk = NetSystem_GetNetId() == FALSE
                     ? FieldPlayer_GetObjCodeByForme(FieldPlayer_GetSex(Field_GetPlayer(field)), 0)
                     : BSubway_GetPlayerObjCode(&bsw->partner);
        break;
    case BSUBWAY_TOOL_GET_NET_PLAYER_1_OBJ_CODE:
        *retWk = NetSystem_GetNetId() == FALSE
                     ? BSubway_GetPlayerObjCode(&bsw->partner)
                     : FieldPlayer_GetObjCodeByForme(FieldPlayer_GetSex(Field_GetPlayer(field)), 0);
        break;
    case BSUBWAY_TOOL_MARK_NET_ERROR_SHOWN:
        GFL_NetErrMarkShown();
        break;
    case BSUBWAY_TOOL_SET_NET_ERROR_PENDING:
        GSYS_SetNetErrorPending(gsys, 1);
        break;
    case BSUBWAY_TOOL_HAS_NET_ERROR:
        *retWk = FALSE;
        if (GFL_NetErrCheck()) {
            *retWk = TRUE;
        }
        break;
    case BSUBWAY_TOOL_CAN_RECORD_BATTLE_VERSION:
        *retWk = FALSE;
        if (bsw->btlSetup != NULL && BattleRec_IsVersionSupported(bsw->btlSetup->commonVersion) == TRUE) {
            *retWk = TRUE;
        }
        break;
    case 102:
    case 201:
    case 202:
    case 501:
        break;
    }
    return result;
}

static BOOL ScriptNative_BSubwaySendWait(VM *vm, void *env) {
    BSubwayScrWork *bsw =
        GameData_GetBSubwayScrWork(GSYS_GetGameData(ScriptWork_GetGameSystem(FieldScriptEnv_GetScriptWork(env))));

    if (BSubwayComm_Send(bsw) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ScriptNative_BSubwayTimingWait(VM *vm, void *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(work));
    BSubwayScrWork *bsw = GameData_GetBSubwayScrWork(gameData);
    u16 *var = ScriptWork_GetWkAddr(work, gameData, bsw->resultVarId);

    if (GFL_NetErrCheck()) {
        *var = 0;
        return TRUE;
    }
    if (BSubwayComm_IsTimingRecv(bsw->timing) == TRUE) {
        *var = 1;
        return TRUE;
    }
    return FALSE;
}

static BOOL ScriptNative_BSubwayParentEntryMenuWait(VM *vm, void *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(work));
    SaveControl *save = GameData_GetSaveControl(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BSubwayScrWork *bsw = GameData_GetBSubwayScrWork(gameData);
    u32 mode = BSubwayPlayData_GetData(SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY), BSUBWAY_PLAY_MODE, NULL);
    BSubwayScoreData *score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    u32 answer = CommEntryMenu_Update(bsw->entryMenu);

    if (answer != COMM_ENTRY_MENU_RUNNING) {
        CommEntryMenu_Free(bsw->entryMenu);
        bsw->entryMenu = NULL;
        switch (answer) {
        case COMM_ENTRY_MENU_DONE:
            *bsw->resultVar = BSUBWAY_ENTRY_DONE;
            return TRUE;
        case COMM_ENTRY_MENU_CANCELLED:
            *bsw->resultVar = BSUBWAY_ENTRY_CANCELLED;
            return TRUE;
        case COMM_ENTRY_MENU_ERROR:
        default:
            *bsw->resultVar = BSUBWAY_ENTRY_ERROR;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL ScriptNative_BSubwayChildEntryMenuWait(VM *vm, void *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(work));
    SaveControl *save = GameData_GetSaveControl(gameData);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BSubwayScrWork *bsw = GameData_GetBSubwayScrWork(gameData);
    u32 mode = BSubwayPlayData_GetData(SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY), BSUBWAY_PLAY_MODE, NULL);
    BSubwayScoreData *score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    u32 answer = CommEntryMenu_Update(bsw->entryMenu);

    if (answer != COMM_ENTRY_MENU_RUNNING) {
        CommEntryMenu_Free(bsw->entryMenu);
        bsw->entryMenu = NULL;
        switch (answer) {
        case COMM_ENTRY_MENU_DONE:
            *bsw->resultVar = BSUBWAY_ENTRY_DONE;
            return TRUE;
        case COMM_ENTRY_MENU_CANCELLED:
        default:
            *bsw->resultVar = BSUBWAY_ENTRY_CANCELLED;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL ScriptNative_BSubwayRecvWait(VM *vm, void *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(work));
    BSubwayScrWork *bsw = GameData_GetBSubwayScrWork(gameData);

    if (BSubwayComm_CheckRecv(bsw, ScriptWork_GetWkAddr(work, gameData, bsw->resultVarId)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

enum {
    STEP_REMOVE_COMMANDS,
    STEP_EXIT_NET,
    STEP_WAIT_NET_END,
};

static GameEventReturnCode EventBSubwayCommEnd_Callback(GameEvent *event, u32 *state, void *data) {
    BSubwayScrWork **bsw = data;

    switch (*state) {
    case STEP_REMOVE_COMMANDS:
        if (GFL_NetIsParent() == TRUE) {
            GFL_NetSetDisconnectError(FALSE);
        }
        if ((*bsw)->useExistingNet == TRUE) {
            BSubwayComm_RemoveCommands(*bsw);
        }
        (*state)++;
        break;
    case STEP_EXIT_NET:
        if (GFL_NetIsParent() == TRUE) {
            if (GFL_NetGetNegotiatedCount() <= 1) {
                GFL_NetExit(0);
                (*state)++;
            }
        } else {
            GFL_NetExit(0);
            (*state)++;
        }
        break;
    case STEP_WAIT_NET_END:
        if (GFL_NetIsEnded() == TRUE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *EventBSubwayCommEnd_Create(GameSystem *gsys, BSubwayScrWork *bsw) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBSubwayCommEnd_Callback, sizeof(BSubwayScrWork *));
    BSubwayScrWork **data = GameEvent_GetData(event);

    *data = bsw;
    return event;
}

enum {
    STEP_SET_WAIT,
    STEP_OPEN_YES_NO,
    STEP_WAIT_YES_NO,
    STEP_CLOSE_MENU,
};

static GameEventReturnCode EventBSubwayMsgWindow_Callback(GameEvent *event, u32 *state, void *data) {
    BSubwayMsgEvent *wk = data;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(wk->env);

    switch (*state) {
    case STEP_SET_WAIT:
        FieldScriptEnv_SetWaitCounter(wk->env, 1);
        (*state)++;
    case STEP_OPEN_YES_NO:
        if (FieldScriptEnv_UpdateWaitCounter(wk->env) == TRUE) {
            wk->window = MsgBGPrinter_CreateYesNoMenu(
                Field_GetMsgBGSys(GSYS_GetField(FieldScriptEnv_GetGameSystem(wk->env))), wk->cursorRow);
            (*state)++;
        }
        break;
    case STEP_WAIT_YES_NO:
        switch (MsgBGPrinter_YesNoMenuMain(wk->window)) {
        case MSGBG_YES_NO_YES:
            *wk->var = MSGBG_YES_NO_YES;
            (*state)++;
            break;
        case MSGBG_YES_NO_WAITING:
            break;
        default:
            *wk->var = MSGBG_YES_NO_NO;
            (*state)++;
            break;
        }
        break;
    case STEP_CLOSE_MENU:
        MsgBGPrinter_DeleteMenuFunc(wk->window);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *EventBSubwayMsgWindow_Create(GameSystem *gsys, FieldScriptEnv *env, u16 *var, u32 cursorRow) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBSubwayMsgWindow_Callback, sizeof(BSubwayMsgEvent));
    BSubwayMsgEvent *wk = GameEvent_GetData(event);

    wk->env = env;
    wk->var = var;
    wk->cursorRow = cursorRow;
    return event;
}

// Places the subway's passengers: in mode 4 the players of the records in block 0x3a, and otherwise the actors of the
// stage the player has reached
static void BSubway_PlacePassengers(BSubwayScrWork *bsw, GameData *gameData, MMSys *mmSys, Field *field) {
    int i = 0;
    u32 zoneId = Field_GetPlayerStateZoneID(field);
    u16 id = 0x80;
    u8 mode = bsw->playMode;

    if (mode == BSUBWAY_MODE_WIFI) {
        const BSubwayActorPos *pos = sActorPositions;
        BSubwayWifiLeader *list = BSubwayWifiData_AllocLeaders(
            SaveControl_GetBlockPtr(GameData_GetSaveControl(gameData), SAVE_BLOCK_BSUBWAY_3A), HEAPID_GAMEEVENT);
        int count = BSubwayWifiLeader_CountList(list);
        BSubwayWifiLeader *record;
        FieldActor *actor;
        u32 index;
        u16 objCode;

        if (count != 0) {
            record = &list[count - 1];
            while (count != 0 && i < 5) {
                index = BSubwayWifiLeader_GetID(record) % 10;
                if (record->female == FALSE) {
                    objCode = sMaleLeaderObjCodes[index];
                } else {
                    objCode = sFemaleLeaderObjCodes[index];
                }
                actor = CreateNewActorByParam(mmSys, pos->x, pos->z, i % 4, id, objCode, MOVE_CODE_TURN_RANDOM, zoneId);
                SetActorUserParam(actor, count - 1, 0);
                SetActorSCRID(actor, 5);
                i++;
                record--;
                id++;
                count--;
                pos++;
            }
        }
        GFL_HeapFree(list);
        return;
    }
    {
        const BSubwayActor *entry = sActors;
        u16 stage = BSubwayScoreData_GetStage(bsw->scoreData, mode);
        FieldActor *actor;

        switch (mode) {
        case BSUBWAY_MODE_SUPER_SINGLE:
        case BSUBWAY_MODE_SUPER_DOUBLE:
        case BSUBWAY_MODE_SUPER_MULTI:
        case BSUBWAY_MODE_SUPER_COMM_MULTI:
            stage += 3;
            break;
        }
        while (entry->objCode != BSUBWAY_PASSENGER_END) {
            if (sStages[entry->stage - 1] == stage) {
                actor = CreateNewActorByParam(mmSys, entry->x, entry->z, i % 4, id, entry->objCode,
                                              MOVE_CODE_TURN_RANDOM, zoneId);
                SetActorUserParam(actor, i, 0);
                SetActorUserParam(actor, entry->messageId, 1);
                SetActorSCRID(actor, 5);
                if (id < 0xff) {
                    id++;
                }
            }
            entry++;
            i++;
        }
    }
}

static u16 BSubway_GetPassengerMessageId(FieldActor *actor) {
    return GetActorUserParam(actor, 1);
}

static BOOL BSubwayCmd_HasPassengerGift(BSubwayScrWork *bsw, u16 index) {
    const BSubwayActor *entry = &sActors[index];

    if (bsw->playMode == BSUBWAY_MODE_WIFI) {
        return FALSE;
    }
    if (entry->itemId != 0) {
        return TRUE;
    }
    return FALSE;
}

static void BSubwayCmd_GetPassengerGiftInfo(BSubwayScrWork *bsw, u16 index, u16 *beforeMessage, u16 *afterMessage,
                                            u16 *giftItem) {
    const BSubwayActor *entry = &sActors[index];

    if (bsw->playMode == BSUBWAY_MODE_WIFI) {
        *beforeMessage = 0;
        *afterMessage = 0;
        *giftItem = 4;
        return;
    }
    if (entry->itemId == 0) {
        *beforeMessage = 0;
        *afterMessage = 0;
        *giftItem = 4;
        return;
    }
    *beforeMessage = entry->messageId;
    *afterMessage = entry->afterGiftMessageId;
    *giftItem = entry->itemId;
}

static BOOL ScriptNative_BSubwayBattleRecSaveWait(VM *vm, void *env) {
    GameData *gameData = GSYS_GetGameData(ScriptWork_GetGameSystem(FieldScriptEnv_GetScriptWork(env)));
    BSubwayScrWork *bsw = GameData_GetBSubwayScrWork(gameData);
    u32 modeIndex = BATTLE_REC_MODE_BATTLE_SUBWAY_SINGLE;
    int count = bsw->videoCounter;
    u32 result;

    switch (bsw->playMode) {
    case BSUBWAY_MODE_DOUBLE:
    case BSUBWAY_MODE_SUPER_DOUBLE:
        modeIndex = BATTLE_REC_MODE_BATTLE_SUBWAY_DOUBLE;
        break;
    case BSUBWAY_MODE_MULTI:
    case BSUBWAY_MODE_COMM_MULTI:
    case BSUBWAY_MODE_SUPER_MULTI:
    case BSUBWAY_MODE_SUPER_COMM_MULTI:
        modeIndex = BATTLE_REC_MODE_BATTLE_SUBWAY_MULTI;
        break;
    }
    if (count > BSUBWAY_COUNT_MAX) {
        count = BSUBWAY_COUNT_MAX;
    }
    result = BattleRec_FinishAndSaveSlot(gameData, 4, modeIndex, count, 0, &bsw->videoSeq, &bsw->videoSaveSeq);
    if (result == SAVE_RESULT_OK || result == SAVE_RESULT_NG) {
        return TRUE;
    }
    return FALSE;
}

// The regulation of a play mode, in arc 106
static u32 BSubway_GetModeRegulation(u32 mode) {
    u32 regulation = BSUBWAY_RULE_SINGLE;

    switch (mode) {
    case BSUBWAY_MODE_DOUBLE:
    case BSUBWAY_MODE_SUPER_DOUBLE:
        regulation = BSUBWAY_RULE_DOUBLE;
        break;
    case BSUBWAY_MODE_MULTI:
    case BSUBWAY_MODE_COMM_MULTI:
    case BSUBWAY_MODE_SUPER_MULTI:
    case BSUBWAY_MODE_SUPER_COMM_MULTI:
        regulation = BSUBWAY_RULE_MULTI;
        break;
    }
    return regulation;
}

static u32 BSubway_GetZonePlayMode(Field *field) {
    u16 zoneId = Field_GetPlayerStateZoneID(field);
    u32 index = BSUBWAY_MODE_SINGLE;

    switch (zoneId) {
    case ZONE_GEAR_STATION_2:
        index = BSUBWAY_MODE_SINGLE;
        break;
    case ZONE_GEAR_STATION_3:
        index = BSUBWAY_MODE_SUPER_SINGLE;
        break;
    case ZONE_GEAR_STATION_4:
        index = BSUBWAY_MODE_DOUBLE;
        break;
    case ZONE_GEAR_STATION_5:
        index = BSUBWAY_MODE_SUPER_DOUBLE;
        break;
    case ZONE_GEAR_STATION_6:
        index = BSUBWAY_MODE_MULTI;
        break;
    case ZONE_GEAR_STATION_7:
        index = BSUBWAY_MODE_SUPER_MULTI;
        break;
    case ZONE_GEAR_STATION_8:
        index = BSUBWAY_MODE_WIFI;
        break;
    }
    return index;
}

static u16 BSubway_GetPlayerObjCode(PlayerInfo *info) {
    u8 version = PlayerInfo_GetVersion(info);
    u32 gender = getTrainerGender(info);

    if (version == VERSION_WHITE2 || version == VERSION_BLACK2) {
        if (gender == GENDER_MALE) {
            return 0xe7;
        }
        return 0xf0;
    }
    if (gender == GENDER_MALE) {
        return 1;
    }
    return 4;
}
