#include "types.h"
#include "app/key_system_flow.h"
#include "app/unova_link.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "field/zone.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/key_info.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "system/bmp_menulist.h"
#include "system/bmp_oam.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/wordset.h"
#include "text/system/key_system.h"

// Phases of KeySystemFlow_SeqGameClear.
enum {
    KEY_GAME_CLEAR_STEP_START_SCENE = 0,
    KEY_GAME_CLEAR_STEP_FADE_IN = 1,
    KEY_GAME_CLEAR_STEP_START_ANIMATION = 2,
    KEY_GAME_CLEAR_STEP_WAIT_ANIMATION = 3,
    KEY_GAME_CLEAR_STEP_UNLOCK = 4,
    KEY_GAME_CLEAR_STEP_FADE_OUT = 5,
    KEY_GAME_CLEAR_STEP_END_SCENE = 6,
    KEY_GAME_CLEAR_STEP_END = 7,
};

// Phases of KeySystemFlow_SeqMenu.
enum {
    KEY_MENU_STEP_START_SCENE = 0,
    KEY_MENU_STEP_INPUT = 1,
    KEY_MENU_STEP_END_SCENE = 2,
    KEY_MENU_STEP_DISPATCH = 3,
    KEY_MENU_STEP_END = 4,
};

// Phases of KeySystemFlow_SeqExchange.
enum {
    KEY_EXCHANGE_STEP_START_SCENE = 0,
    KEY_EXCHANGE_STEP_PRINT_PROMPT = 1,
    KEY_EXCHANGE_STEP_WAIT_PROMPT = 2,
    KEY_EXCHANGE_STEP_WAIT_MENU_DRAW = 3,
    KEY_EXCHANGE_STEP_SELECT = 4,
    KEY_EXCHANGE_STEP_START_RETRY_SCENE = 5,
    KEY_EXCHANGE_STEP_PRINT_RETRY = 6,
    KEY_EXCHANGE_STEP_START_CONNECT_SCENE = 7,
    KEY_EXCHANGE_STEP_CONNECT = 8,
    KEY_EXCHANGE_STEP_WAIT_CONNECTION = 9,
    KEY_EXCHANGE_STEP_WAIT_CANCEL = 10,
    KEY_EXCHANGE_STEP_END_FAILED_SCENE = 11,
    KEY_EXCHANGE_STEP_END_CANCELED_SCENE = 12,
    KEY_EXCHANGE_STEP_SEND_SAVE_INFO = 13,
    KEY_EXCHANGE_STEP_WAIT_SAVE_INFO = 14,
    KEY_EXCHANGE_STEP_SEND_KEY_INFO = 15,
    KEY_EXCHANGE_STEP_WAIT_KEY_INFO = 16,
    KEY_EXCHANGE_STEP_END_CONNECT_SCENE = 17,
    KEY_EXCHANGE_STEP_FIRST_EXCHANGE = 18,
    KEY_EXCHANGE_STEP_SECOND_EXCHANGE = 19,
    KEY_EXCHANGE_STEP_DISCONNECT = 20,
    KEY_EXCHANGE_STEP_WAIT_DISCONNECT = 21,
    KEY_EXCHANGE_STEP_SAVE_RECEIVED = 22,
    KEY_EXCHANGE_STEP_PRINT_DONE = 23,
    KEY_EXCHANGE_STEP_WAIT_DONE = 24,
    KEY_EXCHANGE_STEP_END = 25,
};

// Phases of KeySystemFlow_SeqSettings.
enum {
    KEY_SETTINGS_STEP_START_SCENE = 0,
    KEY_SETTINGS_STEP_INPUT = 1,
    KEY_SETTINGS_STEP_END_SCENE = 2,
    KEY_SETTINGS_STEP_DISPATCH = 3,
    KEY_SETTINGS_STEP_RESTART = 4,
};

// Phases of KeySystemFlow_SeqDifficulty.
enum {
    KEY_DIFFICULTY_STEP_START_SCENE = 0,
    KEY_DIFFICULTY_STEP_INPUT = 1,
    KEY_DIFFICULTY_STEP_SAVE = 2,
    KEY_DIFFICULTY_STEP_PRINT_INFO = 3,
    KEY_DIFFICULTY_STEP_WAIT_SAVE_MESSAGE = 4,
    KEY_DIFFICULTY_STEP_PRINT_CHANGED = 5,
    KEY_DIFFICULTY_STEP_WAIT_CHANGED = 6,
    KEY_DIFFICULTY_STEP_RESET_INPUT = 7,
    KEY_DIFFICULTY_STEP_PRINT_BLOCKED = 8,
    KEY_DIFFICULTY_STEP_WAIT_BLOCKED = 9,
    KEY_DIFFICULTY_STEP_END_SCENE = 10,
    KEY_DIFFICULTY_STEP_END = 11,
};

// Phases of KeySystemFlow_SeqCity.
enum {
    KEY_CITY_STEP_START_SCENE = 0,
    KEY_CITY_STEP_INPUT = 1,
    KEY_CITY_STEP_SAVE = 2,
    KEY_CITY_STEP_PRINT_INFO = 3,
    KEY_CITY_STEP_WAIT_SAVE_MESSAGE = 4,
    KEY_CITY_STEP_PRINT_CHANGED = 5,
    KEY_CITY_STEP_WAIT_CHANGED = 6,
    KEY_CITY_STEP_RESET_INPUT = 7,
    KEY_CITY_STEP_PRINT_BLOCKED = 8,
    KEY_CITY_STEP_WAIT_BLOCKED = 9,
    KEY_CITY_STEP_END_SCENE = 10,
    KEY_CITY_STEP_END = 11,
};

// Phases of KeySystemFlow_SeqChamber.
enum {
    KEY_CHAMBER_STEP_START_SCENE = 0,
    KEY_CHAMBER_STEP_INPUT = 1,
    KEY_CHAMBER_STEP_SAVE = 2,
    KEY_CHAMBER_STEP_PRINT_INFO = 3,
    KEY_CHAMBER_STEP_WAIT_SAVE_MESSAGE = 4,
    KEY_CHAMBER_STEP_PRINT_CHANGED = 5,
    KEY_CHAMBER_STEP_WAIT_CHANGED = 6,
    KEY_CHAMBER_STEP_RESET_INPUT = 7,
    KEY_CHAMBER_STEP_PRINT_BLOCKED = 8,
    KEY_CHAMBER_STEP_WAIT_BLOCKED = 9,
    KEY_CHAMBER_STEP_END_SCENE = 10,
    KEY_CHAMBER_STEP_END = 11,
};

// Phases of KeySystemFlow_SeqReceive.
enum {
    KEY_RECEIVE_STEP_CHECK_KEYS = 0,
    KEY_RECEIVE_STEP_PRINT_NONE = 1,
    KEY_RECEIVE_STEP_WAIT_NONE = 2,
    KEY_RECEIVE_STEP_START_SCENE = 3,
    KEY_RECEIVE_STEP_SYNC = 4,
    KEY_RECEIVE_STEP_WAIT_SYNC = 5,
    KEY_RECEIVE_STEP_FIND_KEY = 6,
    KEY_RECEIVE_STEP_WAIT_ANIMATION = 7,
    KEY_RECEIVE_STEP_END_SCENE = 8,
    KEY_RECEIVE_STEP_END = 9,
};

// Phases of KeySystemFlow_SeqSend.
enum {
    KEY_SEND_STEP_CHECK_KEYS = 0,
    KEY_SEND_STEP_PRINT_NONE = 1,
    KEY_SEND_STEP_WAIT_NONE = 2,
    KEY_SEND_STEP_START_SCENE = 3,
    KEY_SEND_STEP_WAIT_PRINT = 4,
    KEY_SEND_STEP_SYNC = 5,
    KEY_SEND_STEP_WAIT_SYNC = 6,
    KEY_SEND_STEP_FIND_KEY = 7,
    KEY_SEND_STEP_WAIT_KEY_MESSAGE = 8,
    KEY_SEND_STEP_END_SCENE = 9,
    KEY_SEND_STEP_END = 10,
};

// Phases of KeySystemFlow_SeqSave.
enum {
    KEY_SAVE_STEP_PRINT_WAIT = 0,
    KEY_SAVE_STEP_WAIT_PRINT = 1,
    KEY_SAVE_STEP_WAIT_WRITE = 2,
    KEY_SAVE_STEP_PLAY_SOUND = 3,
    KEY_SAVE_STEP_NEXT_FRAME = 4,
    KEY_SAVE_STEP_WAIT_DELAY = 5,
    KEY_SAVE_STEP_WAIT_DONE = 6,
    KEY_SAVE_STEP_END = 7,
};

// The Key System's sequences and scenes: exchanging keys with another Black 2 or White 2 over infrared, and the
// settings that keys unlock, the difficulty, the city and the chamber behind the mystery door. The ROM doesn't name
// this file; its helpers and the scene tables are in key_system_flow_sub.c (the ROM's .rodata order says so: the
// four message tables below pack as one section before the sub file's initializer and scene tables)

static void KeySystemFlow_SeqExchange(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqSettings(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqDifficulty(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqCity(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqChamber(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqReceive(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqSend(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqSave(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartConnectScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartSettingsScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartDifficultyScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartCityScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartKeyAnimScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartSendScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartKeysHeldScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartChamberScene(KeySystemSeq *seq, int *state, void *work);

// What a key that is already enabled says, what an unlocked key says, and the keys' names in the lists of keys
// being sent and held, in each version's order. MWCC heapsorts a unit's data by size, which gives the ROM's
// order of the three equal-size tables (unlocked, send, held) only from this declaration order
static const u16 sKeyEnabledMsgs[KEY_SYSTEM_KEY_COUNT] = { KeySystem_Text_ObtainedEasyKeyKeys,
                                                           KeySystem_Text_ObtainedChallengeKeyKeys, 0,
                                                           KeySystem_Text_ObtainedIronKeyKeys,
                                                           KeySystem_Text_ObtainedIcebergKeyKeys };
static const u16 sHeldKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { KeySystem_Text_EasyKey, KeySystem_Text_ChallengeKey, KeySystem_Text_TreehollowKey, KeySystem_Text_IronKey,
      KeySystem_Text_IcebergKey },
    { KeySystem_Text_EasyKey, KeySystem_Text_ChallengeKey, KeySystem_Text_TowerKey, KeySystem_Text_IronKey,
      KeySystem_Text_IcebergKey },
};
static const u16 sSendKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { KeySystem_Text_EasyKey, KeySystem_Text_ChallengeKey, KeySystem_Text_TreehollowKey, KeySystem_Text_IronKey,
      KeySystem_Text_IcebergKey },
    { KeySystem_Text_EasyKey, KeySystem_Text_ChallengeKey, KeySystem_Text_TowerKey, KeySystem_Text_IronKey,
      KeySystem_Text_IcebergKey },
};
static const u16 sKeyUnlockedMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { KeySystem_Text_ObtainedEasyKeyEasy, KeySystem_Text_ObtainedChallengeKeyChallenge,
      KeySystem_Text_ObtainedTreehollowKeyKeys, KeySystem_Text_ObtainedIronKeyIron,
      KeySystem_Text_ObtainedIcebergKeyIceberg },
    { KeySystem_Text_ObtainedEasyKeyEasy, KeySystem_Text_ObtainedChallengeKeyChallenge,
      KeySystem_Text_ObtainedTowerKeyKeys, KeySystem_Text_ObtainedIronKeyIron,
      KeySystem_Text_ObtainedIcebergKeyIceberg },
};

// The windows of the keys being sent, a title and then a key's name in each
static KeySystemMsgWinTemplate sSendTemplates[] = {
    { 1, 4, 18, 2, KeySystem_Text_KeysHeld },
    { 1, 7, 18, 2, 0 },
    { 1, 10, 18, 2, 0 },
    { 1, 13, 18, 2, 0 },
    { 1, 16, 18, 2, 0 },
};

// The windows of the keys held
static KeySystemMsgWinTemplate sKeysHeldTemplates[] = {
    { 1, 1, 18, 2, KeySystem_Text_KeysHeld },
    { 1, 4, 18, 2, 0 },
    { 1, 7, 18, 2, 0 },
    { 1, 10, 18, 2, 0 },
    { 1, 13, 18, 2, 0 },
    { 1, 16, 18, 2, 0 },
    { 1, 19, 18, 2, 0 },
};

void KeySystemFlow_Init(KeySystemWork *wk, HeapID heapId) {
    KeySystemKeyState_Init(&wk->keys, wk->keyInfo);
    KeySystemSaveInfo_Init(&wk->saveInfo, wk->param->gameData);
    if (wk->param->mode == UNOVA_LINK_MODE_GAME_CLEAR) {
        GFL_SndBGMPush();
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_1_TO_0, 1);
        GFL_NetSetIconScreen(TRUE, heapId);
    }
}

void KeySystemFlow_Exit(KeySystemWork *wk) {
    UnovaLinkParam *param = wk->param;

    if (param->mode == UNOVA_LINK_MODE_GAME_CLEAR) {
        if (param->key <= KEY_SYSTEM_KEY_CHALLENGE) {
            TrainerGameInfo_SetKeyNotice(getTrainerCardDataBlkAddress(param->gameData), 1);
        }
        GFL_SndBGMPop();
    }
}

void KeySystemFlow_SeqGameClear(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    StrBuf *str;
    BOOL enabled;
    u16 msgId;

    switch (*state) {
    case KEY_GAME_CLEAR_STEP_START_SCENE:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartKeyAnimScene);
        (*state)++;
        break;
    case KEY_GAME_CLEAR_STEP_FADE_IN:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case KEY_GAME_CLEAR_STEP_START_ANIMATION:
        enabled = FALSE;
        msgId = sKeyNameMsgs[VERSION_INDEX][wk->param->key];
        str = GFL_MsgDataLoadStrbufNew(wk->msgData, sKeyUnlockedMsgs[VERSION_INDEX][wk->param->key]);
        if (wk->param->key == KEY_SYSTEM_KEY_CITY) {
            enabled = TRUE;
        } else if (keyEnabler(wk->keyInfo, wk->param->key)) {
            enabled = TRUE;
            GFL_StrBufFree(str);
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, sKeyEnabledMsgs[wk->param->key]);
        }
        KeySystemKeyAnim_Start(&wk->keyAnim, wk->param->key, wk->msgData, msgId, str, enabled);
        GFL_StrBufFree(str);
        (*state)++;
        break;
    case KEY_GAME_CLEAR_STEP_WAIT_ANIMATION:
        if (KeySystemKeyAnim_Update(&wk->keyAnim)) {
            (*state)++;
        }
        break;
    case KEY_GAME_CLEAR_STEP_UNLOCK:
        KeyInfo_UnlockKey(wk->keyInfo, wk->param->key);
        if (wk->param->key != KEY_SYSTEM_KEY_CITY) {
            KeyInfo_EnableKey(wk->keyInfo, wk->param->key);
        }
        (*state)++;
        break;
    case KEY_GAME_CLEAR_STEP_FADE_OUT:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
        (*state)++;
        break;
    case KEY_GAME_CLEAR_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        KeySystem_FreeMsgWin(wk);
        (*state)++;
        break;
    case KEY_GAME_CLEAR_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

void KeySystemFlow_SeqMenu(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case KEY_MENU_STEP_START_SCENE:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartMenuScene);
        (*state)++;
        break;
    case KEY_MENU_STEP_INPUT:
        KeySystemList_Update(wk->list);
        if (KeySystemList_IsChanged(wk->list) && KeySystemMsgWin_IsDone(wk->infoWin)) {
            KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemList_GetCursor(wk->list) + 18,
                                     KEY_SYSTEM_MSG_PRINT);
        }
        if (KeySystemList_IsDecided(wk->list)) {
            wk->choice = KeySystemList_GetCursor(wk->list);
            if (wk->choice != KEY_MENU_BACK) {
                KeySystemTags_Set(wk->tags, "KEM", KeySystemList_GetCursor(wk->list));
            }
            (*state)++;
        }
        break;
    case KEY_MENU_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case KEY_MENU_STEP_DISPATCH:
        switch (wk->choice) {
        case KEY_MENU_EXCHANGE:
            if (isWirelessEnabled()) {
                KeySystemSeq_Push(seq, KeySystemFlow_SeqExchange);
            } else {
                KeySystemSeq_Push(seq, KeySystem_SeqWirelessOff);
            }
            *state = KEY_MENU_STEP_START_SCENE;
            break;
        case KEY_MENU_SETTINGS:
            KeySystemSeq_Push(seq, KeySystemFlow_SeqSettings);
            *state = KEY_MENU_STEP_START_SCENE;
            break;
        case KEY_MENU_BACK:
            (*state)++;
            break;
        }
        break;
    case KEY_MENU_STEP_END:
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_0_TO_1, 30);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqExchange(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    s32 result;

    switch (*state) {
    case KEY_EXCHANGE_STEP_START_SCENE:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartKeysHeldScene);
        (*state)++;
        break;
    case KEY_EXCHANGE_STEP_PRINT_PROMPT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_WouldLikeLaunchInfrared,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case KEY_EXCHANGE_STEP_WAIT_PROMPT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystem_CreateYesNoMenu(wk, HEAPID_KEY_SYSTEM);
            (*state)++;
        }
        break;
    case KEY_EXCHANGE_STEP_WAIT_MENU_DRAW:
        if (KeySystemMenu_UpdatePrint(wk->menu)) {
            GFL_BGSysQueueScrLoad(0);
            (*state)++;
        }
        break;
    case KEY_EXCHANGE_STEP_SELECT:
        result = KeySystemMenu_Update(wk->menu);
        if (result == BMPMENULIST_NULL) {
            break;
        }
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        KeySystem_FreeMenu(wk);
        switch (result) {
        case KEY_SYSTEM_YES_NO_YES:
            *state = KEY_EXCHANGE_STEP_START_CONNECT_SCENE;
            break;
        case KEY_SYSTEM_YES_NO_NO:
        default:
            KeySystemMsgWinGroup_Free(wk->msgWinGroup);
            wk->msgWinGroup = NULL;
            GFL_BGSysClearScr(6);
            GFL_BGSysQueueScrLoad(6);
            GFL_BGSysQueueScrLoad(4);
            *state = KEY_EXCHANGE_STEP_END;
            break;
        }
        break;
    case KEY_EXCHANGE_STEP_START_RETRY_SCENE:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartKeysHeldScene);
        (*state)++;
        break;
    case KEY_EXCHANGE_STEP_PRINT_RETRY:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_UnableFindNintendoDs, KEY_SYSTEM_MSG_STREAM);
        *state = KEY_EXCHANGE_STEP_WAIT_PROMPT;
        break;
    case KEY_EXCHANGE_STEP_START_CONNECT_SCENE:
        wk->choice = 0;
        wk->noCancel = FALSE;
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartConnectScene);
        GFL_NetSetIconScreen(TRUE, HEAPID_KEY_SYSTEM);
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_WIRELESS);
        GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_01);
        (*state)++;
        break;
    case KEY_EXCHANGE_STEP_CONNECT: {
        KeySystemNetCallback callback;

        MI_CpuFill8(&callback, 0, sizeof(KeySystemNetCallback));
        callback.arg = wk;
        callback.func = KeySystemFlow_OnInfraredEnded;
        wk->choice = 0;
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_CONNECT, &callback);
        (*state)++;
        break;
    }
    case KEY_EXCHANGE_STEP_WAIT_CONNECTION:
        if (wk->choice) {
            ClActor *actor0 = KeySystemClAct_GetActor(&wk->clact, KEY_SYSTEM_ACTOR_IR_SIGNAL_LOWER);
            ClActor *actor1 = KeySystemClAct_GetActor(&wk->clact, KEY_SYSTEM_ACTOR_IR_SIGNAL_UPPER);
            ClActor *actor2 = KeySystemClAct_GetActor(&wk->clact, KEY_SYSTEM_ACTOR_CONSOLE_LOWER);
            ClActor *actor3 = KeySystemClAct_GetActor(&wk->clact, KEY_SYSTEM_ACTOR_CONSOLE_UPPER);

            ClActor_SetVisible(actor0, FALSE);
            ClActor_SetVisible(actor1, FALSE);
            ClActor_SetSequence(actor2, KEY_SYSTEM_ANIM_CONSOLE_LOWER_WIRELESS);
            ClActor_SetSequence(actor3, KEY_SYSTEM_ANIM_CONSOLE_UPPER_WIRELESS);
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_CommunicatingPleaseStandBy,
                                     KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
            GFL_SndStop();
            GFL_SndSEPlay(SEQ_SE_SYS_12);
            wk->choice = 0;
        } else if (!wk->noCancel && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
            KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_CANCEL, NULL);
        }
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            wk->isParent = GFL_NetIsParent();
            *state = KEY_EXCHANGE_STEP_SEND_SAVE_INFO;
        } else if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            GFL_SndStop();
            *state = KEY_EXCHANGE_STEP_END_FAILED_SCENE;
        } else if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CANCELING) {
            GFL_SndStop();
            *state = KEY_EXCHANGE_STEP_WAIT_CANCEL;
        }
        break;
    case KEY_EXCHANGE_STEP_WAIT_CANCEL:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            *state = KEY_EXCHANGE_STEP_END_CANCELED_SCENE;
        }
        break;
    case KEY_EXCHANGE_STEP_END_FAILED_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        *state = KEY_EXCHANGE_STEP_START_RETRY_SCENE;
        break;
    case KEY_EXCHANGE_STEP_END_CANCELED_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        *state = KEY_EXCHANGE_STEP_END;
        break;
    case KEY_EXCHANGE_STEP_SEND_SAVE_INFO: {
        KeySystemNetSend send;

        send.size = sizeof(KeySystemSaveInfo);
        send.data = &wk->saveInfo;
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_SEND, &send);
        (*state)++;
        break;
    }
    case KEY_EXCHANGE_STEP_WAIT_SAVE_INFO:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            wk->choice = KeySystemNet_GetReceived(wk->net, &wk->partnerSaveInfo, sizeof(KeySystemSaveInfo));
            (*state)++;
        }
        break;
    case KEY_EXCHANGE_STEP_SEND_KEY_INFO: {
        KeySystemNetSend send;

        send.size = sizeof(KeySystemKeyState);
        send.data = &wk->keys;
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_SEND, &send);
        (*state)++;
        break;
    }
    case KEY_EXCHANGE_STEP_WAIT_KEY_INFO:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            wk->choice = KeySystemNet_GetReceived(wk->net, &wk->partnerKeys, sizeof(KeySystemKeyState));
            (*state)++;
        }
        break;
    case KEY_EXCHANGE_STEP_END_CONNECT_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        if (wk->choice) {
            (*state)++;
        } else {
            *state = KEY_EXCHANGE_STEP_DISCONNECT;
        }
        break;
    case KEY_EXCHANGE_STEP_FIRST_EXCHANGE:
        if (wk->isParent) {
            KeySystemSeq_Push(seq, KeySystemFlow_SeqReceive);
        } else {
            KeySystemSeq_Push(seq, KeySystemFlow_SeqSend);
        }
        (*state)++;
        break;
    case KEY_EXCHANGE_STEP_SECOND_EXCHANGE:
        if (!wk->isParent) {
            KeySystemSeq_Push(seq, KeySystemFlow_SeqReceive);
        } else {
            KeySystemSeq_Push(seq, KeySystemFlow_SeqSend);
        }
        (*state)++;
        break;
    case KEY_EXCHANGE_STEP_DISCONNECT:
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_DISCONNECT, NULL);
        (*state)++;
        break;
    case KEY_EXCHANGE_STEP_WAIT_DISCONNECT:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            (*state)++;
        }
        break;
    case KEY_EXCHANGE_STEP_SAVE_RECEIVED:
        if (KeySystemKeyState_CanExchange(&wk->keys, &wk->partnerKeys, TRUE)) {
            KeySystemKeyState_Receive(&wk->keys, &wk->partnerKeys);
            KeySystemKeyState_Save(&wk->keys, wk->keyInfo);
            KeySystemSeq_Push(seq, KeySystemFlow_SeqSave);
        }
        *state = KEY_EXCHANGE_STEP_PRINT_DONE;
        break;
    case KEY_EXCHANGE_STEP_PRINT_DONE:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_SendingReceivingKeysHas,
                                 KEY_SYSTEM_MSG_STREAM_FAST);
        (*state)++;
        break;
    case KEY_EXCHANGE_STEP_WAIT_DONE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case KEY_EXCHANGE_STEP_END:
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_NONE);
        KeySystem_FreeMsgWin(wk);
        GFL_BGSysQueueScrLoad(0);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqSettings(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case KEY_SETTINGS_STEP_START_SCENE:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartSettingsScene);
        (*state)++;
        break;
    case KEY_SETTINGS_STEP_INPUT:
        KeySystemList_Update(wk->list);
        if (KeySystemList_IsChanged(wk->list) && KeySystemMsgWin_IsDone(wk->infoWin)) {
            KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData,
                                     KeySystemFlow_GetSettingsMsg(wk->keyInfo, KeySystemList_GetCursor(wk->list)),
                                     KEY_SYSTEM_MSG_PRINT);
        }
        if (KeySystemList_IsDecided(wk->list)) {
            wk->choice = KeySystemList_GetCursor(wk->list);
            if (wk->choice != KEY_LIST_BACK) {
                KeySystemTags_Set(wk->tags, "KLL", KeySystemList_GetCursor(wk->list));
            }
            (*state)++;
        }
        break;
    case KEY_SETTINGS_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case KEY_SETTINGS_STEP_DISPATCH:
        switch (wk->choice) {
        case KEY_LIST_DIFFICULTY:
            KeySystemSeq_Push(seq, KeySystemFlow_SeqDifficulty);
            *state = KEY_SETTINGS_STEP_RESTART;
            break;
        case KEY_LIST_CITY:
            KeySystemSeq_Push(seq, KeySystemFlow_SeqCity);
            *state = KEY_SETTINGS_STEP_RESTART;
            break;
        case KEY_LIST_CHAMBER:
            KeySystemSeq_Push(seq, KeySystemFlow_SeqChamber);
            *state = KEY_SETTINGS_STEP_RESTART;
            break;
        case KEY_LIST_BACK:
            KeySystemSeq_Pop(seq);
            break;
        }
        break;
    case KEY_SETTINGS_STEP_RESTART:
        *state = KEY_SETTINGS_STEP_START_SCENE;
        break;
    }
}

static void KeySystemFlow_SeqDifficulty(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case KEY_DIFFICULTY_STEP_START_SCENE:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartDifficultyScene);
        (*state)++;
        break;
    case KEY_DIFFICULTY_STEP_INPUT:
        KeySystemKeySelect_Update(&wk->keySelect);
        if (KeySystemKeySelect_IsDecided(&wk->keySelect)) {
            if (KeySystemKeySelect_GetCursor(&wk->keySelect) == KEY_DIFFICULTY_MENU_BACK) {
                *state = KEY_DIFFICULTY_STEP_END_SCENE;
            } else {
                *state = KEY_DIFFICULTY_STEP_SAVE;
            }
        } else if (KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_LOCKED) {
            *state = KEY_DIFFICULTY_STEP_PRINT_BLOCKED;
        }
        break;
    case KEY_DIFFICULTY_STEP_SAVE:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqSave);
        (*state)++;
        break;
    case KEY_DIFFICULTY_STEP_PRINT_INFO:
        KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetDifficultyMsg(wk->keyInfo),
                                 KEY_SYSTEM_MSG_PRINT);
        (*state)++;
        break;
    case KEY_DIFFICULTY_STEP_WAIT_SAVE_MESSAGE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case KEY_DIFFICULTY_STEP_PRINT_CHANGED:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData,
                                 GetGameDifficulty(wk->keyInfo) + KeySystem_Text_DifficultySetEasyMode,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case KEY_DIFFICULTY_STEP_WAIT_CHANGED:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = KEY_DIFFICULTY_STEP_RESET_INPUT;
        }
        break;
    case KEY_DIFFICULTY_STEP_RESET_INPUT:
        KeySystem_FreeMsgWin(wk);
        GFL_BGSysQueueScrLoad(0);
        KeySystemKeySelect_Reset(&wk->keySelect);
        *state = KEY_DIFFICULTY_STEP_INPUT;
        break;
    case KEY_DIFFICULTY_STEP_PRINT_BLOCKED:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        switch (KeySystemKeySelect_GetIndex(&wk->keySelect)) {
        case GAME_DIFFICULTY_EASY:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_CannotSelectItemUntil,
                                     KEY_SYSTEM_MSG_PRINT);
            break;
        case GAME_DIFFICULTY_CHALLENGE:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_CannotSelectItemUntil_2,
                                     KEY_SYSTEM_MSG_PRINT);
            break;
        }
        (*state)++;
        break;
    case KEY_DIFFICULTY_STEP_WAIT_BLOCKED:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            GFL_BGSysQueueScrLoad(0);
            if (GCTX_HIDGetPressedKeys()) {
                *state = KEY_DIFFICULTY_STEP_RESET_INPUT;
            }
        }
        break;
    case KEY_DIFFICULTY_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case KEY_DIFFICULTY_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqCity(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case KEY_CITY_STEP_START_SCENE:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartCityScene);
        (*state)++;
        break;
    case KEY_CITY_STEP_INPUT:
        KeySystemKeySelect_Update(&wk->keySelect);
        if (KeySystemKeySelect_IsDecided(&wk->keySelect)) {
            if (KeySystemKeySelect_GetCursor(&wk->keySelect) == KEY_CITY_MENU_BACK) {
                *state = KEY_CITY_STEP_END_SCENE;
            } else {
                *state = KEY_CITY_STEP_SAVE;
            }
        } else if (KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_LOCKED ||
                   KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_IN_AREA) {
            *state = KEY_CITY_STEP_PRINT_BLOCKED;
        }
        break;
    case KEY_CITY_STEP_SAVE:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqSave);
        (*state)++;
        break;
    case KEY_CITY_STEP_PRINT_INFO:
        KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetCityMsg(wk->keyInfo), KEY_SYSTEM_MSG_PRINT);
        (*state)++;
        break;
    case KEY_CITY_STEP_WAIT_SAVE_MESSAGE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case KEY_CITY_STEP_PRINT_CHANGED:
#ifdef BLACK2
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData,
                                 KeyInfo_GetCityKey(wk->keyInfo) + KeySystem_Text_ChangedAreaBlackCity,
                                 KEY_SYSTEM_MSG_STREAM);
#else
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData,
                                 (KeyInfo_GetCityKey(wk->keyInfo) == KEY_CITY_MENU_DEFAULT ? 1 : 0) +
                                     KeySystem_Text_ChangedAreaBlackCity,
                                 KEY_SYSTEM_MSG_STREAM);
#endif
        (*state)++;
        break;
    case KEY_CITY_STEP_WAIT_CHANGED:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = KEY_CITY_STEP_RESET_INPUT;
        }
        break;
    case KEY_CITY_STEP_RESET_INPUT:
        KeySystem_FreeMsgWin(wk);
        GFL_BGSysQueueScrLoad(0);
        KeySystemKeySelect_Reset(&wk->keySelect);
        *state = KEY_CITY_STEP_INPUT;
        break;
    case KEY_CITY_STEP_PRINT_BLOCKED:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        if (KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_IN_AREA) {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_CurrentlyAreaSettingsCannot,
                                     KEY_SYSTEM_MSG_PRINT);
        } else if (KeySystemKeySelect_GetIndex(&wk->keySelect) == KEY_CITY_MENU_OTHER) {
#ifdef BLACK2
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_CannotSelectItemUntil_3,
                                     KEY_SYSTEM_MSG_PRINT);
#else
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_CannotSelectItemUntil_4,
                                     KEY_SYSTEM_MSG_PRINT);
#endif
        }
        (*state)++;
        break;
    case KEY_CITY_STEP_WAIT_BLOCKED:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            GFL_BGSysQueueScrLoad(0);
            if (GCTX_HIDGetPressedKeys()) {
                *state = KEY_CITY_STEP_RESET_INPUT;
            }
        }
        break;
    case KEY_CITY_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case KEY_CITY_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqChamber(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case KEY_CHAMBER_STEP_START_SCENE:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartChamberScene);
        (*state)++;
        break;
    case KEY_CHAMBER_STEP_INPUT:
        KeySystemKeySelect_Update(&wk->keySelect);
        if (KeySystemKeySelect_IsDecided(&wk->keySelect)) {
            if (KeySystemKeySelect_GetCursor(&wk->keySelect) == KEY_CHAMBER_MENU_BACK) {
                *state = KEY_CHAMBER_STEP_END_SCENE;
            } else {
                *state = KEY_CHAMBER_STEP_SAVE;
            }
        } else if (KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_LOCKED) {
            *state = KEY_CHAMBER_STEP_PRINT_BLOCKED;
        }
        break;
    case KEY_CHAMBER_STEP_SAVE:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqSave);
        (*state)++;
        break;
    case KEY_CHAMBER_STEP_PRINT_INFO:
        KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetChamberMsg(wk->keyInfo),
                                 KEY_SYSTEM_MSG_PRINT);
        (*state)++;
        break;
    case KEY_CHAMBER_STEP_WAIT_SAVE_MESSAGE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case KEY_CHAMBER_STEP_PRINT_CHANGED:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData,
                                 KeyInfo_GetChamberKey(wk->keyInfo) + KeySystem_Text_MysteryDoorNowLeads,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case KEY_CHAMBER_STEP_WAIT_CHANGED:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = KEY_CHAMBER_STEP_RESET_INPUT;
        }
        break;
    case KEY_CHAMBER_STEP_RESET_INPUT:
        KeySystem_FreeMsgWin(wk);
        GFL_BGSysQueueScrLoad(0);
        KeySystemKeySelect_Reset(&wk->keySelect);
        *state = KEY_CHAMBER_STEP_INPUT;
        break;
    case KEY_CHAMBER_STEP_PRINT_BLOCKED:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        switch (KeySystemKeySelect_GetIndex(&wk->keySelect)) {
        case KEY_INFO_CHAMBER_IRON:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_CannotSelectItemUntil_5,
                                     KEY_SYSTEM_MSG_PRINT);
            break;
        case KEY_INFO_CHAMBER_ICEBERG:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_CannotSelectItemUntil_6,
                                     KEY_SYSTEM_MSG_PRINT);
            break;
        }
        (*state)++;
        break;
    case KEY_CHAMBER_STEP_WAIT_BLOCKED:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            GFL_BGSysQueueScrLoad(0);
            if (GCTX_HIDGetPressedKeys()) {
                *state = KEY_CHAMBER_STEP_RESET_INPUT;
            }
        }
        break;
    case KEY_CHAMBER_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case KEY_CHAMBER_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqReceive(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case KEY_RECEIVE_STEP_CHECK_KEYS:
        if (KeySystemKeyState_CanExchange(&wk->keys, &wk->partnerKeys, TRUE)) {
            *state = KEY_RECEIVE_STEP_START_SCENE;
        } else {
            *state = KEY_RECEIVE_STEP_PRINT_NONE;
        }
        break;
    case KEY_RECEIVE_STEP_PRINT_NONE:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_ThereNoKeysCan_2, KEY_SYSTEM_MSG_STREAM_FAST);
        (*state)++;
        break;
    case KEY_RECEIVE_STEP_WAIT_NONE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = KEY_RECEIVE_STEP_END;
        }
        break;
    case KEY_RECEIVE_STEP_START_SCENE:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartKeyAnimScene);
        wk->timer = 0;
        wk->choice = 0;
        (*state)++;
        break;
    case KEY_RECEIVE_STEP_SYNC:
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_SYNC, NULL);
        (*state)++;
        break;
    case KEY_RECEIVE_STEP_WAIT_SYNC:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            (*state)++;
        }
        break;
    case KEY_RECEIVE_STEP_FIND_KEY:
        // The key that is looked at
        while (TRUE) {
            if (wk->choice >= KEY_SYSTEM_KEY_COUNT) {
                *state = KEY_RECEIVE_STEP_END_SCENE;
                return;
            }
            if (KeySystemFlow_FormatKeyMsg(wk, wk->choice, TRUE)) {
                KeySystemKeyAnim_Start(&wk->keyAnim, wk->choice, wk->msgData,
                                       sReceivedKeyNameMsgs[VERSION_INDEX][wk->choice], wk->strBuf, FALSE);
                KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_ReceivingKeyS,
                                         KEY_SYSTEM_MSG_STREAM_FAST);
                (*state)++;
                return;
            }
            wk->choice++;
        }
    case KEY_RECEIVE_STEP_WAIT_ANIMATION:
        if (KeySystemKeyAnim_Update(&wk->keyAnim)) {
            wk->timer = 0;
            wk->choice++;
            *state = KEY_RECEIVE_STEP_SYNC;
        }
        break;
    case KEY_RECEIVE_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case KEY_RECEIVE_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqSend(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case KEY_SEND_STEP_CHECK_KEYS:
        if (KeySystemKeyState_CanExchange(&wk->keys, &wk->partnerKeys, FALSE)) {
            *state = KEY_SEND_STEP_START_SCENE;
        } else {
            *state = KEY_SEND_STEP_PRINT_NONE;
        }
        break;
    case KEY_SEND_STEP_PRINT_NONE:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_ThereNoKeysCan, KEY_SYSTEM_MSG_STREAM_FAST);
        (*state)++;
        break;
    case KEY_SEND_STEP_WAIT_NONE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = KEY_SEND_STEP_END;
        }
        break;
    case KEY_SEND_STEP_START_SCENE:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartSendScene);
        wk->timer = 0;
        wk->choice = 0;
        (*state)++;
        break;
    case KEY_SEND_STEP_WAIT_PRINT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            wk->timer = 0;
            (*state)++;
        }
        break;
    case KEY_SEND_STEP_SYNC:
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_SYNC, NULL);
        (*state)++;
        break;
    case KEY_SEND_STEP_WAIT_SYNC:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            (*state)++;
        }
        break;
    case KEY_SEND_STEP_FIND_KEY:
        while (TRUE) {
            if (wk->choice >= KEY_SYSTEM_KEY_COUNT) {
                *state = KEY_SEND_STEP_END_SCENE;
                return;
            }
            if (KeySystemFlow_FormatKeyMsg(wk, wk->choice, FALSE)) {
                KeySystemMsgWin_PrintStr(wk->msgWin, wk->strBuf, KEY_SYSTEM_MSG_STREAM_FAST);
                (*state)++;
                return;
            }
            wk->choice++;
        }
    case KEY_SEND_STEP_WAIT_KEY_MESSAGE:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            wk->timer = 0;
            wk->choice++;
            *state = KEY_SEND_STEP_SYNC;
        }
        break;
    case KEY_SEND_STEP_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case KEY_SEND_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqSave(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    u32 mode;

    switch (*state) {
    case KEY_SAVE_STEP_PRINT_WAIT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_SavingDontTurnOff,
                                 KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case KEY_SAVE_STEP_WAIT_PRINT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            GFL_BGSysQueueScrLoad(0);
            if (SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData))) {
                GameData_StartSave(wk->param->gameData);
            } else {
                SaveOutside_StartSave(wk->ov331Work);
            }
            wk->timer = 0;
            (*state)++;
        }
        break;
    case KEY_SAVE_STEP_WAIT_WRITE:
        wk->timer++;
        if (SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData))) {
            if (GameData_UpdateSave(wk->param->gameData) == SAVE_RESULT_OK) {
                (*state)++;
            }
        } else if (SaveOutside_Save(wk->ov331Work)) {
            (*state)++;
        }
        break;
    case KEY_SAVE_STEP_PLAY_SOUND:
        GFL_SndSEPlay(SEQ_SE_SAVE);
        *state = KEY_SAVE_STEP_NEXT_FRAME;
        break;
    case KEY_SAVE_STEP_NEXT_FRAME:
        *state = KEY_SAVE_STEP_WAIT_DELAY;
        break;
    case KEY_SAVE_STEP_WAIT_DELAY:
        if (wk->timer++ > 60) {
            mode = KEY_SYSTEM_MSG_STREAM;
            if (GFL_NetIsInitialized()) {
                mode = KEY_SYSTEM_MSG_STREAM_FAST;
            }
            wk->timer = 0;
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_WritingSavedGameFile, mode);
            (*state)++;
        }
        break;
    case KEY_SAVE_STEP_WAIT_DONE:
        if (KeySystemMsgWin_IsDone(wk->msgWin) && !GFL_SndPlayerIsActiveAny()) {
            (*state)++;
        }
        break;
    case KEY_SAVE_STEP_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sMenuSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartConnectScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sConnectSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartSettingsScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sSettingsSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartDifficultyScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sDifficultySceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartCityScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sCitySceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartKeyAnimScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sKeyAnimSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartSendScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sSendSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartKeysHeldScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sKeysHeldSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartChamberScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case UNOVA_SCENE_START_STEP_START:
        KeySystemScene_Start(wk->scene, &sChamberSceneFuncs, HEAPID_KEY_SYSTEM);
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

void KeySystemFlow_MenuSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemListSetup setup;
    u32 i;

    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, KeySystem_Text_KeySystem, KEY_SYSTEM_MSG_PRINT);
    MI_CpuFill8(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.count = 3;
    if (KeySystemTags_Has(wk->tags, "KEM")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "KEM");
        KeySystemTags_Remove(wk->tags, "KEM");
    } else {
        setup.cursor = 0;
    }
    for (i = 0; i < setup.count; i++) {
        setup.items[i].width = 26;
        setup.items[i].height = 2;
        setup.items[i].x = 3;
        setup.items[i].y = (setup.items[i].height + 3) * i + 5;
    }
    setup.items[0].msgId = KeySystem_Text_SendReceiveKeys;
    setup.items[1].msgId = KeySystem_Text_KeySystemSettings;
    setup.items[2].msgId = KeySystem_Text_Back;
    wk->list = KeySystemList_Create(&setup, heapId);
    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, setup.cursor + 18, KEY_SYSTEM_MSG_PRINT);
}

BOOL KeySystemFlow_MenuSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemList_IsPrinted(wk->list);

    if (titleDone && infoDone && listDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_MenuSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystem_FreeInfoWin(wk);
    KeySystemList_Free(wk->list);
    wk->list = NULL;
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

void KeySystemFlow_ConnectSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_LaunchingInfraredCommunicationsDs,
                             KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
    KeySystemClAct_Create(&wk->clact, KEY_SYSTEM_ACTOR_IR_SIGNAL_LOWER, heapId);
    KeySystemClAct_Create(&wk->clact, KEY_SYSTEM_ACTOR_IR_SIGNAL_UPPER, heapId);
    KeySystemClAct_Create(&wk->clact, KEY_SYSTEM_ACTOR_CONSOLE_LOWER, heapId);
    KeySystemClAct_Create(&wk->clact, KEY_SYSTEM_ACTOR_CONSOLE_UPPER, heapId);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_OBJ, GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, 16,
                       6);
    GX_SetVisibleWnd(GX_WNDMASK_W0);
    G2_SetWnd0Position(224, 0, 0, 32);
    G2_SetWnd0InsidePlane(GX_PLANEMASK_ALL, FALSE);
    G2_SetWndOutsidePlane(GX_PLANEMASK_ALL, TRUE);
}

BOOL KeySystemFlow_ConnectSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_ConnectSceneExit(void *work) {
    KeySystemWork *wk = work;

    if (wk->msgWinGroup != NULL) {
        GFL_BGSysClearScr(6);
        KeySystemMsgWinGroup_Free(wk->msgWinGroup);
        wk->msgWinGroup = NULL;
    }
    G2_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    KeySystemClAct_Delete(&wk->clact, KEY_SYSTEM_ACTOR_CONSOLE_UPPER);
    KeySystemClAct_Delete(&wk->clact, KEY_SYSTEM_ACTOR_CONSOLE_LOWER);
    KeySystemClAct_Delete(&wk->clact, KEY_SYSTEM_ACTOR_IR_SIGNAL_UPPER);
    KeySystemClAct_Delete(&wk->clact, KEY_SYSTEM_ACTOR_IR_SIGNAL_LOWER);
    KeySystemMsgWin_StopWaitIcon(wk->msgWin);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysQueueScrLoad(6);
}

void KeySystemFlow_SettingsSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemListSetup setup;
    u32 i;

    MI_CpuFill8(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.count = 4;
    if (KeySystemTags_Has(wk->tags, "KLL")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "KLL");
        KeySystemTags_Remove(wk->tags, "KLL");
    } else {
        setup.cursor = 0;
    }
    for (i = 0; i < setup.count; i++) {
        setup.items[i].width = 26;
        setup.items[i].height = 2;
        setup.items[i].x = 3;
        setup.items[i].y = (setup.items[i].height + 3) * i + 5;
    }
    setup.items[0].msgId = KeySystem_Text_SetDifficulty;
    setup.items[1].msgId = KeySystem_Text_ChangeCity;
    setup.items[2].msgId = KeySystem_Text_MysteryDoor;
    setup.items[3].msgId = KeySystem_Text_Back;
    wk->list = KeySystemList_Create(&setup, heapId);
    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetSettingsMsg(wk->keyInfo, setup.cursor),
                             KEY_SYSTEM_MSG_PRINT);
}

BOOL KeySystemFlow_SettingsSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemList_IsPrinted(wk->list);

    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_SettingsSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemList_Free(wk->list);
    wk->list = NULL;
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

void KeySystemFlow_DifficultySceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetDifficultyMsg(wk->keyInfo),
                             KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_DIFFICULTY, &wk->clact, &wk->list, wk->msgData, wk->font,
                            wk->keyInfo, wk->param->gameData, heapId);
}

BOOL KeySystemFlow_DifficultySceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemKeySelect_IsPrinted(&wk->keySelect);

    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_DifficultySceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

void KeySystemFlow_CitySceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetCityMsg(wk->keyInfo), KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_CITY, &wk->clact, &wk->list, wk->msgData, wk->font, wk->keyInfo,
                            wk->param->gameData, heapId);
}

BOOL KeySystemFlow_CitySceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemKeySelect_IsPrinted(&wk->keySelect);

    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_CitySceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

void KeySystemFlow_KeyAnimSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystemKeyAnim_Init(&wk->keyAnim, &wk->clact, wk->msgWin, wk->font, heapId);
}

BOOL KeySystemFlow_KeyAnimSceneMain(void *work) {
    return TRUE;
}

void KeySystemFlow_KeyAnimSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeyAnim_Free(&wk->keyAnim);
}

void KeySystemFlow_SendSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    u32 count = 0;
    int i;

    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        if (wk->keys.unlocked[i]) {
            // BUG: The fifth key's name goes past the table, into sKeysHeldTemplates' title
            sSendTemplates[1 + count++].msgId = sSendKeyNameMsgs[VERSION_INDEX][i];
        }
    }
    if (count + 1 > NELEMS(sSendTemplates)) {
        count = NELEMS(sSendTemplates) - 1;
    }
    wk->msgWinGroup = KeySystemMsgWinGroup_Create(sSendTemplates, count + 1, 0, 14, wk->font, wk->msgData, heapId);
    KeySystemBG_LoadScreen(wk->bg, 2, KEY_SYSTEM_SCREEN_KEYS_SENT);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_SendingKeyS, KEY_SYSTEM_MSG_STREAM_FAST);
}

BOOL KeySystemFlow_SendSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWinGroup_IsDone(wk->msgWinGroup)) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(2);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_SendSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemMsgWinGroup_Free(wk->msgWinGroup);
    wk->msgWinGroup = NULL;
    if (wk->msgWin != NULL) {
        KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
        KeySystemMsgWin_StopWaitIcon(wk->msgWin);
    }
    KeySystemBG_LoadScreen(wk->bg, 2, KEY_SYSTEM_SCREEN_COMMON);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(2);
}

void KeySystemFlow_KeysHeldSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    u32 count = 0;
    int i;

    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeySystem_Text_Empty, KEY_SYSTEM_MSG_PRINT);
    if (wk->msgWinGroup == NULL) {
        for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
            if (wk->keys.unlocked[i]) {
                sKeysHeldTemplates[1 + count++].msgId = sHeldKeyNameMsgs[VERSION_INDEX][i];
            }
        }
        wk->msgWinGroup =
            KeySystemMsgWinGroup_Create(sKeysHeldTemplates, count + 1, 4, 14, wk->font, wk->msgData, heapId);
    }
    KeySystemBG_LoadScreen(wk->bg, 6, KEY_SYSTEM_SCREEN_KEYS_HELD);
}

BOOL KeySystemFlow_KeysHeldSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL groupDone = KeySystemMsgWinGroup_IsDone(wk->msgWinGroup);
    BOOL msgDone = KeySystemMsgWin_IsDone(wk->msgWin);

    if (groupDone && msgDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(4);
        GFL_BGSysQueueScrLoad(6);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_KeysHeldSceneExit(void *work) {
    GFL_BGSysQueueScrLoad(0);
}

void KeySystemFlow_ChamberSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetChamberMsg(wk->keyInfo), KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_CHAMBER, &wk->clact, &wk->list, wk->msgData, wk->font, wk->keyInfo,
                            wk->param->gameData, heapId);
}

BOOL KeySystemFlow_ChamberSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemKeySelect_IsPrinted(&wk->keySelect);

    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_ChamberSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}
