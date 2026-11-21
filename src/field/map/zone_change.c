#include "types.h"
#include "field/actor/field_actor.h"
#include "field/encounter/encounter.h"
#include "field/event/event_data.h"
#include "field/event/hidden_item.h"
#include "field/map/zone.h"
#include "field/map/zone_change.h"
#include "save/event_work.h"
#include "system/field_status.h"
#include "system/game_data.h"
#include "system/savedata/save_control.h"

void ZoneChange_OnSeamless(GameData *gameData, Field *field) {
    SaveControl_GetEncountSave(GameData_GetSaveControl(gameData));
    Encount_OnZoneChange(gameData);
    EventWork_FlagReset(GameData_GetEventWork(gameData), 0x964);
    GameData_LoadEncountZone(gameData, field);
}

void ZoneChange_OnMapChange(GameData *gameData, u16 zoneId) {
    FieldStatus *status = GameData_GetFieldStatus(gameData);
    EventWork *eventWork;

    if (!(GetZoneFlashFlags(zoneId) & 3)) {
        FieldStatus_SetFlashUsed(status, FALSE);
    }
    eventWork = GameData_GetEventWork(gameData);
    EventWork_FlagReset(eventWork, 0x964);
    TryClearRepeatableHiddenItemFlags(eventWork);
    if (IsZoneAbyssalRuinsInside(zoneId) == FALSE) {
        PlayerSave_SetAbyssalRuinsStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)), 0);
    }
}

void ZoneChange_OnFly(GameData *gameData, u16 zoneId) {
    GameData_ResetPhenomenonSteps(gameData);
    ZoneChange_ResetStrengthRocks(GameData_GetMMSys(gameData));
    PlayerSave_EndStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
}

void ZoneChange_OnTeleport(GameData *gameData, u16 zoneId) {
    GameData_ResetPhenomenonSteps(gameData);
    ZoneChange_ResetStrengthRocks(GameData_GetMMSys(gameData));
    PlayerSave_EndStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
}

void ZoneChange_OnEscape(GameData *gameData, u16 zoneId) {
    GameData_ResetPhenomenonSteps(gameData);
    ZoneChange_ResetStrengthRocks(GameData_GetMMSys(gameData));
    PlayerSave_EndStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
}

void ZoneChange_OnBlackout(GameData *gameData, u16 zoneId) {
    GameData_ResetPhenomenonSteps(gameData);
    ZoneChange_ResetStrengthRocks(GameData_GetMMSys(gameData));
    PlayerSave_EndStepCounter(SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData)));
}

void ZoneChange_OnGameStart(GameData *gameData, u16 zoneId) {
}

void ZoneChange_OnFieldLoad(GameData *gameData, u16 zoneId) {
}

void ZoneChange_ResetStrengthRocks(MMSys *system) {
    ResetStrengthRockSaveDataPosition(system, 0x1e5, 0);
    ResetStrengthRockSaveDataPosition(system, 0x1e5, 1);
    ResetStrengthRockSaveDataPosition(system, 0x1e5, 2);
    ResetStrengthRockSaveDataPosition(system, 0x1e5, 3);
}
