#include "types.h"
#include "app/box2.h"
#include "app/dwc_utility.h"
#include "app/gsync.h"
#include "app/wifi_login.h"
#include "field/event_gsync.h"
#include "field/field_event.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// Game Sync's event phases, including its menu and child procs.
enum {
    GSYNC_EVENT_FADE_OUT_FIELD = 0,
    GSYNC_EVENT_CLOSE_FIELD = 1,
    GSYNC_EVENT_OPEN_MENU = 2,
    GSYNC_EVENT_WAIT_MENU = 3,
    GSYNC_EVENT_DEFER_WIFI_SETTINGS = 5,
    GSYNC_EVENT_OPEN_WIFI_SETTINGS = 6,
    GSYNC_EVENT_WAIT_WIFI_SETTINGS = 7,
    GSYNC_EVENT_OPEN_FIELD = 10,
    GSYNC_EVENT_FADE_IN_FIELD = 11,
    GSYNC_EVENT_RESTORE_FIELD_BGM = 12,
    GSYNC_EVENT_END = 13,
    GSYNC_EVENT_RESTART_SOUND = 14,
    GSYNC_EVENT_RETURN_FROM_WIFI_SETTINGS = 15,
    GSYNC_EVENT_CHECK_POKEMON = 16,
    GSYNC_EVENT_OPEN_LOGIN = 17,
    GSYNC_EVENT_WAIT_LOGIN = 18,
    GSYNC_EVENT_OPEN_GAME_SYNC = 19,
    GSYNC_EVENT_WAIT_GAME_SYNC = 20,
    GSYNC_EVENT_OPEN_POKEMON_SELECTION = 21,
    GSYNC_EVENT_HANDLE_POKEMON_SELECTION = 22,
    GSYNC_EVENT_OPEN_LOGOUT = 23,
    GSYNC_EVENT_WAIT_LOGOUT = 24,
};

GameEventReturnCode EventGameSync_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventGameSync_Callback(GameEvent *event, u32 *state, void *data) {
    EventGameSync *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case GSYNC_EVENT_FADE_OUT_FIELD:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case GSYNC_EVENT_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        wk->fromWifiSettings = FALSE;
        *state = GSYNC_EVENT_OPEN_MENU;
        break;
    case GSYNC_EVENT_OPEN_MENU:
        MI_CpuFill8(&wk->accountInfo, 0, sizeof(wk->accountInfo));
        wk->hasAccountInfo = FALSE;
        GSYS_QueueProc(gsys, OVERLAY_GSYNC, &GSYNC_MENU_PROC_FUNCTIONS, wk);
        (*state)++;
        break;
    case GSYNC_EVENT_WAIT_MENU:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->gsyncResult == GSYNC_RESULT_CONNECT) {
                *state = GSYNC_EVENT_CHECK_POKEMON;
            } else if (wk->gsyncResult == GSYNC_RESULT_WIFI_SETTINGS) {
                *state = GSYNC_EVENT_DEFER_WIFI_SETTINGS;
            } else {
                *state = GSYNC_EVENT_OPEN_FIELD;
            }
        }
        break;
    case GSYNC_EVENT_DEFER_WIFI_SETTINGS:
        (*state)++;
        break;
    case GSYNC_EVENT_OPEN_WIFI_SETTINGS:
        // Nintendo Wi-Fi Connection settings, which need the sound heap's memory
        wk->bgm = GFL_SndBGMGetID();
        GFL_SndDestroyHeap();
        GSYS_QueueProc(gsys, OVERLAY_DWC_UTILITY, &DWC_UTILITY_PROC_FUNCTIONS, wk);
        (*state)++;
        break;
    case GSYNC_EVENT_WAIT_WIFI_SETTINGS:
        if (!GSYS_GetProcMgrState(gsys)) {
            *state = GSYNC_EVENT_RESTART_SOUND;
        }
        break;
    case GSYNC_EVENT_RESTART_SOUND:
        // Back from the Wi-Fi settings
        OS_EnableIrq();
        GFL_SndInit();
        GFL_SndBGMPlay(wk->bgm, SND_CHANNEL_MASK_ALL);
        GFL_SndBGMFadeIn(60);
        *state = GSYNC_EVENT_RETURN_FROM_WIFI_SETTINGS;
        break;
    case GSYNC_EVENT_RETURN_FROM_WIFI_SETTINGS:
        wk->fromWifiSettings = TRUE;
        *state = GSYNC_EVENT_OPEN_MENU;
        break;
    case GSYNC_EVENT_OPEN_FIELD:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case GSYNC_EVENT_FADE_IN_FIELD:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case GSYNC_EVENT_RESTORE_FIELD_BGM:
        if (wk->bgmPushed) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            GFL_SndBGMFadeIn(60);
            wk->bgmPushed = FALSE;
        }
        (*state)++;
        break;
    case GSYNC_EVENT_END:
        return GAMEEVENT_DONE;
    case GSYNC_EVENT_CHECK_POKEMON: {
        BOOL asleep =
            DreamWorldSave_IsPokemonAsleep(getDreamWorldStuffAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));

        if (howManyNormalPokesAreInAllBoxes(GameData_GetBoxSaveAccessor(GSYS_GetGameData(gsys))) == 0 && !asleep) {
            wk->gsyncResult = GSYNC_RESULT_NO_POKEMON;
            *state = GSYNC_EVENT_OPEN_GAME_SYNC;
        } else {
            wk->login.mode = WIFILOGIN_MODE_NORMAL;
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            wk->bgmPushed = TRUE;
            (*state)++;
        }
        break;
    }
    case GSYNC_EVENT_OPEN_LOGIN:
        wk->login.buffer = wk->loginBuffer;
        GSYS_QueueProc(gsys, OVERLAY_WIFILOGIN, &WIFILOGIN_PROC_FUNCTIONS, &wk->login);
        (*state)++;
        break;
    case GSYNC_EVENT_WAIT_LOGIN:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->login.result == WIFILOGIN_RESULT_FAILED) {
                *state = GSYNC_EVENT_OPEN_FIELD;
            } else if (!DreamWorldSave_IsPokemonAsleep(getDreamWorldStuffAddress(wk->save))) {
                wk->gsyncResult = GSYNC_RESULT_WIFI_SETTINGS;
                *state = GSYNC_EVENT_OPEN_GAME_SYNC;
            } else {
                wk->gsyncResult = GSYNC_RESULT_NONE;
                *state = GSYNC_EVENT_OPEN_GAME_SYNC;
            }
        }
        break;
    case GSYNC_EVENT_OPEN_GAME_SYNC:
        GSYS_QueueProc(gsys, OVERLAY_GSYNC, &GSYNC_PROC_FUNCTIONS, wk);
        (*state)++;
        break;
    case GSYNC_EVENT_WAIT_GAME_SYNC:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->gsyncResult == GSYNC_RESULT_ACCOUNT) {
                wk->fromWifiSettings = FALSE;
                *state = GSYNC_EVENT_OPEN_MENU;
            } else if (wk->gsyncResult == GSYNC_RESULT_SELECT_POKEMON) {
                *state = GSYNC_EVENT_OPEN_POKEMON_SELECTION;
            } else if (wk->gsyncResult == GSYNC_RESULT_NO_POKEMON) {
                *state = GSYNC_EVENT_OPEN_POKEMON_SELECTION;
            } else if (wk->gsyncResult == GSYNC_RESULT_RETRY_LOGIN) {
                wk->login.mode = WIFILOGIN_MODE_RETRY;
                *state = GSYNC_EVENT_OPEN_LOGIN;
            } else {
                *state = GSYNC_EVENT_OPEN_LOGOUT;
            }
        }
        break;
    case GSYNC_EVENT_OPEN_POKEMON_SELECTION:
        // Choose the Pokemon to send to the Dream World
        wk->box.gameData = GSYS_GetGameData(gsys);
        wk->box.boxes = GameData_GetBoxSaveAccessor(wk->box.gameData);
        wk->box.party = GameData_GetParty(wk->box.gameData);
        wk->box.bag = GameData_GetBag(wk->box.gameData);
        wk->box.playerInfo = GetGameDataPlayerInfo(wk->box.gameData);
        wk->box.trainerData = getTrainerDataBlkAddress(GameData_GetSaveControl(wk->box.gameData));
        wk->box.sendableSpecies = wk->sendableSpecies;
        wk->box.isNationalDex = FALSE;
        wk->box.mode = BOX2_MODE_DREAM_WORLD;
        GSYS_QueueProcAsEvent(event, OVERLAY_BOX2, &BOX2_PROC_FUNCTIONS, &wk->box);
        (*state)++;
        break;
    case GSYNC_EVENT_HANDLE_POKEMON_SELECTION:
        if (GFL_NetErrCheck()) {
            GFL_NetErrMarkShown();
            GFL_NetErrShow(0);
            wk->login.mode = WIFILOGIN_MODE_RETRY;
            *state = GSYNC_EVENT_OPEN_LOGIN;
        } else {
            wk->boxTray = wk->box.tray;
            wk->boxPosition = wk->box.position;
            if (wk->box.tray == BOX2_GET_NONE && wk->box.position == BOX2_GET_NONE) {
                *state = GSYNC_EVENT_OPEN_LOGOUT;
            } else {
                if (wk->gsyncResult == GSYNC_RESULT_NO_POKEMON) {
                    wk->gsyncResult = GSYNC_RESULT_CONNECT;
                } else {
                    wk->gsyncResult = GSYNC_RESULT_ACCOUNT;
                }
                *state = GSYNC_EVENT_OPEN_GAME_SYNC;
            }
        }
        break;
    case GSYNC_EVENT_OPEN_LOGOUT:
        wk->logout.gameData = GSYS_GetGameData(gsys);
        wk->logout.displayStyle = WIFILOGIN_DISP_STYLE_GSYNC;
        wk->logout.messageStyle = WIFILOGIN_MESSAGE_STYLE_PDWACC;
        GSYS_QueueProc(gsys, OVERLAY_WIFILOGIN, &WIFILOGOUT_PROC_FUNCTIONS, &wk->logout);
        (*state)++;
        break;
    case GSYNC_EVENT_WAIT_LOGOUT:
        if (!GSYS_GetProcMgrState(gsys)) {
            GSYS_TryBootGameComm(gsys);
            reg_GX_POWCNT |= REG_GX_POWCNT_DSEL_MASK;
            wk->fromWifiSettings = FALSE;
            *state = GSYNC_EVENT_OPEN_MENU;
            if (wk->bgmPushed) {
                GFL_SndBGMPop();
                GFL_SndBGMSetPaused(FALSE);
                GFL_SndBGMFadeIn(60);
                wk->bgmPushed = FALSE;
            }
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

// Called through GameEvent_CreateOverlayDelegate, by the C-Gear's Game Sync button in overlay 79
GameEvent *EventGameSync_CreateFromArgs(GameSystem *gsys, void *args) {
    return EventGameSync_Create(gsys);
}

GameEvent *EventGameSync_Create(GameSystem *gsys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameSync_Callback, sizeof(EventGameSync));
    EventGameSync *wk = GameEvent_GetData(event);

    wk->gameData = GSYS_GetGameData(gsys);
    wk->save = GameData_GetSaveControl(wk->gameData);
    wk->gsys = gsys;
    wk->login.gameData = GSYS_GetGameData(gsys);
    wk->login.displayStyle = WIFILOGIN_DISP_STYLE_GSYNC;
    wk->login.messageStyle = WIFILOGIN_MESSAGE_STYLE_PDWACC;
    wk->login.serviceId = WIFILOGIN_SERVICE_GAME_SYNC;
    return event;
}
