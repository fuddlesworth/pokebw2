#ifndef POKEBW2_SYSTEM_ISS_ISS_SWITCH_SYS_H
#define POKEBW2_SYSTEM_ISS_ISS_SWITCH_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/iss/iss_switch_set.h"

// The interactive sound system's BGM switches: the switch set of the BGM that is playing, whose switches fade the
// BGM's tracks in and out, and requests that wait for a BGM to play. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except ISSSwitchSys_ResetSwitches,
// ISSSwitchSys_ResetMuteStateChangeRequests and ISSSwitchSys_ResetMasterVolumeChangeRequest

ISSSwitchSys *ISSSwitchSys_Create(HeapID heapId);
void ISSSwitchSys_Free(ISSSwitchSys *switchSys);
void ISSSwitchSys_Update(ISSSwitchSys *switchSys);
void ISSSwitchSys_Enable(ISSSwitchSys *switchSys);
void ISSSwitchSys_Disable(ISSSwitchSys *switchSys);
// Disables the system when the BGM's switch set does not play in the zone
void ISSSwitchSys_ChangeZone(ISSSwitchSys *switchSys, u16 zoneId);
void ISSSwitchSys_SeqMasterVolumeFade(ISSSwitchSys *switchSys, int volume, u16 frames);
void ISSSwitchSys_ReqSwitchFadeIn(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex);
void ISSSwitchSys_ReqSwitchFadeOut(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex);
BOOL ISSSwitchSys_IsSwitchOn(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex);
void ISSSwitchSys_ResetSwitches(ISSSwitchSys *switchSys);
// Mutes or unmutes a switch's tracks when the BGM is playing, or else when it next plays
void ISSSwitch_ReqSwitchMuteStateChange(ISSSwitchSys *switchSys, ISSSwitchIndex switchIndex, BOOL unmute, u32 bgm);
void ISSSwitchSys_ResetMuteStateChangeRequests(ISSSwitchSys *switchSys);
// Fades the master volume when the BGM is playing, or else when it next plays
void ISSSwitchSys_ReqMasterVolumeChange(ISSSwitchSys *switchSys, int volume, u16 frames, u32 bgm);
void ISSSwitchSys_ResetMasterVolumeChangeRequest(ISSSwitchSys *switchSys);

#endif // POKEBW2_SYSTEM_ISS_SWITCH_SYS_H
