#include "types.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "system/iss/iss_switch.h"

// One switch of the interactive sound system's BGM switch sets: a group of the BGM's tracks that is on or muted, or
// fades between the two. Names and layout from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

typedef enum {
    ISS_SWITCH_ON,
    ISS_SWITCH_MUTED,
    ISS_SWITCH_FADE_IN,
    ISS_SWITCH_FADE_OUT,
} ISSSwitchState;

struct ISSSwitch {
    ISSSwitchState state;
    // The frames the fade has run, and its length
    u16 nowFrame;
    u16 endFrame;
    // The BGM's tracks the switch controls
    u16 trackMask;
};

static ISSSwitch *ISSSwitch_Alloc(HeapID heapId);
static void ISSSwitch_FreeCore(ISSSwitch *sw);
static void ISSSwitch_Reset(ISSSwitch *sw);
static void ISSSwitch_SetCore(ISSSwitch *sw, u16 fadeFrames, u16 trackMask);
static void ISSSwitch_AdvanceFade(ISSSwitch *sw);
static BOOL ISSSwitch_IsFadeOver(ISSSwitch *sw);
static void ISSSwitch_UpdateCore(ISSSwitch *sw);
static void ISSSwitch_Update_ON(ISSSwitch *sw);
static void ISSSwitch_Update_OFF(ISSSwitch *sw);
static void ISSSwitch_Update_FADE_IN(ISSSwitch *sw);
static void ISSSwitch_Update_FADE_OUT(ISSSwitch *sw);
static void ISSSwitch_ReqFadeInCore(ISSSwitch *sw);
static void ISSSwitch_ReqFadeOutCore(ISSSwitch *sw);
static void ISSSwitch_UnmuteCore(ISSSwitch *sw);
static void ISSSwitch_MuteCore(ISSSwitch *sw);
static ISSSwitchState ISSSwitch_GetState(ISSSwitch *sw);
static void ISSSwitch_SetState(ISSSwitch *sw, ISSSwitchState state);
static int ISSSwitch_CalcVolume(ISSSwitch *sw);
static void ISSSwitch_CommitVolumeCore(ISSSwitch *sw, int masterVolume);
static BOOL ISSSwitch_IsOnCore(ISSSwitch *sw);
static BOOL ISSSwitch_IsOffCore(ISSSwitch *sw);

ISSSwitch *ISSSwitch_Create(HeapID heapId) {
    ISSSwitch *sw = ISSSwitch_Alloc(heapId);

    ISSSwitch_Reset(sw);
    return sw;
}

void ISSSwitch_Free(ISSSwitch *sw) {
    ISSSwitch_FreeCore(sw);
}

void ISSSwitch_Set(ISSSwitch *sw, u16 fadeFrames, u16 trackMask) {
    ISSSwitch_SetCore(sw, fadeFrames, trackMask);
}

BOOL ISSSwitch_IsOn(ISSSwitch *sw) {
    return ISSSwitch_IsOnCore(sw);
}

void ISSSwitch_ReqFadeIn(ISSSwitch *sw) {
    ISSSwitch_ReqFadeInCore(sw);
}

void ISSSwitch_ReqFadeOut(ISSSwitch *sw) {
    ISSSwitch_ReqFadeOutCore(sw);
}

void ISSSwitch_Unmute(ISSSwitch *sw) {
    ISSSwitch_UnmuteCore(sw);
}

void ISSSwitch_Mute(ISSSwitch *sw) {
    ISSSwitch_MuteCore(sw);
}

void ISSSwitch_Update(ISSSwitch *sw) {
    ISSSwitch_UpdateCore(sw);
}

void ISSSwitch_CommitVolume(ISSSwitch *sw, int masterVolume) {
    ISSSwitch_CommitVolumeCore(sw, masterVolume);
}

static ISSSwitch *ISSSwitch_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ISSSwitch), FALSE, "iss_switch.c", 278);
}

static void ISSSwitch_FreeCore(ISSSwitch *sw) {
    GFL_HeapFree(sw);
}

static void ISSSwitch_Reset(ISSSwitch *sw) {
    sw->state = ISS_SWITCH_MUTED;
    sw->nowFrame = 0;
    sw->endFrame = 0;
    sw->trackMask = 0;
}

static void ISSSwitch_SetCore(ISSSwitch *sw, u16 fadeFrames, u16 trackMask) {
    sw->endFrame = fadeFrames;
    sw->trackMask = trackMask;
}

static void ISSSwitch_AdvanceFade(ISSSwitch *sw) {
    sw->nowFrame++;
    if (sw->endFrame < sw->nowFrame) {
        sw->nowFrame = sw->endFrame;
    }
}

static BOOL ISSSwitch_IsFadeOver(ISSSwitch *sw) {
    if (sw->endFrame <= sw->nowFrame) {
        return TRUE;
    }
    return FALSE;
}

static void ISSSwitch_UpdateCore(ISSSwitch *sw) {
    switch (ISSSwitch_GetState(sw)) {
    case ISS_SWITCH_ON:
        ISSSwitch_Update_ON(sw);
        break;
    case ISS_SWITCH_MUTED:
        ISSSwitch_Update_OFF(sw);
        break;
    case ISS_SWITCH_FADE_IN:
        ISSSwitch_Update_FADE_IN(sw);
        break;
    case ISS_SWITCH_FADE_OUT:
        ISSSwitch_Update_FADE_OUT(sw);
        break;
    }
}

static void ISSSwitch_Update_ON(ISSSwitch *sw) {
}

static void ISSSwitch_Update_OFF(ISSSwitch *sw) {
}

static void ISSSwitch_Update_FADE_IN(ISSSwitch *sw) {
    ISSSwitch_AdvanceFade(sw);
    if (ISSSwitch_IsFadeOver(sw) == TRUE) {
        ISSSwitch_SetState(sw, ISS_SWITCH_ON);
    }
}

static void ISSSwitch_Update_FADE_OUT(ISSSwitch *sw) {
    ISSSwitch_AdvanceFade(sw);
    if (ISSSwitch_IsFadeOver(sw) == TRUE) {
        ISSSwitch_SetState(sw, ISS_SWITCH_MUTED);
    }
}

static void ISSSwitch_ReqFadeInCore(ISSSwitch *sw) {
    if (ISSSwitch_IsOnCore(sw) != TRUE) {
        ISSSwitch_SetState(sw, ISS_SWITCH_FADE_IN);
    }
}

static void ISSSwitch_ReqFadeOutCore(ISSSwitch *sw) {
    if (ISSSwitch_IsOffCore(sw) != TRUE) {
        ISSSwitch_SetState(sw, ISS_SWITCH_FADE_OUT);
    }
}

static void ISSSwitch_UnmuteCore(ISSSwitch *sw) {
    ISSSwitch_SetState(sw, ISS_SWITCH_ON);
}

static void ISSSwitch_MuteCore(ISSSwitch *sw) {
    ISSSwitch_SetState(sw, ISS_SWITCH_MUTED);
}

static ISSSwitchState ISSSwitch_GetState(ISSSwitch *sw) {
    return sw->state;
}

static void ISSSwitch_SetState(ISSSwitch *sw, ISSSwitchState state) {
    sw->state = state;
    sw->nowFrame = 0;
}

static int ISSSwitch_CalcVolume(ISSSwitch *sw) {
    int volume;

    switch (ISSSwitch_GetState(sw)) {
    case ISS_SWITCH_ON:
        return SND_VOLUME_MAX;
    case ISS_SWITCH_MUTED:
        return 0;
    case ISS_SWITCH_FADE_IN:
        volume = SND_VOLUME_MAX * sw->nowFrame / sw->endFrame;
        break;
    case ISS_SWITCH_FADE_OUT:
        volume = SND_VOLUME_MAX - SND_VOLUME_MAX * sw->nowFrame / sw->endFrame;
        break;
    }
    return volume;
}

static void ISSSwitch_CommitVolumeCore(ISSSwitch *sw, int masterVolume) {
    GFL_SndBGMSetVolume(sw->trackMask, ISSSwitch_CalcVolume(sw) * masterVolume / SND_VOLUME_MAX);
}

static BOOL ISSSwitch_IsOnCore(ISSSwitch *sw) {
    switch (ISSSwitch_GetState(sw)) {
    case ISS_SWITCH_ON:
        return TRUE;
    case ISS_SWITCH_MUTED:
        return FALSE;
    case ISS_SWITCH_FADE_IN:
        return TRUE;
    case ISS_SWITCH_FADE_OUT:
        return FALSE;
    }
    return FALSE;
}

static BOOL ISSSwitch_IsOffCore(ISSSwitch *sw) {
    switch (ISSSwitch_GetState(sw)) {
    case ISS_SWITCH_ON:
        return FALSE;
    case ISS_SWITCH_MUTED:
        return TRUE;
    case ISS_SWITCH_FADE_IN:
        return FALSE;
    case ISS_SWITCH_FADE_OUT:
        return TRUE;
    }
    return FALSE;
}
