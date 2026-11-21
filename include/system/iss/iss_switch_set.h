#ifndef POKEBW2_SYSTEM_ISS_ISS_SWITCH_SET_H
#define POKEBW2_SYSTEM_ISS_ISS_SWITCH_SET_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A BGM's switch set: the switches of its tracks, the zones it plays in and its master volume. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except ISSSwitchSet_ResetState and ISSSwitchSet_ApplyVolume

// A switch of the zone's music. The Battle Subway only fades in switches below ISS_SWITCH_COUNT
typedef enum {
    ISS_SWITCH_COUNT = 9,
} ISSSwitchIndex;

ISSSwitchSet *ISSSwitchSet_Create(HeapID heapId);
void ISSSwitchSet_Free(ISSSwitchSet *set);
// Loads the set's BGM, switches and zones from a file of ARCID_ISS_SWITCH
void ISSSwitchSet_LoadArcData(ISSSwitchSet *set, u32 datId, HeapID heapId);
void ISSSwitchSet_Update(ISSSwitchSet *set);
void ISSSwitchSet_ReqSwitchFadeIn(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
void ISSSwitchSet_UnmuteSwitch(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
void ISSSwitchSet_ReqSwitchFadeOut(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
void ISSSwitchSet_MuteSwitch(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
// Ends any fade, turns on the first switch and mutes the others
void ISSSwitchSet_ResetState(ISSSwitchSet *set);
u32 ISSSwitchSet_GetBGMId(ISSSwitchSet *set);
BOOL ISSSwitchSet_CheckHasZone(ISSSwitchSet *set, u16 zoneId);
BOOL ISSSwitchSet_IsSwitchOn(ISSSwitchSet *set, ISSSwitchIndex switchIndex);
// Sets the volume of each switch's tracks
void ISSSwitchSet_ApplyVolume(ISSSwitchSet *set);
// Fades the master volume to volume over a number of frames, or sets it at once if frames is 0
void ISSSwitchSet_SetMasterVolumeFade(ISSSwitchSet *set, int volume, u16 frames);

#endif // POKEBW2_SYSTEM_ISS_SWITCH_SET_H
