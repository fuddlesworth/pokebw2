#ifndef POKEBW2_APP_PMSIV_MENU_H
#define POKEBW2_APP_PMSIV_MENU_H

#include "types.h"
#include "struct_decls.h"

// The phrase input's buttons on the lower screen, of overlay 185's pmsiv_menu.c: the buttons of the edit area, of the
// categories and of the search, the row of sentence type buttons, and the return button. The names are ours, guessed

// The buttons that PMSIVMenu_StartButton and PMSIVMenu_WaitButton press
enum {
    PMSIV_MENU_ACTION_SEARCH_BACK,
    PMSIV_MENU_ACTION_CATEGORY_MODE,
    PMSIV_MENU_ACTION_SEARCH_ERASE,
    PMSIV_MENU_ACTION_SEARCH_SUBMIT,
};

PMSIVMenu *PMSIVMenu_Create(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk);
void PMSIVMenu_Delete(PMSIVMenu *wk);
// Runs every frame
void PMSIVMenu_Main(PMSIVMenu *wk);
// The buttons of each part of the input
void PMSIVMenu_SetupEditButtons(PMSIVMenu *wk);
void PMSIVMenu_SetupCategoryButtons(PMSIVMenu *wk);
void PMSIVMenu_SetupWordWinButtons(PMSIVMenu *wk);
void PMSIVMenu_UpdateSentenceType(PMSIVMenu *wk);
void PMSIVMenu_UpdateEditButtons(PMSIVMenu *wk);
void PMSIVMenu_UpdateSearchButtons(PMSIVMenu *wk);
void PMSIVMenu_StartButton(PMSIVMenu *wk, u32 button);
// Selects the previous or next sentence arrow
void PMSIVMenu_SetSentenceArrow(PMSIVMenu *wk, u32 which);
BOOL PMSIVMenu_WaitButton(PMSIVMenu *wk, u32 button);
void PMSIVMenu_EndButton(PMSIVMenu *wk, u32 button);
void PMSIVMenu_SetCursor(PMSIVMenu *wk, u8 pos, BOOL active);
void PMSIVMenu_FlashButton(PMSIVMenu *wk, u8 pos, BOOL flash);
BOOL PMSIVMenu_IsFlashFinished(PMSIVMenu *wk, u8 pos);
// Plays the return button's animation, if it is shown and still
BOOL PMSIVMenu_StartReturn(PMSIVMenu *wk);
BOOL PMSIVMenu_WaitReturn(PMSIVMenu *wk);

// Search button slots and matching touch rectangles; separate from the action IDs above.
enum {
    PMSIV_SEARCH_SLOT_SUBMIT = 0,
    PMSIV_SEARCH_SLOT_ERASE = 1,
    PMSIV_SEARCH_SLOT_BACK = 2,
};

// The direction selected by SetSentenceArrow.
enum {
    PMSIV_SENTENCE_ARROW_PREVIOUS = 0,
    PMSIV_SENTENCE_ARROW_NEXT = 1,
};

#endif // POKEBW2_APP_PMSIV_MENU_H
