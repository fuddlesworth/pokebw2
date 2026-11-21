#ifndef POKEBW2_APP_WIN_RECORD_GRAPHIC_H
#define POKEBW2_APP_WIN_RECORD_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// win_record_graphic.c: the BGs and cell actors of the Pokémon World Tournament's win record. The names are ours

typedef struct WinRecordGraphic WinRecordGraphic;

WinRecordGraphic *WinRecordGraphic_Create(u32 layout, HeapID heapId);
void WinRecordGraphic_Delete(WinRecordGraphic *graphic);
// Called every frame
void WinRecordGraphic_Main(WinRecordGraphic *graphic);
// Begin and end the frame's 3D, which the win record doesn't draw
void WinRecordGraphic_Begin3D(WinRecordGraphic *graphic);
void WinRecordGraphic_End3D(WinRecordGraphic *graphic);
ClActUnit *WinRecordGraphic_GetClActUnit(WinRecordGraphic *graphic);

#endif // POKEBW2_APP_WIN_RECORD_GRAPHIC_H
