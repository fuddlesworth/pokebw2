#include "types.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "system/iss/iss_city_unit.h"

// A city sound unit of the interactive sound system. Names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0), except ISSCityUnit_Free

// The number of distances each axis sets a volume at
#define ISS_CITY_UNIT_KEY_COUNT 6
// The size of a map cell, in whole units
#define ISS_CITY_UNIT_CELL_SIZE 16

// A unit as ARCID_ISS_CITY stores it
typedef struct {
    u16 zoneId;
    // The center's cell
    s32 x;
    s32 y;
    s32 z;
    // The volume at each distance, from the farthest
    u8 volume[ISS_CITY_UNIT_KEY_COUNT];
    // The distances from the center along each axis, in cells, from the farthest
    u8 rangeX[ISS_CITY_UNIT_KEY_COUNT];
    u8 rangeY[ISS_CITY_UNIT_KEY_COUNT];
    u8 rangeZ[ISS_CITY_UNIT_KEY_COUNT];
} ISSCityUnitData;

struct ISSCityUnit {
    u16 zoneId;
    s32 x;
    s32 y;
    s32 z;
    u8 volume[ISS_CITY_UNIT_KEY_COUNT];
    u8 rangeX[ISS_CITY_UNIT_KEY_COUNT];
    u8 rangeY[ISS_CITY_UNIT_KEY_COUNT];
    u8 rangeZ[ISS_CITY_UNIT_KEY_COUNT];
};

static void ISSCityUnit_LoadArcData(const ISSCityUnitData *data, ISSCityUnit *unit);
static u8 ISSCityUnit_CalcEmitterVolumeCore(const ISSCityUnit *unit, const VecFx32 *pos);
static u8 ISSCityUnit_CalcEmitterVolumeX(const ISSCityUnit *unit, const VecFx32 *pos);
static u8 ISSCityUnit_CalcEmitterVolumeY(const ISSCityUnit *unit, const VecFx32 *pos);
static u8 ISSCityUnit_CalcEmitterVolumeZ(const ISSCityUnit *unit, const VecFx32 *pos);
static u8 ISSCityUnit_GetKeyIndexX(const ISSCityUnit *unit, const VecFx32 *pos);
static u8 ISSCityUnit_GetKeyIndexY(const ISSCityUnit *unit, const VecFx32 *pos);
static u8 ISSCityUnit_GetKeyIndexZ(const ISSCityUnit *unit, const VecFx32 *pos);
static int ISSCityUnit_GetOriginX(const ISSCityUnit *unit);
static int ISSCityUnit_GetOriginY(const ISSCityUnit *unit);
static int ISSCityUnit_GetOriginZ(const ISSCityUnit *unit);
static int ISSCityUnit_Lerp(int x0, int y0, int x1, int y1, int x);

ISSCityUnit *ISSCityUnit_Create(HeapID heapId, u32 index) {
    ISSCityUnit *unit = GFL_HeapAllocate(heapId, sizeof(ISSCityUnit), FALSE, "iss_city_unit.c", 108);
    ISSCityUnitData *data = GFL_ArcSysReadHeapNewRange(ARCID_ISS_CITY, index, heapId, 0, sizeof(ISSCityUnitData));

    ISSCityUnit_LoadArcData(data, unit);
    GFL_HeapFree(data);
    return unit;
}

void ISSCityUnit_Free(ISSCityUnit *unit) {
    GFL_HeapFree(unit);
}

u16 ISSCityUnit_GetZoneID(const ISSCityUnit *unit) {
    return unit->zoneId;
}

u8 ISSCityUnit_CalcEmitterVolume(const ISSCityUnit *unit, const VecFx32 *pos) {
    return ISSCityUnit_CalcEmitterVolumeCore(unit, pos);
}

static void ISSCityUnit_LoadArcData(const ISSCityUnitData *data, ISSCityUnit *unit) {
    int i;

    unit->zoneId = data->zoneId;
    unit->x = data->x;
    unit->y = data->y;
    unit->z = data->z;
    for (i = 0; i < ISS_CITY_UNIT_KEY_COUNT; i++) {
        unit->volume[i] = data->volume[i];
        unit->rangeX[i] = data->rangeX[i];
        unit->rangeY[i] = data->rangeY[i];
        unit->rangeZ[i] = data->rangeZ[i];
    }
}

// The product of the three axes' volumes
static u8 ISSCityUnit_CalcEmitterVolumeCore(const ISSCityUnit *unit, const VecFx32 *pos) {
    int volumeX = ISSCityUnit_CalcEmitterVolumeX(unit, pos);
    int volumeY = ISSCityUnit_CalcEmitterVolumeY(unit, pos);
    int volumeZ = ISSCityUnit_CalcEmitterVolumeZ(unit, pos);

    return volumeX * volumeY * volumeZ / (SND_VOLUME_MAX * SND_VOLUME_MAX);
}

static u8 ISSCityUnit_CalcEmitterVolumeX(const ISSCityUnit *unit, const VecFx32 *pos) {
    u8 key = ISSCityUnit_GetKeyIndexX(unit, pos);
    u8 prevKey;
    int position;
    int origin;
    int distance;

    if (key == 0) {
        prevKey = 0;
    } else {
        prevKey = key - 1;
    }
    if (key == prevKey) {
        return unit->volume[key];
    }
    position = FX_Whole(pos->x);
    origin = ISSCityUnit_GetOriginX(unit);
    distance = MATH_ABS(position - origin);
    return ISSCityUnit_Lerp(unit->rangeX[key] * ISS_CITY_UNIT_CELL_SIZE, unit->volume[key],
                            unit->rangeX[prevKey] * ISS_CITY_UNIT_CELL_SIZE, unit->volume[prevKey], distance);
}

static u8 ISSCityUnit_CalcEmitterVolumeY(const ISSCityUnit *unit, const VecFx32 *pos) {
    u8 key = ISSCityUnit_GetKeyIndexY(unit, pos);
    u8 prevKey;
    int position;
    int origin;
    int distance;

    if (key == 0) {
        prevKey = 0;
    } else {
        prevKey = key - 1;
    }
    if (key == prevKey) {
        return unit->volume[key];
    }
    position = FX_Whole(pos->y);
    origin = ISSCityUnit_GetOriginY(unit);
    distance = MATH_ABS(position - origin);
    return ISSCityUnit_Lerp(unit->rangeY[key] * ISS_CITY_UNIT_CELL_SIZE, unit->volume[key],
                            unit->rangeY[prevKey] * ISS_CITY_UNIT_CELL_SIZE, unit->volume[prevKey], distance);
}

static u8 ISSCityUnit_CalcEmitterVolumeZ(const ISSCityUnit *unit, const VecFx32 *pos) {
    u8 key = ISSCityUnit_GetKeyIndexZ(unit, pos);
    u8 prevKey;
    int position;
    int origin;
    int distance;

    if (key == 0) {
        prevKey = 0;
    } else {
        prevKey = key - 1;
    }
    if (key == prevKey) {
        return unit->volume[key];
    }
    position = FX_Whole(pos->z);
    origin = ISSCityUnit_GetOriginZ(unit);
    distance = MATH_ABS(position - origin);
    return ISSCityUnit_Lerp(unit->rangeZ[key] * ISS_CITY_UNIT_CELL_SIZE, unit->volume[key],
                            unit->rangeZ[prevKey] * ISS_CITY_UNIT_CELL_SIZE, unit->volume[prevKey], distance);
}

// The first key at or inside the distance, or 0 past the farthest
static u8 ISSCityUnit_GetKeyIndexX(const ISSCityUnit *unit, const VecFx32 *pos) {
    int i;
    int position = FX_Whole(pos->x);
    int origin = ISSCityUnit_GetOriginX(unit);
    int distance = MATH_ABS(position - origin);

    for (i = 0; i < ISS_CITY_UNIT_KEY_COUNT; i++) {
        if (unit->rangeX[i] * ISS_CITY_UNIT_CELL_SIZE <= distance) {
            return i;
        }
    }
    return 0;
}

static u8 ISSCityUnit_GetKeyIndexY(const ISSCityUnit *unit, const VecFx32 *pos) {
    int i;
    int position = FX_Whole(pos->y);
    int origin = ISSCityUnit_GetOriginY(unit);
    int distance = MATH_ABS(position - origin);

    for (i = 0; i < ISS_CITY_UNIT_KEY_COUNT; i++) {
        if (unit->rangeY[i] * ISS_CITY_UNIT_CELL_SIZE <= distance) {
            return i;
        }
    }
    return 0;
}

static u8 ISSCityUnit_GetKeyIndexZ(const ISSCityUnit *unit, const VecFx32 *pos) {
    int i;
    int position = FX_Whole(pos->z);
    int origin = ISSCityUnit_GetOriginZ(unit);
    int distance = MATH_ABS(position - origin);

    for (i = 0; i < ISS_CITY_UNIT_KEY_COUNT; i++) {
        if (unit->rangeZ[i] * ISS_CITY_UNIT_CELL_SIZE <= distance) {
            return i;
        }
    }
    return 0;
}

// The center of the unit's cell, in whole units
static int ISSCityUnit_GetOriginX(const ISSCityUnit *unit) {
    return unit->x * ISS_CITY_UNIT_CELL_SIZE + ISS_CITY_UNIT_CELL_SIZE / 2.0;
}

static int ISSCityUnit_GetOriginY(const ISSCityUnit *unit) {
    return unit->y * ISS_CITY_UNIT_CELL_SIZE + ISS_CITY_UNIT_CELL_SIZE / 2.0;
}

static int ISSCityUnit_GetOriginZ(const ISSCityUnit *unit) {
    return unit->z * ISS_CITY_UNIT_CELL_SIZE + ISS_CITY_UNIT_CELL_SIZE / 2.0;
}

// The y at x on the line through (x0, y0) and (x1, y1)
static int ISSCityUnit_Lerp(int x0, int y0, int x1, int y1, int x) {
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}
