#ifndef POKEBW2_FIELD_RAIL_FIELD_RAIL_LOADER_H
#define POKEBW2_FIELD_RAIL_FIELD_RAIL_LOADER_H

// Overlay 36's field_rail_loader.c: loads the rail data of a map from ARCID_RAIL_HEADERS. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

RailLoader *FieldRailLoader_Create(u32 heapId);
void FieldRailLoader_Free(RailLoader *loader);
void FieldRailLoader_LoadFile(RailLoader *loader, u32 fileId, u32 heapId);
void FieldRailLoader_Reset(RailLoader *loader);
// The points, lines, curves and cameras of the loaded file, which the rail system works on
RailDataHandle *FieldRailLoader_GetRailData(RailLoader *loader);

#endif // POKEBW2_FIELD_FIELD_RAIL_LOADER_H
