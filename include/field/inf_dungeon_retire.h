#ifndef POKEBW2_FIELD_INF_DUNGEON_RETIRE_H
#define POKEBW2_FIELD_INF_DUNGEON_RETIRE_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// inf_dungeon_retire.c, overlay 127, which the ROM names: the touch screen's menu for leaving the dungeon

// The menu's work, which the field subscreen of overlay 36 holds
typedef struct InfDungeonRetire InfDungeonRetire;

// The event opens the retire menu and writes the field app result: FIELD_APP_RESULT_SCRIPT for yes, or
// FIELD_APP_RESULT_RETURN for no
GameEvent *InfDungeonRetire_CreateEvent(GameSystem *gsys, Field *field, u32 screenId, u32 *result);
InfDungeonRetire *InfDungeonRetire_Create(FieldSubscreen *subscreen, GameSystem *gsys, HeapID heapId);
void InfDungeonRetire_Update(InfDungeonRetire *work);
void InfDungeonRetire_Free(InfDungeonRetire *work);

#endif // POKEBW2_FIELD_INF_DUNGEON_RETIRE_H
