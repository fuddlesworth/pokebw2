#ifndef POKEBW2_FIELD_MAP_FIELD_NOGRID_MAPPER_H
#define POKEBW2_FIELD_MAP_FIELD_NOGRID_MAPPER_H

// Overlay 36's field_nogrid_mapper.c: the mapper of the maps without a grid, which ties together the rail system that
// the player moves on, the loader of its data, the rails' tile attributes and the camera areas. Function names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except FieldNoGridMapper_SetLineEnabled

#include "types.h"
#include "field/camera/field_scene_area.h"
#include "gfl/heap.h"
#include "struct_decls.h"

NoGridMapper *FieldNoGridMapper_Create(u32 heapId, FieldCamera *camera, FieldSceneArea *sceneArea,
                                       FieldSceneAreaLoader *sceneAreaLoader);
void FieldNoGridMapper_Free(NoGridMapper *mapper);
// Turns the camera areas on or off; the rail system's camera only follows the player while they are on
void FieldNoGridMapper_SetCameraAreaEnabled(NoGridMapper *mapper, BOOL enabled);
// Loads the rails and camera areas of the file in ARCID_RAIL_DATA
void FieldNoGridMapper_LoadByHeader(NoGridMapper *mapper, u32 railId, u32 heapId);
BOOL FieldNoGridMapper_HasRailData(NoGridMapper *mapper);
void FieldNoGridMapper_UpdateCamera(NoGridMapper *mapper);
void FieldNoGridMapper_ForceUpdateCamera(NoGridMapper *mapper);
RailUnit *FieldNoGridMapper_AcquireUnit(NoGridMapper *mapper);
void FieldNoGridMapper_ReleaseUnit(NoGridMapper *mapper, RailUnit *unit);
// Makes the camera follow the unit, or not
void FieldNoGridMapper_SetCameraParent(NoGridMapper *mapper, RailUnit *unit);
void FieldNoGridMapper_ClearCameraParent(NoGridMapper *mapper);
u32 FieldNoGridMapper_GetTileAtPos(NoGridMapper *mapper, const RailPosition *pos);
void FieldNoGridMapper_SetPlayerPos(NoGridMapper *mapper, const RailPosition *pos);
// Turns a rail line on or off; the rail system treats a line that is off as missing
void FieldNoGridMapper_SetLineEnabled(NoGridMapper *mapper, u32 lineId, BOOL enabled);
// Sets pos to line componentId of another zone's rails, front steps along it and side steps from its edge
void FieldNoGridMapper_CreatePosExternal(NoGridMapper *mapper, u32 zoneId, u16 componentId, u16 front, u16 side,
                                         RailPosition *pos, HeapID heapId);
FieldRailSystem *FieldNoGridMapper_GetRailSystem(NoGridMapper *mapper);

#endif // POKEBW2_FIELD_FIELD_NOGRID_MAPPER_H
