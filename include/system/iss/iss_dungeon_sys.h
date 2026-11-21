#ifndef POKEBW2_SYSTEM_ISS_ISS_DUNGEON_SYS_H
#define POKEBW2_SYSTEM_ISS_ISS_DUNGEON_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The interactive sound system's dungeons: in a zone that the dungeon archive lists, sets the BGM's tempo and the
// pitch of its tracks for the season. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

ISSDungeonSys *ISSDungeonSys_Create(GameData *gameData, PlayerState *playerState, HeapID heapId);
void ISSDungeonSys_Free(ISSDungeonSys *sys);
void ISSDungeonSys_Update(ISSDungeonSys *sys);
// The zone changes on the next update
void ISSDungeonSys_ReqChangeZone(ISSDungeonSys *sys, u16 zoneId);
void ISSDungeonSys_Enable(ISSDungeonSys *sys);
void ISSDungeonSys_Disable(ISSDungeonSys *sys);
BOOL ISSDungeonSys_IsEnabled(ISSDungeonSys *sys);
// Whether the dungeon archive lists the zone
BOOL ISSDungeonSys_IsZoneRegist(ISSDungeonSys *sys, u16 zoneId);

#endif // POKEBW2_SYSTEM_ISS_DUNGEON_SYS_H
