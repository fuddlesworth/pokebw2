#ifndef POKEBW2_APP_PMS_PMSIV_CATEGORY_H
#define POKEBW2_APP_PMS_PMSIV_CATEGORY_H

#include "types.h"
#include "gfl/arc.h"
#include "struct_decls.h"

// The phrase input's categories, of overlay 185's pmsiv_category.c: the buttons of the word groups or of the
// initials on BG1, the search's letters and its results on BG6, and their cursor. The names are ours, guessed

// The cursor's positions besides the categories: the three buttons, the last of which goes back
#define PMSIV_CATEGORY_POS_BUTTON_0 0xfc
#define PMSIV_CATEGORY_POS_BUTTON_1 0xfd
#define PMSIV_CATEGORY_POS_BACK 0xfe

// Not referenced
extern const u32 PMSIV_CATEGORY_UNK_7198[3];

PMSIVCategory *PMSIVCategory_Create(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk);
void PMSIVCategory_Delete(PMSIVCategory *wk);
void PMSIVCategory_SetupGraphicDatas(PMSIVCategory *wk, ArcTool *arc);
void PMSIVCategory_VisibleCursor(PMSIVCategory *wk, BOOL visible);
void PMSIVCategory_MoveCursor(PMSIVCategory *wk, u32 pos);
// Brightens the categories, and darkens them again
void PMSIVCategory_StartEnableBG(PMSIVCategory *wk);
BOOL PMSIVCategory_WaitEnableBG(PMSIVCategory *wk);
void PMSIVCategory_StartDisableBG(PMSIVCategory *wk);
BOOL PMSIVCategory_WaitDisableBG(PMSIVCategory *wk);
void PMSIVCategory_SetDisableBG(PMSIVCategory *wk);
void PMSIVCategory_StartBrightDown(PMSIVCategory *wk);
BOOL PMSIVCategory_WaitBrightDown(PMSIVCategory *wk);
// Fades the categories out and in, for the word window
void PMSIVCategory_StartFadeOut(PMSIVCategory *wk);
BOOL PMSIVCategory_WaitFadeOut(PMSIVCategory *wk);
void PMSIVCategory_StartFadeIn(PMSIVCategory *wk);
BOOL PMSIVCategory_WaitFadeIn(PMSIVCategory *wk);
// Shows the groups or the initials, by the input's category mode
void PMSIVCategory_ChangeModeBG(PMSIVCategory *wk);
void PMSIVCategory_ChangeModeScreen(PMSIVCategory *wk);
BOOL PMSIVCategory_WaitModeChange(PMSIVCategory *wk);
void PMSIVCategory_PrintSearchInput(PMSIVCategory *wk);
// Scrolls the search's results in, printed, or out
void PMSIVCategory_StartResultList(PMSIVCategory *wk, BOOL show);
BOOL PMSIVCategory_WaitResultList(PMSIVCategory *wk, BOOL show);
// Plays the cursor's animation of a category chosen
void PMSIVCategory_StartCursorDecide(PMSIVCategory *wk, u32 pos);
BOOL PMSIVCategory_WaitCursorDecide(PMSIVCategory *wk);

#endif // POKEBW2_APP_PMSIV_CATEGORY_H
