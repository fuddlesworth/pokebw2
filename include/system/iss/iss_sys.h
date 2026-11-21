#ifndef POKEBW2_SYSTEM_ISS_ISS_SYS_H
#define POKEBW2_SYSTEM_ISS_ISS_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The interactive sound system (ISS), which changes the field BGM's tracks as the player moves: it owns the road, city,
// dungeon, zone, switch and 3D sound subsystems and enables the one that the BGM uses. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except ISS_ChangeRoadSysZone, ISSSubsystemFuncs and the
// ISSSubsystem enum's values

// The subsystem a BGM uses, from the BGM info archive
enum {
    ISS_SUBSYSTEM_NONE,
    // 1 and 6 have no subsystem
    ISS_SUBSYSTEM_UNUSED_1,
    ISS_SUBSYSTEM_ROAD,
    ISS_SUBSYSTEM_CITY,
    ISS_SUBSYSTEM_3D_SOUND,
    ISS_SUBSYSTEM_DUNGEON,
    ISS_SUBSYSTEM_UNUSED_6,
    ISS_SUBSYSTEM_SWITCH,
    ISS_SUBSYSTEM_ZONE,
    ISS_SUBSYSTEM_COUNT,
};

ISS *ISS_Create(GameData *gameData, HeapID heapId);
void ISS_Free(ISS *iss);
void ISS_Update(ISS *iss);
void ISS_ChangeZone(ISS *iss, u16 zoneId);
ISSSwitchSys *ISS_GetSwitchSys(ISS *iss);
ISS3DSoundSys *ISS_Get3DSoundSys(ISS *iss);
ISSDungeonSys *ISS_GetDungeonSys(ISS *iss);

#endif // POKEBW2_SYSTEM_ISS_SYS_H
