#ifndef POKEBW2_SYSTEM_ISS_ISS_ROAD_SYS_H
#define POKEBW2_SYSTEM_ISS_ISS_ROAD_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The interactive sound system's routes (iss_road_sys.c): raises the volume of the BGM's extra tracks while the player
// moves and lowers it while they stand still. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0),
// except ISSRoadSys_Free, ISSRoadSys_Disable and ISSRoadSys_DisableCore

ISSRoadSys *ISSRoadSys_Create(PlayerState *player, HeapID heapId);
void ISSRoadSys_Free(ISSRoadSys *sys);
void ISSRoadSys_Update(ISSRoadSys *sys);
void ISSRoadSys_Enable(ISSRoadSys *sys);
void ISSRoadSys_Disable(ISSRoadSys *sys);

#endif // POKEBW2_SYSTEM_ISS_ROAD_SYS_H
