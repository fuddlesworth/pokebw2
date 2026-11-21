#include "types.h"
#include "constants/pokemon.h"
#include "pml/hm_check.h"
#include "pml/item.h"
#include "pml/poke_party.h"

// Whether a Pokémon's moves are HMs. The file's name is a guess, from what it does: the ROM has no string for it

BOOL isPkmMoveHmMove(GameData *gameData, u16 move, HeapID heapId) {
    return PML_MoveIsHM(move);
}

BOOL doesPkmHaveTmMove(BoxPkm *pkm, u32 unused) {
    int i;

    for (i = 0; i < 4; i++) {
        if (PML_MoveIsHM(PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + i, NULL))) {
            return FALSE;
        }
    }
    return TRUE;
}
