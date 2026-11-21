#include "types.h"
#include "field/player/player_state.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "system/game_data.h"
#include "system/iss/iss_3ds_sys.h"
#include "system/iss/iss_city_sys.h"
#include "system/iss/iss_dungeon_sys.h"
#include "system/iss/iss_road_sys.h"
#include "system/iss/iss_switch_sys.h"
#include "system/iss/iss_sys.h"
#include "system/iss/iss_zone_sys.h"

// The interactive sound system. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except
// ISS_ChangeRoadSysZone

struct ISS {
    HeapID heapId;
    GameData *gameData;
    // The BGM that the subsystems were last switched for
    u32 bgm;
    u32 frame;
    ISSCitySys *citySys;
    ISSRoadSys *roadSys;
    ISSDungeonSys *dungeonSys;
    ISSZoneSys *zoneSys;
    ISSSwitchSys *switchSys;
    ISS3DSoundSys *soundSys3D;
};

// What enables and disables a subsystem
typedef struct {
    u8 subsystem;
    void (*enable)(ISS *iss);
    void (*disable)(ISS *iss);
} ISSSubsystemFuncs;

static void ISS_Reset(ISS *iss);
static void ISS_LoadGameData(ISS *iss, GameData *gameData, HeapID heapId);
static void ISS_FreeSubsystems(ISS *iss);
static void ISS_InitRoadSys(ISS *iss);
static void ISS_InitCitySys(ISS *iss);
static void ISS_InitDungeonSys(ISS *iss);
static void ISS_InitZoneSys(ISS *iss);
static void ISS_InitSwitchSys(ISS *iss);
static void ISS_Init3DSoundSys(ISS *iss);
static void ISS_FreeRoadSys(ISS *iss);
static void ISS_FreeCitySys(ISS *iss);
static void ISS_FreeDungeonSys(ISS *iss);
static void ISS_FreeZoneSys(ISS *iss);
static void ISS_FreeSwitchSys(ISS *iss);
static void ISS_Free3DSoundSys(ISS *iss);
static void ISS_UpdateRoadSys(ISS *iss);
static void ISS_UpdateCitySys(ISS *iss);
static void ISS_UpdateDungeonSys(ISS *iss);
static void ISS_UpdateZoneSys(ISS *iss);
static void ISS_UpdateSwitchSys(ISS *iss);
static void ISS_Update3DSoundSys(ISS *iss);
static void ISS_EnableRoadSys(ISS *iss);
static void ISS_EnableCitySys(ISS *iss);
static void ISS_EnableDungeonSys(ISS *iss);
static void ISS_EnableZoneSys(ISS *iss);
static void ISS_EnableSwitchSys(ISS *iss);
static void ISS_Enable3DSoundSys(ISS *iss);
static void ISS_DisableRoadSys(ISS *iss);
static void ISS_DisableCitySys(ISS *iss);
static void ISS_DisableDungeonSys(ISS *iss);
static void ISS_DisableZoneSys(ISS *iss);
static void ISS_DisableSwitchSys(ISS *iss);
static void ISS_Disable3DSoundSys(ISS *iss);
static void ISS_ChangeRoadSysZone(ISS *iss, u16 zoneId);
static void ISS_ChangeCitySysZone(ISS *iss, u16 zoneId);
static void ISS_ChangeDungeonSysZone(ISS *iss, u16 zoneId);
static void ISS_ChangeZoneSysZone(ISS *iss, u16 zoneId);
static void ISS_ChangeSwitchSysZone(ISS *iss, u16 zoneId);
static void ISS_Change3DSoundSysZone(ISS *iss, u16 zoneId);
static PlayerState *ISS_GetPlayerState(ISS *iss);
static u16 ISS_GetZoneID(ISS *iss);
static u32 ISS_GetBGMID(void);
static u8 ISS_DecideUsedSubsystem(ISS *iss);
static BOOL ISS_CheckBGMDesync(ISS *iss);
static void ISS_SwitchSubsystem(ISS *iss);

// By subsystem: a BGM's subsystem is enabled and all the others disabled
static const ISSSubsystemFuncs ISS_SUBSYSTEM_FUNCS[ISS_SUBSYSTEM_COUNT] = {
    { ISS_SUBSYSTEM_NONE, NULL, NULL },
    { ISS_SUBSYSTEM_UNUSED_1, NULL, NULL },
    { ISS_SUBSYSTEM_ROAD, ISS_EnableRoadSys, ISS_DisableRoadSys },
    { ISS_SUBSYSTEM_CITY, ISS_EnableCitySys, ISS_DisableCitySys },
    { ISS_SUBSYSTEM_3D_SOUND, ISS_Enable3DSoundSys, ISS_Disable3DSoundSys },
    { ISS_SUBSYSTEM_DUNGEON, ISS_EnableDungeonSys, ISS_DisableDungeonSys },
    { ISS_SUBSYSTEM_UNUSED_6, NULL, NULL },
    { ISS_SUBSYSTEM_SWITCH, ISS_EnableSwitchSys, ISS_DisableSwitchSys },
    { ISS_SUBSYSTEM_ZONE, ISS_EnableZoneSys, ISS_DisableZoneSys },
};

ISS *ISS_Create(GameData *gameData, HeapID heapId) {
    ISS *iss = GFL_HeapAllocate(heapId, sizeof(ISS), FALSE, "iss_sys.c", 164);

    ISS_Reset(iss);
    ISS_LoadGameData(iss, gameData, heapId);
    return iss;
}

void ISS_Free(ISS *iss) {
    ISS_FreeSubsystems(iss);
    GFL_HeapFree(iss);
}

void ISS_Update(ISS *iss) {
    if (ISS_CheckBGMDesync(iss) == TRUE) {
        ISS_SwitchSubsystem(iss);
        iss->bgm = ISS_GetBGMID();
    }
    ISS_UpdateRoadSys(iss);
    ISS_UpdateCitySys(iss);
    ISS_UpdateDungeonSys(iss);
    ISS_UpdateZoneSys(iss);
    ISS_UpdateSwitchSys(iss);
    ISS_Update3DSoundSys(iss);
    iss->frame++;
}

void ISS_ChangeZone(ISS *iss, u16 zoneId) {
    ISS_ChangeRoadSysZone(iss, zoneId);
    ISS_ChangeCitySysZone(iss, zoneId);
    ISS_ChangeDungeonSysZone(iss, zoneId);
    ISS_ChangeZoneSysZone(iss, zoneId);
    ISS_ChangeSwitchSysZone(iss, zoneId);
    ISS_Change3DSoundSysZone(iss, zoneId);
}

ISSSwitchSys *ISS_GetSwitchSys(ISS *iss) {
    return iss->switchSys;
}

ISS3DSoundSys *ISS_Get3DSoundSys(ISS *iss) {
    return iss->soundSys3D;
}

ISSDungeonSys *ISS_GetDungeonSys(ISS *iss) {
    return iss->dungeonSys;
}

static void ISS_Reset(ISS *iss) {
    iss->heapId = 0;
    iss->gameData = NULL;
    iss->bgm = 0;
    iss->frame = 0;
    iss->citySys = NULL;
    iss->roadSys = NULL;
    iss->dungeonSys = NULL;
    iss->zoneSys = NULL;
    iss->switchSys = NULL;
    iss->soundSys3D = NULL;
}

static void ISS_LoadGameData(ISS *iss, GameData *gameData, HeapID heapId) {
    iss->gameData = gameData;
    iss->bgm = -1;
    iss->heapId = heapId;
    iss->frame = 0;
    ISS_InitRoadSys(iss);
    ISS_InitCitySys(iss);
    ISS_InitDungeonSys(iss);
    ISS_InitZoneSys(iss);
    ISS_InitSwitchSys(iss);
    ISS_Init3DSoundSys(iss);
}

static void ISS_FreeSubsystems(ISS *iss) {
    ISS_FreeRoadSys(iss);
    ISS_FreeCitySys(iss);
    ISS_FreeDungeonSys(iss);
    ISS_FreeZoneSys(iss);
    ISS_FreeSwitchSys(iss);
    ISS_Free3DSoundSys(iss);
}

static void ISS_InitRoadSys(ISS *iss) {
    iss->roadSys = ISSRoadSys_Create(ISS_GetPlayerState(iss), iss->heapId);
}

static void ISS_InitCitySys(ISS *iss) {
    iss->citySys = ISSCitySys_Create(ISS_GetPlayerState(iss), iss->heapId);
}

static void ISS_InitDungeonSys(ISS *iss) {
    iss->dungeonSys = ISSDungeonSys_Create(iss->gameData, ISS_GetPlayerState(iss), iss->heapId);
}

static void ISS_InitZoneSys(ISS *iss) {
    iss->zoneSys = ISSZoneSys_Create(iss->heapId);
}

static void ISS_InitSwitchSys(ISS *iss) {
    iss->switchSys = ISSSwitchSys_Create(iss->heapId);
}

static void ISS_Init3DSoundSys(ISS *iss) {
    iss->soundSys3D = ISS3DSoundSys_Create(iss->heapId);
}

static void ISS_FreeRoadSys(ISS *iss) {
    ISSRoadSys_Free(iss->roadSys);
    iss->roadSys = NULL;
}

static void ISS_FreeCitySys(ISS *iss) {
    ISSCitySys_Free(iss->citySys);
    iss->citySys = NULL;
}

static void ISS_FreeDungeonSys(ISS *iss) {
    ISSDungeonSys_Free(iss->dungeonSys);
    iss->dungeonSys = NULL;
}

static void ISS_FreeZoneSys(ISS *iss) {
    ISSZoneSys_Free(iss->zoneSys);
    iss->zoneSys = NULL;
}

static void ISS_FreeSwitchSys(ISS *iss) {
    ISSSwitchSys_Free(iss->switchSys);
    iss->switchSys = NULL;
}

static void ISS_Free3DSoundSys(ISS *iss) {
    ISS3DSoundSys_Free(iss->soundSys3D);
    iss->soundSys3D = NULL;
}

// The road, city and 3D sound subsystems update every other frame
static void ISS_UpdateRoadSys(ISS *iss) {
    if (iss->frame % 2 == 0) {
        ISSRoadSys_Update(iss->roadSys);
    }
}

static void ISS_UpdateCitySys(ISS *iss) {
    if (iss->frame % 2 == 0) {
        ISSCitySys_Update(iss->citySys);
    }
}

static void ISS_UpdateDungeonSys(ISS *iss) {
    ISSDungeonSys_Update(iss->dungeonSys);
}

static void ISS_UpdateZoneSys(ISS *iss) {
    ISSZoneSys_Update(iss->zoneSys);
}

static void ISS_UpdateSwitchSys(ISS *iss) {
    ISSSwitchSys_Update(iss->switchSys);
}

static void ISS_Update3DSoundSys(ISS *iss) {
    if (iss->frame % 2 == 0) {
        ISS3DSoundSys_Update(iss->soundSys3D);
    }
}

static void ISS_EnableRoadSys(ISS *iss) {
    ISSRoadSys_Enable(iss->roadSys);
}

static void ISS_EnableCitySys(ISS *iss) {
    ISSCitySys_Enable(iss->citySys);
}

static void ISS_EnableDungeonSys(ISS *iss) {
    ISSDungeonSys_Enable(iss->dungeonSys);
}

static void ISS_EnableZoneSys(ISS *iss) {
    ISSZoneSys_Enable(iss->zoneSys, ISS_GetZoneID(iss));
}

static void ISS_EnableSwitchSys(ISS *iss) {
    ISSSwitchSys_Enable(iss->switchSys);
}

static void ISS_Enable3DSoundSys(ISS *iss) {
    ISS3DSoundSys_Enable(iss->soundSys3D);
}

static void ISS_DisableRoadSys(ISS *iss) {
    ISSRoadSys_Disable(iss->roadSys);
}

static void ISS_DisableCitySys(ISS *iss) {
    ISSCitySys_Disable(iss->citySys);
}

static void ISS_DisableDungeonSys(ISS *iss) {
    ISSDungeonSys_Disable(iss->dungeonSys);
}

static void ISS_DisableZoneSys(ISS *iss) {
    ISSZoneSys_Disable(iss->zoneSys);
}

static void ISS_DisableSwitchSys(ISS *iss) {
    ISSSwitchSys_Disable(iss->switchSys);
}

static void ISS_Disable3DSoundSys(ISS *iss) {
    ISS3DSoundSys_Disable(iss->soundSys3D);
}

// The road subsystem doesn't change with the zone
static void ISS_ChangeRoadSysZone(ISS *iss, u16 zoneId) {
}

static void ISS_ChangeCitySysZone(ISS *iss, u16 zoneId) {
    ISSCitySys_ChangeZone(iss->citySys, zoneId);
}

static void ISS_ChangeDungeonSysZone(ISS *iss, u16 zoneId) {
    ISSDungeonSys_ReqChangeZone(iss->dungeonSys, zoneId);
}

static void ISS_ChangeZoneSysZone(ISS *iss, u16 zoneId) {
    ISSZoneSys_ChangeZone(iss->zoneSys, zoneId);
}

static void ISS_ChangeSwitchSysZone(ISS *iss, u16 zoneId) {
    ISSSwitchSys_ChangeZone(iss->switchSys, zoneId);
}

static void ISS_Change3DSoundSysZone(ISS *iss, u16 zoneId) {
    ISS3DSoundSys_ChangeZone(iss->soundSys3D, zoneId);
}

static PlayerState *ISS_GetPlayerState(ISS *iss) {
    return GameData_GetPlayerState(iss->gameData);
}

static u16 ISS_GetZoneID(ISS *iss) {
    return PlayerState_GetZoneID(ISS_GetPlayerState(iss));
}

static u32 ISS_GetBGMID(void) {
    return GFL_SndBGMGetID();
}

static u8 ISS_DecideUsedSubsystem(ISS *iss) {
    return BGMInfo_GetISSSubsystem(GameData_GetBGMInfo(iss->gameData), ISS_GetBGMID());
}

static BOOL ISS_CheckBGMDesync(ISS *iss) {
    if (iss->bgm != ISS_GetBGMID()) {
        return TRUE;
    }
    return FALSE;
}

static void ISS_SwitchSubsystem(ISS *iss) {
    u8 used = ISS_DecideUsedSubsystem(iss);
    int i;

    for (i = 0; i < ISS_SUBSYSTEM_COUNT; i++) {
        if (used != ISS_SUBSYSTEM_FUNCS[i].subsystem && ISS_SUBSYSTEM_FUNCS[i].disable != NULL) {
            ISS_SUBSYSTEM_FUNCS[i].disable(iss);
        }
    }
    if (ISS_SUBSYSTEM_FUNCS[used].enable != NULL) {
        ISS_SUBSYSTEM_FUNCS[used].enable(iss);
    }
}
