#ifndef POKEBW2_BATTLE_B_BAG_UI_H
#define POKEBW2_BATTLE_B_BAG_UI_H

#include "types.h"
#include "struct_decls.h"

// The battle bag's key cursor (b_bag_ui.c, our name)

// Cursor positions after the four pockets or six item slots
enum {
    BBAG_UI_POCKET_LAST_ITEM = 4,
    BBAG_UI_POCKET_CANCEL = 5,
};

enum {
    BBAG_UI_ITEM_CANCEL = 6,
    BBAG_UI_ITEM_PREV_PAGE = 7,
    BBAG_UI_ITEM_NEXT_PAGE = 8,
};

// The chosen item page
enum {
    BBAG_UI_USE_ITEM = 0,
    BBAG_UI_USE_CANCEL = 1,
};

void BBagUi_CreateCursor(BBagWork *work, u8 page, int pos);
void BBagUi_DeleteCursor(BBagWork *work);
void BBagUi_ChangeCursorPage(BBagWork *work, u8 page, int pos);

#endif // POKEBW2_BATTLE_B_BAG_UI_H
