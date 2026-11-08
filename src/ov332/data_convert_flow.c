#include "types.h"
#include "app/mb_parent.h"
#include "app/name_entry.h"
#include "app/unova_link.h"
#include "app/wifi_login.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "save/event_work.h"
#include "save/key_info.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/wordset.h"
#include "text/system/ability_handlers_2.h"

// Phases of DataConvert_SeqMenu.
enum {
    MEMORY_MENU_STEP_START_SCENE = 0,
    MEMORY_MENU_STEP_INPUT = 1,
    MEMORY_MENU_STEP_END_SCENE = 2,
    MEMORY_MENU_STEP_DISPATCH = 3,
    MEMORY_MENU_STEP_END = 4,
};

// Phases of DataConvert_SeqWireless.
enum {
    MEMORY_WIRELESS_STEP_CHECK_WIRELESS = 0,
    MEMORY_WIRELESS_STEP_PRINT_PROMPT = 1,
    MEMORY_WIRELESS_STEP_WAIT_PROMPT = 2,
    MEMORY_WIRELESS_STEP_WAIT_MENU_DRAW = 3,
    MEMORY_WIRELESS_STEP_SELECT = 4,
    MEMORY_WIRELESS_STEP_START_LINK_SCENE = 5,
    MEMORY_WIRELESS_STEP_DISTRIBUTE = 6,
    MEMORY_WIRELESS_STEP_WAIT_DISTRIBUTION = 7,
    MEMORY_WIRELESS_STEP_END_DISTRIBUTION = 8,
    MEMORY_WIRELESS_STEP_CONNECT = 9,
    MEMORY_WIRELESS_STEP_WAIT_CONNECTION = 10,
    MEMORY_WIRELESS_STEP_READ_RESULT = 11,
    MEMORY_WIRELESS_STEP_CHECK_LINKED_PLAYER = 12,
    MEMORY_WIRELESS_STEP_PRINT_REPLACE_PROMPT = 13,
    MEMORY_WIRELESS_STEP_CREATE_REPLACE_MENU = 14,
    MEMORY_WIRELESS_STEP_SELECT_REPLACE = 15,
    MEMORY_WIRELESS_STEP_MERGE = 16,
    MEMORY_WIRELESS_STEP_SAVE = 17,
    MEMORY_WIRELESS_STEP_PRINT_RESULT = 18,
    MEMORY_WIRELESS_STEP_PRINT_CANCELED = 19,
    MEMORY_WIRELESS_STEP_WAIT_RESULT = 20,
    MEMORY_WIRELESS_STEP_WAIT_ERROR = 21,
    MEMORY_WIRELESS_STEP_END_SCENE = 22,
    MEMORY_WIRELESS_STEP_END = 23,
};

// Phases of DataConvert_SeqWifi.
enum {
    MEMORY_WIFI_STEP_FADE_OUT = 0,
    MEMORY_WIFI_STEP_NEXT_FRAME = 1,
    MEMORY_WIFI_STEP_LOGIN = 2,
    MEMORY_WIFI_STEP_START_SCENE_AFTER_LOGIN = 3,
    MEMORY_WIFI_STEP_FADE_IN = 4,
    MEMORY_WIFI_STEP_CHECK_LOGIN = 5,
    MEMORY_WIFI_STEP_PRINT_STATUS_WAIT = 6,
    MEMORY_WIFI_STEP_WAIT_BEFORE_STATUS = 7,
    MEMORY_WIFI_STEP_QUERY_STATUS = 8,
    MEMORY_WIFI_STEP_WAIT_STATUS = 9,
    MEMORY_WIFI_STEP_PRINT_ID_PROMPT = 10,
    MEMORY_WIFI_STEP_WAIT_ID_PROMPT = 11,
    MEMORY_WIFI_STEP_FADE_OUT_FOR_ID = 12,
    MEMORY_WIFI_STEP_END_SCENE_FOR_ID = 13,
    MEMORY_WIFI_STEP_ENTER_ID = 14,
    MEMORY_WIFI_STEP_START_WIFI_SCENE = 15,
    MEMORY_WIFI_STEP_FADE_IN_AFTER_ID = 16,
    MEMORY_WIFI_STEP_CHECK_ID = 17,
    MEMORY_WIFI_STEP_DOWNLOAD_SAVE = 18,
    MEMORY_WIFI_STEP_WAIT_DOWNLOAD = 19,
    MEMORY_WIFI_STEP_CONVERT_SAVE = 20,
    MEMORY_WIFI_STEP_CHECK_LINKED_PLAYER = 21,
    MEMORY_WIFI_STEP_PRINT_REPLACE_PROMPT = 22,
    MEMORY_WIFI_STEP_CREATE_REPLACE_MENU = 23,
    MEMORY_WIFI_STEP_SELECT_REPLACE = 24,
    MEMORY_WIFI_STEP_MERGE = 25,
    MEMORY_WIFI_STEP_SAVE = 26,
    MEMORY_WIFI_STEP_PRINT_RESULT = 27,
    MEMORY_WIFI_STEP_PRINT_INVALID_ID = 28,
    MEMORY_WIFI_STEP_WAIT_RETRY_MESSAGE = 29,
    MEMORY_WIFI_STEP_PRINT_CANCELED = 30,
    MEMORY_WIFI_STEP_PRINT_WRONG_GAME = 31,
    MEMORY_WIFI_STEP_PRINT_NEEDS_GAME_SYNC = 32,
    MEMORY_WIFI_STEP_WAIT_FINAL_MESSAGE = 33,
    MEMORY_WIFI_STEP_FADE_OUT_FOR_LOGOUT = 34,
    MEMORY_WIFI_STEP_END_SCENE_FOR_LOGOUT = 35,
    MEMORY_WIFI_STEP_LOGOUT = 36,
    MEMORY_WIFI_STEP_START_MENU_SCENE = 37,
    MEMORY_WIFI_STEP_FADE_IN_MENU = 38,
    MEMORY_WIFI_STEP_WAIT_BEFORE_END = 39,
    MEMORY_WIFI_STEP_END = 40,
    MEMORY_WIFI_STEP_PRINT_SERVICE_ENDED = 41,
    MEMORY_WIFI_STEP_WAIT_SERVICE_MESSAGE = 42,
};

// Phases of DataConvert_SeqSave.
enum {
    MEMORY_SAVE_STEP_PRINT_WAIT = 0,
    MEMORY_SAVE_STEP_WAIT_PRINT = 1,
    MEMORY_SAVE_STEP_START_WRITE = 2,
    MEMORY_SAVE_STEP_PLAY_SOUND = 3,
    MEMORY_SAVE_STEP_NEXT_FRAME = 4,
    MEMORY_SAVE_STEP_PRINT_DONE = 5,
    MEMORY_SAVE_STEP_WAIT_DONE = 6,
    MEMORY_SAVE_STEP_END = 7,
};

// Phases of DataConvert_SeqSaveOther.
enum {
    MEMORY_SAVE_LINK_STEP_START = 0,
    MEMORY_SAVE_LINK_STEP_WAIT_WRITE = 1,
    MEMORY_SAVE_LINK_STEP_WAIT_DELAY = 2,
    MEMORY_SAVE_LINK_STEP_END = 3,
};

// Phases of DataConvert_SeqSaveGame.
enum {
    MEMORY_SAVE_GAME_STEP_START = 0,
    MEMORY_SAVE_GAME_STEP_WAIT_WRITE = 1,
    MEMORY_SAVE_GAME_STEP_NEXT_FRAME = 2,
    MEMORY_SAVE_GAME_STEP_END = 3,
};

// Phases of DataConvert_SeqSaveBoth.
enum {
    MEMORY_SAVE_BOTH_STEP_SAVE_GAME = 0,
    MEMORY_SAVE_BOTH_STEP_SAVE_LINK = 1,
    MEMORY_SAVE_BOTH_STEP_END = 2,
};

// Phases of DataConvert_SeqMemoryList.
enum {
    MEMORY_LIST_STEP_START_SCENE = 0,
    MEMORY_LIST_STEP_INPUT = 1,
    MEMORY_LIST_STEP_END_SCENE = 2,
    MEMORY_LIST_STEP_DISPATCH = 3,
    MEMORY_LIST_STEP_END = 4,
};

// Phases of DataConvert_SeqMemoryInfo.
enum {
    MEMORY_INFO_STEP_START_SCENE = 0,
    MEMORY_INFO_STEP_PRINT = 1,
    MEMORY_INFO_STEP_WAIT_PRINT = 2,
    MEMORY_INFO_STEP_END_SCENE = 3,
    MEMORY_INFO_STEP_END = 4,
};

// Phases of DataConvert_SeqWifiLogin.
enum {
    MEMORY_LOGIN_STEP_START = 0,
    MEMORY_LOGIN_STEP_NEXT_FRAME = 1,
    MEMORY_LOGIN_STEP_RESTORE_GRAPHICS = 2,
    MEMORY_LOGIN_STEP_END = 3,
};

// Phases of DataConvert_SeqWifiLogout.
enum {
    MEMORY_LOGOUT_STEP_START = 0,
    MEMORY_LOGOUT_STEP_NEXT_FRAME = 1,
    MEMORY_LOGOUT_STEP_RESTORE_GRAPHICS = 2,
    MEMORY_LOGOUT_STEP_END = 3,
};

// Phases of DataConvert_SeqEnterId.
enum {
    MEMORY_ENTER_ID_STEP_START = 0,
    MEMORY_ENTER_ID_STEP_NEXT_FRAME = 1,
    MEMORY_ENTER_ID_STEP_READ_RESULT = 2,
    MEMORY_ENTER_ID_STEP_END = 3,
};

// Memory Link: receiving a Black or White save over DS Wireless Communications, where the other system runs the
// download play program, or by its Game Sync ID over Nintendo Wi-Fi Connection, and the list of the memories it
// unlocks. Named after the ROM's assertion string

// The Memory Link menu
enum {
    LINK_MENU_WIRELESS,
    LINK_MENU_WIFI,
    LINK_MENU_MEMORIES,
    LINK_MENU_BACK,
};

#define MEMORY_COUNT 8
// What the list of memories returns for its last item
#define MEMORY_LIST_BACK 0xff
// The characters of a Game Sync ID, which each give 5 bits
#define GAME_SYNC_ID_LENGTH 10
// A Black or White save as Game Sync downloads it: a header, and then the save
#define WB_SAVE_HEADER_SIZE 0x80
#define WB_SAVE_SIZE (WB_SAVE_HEADER_SIZE + 0x80000)

static void DataConvert_SeqWireless(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqWifi(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqSave(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqSaveOther(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqSaveGame(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqSaveBoth(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqMemoryList(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqMemoryInfo(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartLinkScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartWifiScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartMemoryListScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartMsgScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqWifiLogin(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqWifiLogout(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqEnterId(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_MenuSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_MenuSceneMain(void *work);
static void DataConvert_MenuSceneExit(void *work);
static void DataConvert_LinkSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_LinkSceneMain(void *work);
static void DataConvert_LinkSceneExit(void *work);
static void DataConvert_WifiSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_WifiSceneMain(void *work);
static void DataConvert_WifiSceneExit(void *work);
static void DataConvert_MemoryListSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_MemoryListSceneMain(void *work);
static void DataConvert_MemoryListSceneExit(void *work);
static void DataConvert_MsgSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_MsgSceneMain(void *work);
static void DataConvert_MsgSceneExit(void *work);
static void DataConvert_OnNetError(void *work);
static BOOL DataConvert_NeedsGameSave(KeySystemWork *wk);
static BOOL DataConvert_IsMemoryOpen(KeySystemWork *wk, u32 memory);
static BOOL DataConvert_HasMemories(KeySystemWork *wk);
static BOOL DataConvert_IsMemorySeen(KeySystemWork *wk, u32 memory);

static const KeySystemSceneFuncs sDataConvertWifiSceneFuncs = {
    DataConvert_WifiSceneInit,
    DataConvert_WifiSceneMain,
    DataConvert_WifiSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDataConvertMenuSceneFuncs = {
    DataConvert_MenuSceneInit,
    DataConvert_MenuSceneMain,
    DataConvert_MenuSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDataConvertLinkSceneFuncs = {
    DataConvert_LinkSceneInit,
    DataConvert_LinkSceneMain,
    DataConvert_LinkSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDataConvertMsgSceneFuncs = {
    DataConvert_MsgSceneInit,
    DataConvert_MsgSceneMain,
    DataConvert_MsgSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDataConvertMemoryListSceneFuncs = {
    DataConvert_MemoryListSceneInit,
    DataConvert_MemoryListSceneMain,
    DataConvert_MemoryListSceneExit,
    NULL,
};

// A Game Sync ID's characters: the letters without I and O, and the digits from 2
static const u16 sIdChars[32] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7', '8', '9',
};

void DataConvert_Init(KeySystemWork *wk, HeapID heapId) {
}

void DataConvert_SeqMenu(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    u32 item;

    switch (*state) {
    case MEMORY_MENU_STEP_START_SCENE:
        KeySystemSeq_Push(seq, DataConvert_SeqStartMenuScene);
        (*state)++;
        break;
    case MEMORY_MENU_STEP_INPUT:
        KeySystemList_Update(wk->list);
        if (KeySystemList_IsChanged(wk->list) && KeySystemMsgWin_IsDone(wk->infoWin)) {
            item = KeySystemList_GetCursor(wk->list);
            // Without the memory list, the third item is BACK
            if (!DataConvert_HasMemories(wk) && item == LINK_MENU_MEMORIES) {
                item = LINK_MENU_BACK;
            }
            KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, item + 80, KEY_SYSTEM_MSG_PRINT);
        }
        if (KeySystemList_IsDecided(wk->list)) {
            wk->choice = KeySystemList_GetCursor(wk->list);
            if (DataConvert_HasMemories(wk)) {
                if (wk->choice < LINK_MENU_BACK) {
                    KeySystemTags_Set(wk->tags, "DNM", KeySystemList_GetCursor(wk->list));
                }
            } else if (wk->choice < LINK_MENU_MEMORIES) {
                KeySystemTags_Set(wk->tags, "DNM", KeySystemList_GetCursor(wk->list));
            }
            (*state)++;
        }
        break;
    case MEMORY_MENU_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case MEMORY_MENU_STEP_DISPATCH:
        if (DataConvert_HasMemories(wk)) {
            switch (wk->choice) {
            case LINK_MENU_WIRELESS:
                KeySystemSeq_Push(seq, DataConvert_SeqWireless);
                *state = MEMORY_MENU_STEP_START_SCENE;
                break;
            case LINK_MENU_WIFI:
                KeySystemSeq_Push(seq, DataConvert_SeqWifi);
                *state = MEMORY_MENU_STEP_START_SCENE;
                break;
            case LINK_MENU_MEMORIES:
                KeySystemSeq_Push(seq, DataConvert_SeqMemoryList);
                *state = MEMORY_MENU_STEP_START_SCENE;
                break;
            case LINK_MENU_BACK:
                (*state)++;
                break;
            }
        } else {
            switch (wk->choice) {
            case LINK_MENU_WIRELESS:
                KeySystemSeq_Push(seq, DataConvert_SeqWireless);
                *state = MEMORY_MENU_STEP_START_SCENE;
                break;
            case LINK_MENU_WIFI:
                KeySystemSeq_Push(seq, DataConvert_SeqWifi);
                *state = MEMORY_MENU_STEP_START_SCENE;
                break;
            case LINK_MENU_MEMORIES:
                (*state)++;
                break;
            }
        }
        break;
    case MEMORY_MENU_STEP_END:
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_0_TO_2, 30);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqWireless(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    s32 result;
    BOOL hasSave;
    StrBuf *str;

    switch (*state) {
    case MEMORY_WIRELESS_STEP_CHECK_WIRELESS:
        if (isWirelessEnabled()) {
            *state = MEMORY_WIRELESS_STEP_PRINT_PROMPT;
        } else {
            KeySystemSeq_Push(seq, KeySystem_SeqWirelessOff);
            *state = MEMORY_WIRELESS_STEP_END;
        }
        break;
    case MEMORY_WIRELESS_STEP_PRINT_PROMPT:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        GFL_BGSysQueueScrLoad(0);
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_LaunchDsWirelessCommunications,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case MEMORY_WIRELESS_STEP_WAIT_PROMPT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystem_CreateYesNoMenu(wk, HEAPID_KEY_SYSTEM);
            (*state)++;
        }
        break;
    case MEMORY_WIRELESS_STEP_WAIT_MENU_DRAW:
        if (KeySystemMenu_UpdatePrint(wk->menu)) {
            GFL_BGSysQueueScrLoad(0);
            (*state)++;
        }
        break;
    case MEMORY_WIRELESS_STEP_SELECT:
        result = KeySystemMenu_Update(wk->menu);
        if (result == BMPMENULIST_NULL) {
            break;
        }
        KeySystem_FreeMenu(wk);
        KeySystem_FreeMsgWin(wk);
        switch (result) {
        case KEY_SYSTEM_YES_NO_YES:
            (*state)++;
            break;
        case KEY_SYSTEM_YES_NO_NO:
        default:
            *state = MEMORY_WIRELESS_STEP_END;
            break;
        }
        break;
    case MEMORY_WIRELESS_STEP_START_LINK_SCENE:
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_OV181);
        wk->wbData = NULL;
        hasSave = SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData));
        if (wk->msgStrs[0] != NULL) {
            GFL_StrBufFree(wk->msgStrs[0]);
            wk->msgStrs[0] = NULL;
        }
        if (wk->msgStrs[1] != NULL) {
            GFL_StrBufFree(wk->msgStrs[1]);
            wk->msgStrs[1] = NULL;
        }
        wk->msgStrs[0] = GFL_StrBufCreate(128, HEAPID_KEY_SYSTEM);
#ifdef BLACK2
        wk->msgStrs[1] = GFL_MsgDataLoadStrbufNew(wk->msgData, AbilityHandlers2_Text_PokemonBlackVersion2);
#else
        wk->msgStrs[1] = GFL_MsgDataLoadStrbufNew(wk->msgData, AbilityHandlers2_Text_PokemonWhiteVersion2);
#endif
        if (hasSave) {
            WordSetNumber(wk->wordSet, 0, getTrainerID(GetGameDataPlayerInfo(wk->param->gameData)), 5, 2, TRUE);
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, AbilityHandlers2_Text_MemoryLinkId);
        } else {
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, AbilityHandlers2_Text_MemoryLink_3);
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->msgStrs[0], str);
        GFL_StrBufFree(str);
        KeySystemSeq_Push(seq, DataConvert_SeqStartLinkScene);
        (*state)++;
        break;
    case MEMORY_WIRELESS_STEP_DISTRIBUTE: {
        KeySystemNetOv181Start start = { NULL, NULL, KEY_NET_DISTRIBUTION_RESULT_UNSET };

        start.text = wk->msgStrs[0];
        start.title = wk->msgStrs[1];
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_OV181_START, &start);
        (*state)++;
        break;
    }
    case MEMORY_WIRELESS_STEP_WAIT_DISTRIBUTION:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            if (KeySystemNet_GetRequest(wk->net)->ov181.result == KEY_NET_DISTRIBUTION_RESULT_COMPLETED) {
                *state = MEMORY_WIRELESS_STEP_CONNECT;
            } else {
                *state = MEMORY_WIRELESS_STEP_END_DISTRIBUTION;
            }
        }
        break;
    case MEMORY_WIRELESS_STEP_END_DISTRIBUTION:
        *state = MEMORY_WIRELESS_STEP_END_SCENE;
        break;
    case MEMORY_WIRELESS_STEP_CONNECT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ActivatingMemoryLinkPlease,
                                 KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        {
            KeySystemNetOv181End end = { FALSE, 0, NULL };

            KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_OV181_END, &end);
        }
        (*state)++;
        break;
    case MEMORY_WIRELESS_STEP_WAIT_CONNECTION:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            (*state)++;
        }
        break;
    case MEMORY_WIRELESS_STEP_READ_RESULT: {
        KeySystemNetRequest *request = KeySystemNet_GetRequest(wk->net);

        switch (request->ov181End.result) {
        case MB_DATACONV_RESULT_OK:
            wk->wbData = request->ov181End.data;
            *state = MEMORY_WIRELESS_STEP_CHECK_LINKED_PLAYER;
            break;
        case MB_DATACONV_RESULT_TIMEOUT:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_UnableConnectOtherDs,
                                     KEY_SYSTEM_MSG_STREAM);
            *state = MEMORY_WIRELESS_STEP_WAIT_ERROR;
            break;
        case MB_DATACONV_RESULT_CHILD_ERROR_2:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ErrorReadingSaveFile,
                                     KEY_SYSTEM_MSG_STREAM);
            *state = MEMORY_WIRELESS_STEP_WAIT_ERROR;
            break;
        case MB_DATACONV_RESULT_CHILD_ERROR_1:
        case MB_DATACONV_RESULT_CHILD_ERROR_3:
        default:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_GameInsertedNotCompatible,
                                     KEY_SYSTEM_MSG_STREAM);
            *state = MEMORY_WIRELESS_STEP_WAIT_ERROR;
            break;
        }
        break;
    }
    case MEMORY_WIRELESS_STEP_CHECK_LINKED_PLAYER:
        if (MemoryLinkSave_CanMerge(wk->memoryLink, wk->wbData, GameData_GetSaveControl(wk->param->gameData))) {
            *state = MEMORY_WIRELESS_STEP_MERGE;
        } else {
            *state = MEMORY_WIRELESS_STEP_PRINT_REPLACE_PROMPT;
        }
        break;
    case MEMORY_WIRELESS_STEP_PRINT_REPLACE_PROMPT:
        KeySystemMsgWin_Free(wk->msgWin);
        wk->msgWin = KeySystemMsgWin_Create(0, 2, 4, 28, 12, 14, wk->font, HEAPID_KEY_SYSTEM);
        KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
        KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
        GFL_BGSysQueueScrLoad(0);
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_SavedGameFileTrying,
                                 KEY_SYSTEM_MSG_STREAM);
        *state = MEMORY_WIRELESS_STEP_CREATE_REPLACE_MENU;
        break;
    case MEMORY_WIRELESS_STEP_CREATE_REPLACE_MENU:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystemMenuSetup setup;

            MI_CpuFill8(&setup, 0, sizeof(KeySystemMenuSetup));
            setup.msgData = wk->msgData;
            setup.font = wk->font;
            setup.printQueue = wk->printQueue;
            setup.bg = 0;
            setup.palette = 14;
            setup.framePalette = 15;
            setup.frameChar = 1;
            setup.msgIds[KEY_SYSTEM_YES_NO_YES] = AbilityHandlers2_Text_Yes;
            setup.msgIds[KEY_SYSTEM_YES_NO_NO] = AbilityHandlers2_Text_No;
            setup.count = KEY_SYSTEM_YES_NO_COUNT;
            setup.cancelable = TRUE;
            setup.cancelValue = KEY_SYSTEM_YES_NO_NO;
            setup.cursor = 0;
            wk->menu = KeySystemMenu_CreateAt(&setup, 21, 19, 10, 4, HEAPID_KEY_SYSTEM);
            GFL_BGSysQueueScrLoad(0);
            (*state)++;
        }
        break;
    case MEMORY_WIRELESS_STEP_SELECT_REPLACE:
        result = KeySystemMenu_Update(wk->menu);
        if (result == BMPMENULIST_NULL) {
            break;
        }
        KeySystemMenu_Free(wk->menu);
        wk->menu = NULL;
        switch (result) {
        case KEY_SYSTEM_YES_NO_YES:
            (*state)++;
            break;
        case KEY_SYSTEM_YES_NO_NO:
        default:
            *state = MEMORY_WIRELESS_STEP_PRINT_CANCELED;
            break;
        }
        break;
    case MEMORY_WIRELESS_STEP_MERGE:
        wk->transferResult =
            MemoryLinkSave_Merge(wk->memoryLink, wk->wbData, GameData_GetSaveControl(wk->param->gameData));
        (*state)++;
        break;
    case MEMORY_WIRELESS_STEP_SAVE:
        KeySystemSeq_Push(seq, DataConvert_SeqSave);
        (*state)++;
        break;
    case MEMORY_WIRELESS_STEP_PRINT_RESULT:
        if (wk->transferResult) {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_MemoryLinkSuccessfulSomething,
                                     KEY_SYSTEM_MSG_STREAM_FAST);
        } else {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_MemoryLinkSuccessful,
                                     KEY_SYSTEM_MSG_STREAM_FAST);
        }
        *state = MEMORY_WIRELESS_STEP_WAIT_RESULT;
        break;
    case MEMORY_WIRELESS_STEP_PRINT_CANCELED:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_MemoryLinkCanceledMemory,
                                 KEY_SYSTEM_MSG_STREAM_FAST);
        *state = MEMORY_WIRELESS_STEP_WAIT_RESULT;
        break;
    case MEMORY_WIRELESS_STEP_WAIT_RESULT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = MEMORY_WIRELESS_STEP_END_SCENE;
        }
        break;
    case MEMORY_WIRELESS_STEP_WAIT_ERROR:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = MEMORY_WIRELESS_STEP_END_SCENE;
        }
        break;
    case MEMORY_WIRELESS_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_NONE);
        (*state)++;
        break;
    case MEMORY_WIRELESS_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqWifi(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    s32 result;
    u64 value;
    const u16 *chars;

    switch (*state) {
    case MEMORY_WIFI_STEP_FADE_OUT:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_NEXT_FRAME:
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_LOGIN:
        KeySystemSeq_Push(seq, DataConvert_SeqWifiLogin);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_START_SCENE_AFTER_LOGIN:
        if (wk->loggedIn) {
            wk->transferResult = FALSE;
            KeySystemNet_SetBuffer(wk->net, wk->loginBuffer);
            KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_WIFI);
            KeySystemSeq_Push(seq, DataConvert_SeqStartWifiScene);
        } else {
            KeySystemSeq_Push(seq, DataConvert_SeqStartMenuScene);
        }
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_FADE_IN:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_CHECK_LOGIN:
        if (wk->loggedIn) {
            *state = MEMORY_WIFI_STEP_PRINT_ID_PROMPT;
        } else {
            *state = MEMORY_WIFI_STEP_END;
        }
        break;
    // Nothing goes to states 6 to 9
    case MEMORY_WIFI_STEP_PRINT_STATUS_WAIT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_CommunicatingPleaseStandBy_2,
                                 KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_WAIT_BEFORE_STATUS:
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_QUERY_STATUS: {
        KeySystemNetWifiGet get = { 0, KEY_NET_WIFI_RESULT_UNSET };

        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_WIFI_GET, &get);
        (*state)++;
        break;
    }
    case MEMORY_WIFI_STEP_WAIT_STATUS:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            KeySystemNetRequest *request = KeySystemNet_GetRequest(wk->net);

            switch (request->wifiGet.result) {
            case KEY_NET_WIFI_RESULT_HTTP_ERROR:
            case KEY_NET_WIFI_RESULT_SERVER_ERROR:
                GFL_NetErrShowScreen();
                *state = MEMORY_WIFI_STEP_END;
                break;
            default:
                if (request->wifiGet.playStatus == 4) {
                    *state = MEMORY_WIFI_STEP_PRINT_SERVICE_ENDED;
                } else {
                    (*state)++;
                }
                break;
            }
        }
        break;
    case MEMORY_WIFI_STEP_PRINT_ID_PROMPT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_PleaseEnterGameSync,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_WAIT_ID_PROMPT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystemNet_SetNoErrorCheck(wk->net, TRUE);
            (*state)++;
        }
        break;
    case MEMORY_WIFI_STEP_FADE_OUT_FOR_ID:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_END_SCENE_FOR_ID:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_ENTER_ID:
        KeySystemSeq_Push(seq, DataConvert_SeqEnterId);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_START_WIFI_SCENE:
        KeySystemSeq_Push(seq, DataConvert_SeqStartWifiScene);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_FADE_IN_AFTER_ID:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_CHECK_ID:
        KeySystemNet_SetNoErrorCheck(wk->net, FALSE);
        if (wk->enteredCode == NULL) {
            *state = MEMORY_WIFI_STEP_PRINT_CANCELED;
            break;
        }
        wk->wbSave = GFL_HeapAllocate(HEAPID_USER, WB_SAVE_SIZE, TRUE, "data_convert_flow.c", 849);
        *state = MEMORY_WIFI_STEP_DOWNLOAD_SAVE;
        break;
    case MEMORY_WIFI_STEP_DOWNLOAD_SAVE:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_LinkingSavedGameFile,
                                 KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        {
            KeySystemNetWifiPost post = { 0, KEY_NET_WIFI_RESULT_UNSET, NULL, 0 };
            u64 id = 0;
            int i;
            int j;

            chars = GFL_StrBufGetStringPtr(wk->enteredCode);
            for (i = 0; i < GAME_SYNC_ID_LENGTH; i++) {
                value = 0xffffffff;
                for (j = 0; j < NELEMS(sIdChars); j++) {
                    if (chars[i] == sIdChars[j]) {
                        value = j;
                        break;
                    }
                }
                id += value << (i * 5);
            }
            post.id = id & 0xffffffff;
            post.buffer = wk->wbSave;
            post.size = WB_SAVE_SIZE;
            KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_WIFI_POST, &post);
            KeySystemNet_SetErrorCallback(wk->net, DataConvert_OnNetError, wk);
        }
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_WAIT_DOWNLOAD:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            switch (KeySystemNet_GetRequest(wk->net)->wifi.result) {
            case KEY_NET_WIFI_RESULT_UNSET:
            case KEY_NET_WIFI_RESULT_REQUEST_ERROR:
                break;
            case KEY_NET_WIFI_RESULT_HTTP_ERROR:
            case KEY_NET_WIFI_RESULT_SERVER_ERROR:
                GFL_NetErrShowScreen();
                *state = MEMORY_WIFI_STEP_END;
                break;
            case KEY_NET_WIFI_RESULT_OK:
                *state = MEMORY_WIFI_STEP_CONVERT_SAVE;
                break;
            case KEY_NET_WIFI_RESULT_WRONG_GAME:
                if (wk->wbSave != NULL) {
                    GFL_HeapFree(wk->wbSave);
                    wk->wbSave = NULL;
                }
                *state = MEMORY_WIFI_STEP_PRINT_WRONG_GAME;
                break;
            case KEY_NET_WIFI_RESULT_INVALID_ID:
                if (wk->wbSave != NULL) {
                    GFL_HeapFree(wk->wbSave);
                    wk->wbSave = NULL;
                }
                *state = MEMORY_WIFI_STEP_PRINT_INVALID_ID;
                break;
            case KEY_NET_WIFI_RESULT_NEEDS_GAME_SYNC:
                *state = MEMORY_WIFI_STEP_PRINT_NEEDS_GAME_SYNC;
                break;
            }
        }
        break;
    case MEMORY_WIFI_STEP_CONVERT_SAVE:
        if (wk->wbData != NULL) {
            GFL_HeapFree(wk->wbData);
        }
        wk->wbData = WBSaveConvert_Create((u8 *)wk->wbSave + WB_SAVE_HEADER_SIZE, HEAPID_KEY_SYSTEM);
        GFL_HeapFree(wk->wbSave);
        wk->wbSave = NULL;
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_CHECK_LINKED_PLAYER:
        if (MemoryLinkSave_CanMerge(wk->memoryLink, wk->wbData, GameData_GetSaveControl(wk->param->gameData))) {
            *state = MEMORY_WIFI_STEP_MERGE;
        } else {
            *state = MEMORY_WIFI_STEP_PRINT_REPLACE_PROMPT;
        }
        break;
    case MEMORY_WIFI_STEP_PRINT_REPLACE_PROMPT:
        KeySystemMsgWin_Free(wk->msgWin);
        wk->msgWin = KeySystemMsgWin_Create(0, 2, 4, 28, 12, 14, wk->font, HEAPID_KEY_SYSTEM);
        KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
        KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
        GFL_BGSysQueueScrLoad(0);
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_SavedGameFileTrying,
                                 KEY_SYSTEM_MSG_STREAM);
        *state = MEMORY_WIFI_STEP_CREATE_REPLACE_MENU;
        break;
    case MEMORY_WIFI_STEP_CREATE_REPLACE_MENU:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystemMenuSetup setup;

            MI_CpuFill8(&setup, 0, sizeof(KeySystemMenuSetup));
            setup.msgData = wk->msgData;
            setup.font = wk->font;
            setup.printQueue = wk->printQueue;
            setup.bg = 0;
            setup.palette = 14;
            setup.framePalette = 15;
            setup.frameChar = 1;
            setup.msgIds[KEY_SYSTEM_YES_NO_YES] = AbilityHandlers2_Text_Yes;
            setup.msgIds[KEY_SYSTEM_YES_NO_NO] = AbilityHandlers2_Text_No;
            setup.count = KEY_SYSTEM_YES_NO_COUNT;
            setup.cancelable = TRUE;
            setup.cancelValue = KEY_SYSTEM_YES_NO_NO;
            setup.cursor = 0;
            wk->menu = KeySystemMenu_CreateAt(&setup, 21, 19, 10, 4, HEAPID_KEY_SYSTEM);
            GFL_BGSysQueueScrLoad(0);
            (*state)++;
        }
        break;
    case MEMORY_WIFI_STEP_SELECT_REPLACE:
        result = KeySystemMenu_Update(wk->menu);
        if (result == BMPMENULIST_NULL) {
            break;
        }
        KeySystemMenu_Free(wk->menu);
        wk->menu = NULL;
        switch (result) {
        case KEY_SYSTEM_YES_NO_YES:
            (*state)++;
            break;
        case KEY_SYSTEM_YES_NO_NO:
        default:
            *state = MEMORY_WIFI_STEP_PRINT_CANCELED;
            break;
        }
        break;
    case MEMORY_WIFI_STEP_MERGE:
        wk->transferResult =
            MemoryLinkSave_Merge(wk->memoryLink, wk->wbData, GameData_GetSaveControl(wk->param->gameData));
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_SAVE:
        KeySystemSeq_Push(seq, DataConvert_SeqSave);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_PRINT_RESULT:
        if (wk->transferResult) {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_MemoryLinkSuccessfulSomething,
                                     KEY_SYSTEM_MSG_STREAM_FAST);
        } else {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_MemoryLinkSuccessful,
                                     KEY_SYSTEM_MSG_STREAM_FAST);
        }
        *state = MEMORY_WIFI_STEP_WAIT_FINAL_MESSAGE;
        break;
    case MEMORY_WIFI_STEP_PRINT_INVALID_ID:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_GameSyncIdInvalid,
                                 KEY_SYSTEM_MSG_STREAM);
        *state = MEMORY_WIFI_STEP_WAIT_RETRY_MESSAGE;
        break;
    case MEMORY_WIFI_STEP_WAIT_RETRY_MESSAGE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = MEMORY_WIFI_STEP_PRINT_ID_PROMPT;
        }
        break;
    case MEMORY_WIFI_STEP_PRINT_CANCELED:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_MemoryLinkCanceledMemory,
                                 KEY_SYSTEM_MSG_STREAM);
        *state = MEMORY_WIFI_STEP_WAIT_FINAL_MESSAGE;
        break;
    case MEMORY_WIFI_STEP_PRINT_WRONG_GAME:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_GameSyncIdNot, KEY_SYSTEM_MSG_STREAM);
        *state = MEMORY_WIFI_STEP_WAIT_RETRY_MESSAGE;
        break;
    case MEMORY_WIFI_STEP_PRINT_NEEDS_GAME_SYNC:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_PleaseUseGameSync,
                                 KEY_SYSTEM_MSG_STREAM);
        *state = MEMORY_WIFI_STEP_WAIT_FINAL_MESSAGE;
        break;
    case MEMORY_WIFI_STEP_WAIT_FINAL_MESSAGE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = MEMORY_WIFI_STEP_FADE_OUT_FOR_LOGOUT;
        }
        break;
    case MEMORY_WIFI_STEP_FADE_OUT_FOR_LOGOUT:
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_NONE);
        if (wk->wbSave != NULL) {
            GFL_HeapFree(wk->wbSave);
            wk->wbSave = NULL;
        }
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_END_SCENE_FOR_LOGOUT:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_LOGOUT:
        KeySystemSeq_Push(seq, DataConvert_SeqWifiLogout);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_START_MENU_SCENE:
        KeySystemSeq_Push(seq, DataConvert_SeqStartMenuScene);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_FADE_IN_MENU:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_WAIT_BEFORE_END:
        (*state)++;
        break;
    case MEMORY_WIFI_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    case MEMORY_WIFI_STEP_PRINT_SERVICE_ENDED:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ServiceNoLongerAvailable,
                                 KEY_SYSTEM_MSG_STREAM);
        // BUG: The message starts again every frame, and the next state is never reached. It never shows, since nothing
        // goes to states 6 to 9, which lead here
#ifdef BUGFIX
        *state = MEMORY_WIFI_STEP_WAIT_SERVICE_MESSAGE;
#else
        *state = MEMORY_WIFI_STEP_PRINT_SERVICE_ENDED;
#endif
        break;
    case MEMORY_WIFI_STEP_WAIT_SERVICE_MESSAGE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = MEMORY_WIFI_STEP_END;
        }
        break;
    }
}

static void DataConvert_SeqSave(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case MEMORY_SAVE_STEP_PRINT_WAIT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_SavingDontTurnOff,
                                 KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case MEMORY_SAVE_STEP_WAIT_PRINT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case MEMORY_SAVE_STEP_START_WRITE:
        if (DataConvert_NeedsGameSave(wk)) {
            KeySystemSeq_Push(seq, DataConvert_SeqSaveBoth);
        } else {
            KeySystemSeq_Push(seq, DataConvert_SeqSaveOther);
        }
        (*state)++;
        break;
    case MEMORY_SAVE_STEP_PLAY_SOUND:
        GFL_SndSEPlay(SEQ_SE_SAVE);
        *state = MEMORY_SAVE_STEP_NEXT_FRAME;
        break;
    case MEMORY_SAVE_STEP_NEXT_FRAME:
        *state = MEMORY_SAVE_STEP_PRINT_DONE;
        break;
    case MEMORY_SAVE_STEP_PRINT_DONE:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_WritingSavedGameFile,
                                 KEY_SYSTEM_MSG_STREAM_FAST);
        *state = MEMORY_SAVE_STEP_WAIT_DONE;
        break;
    case MEMORY_SAVE_STEP_WAIT_DONE:
        if (KeySystemMsgWin_IsDone(wk->msgWin) && !GFL_SndPlayerIsActiveAny()) {
            *state = MEMORY_SAVE_STEP_END;
        }
        break;
    case MEMORY_SAVE_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

// Writes ov331's save, which holds the Memory Link
static void DataConvert_SeqSaveOther(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case MEMORY_SAVE_LINK_STEP_START:
        SaveOutside_StartSave(wk->ov331Work);
        wk->timer = 0;
        (*state)++;
        break;
    case MEMORY_SAVE_LINK_STEP_WAIT_WRITE:
        wk->timer++;
        if (SaveOutside_Save(wk->ov331Work)) {
            (*state)++;
        }
        break;
    case MEMORY_SAVE_LINK_STEP_WAIT_DELAY:
        if (wk->timer++ > 60) {
            wk->timer = 0;
            (*state)++;
        }
        break;
    case MEMORY_SAVE_LINK_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqSaveGame(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case MEMORY_SAVE_GAME_STEP_START:
        GameData_StartSave(wk->param->gameData);
        (*state)++;
        break;
    case MEMORY_SAVE_GAME_STEP_WAIT_WRITE:
        if (GameData_UpdateSave(wk->param->gameData) == SAVE_RESULT_OK) {
            (*state)++;
        }
        break;
    case MEMORY_SAVE_GAME_STEP_NEXT_FRAME:
        (*state)++;
        break;
    case MEMORY_SAVE_GAME_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqSaveBoth(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case MEMORY_SAVE_BOTH_STEP_SAVE_GAME:
        KeySystemSeq_Push(seq, DataConvert_SeqSaveGame);
        (*state)++;
        break;
    case MEMORY_SAVE_BOTH_STEP_SAVE_LINK:
        KeySystemSeq_Push(seq, DataConvert_SeqSaveOther);
        (*state)++;
        break;
    case MEMORY_SAVE_BOTH_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqMemoryList(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    u32 cursor;
    u32 top;

    switch (*state) {
    case MEMORY_LIST_STEP_START_SCENE:
        KeySystemSeq_Push(seq, DataConvert_SeqStartMemoryListScene);
        (*state)++;
        break;
    case MEMORY_LIST_STEP_INPUT:
        wk->choice = KeySystemScrollList_Update(wk->scrollList);
        if (wk->choice == (u32)-1) {
            break;
        }
        if (wk->choice != MEMORY_LIST_BACK) {
            KeySystemScrollList_GetPos(wk->scrollList, &cursor, &top);
            KeySystemTags_Set(wk->tags, "DI1", cursor);
            KeySystemTags_Set(wk->tags, "DI2", top);
        }
        (*state)++;
        break;
    case MEMORY_LIST_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case MEMORY_LIST_STEP_DISPATCH:
        if (wk->choice == MEMORY_LIST_BACK) {
            (*state)++;
        } else {
            KeySystemSeq_Push(seq, DataConvert_SeqMemoryInfo);
            *state = MEMORY_LIST_STEP_START_SCENE;
        }
        break;
    case MEMORY_LIST_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqMemoryInfo(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case MEMORY_INFO_STEP_START_SCENE:
        KeySystemSeq_Push(seq, DataConvert_SeqStartMsgScene);
        (*state)++;
        break;
    case MEMORY_INFO_STEP_PRINT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, wk->choice + AbilityHandlers2_Text_ListenCressChiliCilans,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case MEMORY_INFO_STEP_WAIT_PRINT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case MEMORY_INFO_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case MEMORY_INFO_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sDataConvertMenuSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case UNOVA_SCENE_START_STEP_WAIT:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case UNOVA_SCENE_START_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartLinkScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sDataConvertLinkSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case UNOVA_SCENE_START_STEP_WAIT:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case UNOVA_SCENE_START_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartWifiScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sDataConvertWifiSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case UNOVA_SCENE_START_STEP_WAIT:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case UNOVA_SCENE_START_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartMemoryListScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sDataConvertMemoryListSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case UNOVA_SCENE_START_STEP_WAIT:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case UNOVA_SCENE_START_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartMsgScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sDataConvertMsgSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case UNOVA_SCENE_START_STEP_WAIT:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case UNOVA_SCENE_START_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

// Unova Link frees its graphics while the login runs, and sets them up again after
static void DataConvert_SeqWifiLogin(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    WifiLoginParam *param;

    switch (*state) {
    case MEMORY_LOGIN_STEP_START:
        KeySystem_Teardown(wk, FALSE);
        param = GFL_HeapAllocate(HEAPID_KEY_SYSTEM, sizeof(WifiLoginParam), TRUE, "data_convert_flow.c", 1705);
        param->gameData = wk->param->gameData;
        param->displayStyle = WIFILOGIN_DISP_STYLE_PDWACC;
        param->messageStyle = WIFILOGIN_MESSAGE_STYLE_GSYNC;
        param->buffer = wk->loginBuffer;
        param->keepBgm = 0;
        param->serviceId = WIFILOGIN_SERVICE_MEMORY_LINK;
        if (SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData))) {
            param->mode = WIFILOGIN_MODE_NORMAL;
        } else {
            param->mode = WIFILOGIN_MODE_NO_SAVE;
        }
        wk->subProcParam = param;
        GCTX_ProcMgrQueueProc(OVERLAY_ID(190), &WIFILOGIN_PROC_FUNCTIONS, param);
        (*state)++;
        break;
    case MEMORY_LOGIN_STEP_NEXT_FRAME:
        (*state)++;
        break;
    case MEMORY_LOGIN_STEP_RESTORE_GRAPHICS:
        if (((WifiLoginParam *)wk->subProcParam)->result == WIFILOGIN_RESULT_OK) {
            wk->loggedIn = TRUE;
        } else {
            wk->loggedIn = FALSE;
        }
        GFL_HeapFree(wk->subProcParam);
        wk->subProcParam = NULL;
        KeySystem_Setup(wk, HEAPID_KEY_SYSTEM);
        KeySystemBG_LoadScreen(wk->bg, 2, KEY_SYSTEM_SCREEN_COMMON);
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_2_TO_0, 1);
        GFL_NetSetIconScreen(TRUE, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case MEMORY_LOGIN_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqWifiLogout(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    WifiLogoutParam *param;

    switch (*state) {
    case MEMORY_LOGOUT_STEP_START:
        KeySystem_Teardown(wk, FALSE);
        param = GFL_HeapAllocate(HEAPID_KEY_SYSTEM, sizeof(WifiLogoutParam), TRUE, "data_convert_flow.c", 1789);
        param->gameData = wk->param->gameData;
        param->displayStyle = WIFILOGIN_DISP_STYLE_PDWACC;
        param->messageStyle = WIFILOGIN_MESSAGE_STYLE_GSYNC;
        param->keepBgm = 0;
        param->wipeFlags = 0;
        wk->subProcParam = param;
        GCTX_ProcMgrQueueProc(OVERLAY_ID(190), &WIFILOGOUT_PROC_FUNCTIONS, param);
        (*state)++;
        break;
    case MEMORY_LOGOUT_STEP_NEXT_FRAME:
        (*state)++;
        break;
    case MEMORY_LOGOUT_STEP_RESTORE_GRAPHICS:
        GFL_HeapFree(wk->subProcParam);
        wk->subProcParam = NULL;
        KeySystem_Setup(wk, HEAPID_KEY_SYSTEM);
        KeySystemBG_LoadScreen(wk->bg, 2, KEY_SYSTEM_SCREEN_COMMON);
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_2_TO_0, 1);
        (*state)++;
        break;
    case MEMORY_LOGOUT_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

// The player enters the Game Sync ID in the name entry
static void DataConvert_SeqEnterId(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    NameEntryParam *param;

    switch (*state) {
    case MEMORY_ENTER_ID_STEP_START:
        KeySystem_Teardown(wk, TRUE);
        param = GFL_HeapAllocate(HEAPID_KEY_SYSTEM, sizeof(NameEntryParam), TRUE, "data_convert_flow.c", 1848);
        param->mode = NAME_ENTRY_GAME_SYNC_ID;
        param->maxLength = GAME_SYNC_ID_LENGTH;
        param->skipChecks = NAME_ENTRY_SKIP_DIGIT_AND_PROFANITY_CHECKS;
        param->name = GFL_StrBufCreate(GAME_SYNC_ID_LENGTH + 1, HEAPID_KEY_SYSTEM);
        wk->subProcParam = param;
        GCTX_ProcMgrQueueProc(OVERLAY_ID(280), &NAME_ENTRY_PROC_FUNCTIONS, param);
        (*state)++;
        break;
    case MEMORY_ENTER_ID_STEP_NEXT_FRAME:
        (*state)++;
        break;
    case MEMORY_ENTER_ID_STEP_READ_RESULT:
        param = wk->subProcParam;
        if (param->canceled) {
            if (wk->enteredCode != NULL) {
                GFL_StrBufFree(wk->enteredCode);
                wk->enteredCode = NULL;
            }
        } else {
            if (wk->enteredCode == NULL) {
                wk->enteredCode = GFL_StrBufCreate(GAME_SYNC_ID_LENGTH + 1, HEAPID_KEY_SYSTEM);
            }
            GFL_StrBufCopy(wk->enteredCode, param->name);
        }
        GFL_StrBufFree(param->name);
        GFL_HeapFree(param);
        wk->subProcParam = NULL;
        KeySystem_Setup(wk, HEAPID_KEY_SYSTEM);
        KeySystemBG_LoadScreen(wk->bg, 2, KEY_SYSTEM_SCREEN_COMMON);
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_2_TO_0, 1);
        GFL_NetSetIconScreen(TRUE, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case MEMORY_ENTER_ID_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_MenuSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemListSetup setup;
    int i;

    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, AbilityHandlers2_Text_MemoryLink_2, KEY_SYSTEM_MSG_PRINT);
    MI_CpuFill8(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.count = 3;
    if (KeySystemTags_Has(wk->tags, "DNM")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "DNM");
        KeySystemTags_Remove(wk->tags, "DNM");
    } else {
        setup.cursor = 0;
    }
    for (i = 0; i < KEY_SYSTEM_LIST_MAX; i++) {
        setup.items[i].width = 26;
        setup.items[i].height = 2;
        setup.items[i].x = 3;
        setup.items[i].y = (setup.items[i].height + 3) * i + 5;
    }
    setup.items[0].msgId = AbilityHandlers2_Text_LinkViaDsWireless;
    setup.items[1].msgId = AbilityHandlers2_Text_LinkViaNintendoWfc;
    if (DataConvert_HasMemories(wk)) {
        setup.items[2].msgId = AbilityHandlers2_Text_MemoryList;
        setup.items[3].msgId = AbilityHandlers2_Text_Back_3;
        setup.count = 4;
    } else {
        setup.items[2].msgId = AbilityHandlers2_Text_Back_3;
    }
    wk->list = KeySystemList_Create(&setup, heapId);
    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, setup.cursor + 80, KEY_SYSTEM_MSG_PRINT);
}

static BOOL DataConvert_MenuSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone;
    BOOL listDone;

    // The title isn't waited for
    KeySystemMsgWin_IsDone(wk->titleWin);
    infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    listDone = KeySystemList_IsPrinted(wk->list);
    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_MenuSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemList_Free(wk->list);
    wk->list = NULL;
    KeySystemMsgWin_Clear(wk->titleWin);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

// What to do on the other system, with the Memory Link ID, and the picture of two systems
static void DataConvert_LinkSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    ArcTool *arc;

    wk->msgWin = KeySystemMsgWin_Create(0, 2, 4, 28, 16, 14, wk->font, HEAPID_KEY_SYSTEM);
    KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
    KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
    if (!SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData))) {
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_PleaseInsertPokemonBlack_2,
                                 KEY_SYSTEM_MSG_PRINT);
    } else {
        PlayerInfo *player = GetGameDataPlayerInfo(wk->param->gameData);
        StrBuf *src = GFL_MsgDataLoadStrbufNew(wk->msgData, AbilityHandlers2_Text_PleaseInsertPokemonBlack);
        StrBuf *dest = GFL_StrBufCreate(337, heapId);

        WordSetNumber(wk->wordSet, 0, getTrainerID(player), 5, 2, TRUE);
        GFL_WordSetFormatStrbuf(wk->wordSet, dest, src);
        KeySystemMsgWin_PrintStr(wk->msgWin, dest, KEY_SYSTEM_MSG_PRINT);
        GFL_StrBufFree(dest);
        GFL_StrBufFree(src);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_DATA_CONVERT, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 2, 4, 0x120, 0x120, 0x60, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 3, 4, 0x1a0, 0x1a0, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 4, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 6, 6, 0, 0x6000, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 11, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 10, 6, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

static BOOL DataConvert_LinkSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(4);
        GFL_BGSysQueueScrLoad(6);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_LinkSceneExit(void *work) {
    KeySystemWork *wk = work;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_KEY_SYSTEM, HEAPID_KEY_SYSTEM);

    GFL_BGSysLoadArcNCGRStatic(arc, 3, 6, 0, 0, FALSE, HEAPID_KEY_SYSTEM);
    GFL_ArcToolFree(arc);
    LoadSysMsgBox(4, 1, 15, 0, HEAPID_KEY_SYSTEM);
    KeySystem_FreeMsgWin(wk);
    GFL_BGSysClearScr(4);
    GFL_BGSysClearScr(6);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysQueueScrLoad(6);
}

static void DataConvert_WifiSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    wk->msgWin = KeySystemMsgWin_Create(0, 2, 4, 28, 16, 14, wk->font, HEAPID_KEY_SYSTEM);
    KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
    KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_Empty, KEY_SYSTEM_MSG_PRINT);
    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, AbilityHandlers2_Text_MemoryLink_2, KEY_SYSTEM_MSG_PRINT);
}

static BOOL DataConvert_WifiSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL msgDone = KeySystemMsgWin_IsDone(wk->msgWin);
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);

    if (msgDone && titleDone) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_WifiSceneExit(void *work) {
}

static void DataConvert_MemoryListSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemScrollListSetup setup;
    int i;

    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, AbilityHandlers2_Text_MemoryList_2, KEY_SYSTEM_MSG_PRINT);
    MI_CpuFill8(&setup, 0, sizeof(KeySystemScrollListSetup));
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.arrowUp = KeySystemClAct_Create(&wk->clact, KEY_SYSTEM_ACTOR_SCROLL_UP, heapId);
    setup.arrowDown = KeySystemClAct_Create(&wk->clact, KEY_SYSTEM_ACTOR_SCROLL_DOWN, heapId);
    for (i = 0; i < MEMORY_COUNT; i++) {
        if (DataConvert_IsMemoryOpen(wk, i)) {
            setup.msgIds[setup.count] = i + AbilityHandlers2_Text_TripleTeam;
            setup.values[setup.count] = i;
            if (DataConvert_IsMemorySeen(wk, i)) {
                setup.icons[setup.count] =
                    KeySystemClAct_Create(&wk->clact, i + KEY_SYSTEM_ACTOR_MEMORY_TRIPLE_TEAM, heapId);
            }
            setup.count++;
        }
    }
    setup.msgIds[setup.count] = AbilityHandlers2_Text_Back_4;
    setup.values[setup.count] = MEMORY_LIST_BACK;
    setup.icons[setup.count] = NULL;
    setup.count++;
    setup.bg = 1;
    setup.palette = 1;
    setup.frameChar = 10;
    if (KeySystemTags_Has(wk->tags, "DI1")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "DI1");
        setup.scroll = KeySystemTags_Get(wk->tags, "DI2");
        KeySystemTags_Remove(wk->tags, "DI1");
        KeySystemTags_Remove(wk->tags, "DI2");
    }
    wk->scrollList = KeySystemScrollList_Create(&setup, heapId);
    GX_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3);
    wk->timer = 0;
}

static BOOL DataConvert_MemoryListSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL started = KeySystemScrollList_Start(wk->scrollList);
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);

    // The list is shown two frames after it is drawn
    if (started && titleDone && wk->timer++ > 1) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(1);
        GX_SetVisiblePlane(GX_PLANEMASK_ALL);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_MemoryListSceneExit(void *work) {
    KeySystemWork *wk = work;
    int i;

    KeySystemScrollList_Free(wk->scrollList);
    wk->scrollList = NULL;
    for (i = KEY_SYSTEM_ACTOR_MEMORY_TRIPLE_TEAM; i < KEY_SYSTEM_ACTOR_MEMORY_TRIPLE_TEAM + MEMORY_COUNT; i++) {
        KeySystemClAct_Delete(&wk->clact, i);
    }
    KeySystemClAct_Delete(&wk->clact, KEY_SYSTEM_ACTOR_SCROLL_UP);
    KeySystemClAct_Delete(&wk->clact, KEY_SYSTEM_ACTOR_SCROLL_DOWN);
    GFL_BGSysClearScr(1);
    GFL_BGSysQueueScrLoad(1);
}

static void DataConvert_MsgSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateMsgWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_Empty, KEY_SYSTEM_MSG_PRINT);
}

static BOOL DataConvert_MsgSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_MsgSceneExit(void *work) {
    KeySystem_FreeMsgWin(work);
    GFL_BGSysQueueScrLoad(0);
}

static void DataConvert_OnNetError(void *work) {
    KeySystemWork *wk = work;

    KeySystemMsgWin_ClearFrame(wk->msgWin);
    GFL_BGSysLoadScr(0);
}

// Always FALSE: only ov331's save is written
static BOOL DataConvert_NeedsGameSave(KeySystemWork *wk) {
    SaveControl *save = GameData_GetSaveControl(wk->param->gameData);

    if (SaveControl_IsDataAlreadyPresent(save)) {
        getConstDataBlock(save);
        return FALSE;
    }
    return FALSE;
}

static BOOL DataConvert_IsMemoryOpen(KeySystemWork *wk, u32 memory) {
    if (MemoryLinkSave_CheckMemoryCondition(wk->memoryLink, memory) || DataConvert_IsMemorySeen(wk, memory)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL DataConvert_HasMemories(KeySystemWork *wk) {
    int i;

    for (i = 0; i < MEMORY_COUNT; i++) {
        if (DataConvert_IsMemoryOpen(wk, i)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL DataConvert_IsMemorySeen(KeySystemWork *wk, u32 memory) {
    EventWork *eventWork = getConstDataBlock(GameData_GetSaveControl(wk->param->gameData));

    if (*EventWork_GetWkPtr(eventWork, MemoryLink_GetMemoryEventWork(memory)) == 1) {
        return TRUE;
    }
    return FALSE;
}
