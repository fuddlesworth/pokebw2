#ifndef POKEBW2_APP_MUSICAL_MUS_SHOT_PHOTO_H
#define POKEBW2_APP_MUSICAL_MUS_SHOT_PHOTO_H

// Overlay 209's mus_shot_photo.c: the photo of a musical's finale, drawn on the main screen with the stage's actors

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/savedata/musical_savedata.h"
#include "system/savedata/save_control.h"

MusShotPhoto *MusShotPhoto_Create(MusicalShot *shot, HeapID heapId);
void MusShotPhoto_Delete(MusShotPhoto *photo);
void MusShotPhoto_Main(MusShotPhoto *photo);

#endif // POKEBW2_APP_MUSICAL_MUS_SHOT_PHOTO_H
