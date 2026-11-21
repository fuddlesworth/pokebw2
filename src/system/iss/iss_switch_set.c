#include "types.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/math.h"
#include "system/iss/iss_switch.h"
#include "system/iss/iss_switch_set.h"

// A BGM's switch set in the interactive sound system: the switches of the BGM's tracks, the zones the set plays in and
// the master volume, which fades. Names and layout from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0),
// except ISSSwitchSet_ResetState and ISSSwitchSet_ApplyVolume. swan names ISSSwitchSys_GetZoneIDCount and
// ISSSwitchSys_GetZoneID after the switch system, though they read the set

#define ISS_SWITCH_SET_ZONE_MAX 9

typedef enum {
    ISS_SWITCH_SET_VOLUME_KEEP,
    ISS_SWITCH_SET_VOLUME_LOWER,
    ISS_SWITCH_SET_VOLUME_RAISE,
} ISSSwitchSetVolumeFadeState;

struct ISSSwitchSet {
    ISSSwitchSetVolumeFadeState fadeStatus;
    u32 bgmId;
    // The zones the set plays in
    u16 zoneIds[ISS_SWITCH_SET_ZONE_MAX];
    u8 zoneIdCount;
    ISSSwitch *switches[ISS_SWITCH_COUNT];
    int masterVolume;
    // The fade's change of volume, and the volume it started from
    int fadeVolumeAddend;
    int baseVolume;
    // The frames the fade has run, and its length
    u16 nowFrame;
    u16 endFrame;
};

static void ISSSwitchSet_Init(ISSSwitchSet *set);
static ISSSwitchSet *ISSSwitchSet_Alloc(HeapID heapId);
static void ISSSwitchSet_FreeCore(ISSSwitchSet *set);
static void ISSSwitchSet_CreateSwitches(ISSSwitchSet *set, HeapID heapId);
static void ISSSwitchSet_FreeSwitches(ISSSwitchSet *set);
static void ISSSwitchSet_LoadArcDataCore(ISSSwitchSet *set, u32 datId);
static void ISSSwitchSet_UpdateCore(ISSSwitchSet *set);
static void ISSSwitchSet_ReqSwitchFadeInCore(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
static void ISSSwitchSet_ReqSwitchFadeOutCore(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
static void ISSSwitchSet_Reset(ISSSwitchSet *set);
static u32 ISSSwitchSet_GetBGMIdCore(ISSSwitchSet *set);
static u8 ISSSwitchSys_GetZoneIDCount(ISSSwitchSet *set);
static u16 ISSSwitchSys_GetZoneID(ISSSwitchSet *set, int index);
static BOOL ISSSwitchSet_CheckHasZoneCore(ISSSwitchSet *set, u16 zoneId);
static BOOL ISSSwitchSet_IsSwitchOnCore(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
static ISSSwitch *ISSSwitchSet_GetSwitch(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
static void ISSSwitchSet_CommitVolume(ISSSwitchSet *set);
static void ISSSwitchSet_UpdateFadeIn(ISSSwitchSet *set);
static void ISSSwitchSet_UpdateFadeOut(ISSSwitchSet *set);
static void ISSSwitchSet_AdvanceFade(ISSSwitchSet *set);
static BOOL ISSSwitchSet_IsFadeOver(ISSSwitchSet *set);

ISSSwitchSet *ISSSwitchSet_Create(HeapID heapId) {
    ISSSwitchSet *set = ISSSwitchSet_Alloc(heapId);

    ISSSwitchSet_Init(set);
    ISSSwitchSet_CreateSwitches(set, heapId);
    return set;
}

void ISSSwitchSet_Free(ISSSwitchSet *set) {
    ISSSwitchSet_FreeSwitches(set);
    ISSSwitchSet_FreeCore(set);
}

void ISSSwitchSet_LoadArcData(ISSSwitchSet *set, u32 datId, HeapID heapId) {
    ISSSwitchSet_LoadArcDataCore(set, datId);
}

void ISSSwitchSet_Update(ISSSwitchSet *set) {
    ISSSwitchSet_UpdateCore(set);
}

void ISSSwitchSet_ReqSwitchFadeIn(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    ISSSwitchSet_ReqSwitchFadeInCore(set, switchIndex);
}

void ISSSwitchSet_UnmuteSwitch(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    ISSSwitch_Unmute(ISSSwitchSet_GetSwitch(set, switchIndex));
}

void ISSSwitchSet_ReqSwitchFadeOut(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    ISSSwitchSet_ReqSwitchFadeOutCore(set, switchIndex);
}

void ISSSwitchSet_MuteSwitch(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    ISSSwitch_Mute(ISSSwitchSet_GetSwitch(set, switchIndex));
}

void ISSSwitchSet_ResetState(ISSSwitchSet *set) {
    ISSSwitchSet_Reset(set);
}

u32 ISSSwitchSet_GetBGMId(ISSSwitchSet *set) {
    return ISSSwitchSet_GetBGMIdCore(set);
}

BOOL ISSSwitchSet_CheckHasZone(ISSSwitchSet *set, u16 zoneId) {
    return ISSSwitchSet_CheckHasZoneCore(set, zoneId);
}

BOOL ISSSwitchSet_IsSwitchOn(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    return ISSSwitchSet_IsSwitchOnCore(set, switchIndex);
}

void ISSSwitchSet_ApplyVolume(ISSSwitchSet *set) {
    ISSSwitchSet_CommitVolume(set);
}

void ISSSwitchSet_SetMasterVolumeFade(ISSSwitchSet *set, int volume, u16 frames) {
    if (frames == 0) {
        set->masterVolume = volume;
        set->fadeStatus = ISS_SWITCH_SET_VOLUME_KEEP;
        return;
    }
    if (set->masterVolume < volume) {
        set->fadeStatus = ISS_SWITCH_SET_VOLUME_RAISE;
    } else if (set->masterVolume > volume) {
        set->fadeStatus = ISS_SWITCH_SET_VOLUME_LOWER;
    } else {
        set->fadeStatus = ISS_SWITCH_SET_VOLUME_KEEP;
        return;
    }
    set->nowFrame = 0;
    set->endFrame = frames;
    set->fadeVolumeAddend = MATH_ABS(set->masterVolume - volume);
    set->baseVolume = set->masterVolume;
}

static void ISSSwitchSet_Init(ISSSwitchSet *set) {
    int i;

    set->fadeStatus = ISS_SWITCH_SET_VOLUME_KEEP;
    set->bgmId = 0;
    set->zoneIdCount = 0;
    for (i = 0; i < ISS_SWITCH_SET_ZONE_MAX; i++) {
        set->zoneIds[i] = 0;
    }
    for (i = 0; i < ISS_SWITCH_COUNT; i++) {
        set->switches[i] = NULL;
    }
    set->masterVolume = SND_VOLUME_MAX;
    set->fadeVolumeAddend = SND_VOLUME_MAX;
    set->baseVolume = SND_VOLUME_MAX;
    set->nowFrame = 0;
    set->endFrame = 0;
}

static ISSSwitchSet *ISSSwitchSet_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ISSSwitchSet), FALSE, "iss_switch_set.c", 408);
}

static void ISSSwitchSet_FreeCore(ISSSwitchSet *set) {
    GFL_HeapFree(set);
}

static void ISSSwitchSet_CreateSwitches(ISSSwitchSet *set, HeapID heapId) {
    int i;

    for (i = 0; i < ISS_SWITCH_COUNT; i++) {
        set->switches[i] = ISSSwitch_Create(heapId);
    }
}

static void ISSSwitchSet_FreeSwitches(ISSSwitchSet *set) {
    int i;

    for (i = 0; i < ISS_SWITCH_COUNT; i++) {
        ISSSwitch_Free(set->switches[i]);
        set->switches[i] = NULL;
    }
}

// The file holds the BGM, each switch's tracks, the length of the switches' fades, then the count of zones and the
// zones
static void ISSSwitchSet_LoadArcDataCore(ISSSwitchSet *set, u32 datId) {
    u16 fadeFrames;
    u16 trackMasks[ISS_SWITCH_COUNT];
    // Both start at 0 in their declarations, as the original's shared zero register shows
    int i = 0;
    u32 offset = 0;

    GFL_ArcSysReadRange(&set->bgmId, ARCID_ISS_SWITCH, datId, offset, sizeof(set->bgmId));
    offset += sizeof(set->bgmId);
    for (; i < ISS_SWITCH_COUNT; i++) {
        GFL_ArcSysReadRange(&trackMasks[i], ARCID_ISS_SWITCH, datId, offset, sizeof(u16));
        offset += sizeof(u16);
    }
    GFL_ArcSysReadRange(&fadeFrames, ARCID_ISS_SWITCH, datId, offset, sizeof(u16));
    offset += sizeof(u16);
    GFL_ArcSysReadRange(&set->zoneIdCount, ARCID_ISS_SWITCH, datId, offset, sizeof(u8));
    offset += sizeof(u8);
    GFL_ArcSysReadRange(set->zoneIds, ARCID_ISS_SWITCH, datId, offset, set->zoneIdCount * sizeof(u16));
    for (i = 0; i < ISS_SWITCH_COUNT; i++) {
        ISSSwitch_Set(ISSSwitchSet_GetSwitch(set, i), fadeFrames, trackMasks[i]);
    }
}

static void ISSSwitchSet_UpdateCore(ISSSwitchSet *set) {
    int i;

    switch (set->fadeStatus) {
    case ISS_SWITCH_SET_VOLUME_KEEP:
        break;
    case ISS_SWITCH_SET_VOLUME_RAISE:
        ISSSwitchSet_UpdateFadeIn(set);
        break;
    case ISS_SWITCH_SET_VOLUME_LOWER:
        ISSSwitchSet_UpdateFadeOut(set);
        break;
    }
    for (i = 0; i < ISS_SWITCH_COUNT; i++) {
        ISSSwitch_Update(ISSSwitchSet_GetSwitch(set, i));
    }
}

static void ISSSwitchSet_ReqSwitchFadeInCore(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    ISSSwitch_ReqFadeIn(ISSSwitchSet_GetSwitch(set, switchIndex));
}

static void ISSSwitchSet_ReqSwitchFadeOutCore(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    ISSSwitch_ReqFadeOut(ISSSwitchSet_GetSwitch(set, switchIndex));
}

static void ISSSwitchSet_Reset(ISSSwitchSet *set) {
    int i;

    set->fadeStatus = ISS_SWITCH_SET_VOLUME_KEEP;
    set->masterVolume = SND_VOLUME_MAX;
    set->fadeVolumeAddend = SND_VOLUME_MAX;
    set->baseVolume = SND_VOLUME_MAX;
    set->nowFrame = 0;
    set->endFrame = 0;
    ISSSwitch_Unmute(ISSSwitchSet_GetSwitch(set, 0));
    for (i = 1; i < ISS_SWITCH_COUNT; i++) {
        ISSSwitch_Mute(ISSSwitchSet_GetSwitch(set, i));
    }
}

static u32 ISSSwitchSet_GetBGMIdCore(ISSSwitchSet *set) {
    return set->bgmId;
}

static u8 ISSSwitchSys_GetZoneIDCount(ISSSwitchSet *set) {
    return set->zoneIdCount;
}

static u16 ISSSwitchSys_GetZoneID(ISSSwitchSet *set, int index) {
    return set->zoneIds[index];
}

static BOOL ISSSwitchSet_CheckHasZoneCore(ISSSwitchSet *set, u16 zoneId) {
    int count = ISSSwitchSys_GetZoneIDCount(set);
    int i;

    for (i = 0; i < count; i++) {
        if (zoneId == ISSSwitchSys_GetZoneID(set, i)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL ISSSwitchSet_IsSwitchOnCore(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    return ISSSwitch_IsOn(ISSSwitchSet_GetSwitch(set, switchIndex));
}

static ISSSwitch *ISSSwitchSet_GetSwitch(ISSSwitchSet *set, ISSSwitchIndex switchIndex) {
    return set->switches[switchIndex];
}

static void ISSSwitchSet_CommitVolume(ISSSwitchSet *set) {
    int i;

    for (i = 0; i < ISS_SWITCH_COUNT; i++) {
        ISSSwitch_CommitVolume(ISSSwitchSet_GetSwitch(set, i), set->masterVolume);
    }
}

static void ISSSwitchSet_UpdateFadeIn(ISSSwitchSet *set) {
    ISSSwitchSet_AdvanceFade(set);
    set->masterVolume = set->baseVolume + set->fadeVolumeAddend * set->nowFrame / set->endFrame;
    if (ISSSwitchSet_IsFadeOver(set) == TRUE) {
        set->fadeStatus = ISS_SWITCH_SET_VOLUME_KEEP;
    }
}

static void ISSSwitchSet_UpdateFadeOut(ISSSwitchSet *set) {
    ISSSwitchSet_AdvanceFade(set);
    set->masterVolume = set->baseVolume - set->fadeVolumeAddend * set->nowFrame / set->endFrame;
    if (ISSSwitchSet_IsFadeOver(set) == TRUE) {
        set->fadeStatus = ISS_SWITCH_SET_VOLUME_KEEP;
    }
}

static void ISSSwitchSet_AdvanceFade(ISSSwitchSet *set) {
    set->nowFrame++;
    if (set->endFrame < set->nowFrame) {
        set->nowFrame = set->endFrame;
    }
}

static BOOL ISSSwitchSet_IsFadeOver(ISSSwitchSet *set) {
    if (set->endFrame <= set->nowFrame) {
        return TRUE;
    }
    return FALSE;
}
