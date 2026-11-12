// The event that turns the C-Gear on: it stops the game's communication, starts it again when asked to, and changes the
// subscreen to the C-Gear. Function names from swan; the file's name is descriptive, after event_cgear_shutdown.c
#include "types.h"
#include "field/event_cgear_poweron.h"
#include "field/field.h"
#include "field/subscreen.h"
#include "gfl/std.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

static void EventCGearPowerOn_SubscreenCallback(void *work) {
    CGearPowerOnData *data = work;

    data->subscreenChanged = TRUE;
}

// EventCGearPowerOn_Callback's steps: stopping the communication, booting it when asked to, and the C-Gear's screen
enum {
    CGEAR_POWER_ON_STEP_STOP_COMM,
    CGEAR_POWER_ON_STEP_WAIT_COMM_EXIT,
    CGEAR_POWER_ON_STEP_BOOT_COMM,
    CGEAR_POWER_ON_STEP_CHANGE_SUBSCREEN,
    CGEAR_POWER_ON_STEP_WAIT_SUBSCREEN,
    CGEAR_POWER_ON_STEP_END,
};

static GameEventReturnCode EventCGearPowerOn_Callback(GameEvent *event, u32 *state, void *work) {
    CGearPowerOnData *data = work;
    GameSystem *gsys = data->gsys;
    FieldSubscreen *subscreen = Field_GetSubscreen(GSYS_GetField(gsys));
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);

    switch (*state) {
    case CGEAR_POWER_ON_STEP_STOP_COMM:
        if (GameCommSys_BootCheck(comm)) {
            GameCommSys_ExitReq(comm);
        }
        (*state)++;
        break;
    case CGEAR_POWER_ON_STEP_WAIT_COMM_EXIT:
        if (!GameCommSys_BootCheck(comm)) {
            GSYS_SetCGearOn(gsys, 0);
            (*state)++;
        }
        break;
    case CGEAR_POWER_ON_STEP_BOOT_COMM:
        if (data->bootComm) {
            GSYS_SetCGearOn(gsys, 1);
            GSYS_TryBootGameComm(gsys);
            GameData_SetCGearPowerRequest(GSYS_GetGameData(gsys), 0);
        }
        (*state)++;
        break;
    case CGEAR_POWER_ON_STEP_CHANGE_SUBSCREEN:
        FieldSubscreen_ReqChangeEx(subscreen, FLD_SUBSCREEN_ID_CGEAR_WELCOME, EventCGearPowerOn_SubscreenCallback,
                                   data);
        (*state)++;
        break;
    case CGEAR_POWER_ON_STEP_WAIT_SUBSCREEN:
        if (data->subscreenChanged) {
            (*state)++;
        }
        break;
    case CGEAR_POWER_ON_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventCGearPowerOn_Create(GameSystem *gsys, BOOL bootComm) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventCGearPowerOn_Callback, sizeof(CGearPowerOnData));
    CGearPowerOnData *data = GameEvent_GetData(event);

    MI_CpuFill8(data, 0, sizeof(CGearPowerOnData));
    data->gsys = gsys;
    data->bootComm = bootComm;
    return event;
}
