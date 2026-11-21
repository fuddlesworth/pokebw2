#include "types.h"
#include "app/townmap/townmap_util.h"
#include "app/unity_tower/unity_tower.h"
#include "constants/town_map.h"
#include "constants/vars.h"
#include "constants/zones.h"
#include "entralink/hidden_hollow.h"
#include "entralink/rival_select.h"
#include "field/map/zone.h"
#include "save/event_work.h"
#include "save/key_info.h"
#include "save/trainer_card.h"
#include "system/game_data.h"
#include "system/savedata/save_control.h"

// The Union Room, the Plasma Frigate, the Hidden Grottoes and the Black Tower or White Treehollow with Black City's or
// White Forest's gates show on the map where the player entered them, or where the game state puts them
u16 TownMap_GetMapZone(GameData *gameData, u16 zoneId) {
    u16 parent = GetZoneParentZone(zoneId);

    if (parent == ZONE_UNION_ROOM) {
        return GetZoneParentZone(GameData_GetNextZone(gameData)->zoneId);
    }
    if (parent == ZONE_PLASMA_FRIGATE) {
        switch (*EventWork_GetWkPtr(GameData_GetEventWork(gameData), EVENT_WORK_PLASMA_FRIGATE_LOCATION)) {
        default:
        case 0:
            return ZONE_PWT;
        case 1:
            return ZONE_ROUTE_21;
        case 2:
            return ZONE_GIANT_CHASM;
        case 3:
            return ZONE_PLASMA_FRIGATE;
        }
    }
    if (parent == ZONE_HIDDEN_GROTTO) {
        return GetZoneParentZone(GetHiddenHollowEntranceParam(
            getHollowNum(getHollow_RivalData(GameData_GetSaveControl(gameData))), HIDDEN_HOLLOW_ENTRANCE_ZONE));
    }
    if (parent == ZONE_WHITE_TREEHOLLOW || IsZoneBlackCityOrWhiteForestLobby(parent)) {
#ifdef BLACK2
        if (KeyInfo_GetCityKey(getKeyInfoSaveBlk(GameData_GetSaveControl(gameData))) == 0) {
            return ZONE_BLACK_CITY;
        }
        return ZONE_WHITE_FOREST;
#else
        if (KeyInfo_GetCityKey(getKeyInfoSaveBlk(GameData_GetSaveControl(gameData))) == 0) {
            return ZONE_WHITE_FOREST;
        }
        return ZONE_BLACK_CITY;
#endif
    }
    return parent;
}

BOOL TownMap_CheckFlag(GameData *gameData, u16 flag) {
    EventWork *eventWork = GameData_GetEventWork(gameData);
    TrainerCardSave *trainerCard = getTrainerCardDataBlkAddress(gameData);
    PlayerInfo *info = GetGameDataPlayerInfo(gameData);
    UnityTowerSurveySave *survey = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(gameData));
    BOOL result;

    switch (flag) {
    case TOWNMAP_FLAG_ONE_SHOT_DR:
        return isOneShotDRObtained(trainerCard, 1, info);
    case TOWNMAP_FLAG_UNITY_TOWER_VISITED:
        result = FALSE;
        if (UnityTower_GetCountryCountForPlayer(survey, info, 0) >= 1) {
            result = TRUE;
        }
        return result;
    case TOWNMAP_FLAG_PLASMA_FRIGATE:
        if (*EventWork_GetWkPtr(eventWork, EVENT_WORK_PLASMA_FRIGATE_LOCATION) == 3) {
            return TRUE;
        }
        return FALSE;
    }
    return EventWork_FlagGet(eventWork, flag);
}
