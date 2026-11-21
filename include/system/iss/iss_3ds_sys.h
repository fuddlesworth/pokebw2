#ifndef POKEBW2_SYSTEM_ISS_ISS_3DS_SYS_H
#define POKEBW2_SYSTEM_ISS_ISS_3DS_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The interactive sound system's 3D sound: sets the volume and pan of a BGM's tracks from where each of up to 16 sound
// units is relative to a listener. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// A sound unit, by its index
typedef enum {
    ISS_3DS_UNIT_COUNT = 16,
} ISS3DSoundUnitIndex;

ISS3DSoundSys *ISS3DSoundSys_Create(HeapID heapId);
void ISS3DSoundSys_Free(ISS3DSoundSys *sys);
void ISS3DSoundSys_Update(ISS3DSoundSys *sys);
void ISS3DSoundSys_Enable(ISS3DSoundSys *sys);
void ISS3DSoundSys_Disable(ISS3DSoundSys *sys);
void ISS3DSoundSys_ChangeZone(ISS3DSoundSys *sys, u16 zoneId);
// The master volume moves toward volume, at most 127, a few steps each update
void ISS3DSoundSys_ReqChangeMasterVolume(ISS3DSoundSys *sys, u8 volume);
// A unit plays its tracks at volume where it is, fading to silence at range from it
void ISS3DSoundSys_EnableUnit(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, fx32 range, s32 volume);
BOOL ISS3DSoundSys_IsUnitEnabled(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit);
void ISS3DSoundSys_SetUnitLocation(ISS3DSoundSys *sys, ISS3DSoundUnitIndex unit, const VecFx32 *pos);
// The listener is at pos, facing target
void ISS3DSoundSys_SetListener(ISS3DSoundSys *sys, const VecFx32 *pos, const VecFx32 *target);

#endif // POKEBW2_SYSTEM_ISS_3DS_SYS_H
