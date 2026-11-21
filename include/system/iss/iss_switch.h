#ifndef POKEBW2_SYSTEM_ISS_ISS_SWITCH_H
#define POKEBW2_SYSTEM_ISS_ISS_SWITCH_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// One switch of a BGM's switch set: a group of tracks that fades in and out. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

ISSSwitch *ISSSwitch_Create(HeapID heapId);
void ISSSwitch_Free(ISSSwitch *sw);
// Sets the length of the switch's fades, in frames, and its tracks
void ISSSwitch_Set(ISSSwitch *sw, u16 fadeFrames, u16 trackMask);
BOOL ISSSwitch_IsOn(ISSSwitch *sw);
void ISSSwitch_ReqFadeIn(ISSSwitch *sw);
void ISSSwitch_ReqFadeOut(ISSSwitch *sw);
void ISSSwitch_Unmute(ISSSwitch *sw);
void ISSSwitch_Mute(ISSSwitch *sw);
void ISSSwitch_Update(ISSSwitch *sw);
// Sets the volume of the switch's tracks, scaled by the switch set's master volume
void ISSSwitch_CommitVolume(ISSSwitch *sw, int masterVolume);

#endif // POKEBW2_SYSTEM_ISS_SWITCH_H
