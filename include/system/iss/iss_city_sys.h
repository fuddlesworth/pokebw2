#ifndef POKEBW2_SYSTEM_ISS_ISS_CITY_SYS_H
#define POKEBW2_SYSTEM_ISS_ISS_CITY_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The interactive sound system's cities (iss_city_sys.c): while the player is in a zone that has a city sound unit,
// sets the volume of the BGM's city tracks from how far the player is from the unit's center, and fades them in and
// out on zone changes. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except ISSCitySys_Free,
// ISSCitySys_Init, ISSCitySys_UnloadUnits and ISSCitySys_UnloadUnitsCore

ISSCitySys *ISSCitySys_Create(PlayerState *player, HeapID heapId);
void ISSCitySys_Free(ISSCitySys *sys);
void ISSCitySys_Update(ISSCitySys *sys);
void ISSCitySys_Enable(ISSCitySys *sys);
void ISSCitySys_Disable(ISSCitySys *sys);
void ISSCitySys_ChangeZone(ISSCitySys *sys, u16 zoneId);

#endif // POKEBW2_SYSTEM_ISS_CITY_SYS_H
