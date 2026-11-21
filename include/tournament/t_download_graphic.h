#ifndef POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_GRAPHIC_H
#define POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// t_download_graphic.c: the BGs and cell actors of the Pokémon World Tournament's downloaded tournaments. The names
// are ours

typedef struct TDownloadGraphic TDownloadGraphic;

TDownloadGraphic *TDownloadGraphic_Create(u32 layout, HeapID heapId);
void TDownloadGraphic_Delete(TDownloadGraphic *graphic);
// Called every frame
void TDownloadGraphic_Main(TDownloadGraphic *graphic);
// Begin and end the frame's 3D, which the screen doesn't draw
void TDownloadGraphic_Begin3D(TDownloadGraphic *graphic);
void TDownloadGraphic_End3D(TDownloadGraphic *graphic);
ClActUnit *TDownloadGraphic_GetClActUnit(TDownloadGraphic *graphic);

#endif // POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_GRAPHIC_H
