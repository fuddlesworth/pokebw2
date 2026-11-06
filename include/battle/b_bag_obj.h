#ifndef POKEBW2_BATTLE_B_BAG_OBJ_H
#define POKEBW2_BATTLE_B_BAG_OBJ_H

#include "types.h"
#include "struct_decls.h"

// The battle bag's OBJ resources and cell actors (b_bag_obj.c, our name)

// Character/palette resource slots, and cell/animation resource slots
enum {
    BBAG_RES_ITEM = 0,
    BBAG_RES_SHOWN_ITEM = 6,
    BBAG_RES_GAUGE = 7,
    BBAG_RES_CURSOR = 8,
};

enum {
    BBAG_CELL_RES_ITEM = 0,
    BBAG_CELL_RES_GAUGE = 1,
    BBAG_CELL_RES_CURSOR = 2,
};

// Two energy units fit in each gauge pip; cost-number sequences use the number itself
enum {
    BBAG_GAUGE_ANM_EMPTY = 15,
    BBAG_GAUGE_ANM_UNSPENT_ONE = 16,
    BBAG_GAUGE_ANM_UNSPENT_TWO = 17,
    BBAG_GAUGE_ANM_UNSPENT_ONE_SPENT_ONE = 18,
    BBAG_GAUGE_ANM_SPENT_ONE = 19,
    BBAG_GAUGE_ANM_SPENT_TWO = 20,
};

void BBagObj_Init(BBagWork *wk);
void BBagObj_Exit(BBagWork *wk);
void BBagObj_SetPage(BBagWork *wk, u8 page);

#endif // POKEBW2_BATTLE_B_BAG_OBJ_H
