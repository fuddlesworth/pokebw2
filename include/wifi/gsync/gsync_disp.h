#ifndef POKEBW2_APP_GSYNC_GSYNC_DISP_H
#define POKEBW2_APP_GSYNC_GSYNC_DISP_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Game Sync's screens (gsync_disp.c): the BGs, the actors, the Pokémon sent's icon, the wave and the progress gauge
// of the top screen, and the items and Pokémon of the Dream World that drift down it. The parameter types are guessed
// from this file's code

// The floating items, and after them the floating Pokémon
#define GSYNC_DISP_FLOAT_ITEM_COUNT 20
#define GSYNC_DISP_FLOAT_POKEMON_COUNT 10

GSyncDisp *GSyncDisp_Create(HeapID heapId);
void GSyncDisp_Main(GSyncDisp *disp);
void GSyncDisp_Free(GSyncDisp *disp);
// The actor on the touch screen
void GSyncDisp_CreateSubActor(GSyncDisp *disp);
// The actors of the top screen, one for each of their sequences
void GSyncDisp_CreateActor(GSyncDisp *disp, int index);
void GSyncDisp_SetActorSequence(GSyncDisp *disp, int index, int sequence);
void GSyncDisp_SetActorCallback(GSyncDisp *disp, int index, const ClActorCallback *callback);
void GSyncDisp_DeleteActor(GSyncDisp *disp, int index);
// The icon of the Pokémon sent to the Dream World, and its animations
void GSyncDisp_PokeIconSequence2(GSyncDisp *disp);
void GSyncDisp_CreatePokeIcon(GSyncDisp *disp, BoxPkm *pkm, u32 surface);
void GSyncDisp_StartPokeIcon(GSyncDisp *disp);
void GSyncDisp_PokeIconSequence1(GSyncDisp *disp);
// The top screen's wave, and its fade in or out
void GSyncDisp_StartWave(GSyncDisp *disp);
void GSyncDisp_StartFade(GSyncDisp *disp, BOOL fadeIn);
// Fills the progress gauge to percent
void GSyncDisp_SetProgress(GSyncDisp *disp, int percent);
void GSyncDisp_UpdateFloats(GSyncDisp *disp);
// The Pokémon and items that drift down the screen, whose graphics GSyncDisp_LoadFloatGraphics then loads
void GSyncDisp_SetFloatPokemon(GSyncDisp *disp, int index, int species, int form, int sex);
void GSyncDisp_SetFloatItem(GSyncDisp *disp, int index, u16 item);
void GSyncDisp_LoadFloatGraphics(GSyncDisp *disp);

#endif // POKEBW2_APP_GSYNC_GSYNC_DISP_H
