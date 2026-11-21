// The camera areas of a zone: the parts of the map where the camera changes, which are checked against the player's
// position each frame. The name is the ROM's own, from GFL_HeapAllocate's file argument. Function names and layouts
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_scene_area.h"
#include "gfl/heap.h"

static BOOL FieldCameraArea_CallCollCheckFunction(const FieldDynCameraFunctions *funcs, u16 index, FieldSceneArea *area,
                                                  const CameraArea *cameraArea, const VecFx32 *pos);
static void FieldCameraArea_CallCalcFunction(const FieldDynCameraFunctions *funcs, u16 index, FieldSceneArea *area,
                                             const CameraArea *cameraArea, const VecFx32 *pos);

FieldSceneArea *FieldSceneArea_Create(u32 heapId, FieldCamera *camera, Field *field) {
    FieldSceneArea *area = GFL_HeapAllocate(heapId, sizeof(FieldSceneArea), TRUE, "fld_scenearea.c", 81);

    area->camera = camera;
    area->field = field;
    area->lastCamIdx = -1;
    area->isCameraAreaEnable = TRUE;
    return area;
}

void FreeFieldSceneArea(FieldSceneArea *area) {
    ResetSceneArea(area);
    GFL_HeapFree(area);
}

void BuildSceneArea(FieldSceneArea *area, CameraArea *cameraData, u32 count, const FieldDynCameraFunctions *funcs) {
    ResetSceneArea(area);
    area->cameraData = cameraData;
    area->cameraFunctions = funcs;
    area->cameraCount = count;
    area->lastCamIdx = -1;
}

void ResetSceneArea(FieldSceneArea *area) {
    area->cameraData = NULL;
    area->cameraFunctions = NULL;
    area->cameraCount = 0;
    area->lastCamIdx = -1;
}

int FieldCameraArea_Update(FieldSceneArea *area, const VecFx32 *pos) {
    u32 i;
    int hit;

    if (!area->isCameraAreaEnable) {
        return -1;
    }
    area->currentCamDataFlags = 0xffff;
    hit = -1;
    for (i = 0; i < area->cameraCount; i++) {
        if (FieldCameraArea_CallCollCheckFunction(area->cameraFunctions, area->cameraData[i].collCheckFunc, area,
                                                  &area->cameraData[i], pos)) {
            hit = i;
        }
    }
    if (hit != area->lastCamIdx) {
        if (area->lastCamIdx != -1) {
            FieldCameraArea_CallCalcFunction(area->cameraFunctions, area->cameraData[area->lastCamIdx].exitFunc, area,
                                             &area->cameraData[area->lastCamIdx], pos);
            area->currentCamDataFlags = area->cameraData[area->lastCamIdx].exitFunc;
        }
        if (hit != -1) {
            FieldCameraArea_CallCalcFunction(area->cameraFunctions, area->cameraData[hit].enterFunc, area,
                                             &area->cameraData[hit], pos);
            area->currentCamDataFlags = area->cameraData[hit].enterFunc;
        }
        area->lastCamIdx = hit;
    } else if (area->lastCamIdx != -1) {
        FieldCameraArea_CallCalcFunction(area->cameraFunctions, area->cameraData[area->lastCamIdx].stayFunc, area,
                                         &area->cameraData[area->lastCamIdx], pos);
        area->currentCamDataFlags = area->cameraData[area->lastCamIdx].stayFunc;
    }
    return area->lastCamIdx;
}

void SetFieldSceneAreaCameraAreaEnable(FieldSceneArea *area, BOOL enable) {
    area->isCameraAreaEnable = enable;
}

FieldCamera *GetFieldSceneAreaCamera(FieldSceneArea *area) {
    return area->camera;
}

Field *GetFieldSceneAreaField(FieldSceneArea *area) {
    return area->field;
}

static BOOL IsCamCollCheckFuncValid(const FieldDynCameraFunctions *funcs, u16 index) {
    if (funcs->collCheckFuncCount > index && funcs->collCheckFuncs[index] != NULL) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IsCamCalcFuncValid(const FieldDynCameraFunctions *funcs, u16 index) {
    if (funcs->calcFuncCount > index && funcs->calcFuncs[index] != NULL) {
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldCameraArea_CallCollCheckFunction(const FieldDynCameraFunctions *funcs, u16 index, FieldSceneArea *area,
                                                  const CameraArea *cameraArea, const VecFx32 *pos) {
    if (IsCamCollCheckFuncValid(funcs, index)) {
        return funcs->collCheckFuncs[index](area, cameraArea, pos);
    }
    return FALSE;
}

static void FieldCameraArea_CallCalcFunction(const FieldDynCameraFunctions *funcs, u16 index, FieldSceneArea *area,
                                             const CameraArea *cameraArea, const VecFx32 *pos) {
    if (IsCamCalcFuncValid(funcs, index)) {
        funcs->calcFuncs[index](area, cameraArea, pos);
    }
}
