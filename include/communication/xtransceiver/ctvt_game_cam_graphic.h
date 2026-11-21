#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_GAME_CAM_GRAPHIC_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_GAME_CAM_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The BGs and cell actors of the camera game's screens

CtvtGameCamGraphic *CtvtGameCamGraphic_Create(u32 layout, HeapID heapId);
void CtvtGameCamGraphic_Free(CtvtGameCamGraphic *graphic);
ClActUnit *CtvtGameCamGraphic_GetClActUnit(CtvtGameCamGraphic *graphic);

#endif // POKEBW2_APP_COMM_TVT_CTVT_GAME_CAM_GRAPHIC_H
