#ifndef POKEBW2_BATTLE_B_BAG_ANM_H
#define POKEBW2_BATTLE_B_BAG_ANM_H

#include "types.h"
#include "struct_decls.h"

// The frames: four pockets, six item slots, and the page controls
enum {
    BBAG_BUTTON_POCKET_HP_PP = 0,
    BBAG_BUTTON_POCKET_STATUS = 1,
    BBAG_BUTTON_POCKET_BALLS = 2,
    BBAG_BUTTON_POCKET_BATTLE_ITEMS = 3,
    BBAG_BUTTON_ITEM = 4,
    BBAG_BUTTON_BACK = 10,
    BBAG_BUTTON_PREV_PAGE = 11,
    BBAG_BUTTON_NEXT_PAGE = 12,
    BBAG_BUTTON_ACTION = 13,
    BBAG_BUTTON_MAX = 14,
};

// The palette rows for page controls and for pocket/item grid buttons
enum {
    BBAG_CONTROL_PALETTE_NORMAL = 1,
    BBAG_CONTROL_PALETTE_PRESSED = 2,
    BBAG_CONTROL_PALETTE_DISABLED = 3,
    BBAG_GRID_PALETTE_NORMAL = 4,
    BBAG_GRID_PALETTE_PRESSED = 5,
    BBAG_GRID_PALETTE_DISABLED = 6,
};

// A button press and its two timed waits
enum {
    BBAG_BUTTON_ANM_PRESS = 0,
    BBAG_BUTTON_ANM_WAIT_PRESS = 1,
    BBAG_BUTTON_ANM_WAIT_RELEASE = 2,
};

void BBagAnm_CreateButtons(BBagWork *work);
void BBagAnm_DeleteButtons(BBagWork *work);
void BBagAnm_PutPageButtons(BBagWork *work, u8 page);
void BBagAnm_StartButtonAnm(BBagWork *work, u8 button);
void BBagAnm_MainButtonAnm(BBagWork *work);

#endif // POKEBW2_BATTLE_B_BAG_ANM_H
