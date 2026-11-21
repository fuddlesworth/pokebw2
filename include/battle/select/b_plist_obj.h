#ifndef POKEBW2_BATTLE_SELECT_B_PLIST_OBJ_H
#define POKEBW2_BATTLE_SELECT_B_PLIST_OBJ_H

#include "types.h"
#include "struct_decls.h"

// The battle party list's OBJ resources and cell actors (b_plist_obj.c, our name)

void BPlistObj_Init(BPlistWork *wk);
void BPlistObj_Exit(BPlistWork *wk);
void BPlistObj_SetPage(BPlistWork *wk, u8 page);
void BPlistObj_ShowMoveTypes(BPlistWork *wk);
void BPlistObj_UpdatePokeIconAnims(BPlistWork *wk);
void BPlistObj_MovePlate(BPlistWork *wk, int pos, s16 dx);
void BPlistObj_SwapPlates(BPlistWork *wk, int posA, int posB);

#endif // POKEBW2_BATTLE_B_PLIST_OBJ_H
