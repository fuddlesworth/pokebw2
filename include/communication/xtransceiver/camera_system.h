#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_CAMERA_SYSTEM_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_CAMERA_SYSTEM_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "twl/camera.h"

// Gets each captured frame, with the work given to CameraSystem_SetFrameCallback
typedef void (*CameraFrameCallback)(void *frame, void *work);

CameraSystem *CameraSystem_Create(HeapID heapId);
void CameraSystem_Delete(CameraSystem *sys);
void CameraSystem_UpdateSound(CameraSystem *sys);
void CameraSystem_InitDsp(CameraSystem *sys);
void CameraSystem_ExitDsp(CameraSystem *sys);
void CameraSystem_SetFrameCallback(CameraSystem *sys, CameraFrameCallback callback, void *work);
void CameraSystem_AllocBuffers(CameraSystem *sys, int count, HeapID heapId);
void CameraSystem_Start(CameraSystem *sys);
void CameraSystem_Stop(CameraSystem *sys);
BOOL CameraSystem_IsSoundDone(CameraSystem *sys);
void CameraSystem_SetSize(CameraSystem *sys, CAMERASize size);
void CameraSystem_SwitchCamera(CameraSystem *sys, CAMERASelect camera);
void CameraSystem_PlayVideoStartSound(CameraSystem *sys);
void CameraSystem_PlayVideoEndSound(CameraSystem *sys);
void CameraSystem_PlayShutterSound(CameraSystem *sys);
BOOL CameraSystem_IsShutterSoundPlaying(CameraSystem *sys);

#endif // POKEBW2_APP_COMM_TVT_CAMERA_SYSTEM_H
