#ifndef POKEBW2_DEMO_EGG_EGG_DEMO_H
#define POKEBW2_DEMO_EGG_EGG_DEMO_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// An egg hatching, overlay 307. The egg shakes and cracks, the Pokémon appears and cries, and the player may give it a
// nickname
#define OVERLAY_EGG_DEMO OVERLAY_ID(307)

typedef struct {
    GameData *gameData;
    PartyPkm *pkm;
} EggDemoParam;

typedef struct EggDemoGraphic EggDemoGraphic;
typedef struct EggDemoView EggDemoView;

extern const GameProcFunctions EGG_DEMO_PROC_FUNCTIONS;

// egg_demo_graphic.c
EggDemoGraphic *EggDemoGraphic_Create(u32 layout, HeapID heapId);
void EggDemoGraphic_Free(EggDemoGraphic *graphic);
void EggDemoGraphic_Update(EggDemoGraphic *graphic);
void EggDemoGraphic_Begin3D(EggDemoGraphic *graphic);
void EggDemoGraphic_End3D(EggDemoGraphic *graphic);
ClActUnit *EggDemoGraphic_GetClActUnit(EggDemoGraphic *graphic);

// egg_demo_view.c: the egg and the Pokémon
EggDemoView *EggDemoView_Create(HeapID heapId, PartyPkm *pkm);
void EggDemoView_Free(EggDemoView *view);
void EggDemoView_Update(EggDemoView *view);
void EggDemoView_Draw(EggDemoView *view);
// Starts the egg shaking until it hatches
void EggDemoView_Start(EggDemoView *view);
BOOL EggDemoView_IsHatched(EggDemoView *view);
// Shows the hatched Pokémon, first as white
void EggDemoView_ShowPokemon(EggDemoView *view, PartyPkm *pkm);
BOOL EggDemoView_IsWhite(EggDemoView *view);
// Fades the Pokémon in from white, then plays its cry
void EggDemoView_Reveal(EggDemoView *view);
BOOL EggDemoView_IsDone(EggDemoView *view);

#endif // POKEBW2_DEMO_EGG_DEMO_H
