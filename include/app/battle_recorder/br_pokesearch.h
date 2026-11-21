#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_POKESEARCH_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_POKESEARCH_H

#include "types.h"
#include "app/battle_recorder/br_util.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/bmp_oam.h"
#include "system/savedata/save_control.h"

// The Battle Recorder's Pokémon search (br_pokesearch.c): picks a Pokémon by the first letter of its name and then
// from a list

// What BrPokeSearch_GetSelect gives
enum {
    BR_POKESEARCH_SELECT_NONE,
    BR_POKESEARCH_SELECT_CANCEL,
    BR_POKESEARCH_SELECT_DECIDE,
};

BrPokeSearch *BrPokeSearch_Init(PokeDexSave *pokedex, BrRes *res, ClActUnit *unit, BmpOamSys *bmpoam, BrFade *fade,
                                BrBallEffect *ballMain, BrBallEffect *ballSub, HeapID heapId);
void BrPokeSearch_Exit(BrPokeSearch *p_wk);
void BrPokeSearch_Main(BrPokeSearch *p_wk);
// TRUE once its text is printed
BOOL BrPokeSearch_PrintMain(BrPokeSearch *p_wk);
void BrPokeSearch_StartUp(BrPokeSearch *p_wk);
void BrPokeSearch_CleanUp(BrPokeSearch *p_wk);
// A BR_POKESEARCH_SELECT_*, with the species in species once one is decided
u32 BrPokeSearch_GetSelect(const BrPokeSearch *cp_wk, u16 *species);

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_POKESEARCH_H
