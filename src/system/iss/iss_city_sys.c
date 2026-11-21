#include "types.h"
#include "constants/arc.h"
#include "field/player/player_state.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "system/iss/iss_city_sys.h"
#include "system/iss/iss_city_unit.h"

// The interactive sound system's cities. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except
// ISSCitySys_Free, ISSCitySys_Init, ISSCitySys_UnloadUnits and ISSCitySys_UnloadUnitsCore

// The BGM's tracks that the city sets the volume of
#define ISS_CITY_TRACK_MASK ((1 << 8) | (1 << 9))
// The master volume's change per frame while it fades
#define ISS_CITY_FADE_STEP 2
// The unit index while the player's zone has no unit
#define ISS_CITY_UNIT_NONE 0xff

enum {
    // Follows the emitter volume at full master volume
    ISS_CITY_STATE_EMITTER_ONLY,
    // Fades the master volume in
    ISS_CITY_STATE_EMITTER_MASTER_UP,
    // Fades the master volume out
    ISS_CITY_STATE_EMITTER_MASTER_DOWN,
};

struct ISSCitySys {
    PlayerState *player;
    BOOL enabled;
    u32 state;
    // The volume at the player's position, from the unit
    int emitterVolume;
    int masterVolume;
    ISSCityUnit **units;
    u8 unitCount;
    u8 nowUnitIndex;
};

static void ISSCitySys_Init(ISSCitySys *sys, HeapID heapId, PlayerState *player);
static void ISSCitySys_LoadUnits(ISSCitySys *sys, HeapID heapId);
static void ISSCitySys_UnloadUnits(ISSCitySys *sys);
static void ISSCitySys_LoadUnitsCore(ISSCitySys *sys, HeapID heapId);
static void ISSCitySys_UnloadUnitsCore(ISSCitySys *sys);
static u32 ISSCitySys_GetState(ISSCitySys *sys);
static void ISSCitySys_SetState(ISSCitySys *sys, u32 state);
static void ISSCitySys_EnableCore(ISSCitySys *sys);
static void ISSCitySys_DisableCore(ISSCitySys *sys);
static void ISSCitySys_ChangeZoneCore(ISSCitySys *sys, u16 zoneId);
static u16 ISSCitySys_GetNowZoneID(ISSCitySys *sys);
static BOOL ISSCitySys_IsEnabled(ISSCitySys *sys);
static void ISSCitySys_UpdateCore(ISSCitySys *sys);
static void ISSCitySys_UpdateState_EmitterOnly(ISSCitySys *sys);
static void ISSCitySys_UpdateState_EmitterMasterUp(ISSCitySys *sys);
static void ISSCitySys_UpdateState_EmitterMasterDown(ISSCitySys *sys);
static ISSCityUnit *ISSCitySys_GetNowUnit(ISSCitySys *sys);
static u8 ISSCitySys_GetNowUnitIndex(ISSCitySys *sys);
static BOOL ISSCitySys_IsUnitBound(ISSCitySys *sys);
static BOOL ISSCitySys_IsZoneRegist(ISSCitySys *sys, u16 zoneId);
static BOOL ISSCitySys_SetUnitByZone(ISSCitySys *sys, u16 zoneId);
static u8 ISSCitySys_FindUnitByZone(ISSCitySys *sys, u16 zoneId);
static int ISSCitySys_GetMasterVolume(ISSCitySys *sys);
static BOOL ISSCitySys_RaiseVolume(ISSCitySys *sys);
static BOOL ISSCitySys_LowerVolume(ISSCitySys *sys);
static void ISSCitySys_SetMasterVolume(ISSCitySys *sys, int volume);
static int ISSCitySys_GetNowEmitterVolume(ISSCitySys *sys);
static BOOL ISSCitySys_UpdateEmitter(ISSCitySys *sys);
static u8 ISSCitySys_CalcEmitterVolume(ISSCitySys *sys);
static void ISSCitySys_SetNowEmitterVolume(ISSCitySys *sys, int volume);
static void ISSCitySys_CommitVolume(ISSCitySys *sys);
static int ISSCitySys_CalcVolume(ISSCitySys *sys);

ISSCitySys *ISSCitySys_Create(PlayerState *player, HeapID heapId) {
    ISSCitySys *sys = GFL_HeapAllocate(heapId, sizeof(ISSCitySys), FALSE, "iss_city_sys.c", 147);
    ISSCitySys_Init(sys, heapId, player);
    ISSCitySys_LoadUnits(sys, heapId);
    return sys;
}

void ISSCitySys_Free(ISSCitySys *sys) {
    ISSCitySys_UnloadUnits(sys);
    GFL_HeapFree(sys);
}

void ISSCitySys_Update(ISSCitySys *sys) {
    if (ISSCitySys_IsEnabled(sys)) {
        ISSCitySys_UpdateCore(sys);
    }
}

void ISSCitySys_Enable(ISSCitySys *sys) {
    ISSCitySys_EnableCore(sys);
}

void ISSCitySys_Disable(ISSCitySys *sys) {
    ISSCitySys_DisableCore(sys);
}

void ISSCitySys_ChangeZone(ISSCitySys *sys, u16 zoneId) {
    ISSCitySys_ChangeZoneCore(sys, zoneId);
}

static void ISSCitySys_Init(ISSCitySys *sys, HeapID heapId, PlayerState *player) {
    sys->player = player;
    sys->enabled = FALSE;
    sys->state = ISS_CITY_STATE_EMITTER_ONLY;
    sys->emitterVolume = 0;
    sys->masterVolume = 0;
    sys->units = NULL;
    sys->unitCount = 0;
    sys->nowUnitIndex = ISS_CITY_UNIT_NONE;
}

static void ISSCitySys_LoadUnits(ISSCitySys *sys, HeapID heapId) {
    ISSCitySys_LoadUnitsCore(sys, heapId);
}

static void ISSCitySys_UnloadUnits(ISSCitySys *sys) {
    ISSCitySys_UnloadUnitsCore(sys);
}

static void ISSCitySys_LoadUnitsCore(ISSCitySys *sys, HeapID heapId) {
    int i;
    int count = GFL_ArcSysGetDataMax(ARCID_ISS_CITY);

    sys->unitCount = count;
    sys->units = GFL_HeapAllocate(heapId, count * sizeof(ISSCityUnit *), FALSE, "iss_city_sys.c", 334);
    for (i = 0; i < count; i++) {
        sys->units[i] = ISSCityUnit_Create(heapId, i);
    }
}

static void ISSCitySys_UnloadUnitsCore(ISSCitySys *sys) {
    int i;
    int count = sys->unitCount;

    for (i = 0; i < count; i++) {
        ISSCityUnit_Free(sys->units[i]);
    }
    GFL_HeapFree(sys->units);
    sys->units = NULL;
}

static u32 ISSCitySys_GetState(ISSCitySys *sys) {
    return sys->state;
}

static void ISSCitySys_SetState(ISSCitySys *sys, u32 state) {
    sys->state = state;
}

static void ISSCitySys_EnableCore(ISSCitySys *sys) {
    if (ISSCitySys_IsEnabled(sys)) {
        return;
    }
    sys->enabled = TRUE;
    ISSCitySys_SetNowEmitterVolume(sys, 0);
    ISSCitySys_CommitVolume(sys);
    ISSCitySys_SetUnitByZone(sys, ISSCitySys_GetNowZoneID(sys));
    if (ISSCitySys_IsUnitBound(sys) == TRUE) {
        ISSCitySys_SetState(sys, ISS_CITY_STATE_EMITTER_MASTER_UP);
    } else {
        ISSCitySys_SetState(sys, ISS_CITY_STATE_EMITTER_ONLY);
    }
}

static void ISSCitySys_DisableCore(ISSCitySys *sys) {
    if (ISSCitySys_IsEnabled(sys)) {
        sys->enabled = FALSE;
    }
}

static void ISSCitySys_ChangeZoneCore(ISSCitySys *sys, u16 zoneId) {
    if (!ISSCitySys_IsEnabled(sys)) {
        return;
    }
    // Going from one unit's zone straight into another's turns the system off
    if (ISSCitySys_IsUnitBound(sys) == TRUE && ISSCitySys_IsZoneRegist(sys, zoneId) == TRUE) {
        ISSCitySys_DisableCore(sys);
        return;
    }
    ISSCitySys_SetUnitByZone(sys, zoneId);
    if (ISSCitySys_IsUnitBound(sys) == TRUE) {
        ISSCitySys_SetMasterVolume(sys, 0);
        ISSCitySys_SetState(sys, ISS_CITY_STATE_EMITTER_MASTER_UP);
    } else {
        ISSCitySys_SetState(sys, ISS_CITY_STATE_EMITTER_MASTER_DOWN);
    }
}

static u16 ISSCitySys_GetNowZoneID(ISSCitySys *sys) {
    return PlayerState_GetZoneID(sys->player);
}

static BOOL ISSCitySys_IsEnabled(ISSCitySys *sys) {
    return sys->enabled;
}

static void ISSCitySys_UpdateCore(ISSCitySys *sys) {
    switch (ISSCitySys_GetState(sys)) {
    case ISS_CITY_STATE_EMITTER_ONLY:
        ISSCitySys_UpdateState_EmitterOnly(sys);
        break;
    case ISS_CITY_STATE_EMITTER_MASTER_UP:
        ISSCitySys_UpdateState_EmitterMasterUp(sys);
        break;
    case ISS_CITY_STATE_EMITTER_MASTER_DOWN:
        ISSCitySys_UpdateState_EmitterMasterDown(sys);
        break;
    }
}

static void ISSCitySys_UpdateState_EmitterOnly(ISSCitySys *sys) {
    if (ISSCitySys_UpdateEmitter(sys)) {
        ISSCitySys_CommitVolume(sys);
    }
}

static void ISSCitySys_UpdateState_EmitterMasterUp(ISSCitySys *sys) {
    BOOL masterChanged = ISSCitySys_RaiseVolume(sys);
    BOOL emitterChanged = ISSCitySys_UpdateEmitter(sys);

    if (masterChanged || emitterChanged) {
        ISSCitySys_CommitVolume(sys);
    }
    if (ISSCitySys_GetMasterVolume(sys) >= SND_VOLUME_MAX) {
        ISSCitySys_SetState(sys, ISS_CITY_STATE_EMITTER_ONLY);
    }
}

static void ISSCitySys_UpdateState_EmitterMasterDown(ISSCitySys *sys) {
    BOOL masterChanged = ISSCitySys_LowerVolume(sys);
    BOOL emitterChanged = ISSCitySys_UpdateEmitter(sys);

    if (masterChanged || emitterChanged) {
        ISSCitySys_CommitVolume(sys);
    }
    if (ISSCitySys_GetMasterVolume(sys) <= 0) {
        ISSCitySys_SetState(sys, ISS_CITY_STATE_EMITTER_ONLY);
    }
}

static ISSCityUnit *ISSCitySys_GetNowUnit(ISSCitySys *sys) {
    return sys->units[ISSCitySys_GetNowUnitIndex(sys)];
}

static u8 ISSCitySys_GetNowUnitIndex(ISSCitySys *sys) {
    return sys->nowUnitIndex;
}

static BOOL ISSCitySys_IsUnitBound(ISSCitySys *sys) {
    if (ISSCitySys_GetNowUnitIndex(sys) == ISS_CITY_UNIT_NONE) {
        return FALSE;
    }
    return TRUE;
}

static BOOL ISSCitySys_IsZoneRegist(ISSCitySys *sys, u16 zoneId) {
    if (ISSCitySys_FindUnitByZone(sys, zoneId) == ISS_CITY_UNIT_NONE) {
        return FALSE;
    }
    return TRUE;
}

static BOOL ISSCitySys_SetUnitByZone(ISSCitySys *sys, u16 zoneId) {
    u8 nowIndex = ISSCitySys_GetNowUnitIndex(sys);
    u8 index = ISSCitySys_FindUnitByZone(sys, zoneId);

    if (nowIndex == index) {
        return FALSE;
    }
    sys->nowUnitIndex = index;
    return TRUE;
}

static u8 ISSCitySys_FindUnitByZone(ISSCitySys *sys, u16 zoneId) {
    int i;
    int count = sys->unitCount;

    for (i = 0; i < count; i++) {
        if (zoneId == ISSCityUnit_GetZoneID(sys->units[i])) {
            return i;
        }
    }
    return ISS_CITY_UNIT_NONE;
}

static int ISSCitySys_GetMasterVolume(ISSCitySys *sys) {
    return sys->masterVolume;
}

static BOOL ISSCitySys_RaiseVolume(ISSCitySys *sys) {
    int volume = ISSCitySys_GetMasterVolume(sys) + ISS_CITY_FADE_STEP;

    if (volume >= SND_VOLUME_MAX) {
        volume = SND_VOLUME_MAX;
    }
    if (volume == ISSCitySys_GetMasterVolume(sys)) {
        return FALSE;
    }
    ISSCitySys_SetMasterVolume(sys, volume);
    return TRUE;
}

static BOOL ISSCitySys_LowerVolume(ISSCitySys *sys) {
    int volume = ISSCitySys_GetMasterVolume(sys) - ISS_CITY_FADE_STEP;

    if (volume <= 0) {
        volume = 0;
    }
    if (volume == ISSCitySys_GetMasterVolume(sys)) {
        return FALSE;
    }
    ISSCitySys_SetMasterVolume(sys, volume);
    return TRUE;
}

static void ISSCitySys_SetMasterVolume(ISSCitySys *sys, int volume) {
    sys->masterVolume = volume;
}

static int ISSCitySys_GetNowEmitterVolume(ISSCitySys *sys) {
    return sys->emitterVolume;
}

static BOOL ISSCitySys_UpdateEmitter(ISSCitySys *sys) {
    int volume;

    if (ISSCitySys_IsUnitBound(sys) == FALSE) {
        return FALSE;
    }
    volume = ISSCitySys_CalcEmitterVolume(sys);
    if (volume == ISSCitySys_GetNowEmitterVolume(sys)) {
        return FALSE;
    }
    ISSCitySys_SetNowEmitterVolume(sys, volume);
    return TRUE;
}

static u8 ISSCitySys_CalcEmitterVolume(ISSCitySys *sys) {
    ISSCityUnit *unit = ISSCitySys_GetNowUnit(sys);
    return ISSCityUnit_CalcEmitterVolume(unit, PlayerState_GetWPos(sys->player));
}

static void ISSCitySys_SetNowEmitterVolume(ISSCitySys *sys, int volume) {
    sys->emitterVolume = volume;
}

static void ISSCitySys_CommitVolume(ISSCitySys *sys) {
    GFL_SndBGMSetVolume(ISS_CITY_TRACK_MASK, ISSCitySys_CalcVolume(sys));
}

static int ISSCitySys_CalcVolume(ISSCitySys *sys) {
    return ISSCitySys_GetNowEmitterVolume(sys) * ISSCitySys_GetMasterVolume(sys) / SND_VOLUME_MAX;
}
