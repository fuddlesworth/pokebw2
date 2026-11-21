#ifndef POKEBW2_SYSTEM_ISS_ISS_CITY_UNIT_H
#define POKEBW2_SYSTEM_ISS_ISS_CITY_UNIT_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// A city sound unit of the interactive sound system (iss_city_unit.c): a zone's center and, along each axis, the
// volume of the city tracks at six distances from it. Names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0), except ISSCityUnit_Free

// Loads unit `index` of ARCID_ISS_CITY
ISSCityUnit *ISSCityUnit_Create(HeapID heapId, u32 index);
void ISSCityUnit_Free(ISSCityUnit *unit);
u16 ISSCityUnit_GetZoneID(const ISSCityUnit *unit);
// The city tracks' volume, 0 to 127, at a position
u8 ISSCityUnit_CalcEmitterVolume(const ISSCityUnit *unit, const VecFx32 *pos);

#endif // POKEBW2_SYSTEM_ISS_CITY_UNIT_H
