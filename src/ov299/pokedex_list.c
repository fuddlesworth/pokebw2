#include "types.h"
#include "app/pokedex_list.h"
#include "app/pokedex_list_main.h"
#include "app/pokedex_list_seq.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#ifdef POKEBW2_PC
#include "save/pokedex_savedata.h"
#endif

// The process of the Pokédex's list. The ROM doesn't name this file; pokedex_list.c is a guess after the search
// screen's pokedex_search.c

static BOOL PokedexList_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexList_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions PokedexList_ProcFunctions = {
    PokedexList_ProcInit,
    PokedexList_ProcMain,
    PokedexList_ProcEnd,
};
#ifdef POKEBW2_PC
static PokedexListWork *sNativeList;
static u16 sNativeSpecies;

void PokedexList_SetNativeSpecies(u16 species) {
    sNativeSpecies = species;
}

BOOL PokedexList_GetNativeState(PokedexNativeListState *state) {
    PokedexListWork *wk = sNativeList;

    if (wk == NULL || wk->frameList == NULL || wk->font == NULL || wk->param == NULL)
        return FALSE;
    state->species = wk->param->list;
    state->count = wk->param->count;
    state->selected = FrameList_GetScroll(wk->frameList) + FrameList_GetCursor(wk->frameList);
    state->font = wk->font;
    state->pokedex = wk->param->pokedex;
    state->gameData = wk->param->gameData;
    return TRUE;
}
#endif

static BOOL PokedexList_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexListWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX_LIST, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexListWork), HEAPID_POKEDEX_LIST);
    MI_CpuFill8(wk, 0, sizeof(PokedexListWork));
    wk->param = param;
#ifdef POKEBW2_PC
    sNativeList = wk;
    sNativeSpecies = 0;
#endif
    return TRUE;
}

static BOOL PokedexList_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    if (PokedexListSeq_Main(work) == FALSE) {
#ifdef POKEBW2_PC
        sNativeList = NULL;
#endif
        return TRUE;
    }
    return FALSE;
}

static BOOL PokedexList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
#ifdef POKEBW2_PC
    if (sNativeSpecies != 0) {
        PokedexListWork *wk = work;
        PokeDex_SetLastSpecies(wk->param->pokedex, sNativeSpecies);
    }
    sNativeList = NULL;
#endif
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_POKEDEX_LIST);
    return TRUE;
}
