#include "types.h"
#include "app/demo3d.h"
#include "field/event_3d_demo.h"
#include "field/event_sound.h"
#include "field/game_beacon_set.h"
#include "gfl/std.h"
#include "nitro/rtc.h"
#include "save/player_info.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/season.h"
#include "system/zone_weather.h"

// The demos the event reacts to: the Ferris wheel, whose beacon its start sends, and the two cargo plane flights
// between Mistralton City and Lentimas Town, which mistralton_city_3.s and lentimas_town.s start with these ids
enum {
    DEMO3D_ID_FERRIS_WHEEL = 3,
    DEMO3D_ID_MISTRALTON_FLIGHT = 24,
    DEMO3D_ID_LENTIMAS_FLIGHT = 25,
};

// Event3DDemo_Callback's steps: the communication's pause, the demo, and the music and weather of its end
enum {
    EVENT_3D_DEMO_STEP_PAUSE_COMM,
    EVENT_3D_DEMO_STEP_WAIT_COMM_EXIT,
    EVENT_3D_DEMO_STEP_START_DEMO,
    EVENT_3D_DEMO_STEP_WAIT_DEMO,
    EVENT_3D_DEMO_STEP_END,
};

GameEvent *Event3DDemo_Create(GameSystem *gsys, GameEvent *parent, u32 demoId, u32 param, u32 realTime) {
    GameEvent *event = GameEvent_Create(gsys, parent, Event3DDemo_Callback, sizeof(Event3DDemoWork));
    Event3DDemoWork *work = GameEvent_GetData(event);
    MI_CpuFill8(work, 0, sizeof(Event3DDemoWork));
    work->gameSystem = gsys;
    work->gameCommSys = GSYS_GetGameCommSystem(gsys);
    work->demoId = demoId;
    if (realTime) {
        SetupPlaySequenceEventRealTime(&work->param, gsys, demoId, param);
    } else {
        SetupPlaySequenceEventGameTime(&work->param, gsys, demoId, param);
    }
    if (demoId == DEMO3D_ID_FERRIS_WHEEL) {
        GameBeacon_BroadcastFerrisWheel();
    }
    return event;
}

GameEventReturnCode Event3DDemo_Callback(GameEvent *event, u32 *state, void *data) {
    Event3DDemoWork *work = data;

    switch (*state) {
    case EVENT_3D_DEMO_STEP_PAUSE_COMM:
        work->savedState = GSYS_IsGameCommPaused(work->gameSystem);
        GSYS_SetGameCommPaused(work->gameSystem, 1);
        if (!GameCommSys_BootCheck(work->gameCommSys)) {
            *state += 2;
        } else {
            GameCommSys_ExitReq(work->gameCommSys);
            (*state)++;
        }
        break;
    case EVENT_3D_DEMO_STEP_WAIT_COMM_EXIT:
        if (!GameCommSys_BootCheck(work->gameCommSys)) {
            (*state)++;
        }
        break;
    case EVENT_3D_DEMO_STEP_START_DEMO:
        GSYS_QueueProc(work->gameSystem, OVERLAY_DEMO3D, &gDemo3DProcFunctions, &work->param);
        (*state)++;
        break;
    case EVENT_3D_DEMO_STEP_WAIT_DEMO:
        if (GSYS_GetProcMgrState(work->gameSystem)) {
            break;
        }
        GSYS_SetGameCommPaused(work->gameSystem, work->savedState);
        if (work->demoId == DEMO3D_ID_MISTRALTON_FLIGHT || work->demoId == DEMO3D_ID_LENTIMAS_FLIGHT) {
            GameEvent_ChainNext(event, CreateBGMFadeOutEvent(work->gameSystem, 0));
            if (work->demoId == DEMO3D_ID_LENTIMAS_FLIGHT) {
                SetNowWeather(GSYS_GetGameData(work->gameSystem), WEATHER_SUNNY);
            }
        }
        (*state)++;
        break;
    case EVENT_3D_DEMO_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void CreateSeqLoadParam(Demo3DParam *out, GameSystem *gsys, u32 demoId, u8 param, u32 startFrame, u8 hour, u8 minute,
                        u8 season) {
    MI_CpuFill8(out, 0, sizeof(Demo3DParam));
    out->gsys = gsys;
    out->demoId = demoId;
    out->param = param;
    out->startFrame = startFrame;
    out->hour = hour;
    out->minute = minute;
    out->season = season;
    out->playerSex = getTrainerGender(GetGameDataPlayerInfo(GSYS_GetGameData(gsys)));
}

void SetupPlaySequenceEventRealTime(Demo3DParam *out, GameSystem *gsys, u8 demoId, u8 param) {
    RTCTime time;
    u8 season;

    RTC_GetCachedTime(&time);
    season = Season_GetRealTime();
    CreateSeqLoadParam(out, gsys, demoId, param, 0, time.hour, time.minute, season);
}

void SetupPlaySequenceEventGameTime(Demo3DParam *out, GameSystem *gsys, u8 demoId, u8 param) {
    GameData *data;
    u32 hour;
    u32 minute;
    u8 season;

    data = GSYS_GetGameData(gsys);
    hour = getCurrentHour(data);
    minute = getCurrentMinute(data);
    season = GameData_GetSeason(data);
    CreateSeqLoadParam(out, gsys, demoId, param, 0, hour, minute, season);
}
