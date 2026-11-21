#include "types.h"
#include "app/key_system_flow.h"
#include "app/unova_link.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "field/zone.h"
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

// Phases of KeySystemKeyAnim_Unlock.
enum {
    KEY_UNLOCK_STEP_PRINT_NAME = 0,
    KEY_UNLOCK_STEP_WAIT_NAME = 1,
    KEY_UNLOCK_STEP_START_PLATE_RISE = 2,
    KEY_UNLOCK_STEP_MOVE_PLATE_UP = 3,
    KEY_UNLOCK_STEP_WAIT_PLATE = 4,
    KEY_UNLOCK_STEP_START_KEY_DROP = 5,
    KEY_UNLOCK_STEP_MOVE_KEY_DOWN = 6,
    KEY_UNLOCK_STEP_WAIT_KEY = 7,
    KEY_UNLOCK_STEP_START_KEY_TURN = 8,
    KEY_UNLOCK_STEP_MOVE_KEY_OUT = 9,
    KEY_UNLOCK_STEP_START_LOCK_OPEN = 10,
    KEY_UNLOCK_STEP_WAIT_LOCK = 11,
    KEY_UNLOCK_STEP_START_EFFECT = 12,
    KEY_UNLOCK_STEP_WAIT_EFFECT = 13,
    KEY_UNLOCK_STEP_WAIT_EFFECT_END = 14,
    KEY_UNLOCK_STEP_PRINT_RESULT = 15,
    KEY_UNLOCK_STEP_WAIT_RESULT = 16,
    KEY_UNLOCK_STEP_START_PLATE_DROP = 17,
    KEY_UNLOCK_STEP_MOVE_PLATE_DOWN = 18,
    KEY_UNLOCK_STEP_DONE = 19,
};

// Phases of KeySystemKeyAnim_Show.
enum {
    KEY_SHOW_STEP_START_DROP = 0,
    KEY_SHOW_STEP_MOVE_DOWN = 1,
    KEY_SHOW_STEP_WAIT_KEY = 2,
    KEY_SHOW_STEP_PRINT_RESULT = 3,
    KEY_SHOW_STEP_WAIT_RESULT = 4,
    KEY_SHOW_STEP_START_EXIT = 5,
    KEY_SHOW_STEP_MOVE_OUT = 6,
    KEY_SHOW_STEP_DONE = 7,
};

// Message columns after expanding the version-dependent city key into two city messages.
enum {
    KEY_EXCHANGE_MSG_COLUMN_EASY = 0,
    KEY_EXCHANGE_MSG_COLUMN_CHALLENGE = 1,
    KEY_EXCHANGE_MSG_COLUMN_TREEHOLLOW = 2,
    KEY_EXCHANGE_MSG_COLUMN_TOWER = 3,
    KEY_EXCHANGE_MSG_COLUMN_IRON = 4,
    KEY_EXCHANGE_MSG_COLUMN_ICEBERG = 5,
};

// The Key System's helpers: the key state, the key lists, the key messages and the key animations, and the scene
// tables of key_system_flow.c. The ROM doesn't name this file

// The frames of the key's moves in and out of view, of the plate's moves down and up and of a setting's reveal and
// hide (key_system_key_frames.c and key_system_plate_frames.c hold the same values as tables nothing reads)
#define KEY_IN_FRAMES 150
#define KEY_OUT_FRAMES 80
#define SHOW_IN_FRAMES 150
#define PLATE_DOWN_FRAMES 40
#define PLATE_UP_FRAMES 70
#define SHOW_OUT_FRAMES 40

static int KeySystemKeySelect_Select(int index, void *arg);
static void KeySystemKeySelect_SetupDifficulty(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectDifficulty(int index, KeySystemKeySelect *keySelect);
static void KeySystemKeySelect_SetupCity(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectCity(int index, KeySystemKeySelect *keySelect);
static void KeySystemKeySelect_SetupChamber(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectChamber(int index, KeySystemKeySelect *keySelect);
static BOOL KeySystemKeyAnim_Unlock(KeySystemKeyAnim *anim);
static BOOL KeySystemKeyAnim_Show(KeySystemKeyAnim *anim);

// The scenes' functions, for key_system_flow.c. MWCC heapsorts a unit's data by size, which for these nine
// equal-size tables gives the ROM's order (menu, connect, settings, difficulty, city, key animation, send,
// keys held, chamber) only from this declaration order
const KeySystemSceneFuncs sKeysHeldSceneFuncs = {
    KeySystemFlow_KeysHeldSceneInit,
    KeySystemFlow_KeysHeldSceneMain,
    KeySystemFlow_KeysHeldSceneExit,
    NULL,
};

const KeySystemSceneFuncs sSendSceneFuncs = {
    KeySystemFlow_SendSceneInit,
    KeySystemFlow_SendSceneMain,
    KeySystemFlow_SendSceneExit,
    NULL,
};

const KeySystemSceneFuncs sConnectSceneFuncs = {
    KeySystemFlow_ConnectSceneInit,
    KeySystemFlow_ConnectSceneMain,
    KeySystemFlow_ConnectSceneExit,
    NULL,
};

const KeySystemSceneFuncs sCitySceneFuncs = {
    KeySystemFlow_CitySceneInit,
    KeySystemFlow_CitySceneMain,
    KeySystemFlow_CitySceneExit,
    NULL,
};

const KeySystemSceneFuncs sDifficultySceneFuncs = {
    KeySystemFlow_DifficultySceneInit,
    KeySystemFlow_DifficultySceneMain,
    KeySystemFlow_DifficultySceneExit,
    NULL,
};

const KeySystemSceneFuncs sSettingsSceneFuncs = {
    KeySystemFlow_SettingsSceneInit,
    KeySystemFlow_SettingsSceneMain,
    KeySystemFlow_SettingsSceneExit,
    NULL,
};

const KeySystemSceneFuncs sKeyAnimSceneFuncs = {
    KeySystemFlow_KeyAnimSceneInit,
    KeySystemFlow_KeyAnimSceneMain,
    KeySystemFlow_KeyAnimSceneExit,
    NULL,
};

const KeySystemSceneFuncs sMenuSceneFuncs = {
    KeySystemFlow_MenuSceneInit,
    KeySystemFlow_MenuSceneMain,
    KeySystemFlow_MenuSceneExit,
    NULL,
};

const KeySystemSceneFuncs sChamberSceneFuncs = {
    KeySystemFlow_ChamberSceneInit,
    KeySystemFlow_ChamberSceneMain,
    KeySystemFlow_ChamberSceneExit,
    NULL,
};

void KeySystemKeyState_Init(KeySystemKeyState *keys, KeyInfoSave *keyInfo) {
    int i;

    MI_CpuFill8(keys, 0, sizeof(KeySystemKeyState));
    keys->version = GAME_VERSION;
    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        keys->unlocked[i] = KeyInfo_IsKeyUnlocked(keyInfo, i);
        keys->enabled[i] = keyEnabler(keyInfo, i);
    }
}

void KeySystemKeyState_Save(KeySystemKeyState *keys, KeyInfoSave *keyInfo) {
    int i;

    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        if (keys->unlocked[i]) {
            KeyInfo_UnlockKey(keyInfo, i);
        }
        if (keys->enabled[i]) {
            KeyInfo_EnableKey(keyInfo, i);
        }
    }
}

// Whether a key can be received from the other game, or sent to it. The city key only goes between the two versions
BOOL KeySystemKeyState_CanExchange(KeySystemKeyState *keys, KeySystemKeyState *partner, BOOL receive) {
    int i;
    int count = 0;

    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        if (receive) {
            if (!keys->enabled[i] && partner->unlocked[i] == TRUE) {
                if (i == KEY_SYSTEM_KEY_CITY) {
                    if (keys->version != partner->version) {
                        count++;
                    }
                } else {
                    count++;
                }
            }
        } else {
            if (keys->unlocked[i] == TRUE && !partner->enabled[i]) {
                if (i == KEY_SYSTEM_KEY_CITY) {
                    if (keys->version != partner->version) {
                        count++;
                    }
                } else {
                    count++;
                }
            }
        }
    }
    return count != 0 ? TRUE : FALSE;
}

void KeySystemKeyState_Receive(KeySystemKeyState *keys, KeySystemKeyState *partner) {
    int i;

    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        if (partner->unlocked[i]) {
            if (i == KEY_SYSTEM_KEY_CITY) {
                if (keys->version != partner->version) {
                    keys->enabled[i] = TRUE;
                }
            } else {
                keys->enabled[i] = TRUE;
            }
        }
    }
}

void KeySystemSaveInfo_Init(KeySystemSaveInfo *info, GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);

    MI_CpuFill8(info, 0, sizeof(KeySystemSaveInfo));
    info->hasSave = SaveControl_IsDataAlreadyPresent(save);
    if (info->hasSave) {
        MI_CpuCopy8(GetGameDataPlayerInfo(gameData), &info->player, sizeof(PlayerInfo));
    }
}

void KeySystemKeySelect_Init(KeySystemKeySelect *keySelect, u32 mode, KeySystemClAct *clact, KeySystemList **list,
                             MsgData *msgData, Font *font, KeyInfoSave *keyInfo, GameData *gameData, HeapID heapId) {
    KeySystemListSetup setup;
    ClActorPos pos;
    int i;

    MI_CpuFill8(keySelect, 0, sizeof(KeySystemKeySelect));
    keySelect->clact = clact;
    keySelect->keyInfo = keyInfo;
    keySelect->list = list;
    keySelect->gameData = gameData;
    switch (mode) {
    case KEY_LIST_DIFFICULTY:
        keySelect->setup = KeySystemKeySelect_SetupDifficulty;
        break;
    case KEY_LIST_CITY:
        keySelect->setup = KeySystemKeySelect_SetupCity;
        break;
    case KEY_LIST_CHAMBER:
        keySelect->setup = KeySystemKeySelect_SetupChamber;
        break;
    }
    // The locks of the items
    for (i = KEY_SYSTEM_ACTOR_LOCK_FIRST; i <= KEY_SYSTEM_ACTOR_LOCK_THIRD; i++) {
        ClActor_SetVisible(KeySystemClAct_Create(clact, i, heapId), FALSE);
    }
    // The cursor at the setting
    ClActor_SetVisible(KeySystemClAct_Create(clact, KEY_SYSTEM_ACTOR_CURRENT_SETTING, heapId), FALSE);
    MI_CpuFill8(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = msgData;
    setup.font = font;
    setup.cursor = 0;
    setup.arg = keySelect;
    setup.select = KeySystemKeySelect_Select;
    for (i = 0; i < KEY_SYSTEM_LIST_MAX; i++) {
        setup.items[i].width = 15;
        setup.items[i].height = 2;
        setup.items[i].x = 16;
        setup.items[i].y = i * (setup.items[0].height + 2) + 3;
        if (i + KEY_SYSTEM_ACTOR_LOCK_FIRST <= KEY_SYSTEM_ACTOR_LOCK_THIRD) {
            ClActor *lock = KeySystemClAct_GetActor(clact, i + KEY_SYSTEM_ACTOR_LOCK_FIRST);

            pos.x = (setup.items[i].x + setup.items[i].width - 1) * 8;
            pos.y = (setup.items[i].y + 1) * 8;
            ClActor_SetSurfacePos(lock, &pos, CLACT_SURFACE_MAIN);
        }
    }
    keySelect->setup(keySelect, &setup);
    (setup.items + setup.count - 1)->x += 3;
    (setup.items + setup.count - 1)->width -= 3;
    *keySelect->list = KeySystemList_Create(&setup, heapId);
}

void KeySystemKeySelect_Free(KeySystemKeySelect *keySelect) {
    KeySystemList_Free(*keySelect->list);
    *keySelect->list = NULL;
    KeySystemClAct_Delete(keySelect->clact, KEY_SYSTEM_ACTOR_LOCK_THIRD);
    KeySystemClAct_Delete(keySelect->clact, KEY_SYSTEM_ACTOR_LOCK_SECOND);
    KeySystemClAct_Delete(keySelect->clact, KEY_SYSTEM_ACTOR_LOCK_FIRST);
    KeySystemClAct_Delete(keySelect->clact, KEY_SYSTEM_ACTOR_CURRENT_SETTING);
}

void KeySystemKeySelect_Update(KeySystemKeySelect *keySelect) {
    KeySystemList_Update(*keySelect->list);
}

BOOL KeySystemKeySelect_IsPrinted(KeySystemKeySelect *keySelect) {
    return KeySystemList_IsPrinted(*keySelect->list);
}

u32 KeySystemKeySelect_GetCursor(KeySystemKeySelect *keySelect) {
    return KeySystemList_GetCursor(*keySelect->list);
}

BOOL KeySystemKeySelect_IsDecided(KeySystemKeySelect *keySelect) {
    return KeySystemList_IsDecided(*keySelect->list);
}

void KeySystemKeySelect_Reset(KeySystemKeySelect *keySelect) {
    keySelect->result = KEY_SELECT_NONE;
    keySelect->index = 0;
    KeySystemList_Reset(*keySelect->list);
}

int KeySystemKeySelect_GetResult(KeySystemKeySelect *keySelect) {
    return keySelect->result;
}

int KeySystemKeySelect_GetIndex(KeySystemKeySelect *keySelect) {
    return keySelect->index;
}

static int KeySystemKeySelect_Select(int index, void *arg) {
    KeySystemKeySelect *keySelect = arg;
    int result = keySelect->select(index, keySelect);

    switch (result) {
    case KEY_SELECT_CURRENT:
        return FALSE;
    case KEY_SELECT_LOCKED:
    case KEY_SELECT_IN_AREA:
        keySelect->result = result;
        keySelect->index = index;
        return FALSE;
    case KEY_SELECT_NONE:
    case KEY_SELECT_DONE:
    default:
        return TRUE;
    }
}

// Moves the cursor of the list to the setting
static inline void KeySystemKeySelect_MoveCursor(ClActor *cursor, const KeySystemListSetup *setup, int index) {
    ClActorPos pos;

    pos.x = (setup->items[index].x - 2) * 8;
    pos.y = (setup->items[index].y + 1) * 8;
    ClActor_SetSurfacePos(cursor, &pos, CLACT_SURFACE_MAIN);
    ClActor_SetVisible(cursor, TRUE);
}

static void KeySystemKeySelect_SetupDifficulty(KeySystemKeySelect *keySelect, KeySystemListSetup *setup) {
    ClActor *cursor;

    setup->count = 4;
    setup->items[GAME_DIFFICULTY_EASY].msgId = KeySystem_Text_EasyMode;
    setup->items[GAME_DIFFICULTY_NORMAL].msgId = KeySystem_Text_NormalMode;
    setup->items[GAME_DIFFICULTY_CHALLENGE].msgId = KeySystem_Text_ChallengeMode;
    setup->items[KEY_DIFFICULTY_MENU_BACK].msgId = KeySystem_Text_Back;
    keySelect->select = KeySystemKeySelect_SelectDifficulty;
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_EASY)) {
        ClActor_SetVisible(KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_LOCK_FIRST), TRUE);
    }
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_CHALLENGE)) {
        ClActor_SetVisible(KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_LOCK_THIRD), TRUE);
    }
    cursor = KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_CURRENT_SETTING);
    KeySystemKeySelect_MoveCursor(cursor, setup, GetGameDifficulty(keySelect->keyInfo));
}

static int KeySystemKeySelect_SelectDifficulty(int index, KeySystemKeySelect *keySelect) {
    const KeySystemListSetup *setup;

    if (index == GetGameDifficulty(keySelect->keyInfo)) {
        return KEY_SELECT_CURRENT;
    }
    if (index < KEY_DIFFICULTY_MENU_BACK) {
        switch (index) {
        case GAME_DIFFICULTY_EASY:
            if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_EASY)) {
                return KEY_SELECT_LOCKED;
            }
            break;
        case GAME_DIFFICULTY_CHALLENGE:
            if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_CHALLENGE)) {
                return KEY_SELECT_LOCKED;
            }
            break;
        }
        SetGameDifficulty(keySelect->keyInfo, index);
        setup = KeySystemList_GetSetup(*keySelect->list);
        KeySystemKeySelect_MoveCursor(KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_CURRENT_SETTING),
                                      setup, index);
    }
    return KEY_SELECT_DONE;
}

static void KeySystemKeySelect_SetupCity(KeySystemKeySelect *keySelect, KeySystemListSetup *setup) {
    ClActor *cursor;

    setup->count = KEY_CITY_MENU_COUNT;
#ifdef BLACK2
    setup->items[KEY_CITY_MENU_DEFAULT].msgId = KeySystem_Text_BlackCity;
    setup->items[KEY_CITY_MENU_OTHER].msgId = KeySystem_Text_WhiteForest;
#else
    setup->items[KEY_CITY_MENU_DEFAULT].msgId = KeySystem_Text_WhiteForest;
    setup->items[KEY_CITY_MENU_OTHER].msgId = KeySystem_Text_BlackCity;
#endif
    setup->items[KEY_CITY_MENU_BACK].msgId = KeySystem_Text_Back;
    keySelect->select = KeySystemKeySelect_SelectCity;
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_CITY)) {
        ClActor_SetVisible(KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_LOCK_SECOND), TRUE);
    }
    cursor = KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_CURRENT_SETTING);
    KeySystemKeySelect_MoveCursor(cursor, setup, KeyInfo_GetCityKey(keySelect->keyInfo));
}

// The two zones after the city's
static inline BOOL KeySystemFlow_IsCityZone(s16 zoneId) {
    s16 offset = zoneId - 478;

    return (u16)offset <= 1;
}

static int KeySystemKeySelect_SelectCity(int index, KeySystemKeySelect *keySelect) {
    const KeySystemListSetup *setup;
    ZoneSpawnInfo *spawn;
    u32 city = KeyInfo_GetCityKey(keySelect->keyInfo);

    if (city == index) {
        return KEY_SELECT_CURRENT;
    }
    if (index < KEY_CITY_MENU_BACK) {
        if (SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(keySelect->gameData))) {
            spawn = GetGameDataNowSpawnZone(keySelect->gameData);
            // The city, its tower or treehollow, or the zones next to them
            if (IsZoneBlackCityOrWhiteForestLobby(GetZoneParentZone(spawn->zoneId)) ||
                IsZoneBlackTowerOrWhiteTreehollow(spawn->zoneId) || KeySystemFlow_IsCityZone(spawn->zoneId)) {
                return KEY_SELECT_IN_AREA;
            }
        }
        if (index == KEY_CITY_MENU_OTHER && !keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_CITY)) {
            return KEY_SELECT_LOCKED;
        }
        KeyInfo_SetCityKey(keySelect->keyInfo, index);
        setup = KeySystemList_GetSetup(*keySelect->list);
        KeySystemKeySelect_MoveCursor(KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_CURRENT_SETTING),
                                      setup, index);
    }
    return KEY_SELECT_DONE;
}

static void KeySystemKeySelect_SetupChamber(KeySystemKeySelect *keySelect, KeySystemListSetup *setup) {
    ClActor *cursor;

    setup->count = 4;
    setup->items[KEY_INFO_CHAMBER_ROCK_PEAK].msgId = KeySystem_Text_RockPeakChamber;
    setup->items[KEY_INFO_CHAMBER_IRON].msgId = KeySystem_Text_IronChamber;
    setup->items[KEY_INFO_CHAMBER_ICEBERG].msgId = KeySystem_Text_IcebergChamber;
    setup->items[KEY_CHAMBER_MENU_BACK].msgId = KeySystem_Text_Back;
    keySelect->select = KeySystemKeySelect_SelectChamber;
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_IRON)) {
        ClActor_SetVisible(KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_LOCK_SECOND), TRUE);
    }
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_ICEBERG)) {
        ClActor_SetVisible(KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_LOCK_THIRD), TRUE);
    }
    cursor = KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_CURRENT_SETTING);
    KeySystemKeySelect_MoveCursor(cursor, setup, KeyInfo_GetChamberKey(keySelect->keyInfo));
}

static int KeySystemKeySelect_SelectChamber(int index, KeySystemKeySelect *keySelect) {
    const KeySystemListSetup *setup;

    if (index == KeyInfo_GetChamberKey(keySelect->keyInfo)) {
        return KEY_SELECT_CURRENT;
    }
    if (index < KEY_CHAMBER_MENU_BACK) {
        switch (index) {
        case KEY_INFO_CHAMBER_IRON:
            if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_IRON)) {
                return KEY_SELECT_LOCKED;
            }
            break;
        case KEY_INFO_CHAMBER_ICEBERG:
            if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_ICEBERG)) {
                return KEY_SELECT_LOCKED;
            }
            break;
        }
        KeyInfo_SetChamberKey(keySelect->keyInfo, index);
        setup = KeySystemList_GetSetup(*keySelect->list);
        KeySystemKeySelect_MoveCursor(KeySystemClAct_GetActor(keySelect->clact, KEY_SYSTEM_ACTOR_CURRENT_SETTING),
                                      setup, index);
    }
    return KEY_SELECT_DONE;
}

u16 KeySystemFlow_GetDifficultyMsg(KeyInfoSave *keyInfo) {
    return GetGameDifficulty(keyInfo) + KeySystem_Text_ChangeDifficultyCurrentMode;
}

u16 KeySystemFlow_GetCityMsg(KeyInfoSave *keyInfo) {
#ifdef BLACK2
    return KeyInfo_GetCityKey(keyInfo) + KeySystem_Text_SwapWhiteForestBlack;
#else
    return KeySystem_Text_SwapWhiteForestBlack_2 - KeyInfo_GetCityKey(keyInfo);
#endif
}

u16 KeySystemFlow_GetChamberMsg(KeyInfoSave *keyInfo) {
    return KeyInfo_GetChamberKey(keyInfo) + KeySystem_Text_ChangeWhatLiesBeyond;
}

u16 KeySystemFlow_GetSettingsMsg(KeyInfoSave *keyInfo, u16 item) {
    switch (item) {
    case KEY_LIST_DIFFICULTY:
        return KeySystemFlow_GetDifficultyMsg(keyInfo);
    case KEY_LIST_CITY:
        return KeySystemFlow_GetCityMsg(keyInfo);
    case KEY_LIST_CHAMBER:
        return KeySystemFlow_GetChamberMsg(keyInfo);
    case KEY_LIST_BACK:
        return KeySystem_Text_GoBackKeySystems;
    }
    return 0;
}

// Formats into wk->strBuf what receiving or sending a key says, and returns whether the key goes
BOOL KeySystemFlow_FormatKeyMsg(KeySystemWork *wk, int key, BOOL receive) {
    BOOL exchange;
    u16 msgId;
    StrBuf *str;

    if (receive) {
        exchange = FALSE;
        if (!wk->keys.enabled[key] && wk->partnerKeys.unlocked[key] == TRUE) {
            exchange = TRUE;
        }
    } else {
        exchange = FALSE;
        if (wk->keys.unlocked[key] == TRUE && !wk->partnerKeys.enabled[key]) {
            exchange = TRUE;
        }
    }
    if (key == KEY_SYSTEM_KEY_CITY && wk->keys.version == wk->partnerKeys.version) {
        return FALSE;
    }
    if (exchange) {
        // With the other player's name or without it
        if (wk->partnerSaveInfo.hasSave) {
            msgId = receive ? KeySystem_Text_EasyModeUnlockedBy : KeySystem_Text_SentEasyKey;
        } else {
            msgId = receive ? KeySystem_Text_EasyModeUnlockedBy_2 : KeySystem_Text_SentEasyKey_2;
        }
        // The city key has a message for each city
        if (key == KEY_SYSTEM_KEY_CITY) {
#ifdef BLACK2
            key = receive ? KEY_EXCHANGE_MSG_COLUMN_TREEHOLLOW : KEY_EXCHANGE_MSG_COLUMN_TOWER;
#else
            key = receive ? KEY_EXCHANGE_MSG_COLUMN_TOWER : KEY_EXCHANGE_MSG_COLUMN_TREEHOLLOW;
#endif
        } else if (key > KEY_SYSTEM_KEY_CITY) {
            key++;
        }
        msgId += (u16)key;
        str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
        if (wk->partnerSaveInfo.hasSave) {
            copyVarForText(wk->wordSet, 0, &wk->partnerSaveInfo.player);
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strBuf, str);
        GFL_StrBufFree(str);
        return TRUE;
    }
    return FALSE;
}

void KeySystemKeyAnim_Init(KeySystemKeyAnim *anim, KeySystemClAct *clact, KeySystemMsgWin *msgWin, Font *font,
                           HeapID heapId) {
    MI_CpuFill8(anim, 0, sizeof(KeySystemKeyAnim));
    anim->clact = clact;
    anim->msgWin = msgWin;
    anim->font = font;
    anim->oamSys = BmpOam_Init(heapId, KeySystemClAct_GetUnit(clact));
    anim->str = GFL_StrBufCreate(281, heapId);
    anim->keyActor = KeySystemClAct_Create(clact, KEY_SYSTEM_ACTOR_KEY, heapId);
    anim->effectActor = KeySystemClAct_Create(clact, KEY_SYSTEM_ACTOR_UNLOCK_EFFECT, heapId);
    anim->lockActor = KeySystemClAct_Create(clact, KEY_SYSTEM_ACTOR_ANIMATED_LOCK, heapId);
    anim->plateActor = KeySystemClAct_Create(clact, KEY_SYSTEM_ACTOR_NAMEPLATE, heapId);
    {
        ClActorSetup setup = { 192, 98, 0, 1, 1 };

        anim->text =
            KeySystemOamText_Create(&setup, 16, 2, KeySystemClAct_GetResource(clact, KEY_SYSTEM_RESOURCE_PALETTE), 3,
                                    CLACT_SURFACE_MAIN, anim->oamSys, heapId);
        BmpOam_ActorSetDrawEnable(KeySystemOamText_GetActor(anim->text), FALSE);
    }
}

void KeySystemKeyAnim_Free(KeySystemKeyAnim *anim) {
    KeySystemOamText_Free(anim->text);
    KeySystemClAct_Delete(anim->clact, KEY_SYSTEM_ACTOR_NAMEPLATE);
    KeySystemClAct_Delete(anim->clact, KEY_SYSTEM_ACTOR_ANIMATED_LOCK);
    KeySystemClAct_Delete(anim->clact, KEY_SYSTEM_ACTOR_UNLOCK_EFFECT);
    KeySystemClAct_Delete(anim->clact, KEY_SYSTEM_ACTOR_KEY);
    GFL_StrBufFree(anim->str);
    BmpOam_Exit(anim->oamSys);
}

void KeySystemKeyAnim_Start(KeySystemKeyAnim *anim, u32 key, MsgData *msgData, u16 msgId, StrBuf *str, BOOL enabled) {
    u32 palette;

    anim->frame = 0;
    anim->state = 0;
    anim->timer = 0;
    anim->key = key;
    anim->msgId = msgId;
    anim->active = TRUE;
    anim->msgData = msgData;
    GFL_StrBufCopy(anim->str, str);
    palette = sKeyPalettes[key];
    if (key == KEY_SYSTEM_KEY_CITY) {
#ifdef BLACK2
        palette = 1;
        if (!enabled) {
            palette = 2;
        }
#else
        palette = 2;
        if (!enabled) {
            palette = 1;
        }
#endif
    }
    ClActor_SetPalette(anim->keyActor, palette, 1);
    if (enabled) {
        anim->func = KeySystemKeyAnim_Show;
    } else {
        anim->func = KeySystemKeyAnim_Unlock;
    }
}

BOOL KeySystemKeyAnim_Update(KeySystemKeyAnim *anim) {
    return anim->func(anim);
}

// The key comes down, the plate with its name rises, and the lock opens
static BOOL KeySystemKeyAnim_Unlock(KeySystemKeyAnim *anim) {
    if (!anim->active) {
        return TRUE;
    }
    anim->frame++;
    switch (anim->state) {
    case KEY_UNLOCK_STEP_PRINT_NAME:
        KeySystemOamText_SetColor(anim->text, PRINT_COLOR(1, 2, 0));
        KeySystemOamText_Print(anim->text, anim->msgData, anim->msgId, anim->font);
        anim->state++;
    case KEY_UNLOCK_STEP_WAIT_NAME:
        KeySystemOamText_Update(anim->text);
        if (KeySystemOamText_IsDone(anim->text)) {
            anim->state++;
        }
        break;
    case KEY_UNLOCK_STEP_START_PLATE_RISE:
        KeySystemAccelMove_Init(&anim->move, &sPlateUpStart, &sPlateUpEnd, FX32_CONST(-3), PLATE_UP_FRAMES);
        ClActor_SetVisible(anim->plateActor, TRUE);
        ClActor_SetVisible(anim->lockActor, TRUE);
        ClActor_SetSequence(anim->lockActor, KEY_SYSTEM_ANIM_LOCK_CLOSED);
        BmpOam_ActorSetDrawEnable(KeySystemOamText_GetActor(anim->text), TRUE);
        anim->state++;
    case KEY_UNLOCK_STEP_MOVE_PLATE_UP: {
        ClActorPos pos;
        ClActorPos actorPos;
        BmpOamActor *actor;

        if (KeySystemAccelMove_Update(&anim->move)) {
            anim->state++;
        }
        KeySystemAccelMove_GetPos(&anim->move, &pos);
        actorPos = pos;
        ClActor_SetSurfacePos(anim->plateActor, &actorPos, CLACT_SURFACE_MAIN);
        actorPos = pos;
        actorPos.x += 56;
        ClActor_SetSurfacePos(anim->lockActor, &actorPos, CLACT_SURFACE_MAIN);
        actor = KeySystemOamText_GetActor(anim->text);
        actorPos = pos;
        actorPos.x -= 64;
        actorPos.y -= 8;
        BmpOam_ActorSetPos(actor, actorPos.x, actorPos.y);
        break;
    }
    case KEY_UNLOCK_STEP_WAIT_PLATE:
        if (anim->timer++ > 60) {
            anim->timer = 0;
            anim->state++;
        }
        break;
    case KEY_UNLOCK_STEP_START_KEY_DROP:
        KeySystemTween_Init(&anim->tween, &sKeyInStart, &sKeyInEnd, KEY_IN_FRAMES);
        ClActor_SetVisible(anim->keyActor, TRUE);
        ClActor_SetSequence(anim->keyActor, KEY_SYSTEM_ANIM_KEY_DROP);
        ClActor_SetAutoAnim(anim->keyActor, TRUE);
        GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_02);
        anim->state++;
    case KEY_UNLOCK_STEP_MOVE_KEY_DOWN: {
        ClActorPos pos;

        if (KeySystemTween_Update(&anim->tween) && ClActor_GetAnimFrame(anim->keyActor) == 0) {
            ClActor_StopAnim(anim->keyActor);
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        ClActor_SetSurfacePos(anim->keyActor, &pos, CLACT_SURFACE_MAIN);
        break;
    }
    case KEY_UNLOCK_STEP_WAIT_KEY:
        if (anim->timer++ > 60) {
            anim->timer = 0;
            anim->state++;
        }
        break;
    case KEY_UNLOCK_STEP_START_KEY_TURN:
        KeySystemTween_Init(&anim->tween, &sKeyOutStart, &sKeyOutEnd, KEY_OUT_FRAMES);
        ClActor_SetSequence(anim->keyActor, KEY_SYSTEM_ANIM_KEY_TURN);
        anim->state++;
    case KEY_UNLOCK_STEP_MOVE_KEY_OUT: {
        ClActorPos pos;

        if (KeySystemTween_Update(&anim->tween) && !ClActor_IsAnimActive(anim->keyActor)) {
            GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_03);
            ClActor_SetVisible(anim->keyActor, FALSE);
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        ClActor_SetSurfacePos(anim->keyActor, &pos, CLACT_SURFACE_MAIN);
        break;
    }
    case KEY_UNLOCK_STEP_START_LOCK_OPEN:
        ClActor_SetSequence(anim->lockActor, KEY_SYSTEM_ANIM_LOCK_OPEN);
        ClActor_SetAutoAnim(anim->lockActor, TRUE);
        anim->state++;
        break;
    case KEY_UNLOCK_STEP_WAIT_LOCK:
        if (!ClActor_IsAnimActive(anim->lockActor)) {
            ClActor_SetVisible(anim->lockActor, FALSE);
            anim->state++;
        }
        break;
    case KEY_UNLOCK_STEP_START_EFFECT:
        ClActor_SetSurfacePos(anim->effectActor, &sEffectPos, CLACT_SURFACE_MAIN);
        ClActor_SetVisible(anim->effectActor, TRUE);
        ClActor_SetSequence(anim->effectActor, KEY_SYSTEM_ANIM_UNLOCK_EFFECT);
        ClActor_SetAutoAnim(anim->effectActor, TRUE);
        GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_04);
        anim->state++;
        break;
    case KEY_UNLOCK_STEP_WAIT_EFFECT:
        if (!ClActor_IsAnimActive(anim->effectActor)) {
            ClActor_SetVisible(anim->effectActor, FALSE);
            anim->state++;
        }
        break;
    case KEY_UNLOCK_STEP_WAIT_EFFECT_END:
        if (anim->timer++ > 120) {
            anim->timer = 0;
            anim->state++;
        }
        break;
    case KEY_UNLOCK_STEP_PRINT_RESULT:
        KeySystemMsgWin_PrintStr(anim->msgWin, anim->str, KEY_SYSTEM_MSG_STREAM_FAST);
        GFL_BGSysQueueScrLoad(0);
        anim->state++;
        break;
    case KEY_UNLOCK_STEP_WAIT_RESULT:
        if (KeySystemMsgWin_IsDone(anim->msgWin)) {
            anim->state++;
        }
        break;
    case KEY_UNLOCK_STEP_START_PLATE_DROP:
        KeySystemTween_Init(&anim->tween, &sPlateDownStart, &sPlateDownEnd, PLATE_DOWN_FRAMES);
        anim->state++;
    case KEY_UNLOCK_STEP_MOVE_PLATE_DOWN: {
        ClActorPos pos;
        ClActorPos actorPos;
        BmpOamActor *actor;

        if (KeySystemTween_Update(&anim->tween)) {
            ClActor_SetVisible(anim->plateActor, FALSE);
            ClActor_SetVisible(anim->lockActor, FALSE);
            BmpOam_ActorSetDrawEnable(KeySystemOamText_GetActor(anim->text), FALSE);
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        actorPos = pos;
        ClActor_SetSurfacePos(anim->plateActor, &actorPos, CLACT_SURFACE_MAIN);
        actorPos = pos;
        actorPos.x += 56;
        ClActor_SetSurfacePos(anim->lockActor, &actorPos, CLACT_SURFACE_MAIN);
        actor = KeySystemOamText_GetActor(anim->text);
        actorPos = pos;
        actorPos.x -= 64;
        actorPos.y -= 8;
        BmpOam_ActorSetPos(actor, actorPos.x, actorPos.y);
        break;
    }
    case KEY_UNLOCK_STEP_DONE:
        anim->active = FALSE;
        return TRUE;
    }
    return FALSE;
}

// The key comes down and goes, for a key whose setting is enabled already
static BOOL KeySystemKeyAnim_Show(KeySystemKeyAnim *anim) {
    if (!anim->active) {
        return TRUE;
    }
    anim->frame++;
    switch (anim->state) {
    case KEY_SHOW_STEP_START_DROP:
        KeySystemTween_Init(&anim->tween, &sShowInStart, &sShowInEnd, SHOW_IN_FRAMES);
        ClActor_SetVisible(anim->keyActor, TRUE);
        ClActor_SetAutoAnim(anim->keyActor, TRUE);
        GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_02);
        anim->state++;
    case KEY_SHOW_STEP_MOVE_DOWN: {
        ClActorPos pos;

        if (KeySystemTween_Update(&anim->tween) && ClActor_GetAnimFrame(anim->keyActor) == 0) {
            ClActor_StopAnim(anim->keyActor);
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        ClActor_SetSurfacePos(anim->keyActor, &pos, CLACT_SURFACE_MAIN);
        break;
    }
    case KEY_SHOW_STEP_WAIT_KEY:
        if (anim->timer++ > 60) {
            anim->timer = 0;
            anim->state++;
        }
        break;
    case KEY_SHOW_STEP_PRINT_RESULT:
        KeySystemMsgWin_PrintStr(anim->msgWin, anim->str, KEY_SYSTEM_MSG_STREAM_FAST);
        GFL_BGSysQueueScrLoad(0);
        anim->state++;
        break;
    case KEY_SHOW_STEP_WAIT_RESULT:
        if (KeySystemMsgWin_IsDone(anim->msgWin)) {
            anim->state++;
        }
        break;
    case KEY_SHOW_STEP_START_EXIT:
        KeySystemTween_Init(&anim->tween, &sShowOutStart, &sShowOutEnd, SHOW_OUT_FRAMES);
        anim->state++;
    case KEY_SHOW_STEP_MOVE_OUT: {
        ClActorPos pos;

        if (KeySystemTween_Update(&anim->tween)) {
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        ClActor_SetSurfacePos(anim->keyActor, &pos, CLACT_SURFACE_MAIN);
        break;
    }
    case KEY_SHOW_STEP_DONE:
        anim->active = FALSE;
        return TRUE;
    }
    return FALSE;
}

// The infrared phase ended; the exchange continues over wireless.
void KeySystemFlow_OnInfraredEnded(void *work) {
    KeySystemWork *wk = work;

    wk->choice = TRUE;
    wk->noCancel = TRUE;
}
