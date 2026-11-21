#ifndef POKEBW2_SYSTEM_ISS_ISS_ZONE_SYS_H
#define POKEBW2_SYSTEM_ISS_ISS_ZONE_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The interactive sound system's zone fades: on a zone change, fades in and out the BGM's tracks that each zone sets.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except ISSZoneSys_Free, ISSZoneSys_Disable
// and ISSZoneSys_DisableCore

ISSZoneSys *ISSZoneSys_Create(HeapID heapId);
void ISSZoneSys_Free(ISSZoneSys *sys);
void ISSZoneSys_Update(ISSZoneSys *sys);
void ISSZoneSys_ChangeZone(ISSZoneSys *sys, u16 zoneId);
void ISSZoneSys_Enable(ISSZoneSys *sys, u16 zoneId);
void ISSZoneSys_Disable(ISSZoneSys *sys);

#endif // POKEBW2_SYSTEM_ISS_ZONE_SYS_H
