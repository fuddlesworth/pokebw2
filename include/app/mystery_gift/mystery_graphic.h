#ifndef POKEBW2_APP_MYSTERY_GIFT_MYSTERY_GRAPHIC_H
#define POKEBW2_APP_MYSTERY_GIFT_MYSTERY_GRAPHIC_H

// Mystery Gift's display setup: BGs, cell actors and the 3D for the card's effects (ov197, mystery_graphic.c). Our
// names; swan has none for this overlay

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

MysteryGraphic *MysteryGraphic_Create(u32 mode, HeapID heapId);
void MysteryGraphic_Delete(MysteryGraphic *graphic);
void MysteryGraphic_Update(MysteryGraphic *graphic);
void MysteryGraphic_BeginFrame3D(MysteryGraphic *graphic);
void MysteryGraphic_EndFrame3D(MysteryGraphic *graphic);
ClActUnit *MysteryGraphic_GetClactUnit(MysteryGraphic *graphic);
// Switch the main screen to 3D for the effect of a received gift, and back
void MysteryGraphic_Start3D(MysteryGraphic *graphic);
void MysteryGraphic_End3D(MysteryGraphic *graphic);

#endif // POKEBW2_APP_MYSTERY_MYSTERY_GRAPHIC_H
