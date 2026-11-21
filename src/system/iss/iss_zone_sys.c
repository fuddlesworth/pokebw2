#include "types.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "system/iss/iss_zone_sys.h"

// The interactive sound system's zone fades. Names and layouts from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except ISSZoneSys_Free, ISSZoneSys_Disable and
// ISSZoneSys_DisableCore

#define ISS_ZONE_NONE 0xff

// The tracks that a zone fades in and out when the player enters it, read from ARCID_ISS_ZONE
typedef struct {
    u16 zoneId;
    u16 trackMaskIn;
    u16 trackMaskOut;
    u16 fadeTime;
} ISSZoneConfig;

struct ISSZoneSys {
    BOOL isEnabled;
    u8 zoneCount;
    ISSZoneConfig *zones;
    // The index in zones of the zone the player is in
    u8 lastZoneDesc;
    // The tracks that fade in and out
    u16 trackMaskIn;
    u16 trackMaskOut;
    // The fade's length and the frames it has run, in frames
    u16 fadeTime;
    u16 counter;
};

static void ISSZoneSys_LoadArcData(ISSZoneSys *sys, HeapID heapId);
static void ISSZoneSys_EnableCore(ISSZoneSys *sys, u16 zoneId);
static void ISSZoneSys_DisableCore(ISSZoneSys *sys);
static void ISSZoneSys_ChangeZoneCore(ISSZoneSys *sys, u16 zoneId);
static u8 ISSZoneSys_FindZoneDesc(const ISSZoneSys *sys, u16 zoneId);
static void ISSZoneSys_UpdateCore(ISSZoneSys *sys);

ISSZoneSys *ISSZoneSys_Create(HeapID heapId) {
    ISSZoneSys *sys = GFL_HeapAllocate(heapId, sizeof(ISSZoneSys), FALSE, "iss_zone_sys.c", 85);

    sys->isEnabled = FALSE;
    sys->zoneCount = 0;
    sys->zones = NULL;
    sys->lastZoneDesc = ISS_ZONE_NONE;
    sys->trackMaskIn = 0;
    sys->trackMaskOut = 0;
    sys->fadeTime = 0;
    sys->counter = 0;
    ISSZoneSys_LoadArcData(sys, heapId);
    return sys;
}

void ISSZoneSys_Free(ISSZoneSys *sys) {
    if (sys->zones != NULL) {
        GFL_HeapFree(sys->zones);
    }
    GFL_HeapFree(sys);
}

void ISSZoneSys_Update(ISSZoneSys *sys) {
    if (sys->isEnabled) {
        ISSZoneSys_UpdateCore(sys);
    }
}

void ISSZoneSys_ChangeZone(ISSZoneSys *sys, u16 zoneId) {
    if (sys->isEnabled) {
        ISSZoneSys_ChangeZoneCore(sys, zoneId);
    }
}

void ISSZoneSys_Enable(ISSZoneSys *sys, u16 zoneId) {
    ISSZoneSys_EnableCore(sys, zoneId);
}

void ISSZoneSys_Disable(ISSZoneSys *sys) {
    ISSZoneSys_DisableCore(sys);
}

static void ISSZoneSys_LoadArcData(ISSZoneSys *sys, HeapID heapId) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(ARCID_ISS_ZONE, HEAPID_TAIL(heapId));
    int count = GFL_ArcToolGetDataMax(handle);
    int i;

    sys->zoneCount = count;
    sys->zones = GFL_HeapAllocate(heapId, count * sizeof(ISSZoneConfig), FALSE, "iss_zone_sys.c", 210);
    for (i = 0; i < count; i++) {
        GFL_ArcToolReadRange(handle, i, 0, sizeof(ISSZoneConfig), &sys->zones[i]);
    }
    GFL_ArcToolFree(handle);
}

static void ISSZoneSys_EnableCore(ISSZoneSys *sys, u16 zoneId) {
    u8 index;

    if (sys->isEnabled) {
        return;
    }
    index = ISSZoneSys_FindZoneDesc(sys, zoneId);
    if (index == ISS_ZONE_NONE) {
        return;
    }
    sys->lastZoneDesc = index;
    sys->isEnabled = TRUE;
    sys->trackMaskIn = sys->zones[index].trackMaskIn;
    sys->trackMaskOut = sys->zones[index].trackMaskOut;
    sys->fadeTime = 1;
    sys->counter = 0;
}

static void ISSZoneSys_DisableCore(ISSZoneSys *sys) {
    if (sys->isEnabled) {
        sys->isEnabled = FALSE;
    }
}

static void ISSZoneSys_ChangeZoneCore(ISSZoneSys *sys, u16 zoneId) {
    u8 lastIndex = sys->lastZoneDesc;
    u8 index = ISSZoneSys_FindZoneDesc(sys, zoneId);
    ISSZoneConfig *zones = sys->zones;

    if (index != ISS_ZONE_NONE) {
        // Fade in the tracks that the last zone faded out and this one fades in, and the other way around
        sys->trackMaskIn = zones[lastIndex].trackMaskOut & zones[index].trackMaskIn;
        sys->trackMaskOut = zones[lastIndex].trackMaskIn & zones[index].trackMaskOut;
        sys->fadeTime = zones[index].fadeTime;
        sys->counter = 0;
        sys->lastZoneDesc = index;
    }
}

static u8 ISSZoneSys_FindZoneDesc(const ISSZoneSys *sys, u16 zoneId) {
    u8 i;

    for (i = 0; i < sys->zoneCount; i++) {
        if (zoneId == sys->zones[i].zoneId) {
            return i;
        }
    }
    return ISS_ZONE_NONE;
}

static void ISSZoneSys_UpdateCore(ISSZoneSys *sys) {
    if (sys->fadeTime > sys->counter) {
        sys->counter++;
        if (sys->trackMaskIn != 0) {
            GFL_SndBGMSetVolume(sys->trackMaskIn, SND_VOLUME_MAX * ((f32)sys->counter / sys->fadeTime));
        }
        if (sys->trackMaskOut != 0) {
            GFL_SndBGMSetVolume(sys->trackMaskOut, SND_VOLUME_MAX * (1.0f - (f32)sys->counter / sys->fadeTime));
        }
    }
}
