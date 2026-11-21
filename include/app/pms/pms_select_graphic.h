#ifndef POKEBW2_APP_PMS_PMS_SELECT_GRAPHIC_H
#define POKEBW2_APP_PMS_PMS_SELECT_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// pms_select_graphic.c: the BGs and cell actors of the phrase select of overlay 185. The names are ours

typedef struct PMSSelectGraphic PMSSelectGraphic;

PMSSelectGraphic *PMSSelectGraphic_Create(u32 layout, HeapID heapId);
void PMSSelectGraphic_Delete(PMSSelectGraphic *graphic);
// Called every frame
void PMSSelectGraphic_Main(PMSSelectGraphic *graphic);
ClActUnit *PMSSelectGraphic_GetClActUnit(PMSSelectGraphic *graphic);

#endif // POKEBW2_APP_PMS_SELECT_GRAPHIC_H
