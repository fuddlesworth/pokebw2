#ifndef POKEBW2_APP_PMS_PMSIV_EDIT_H
#define POKEBW2_APP_PMS_PMSIV_EDIT_H

#include "types.h"
#include "gfl/arc.h"
#include "gfl/touchpanel.h"
#include "struct_decls.h"

// The phrase input's edit area, the sentence being written, drawn on both screens, of overlay 185's pmsiv_edit.c.
// The names are ours, guessed

PMSIVEdit *PMSIVEdit_Create(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk);
void PMSIVEdit_Delete(PMSIVEdit *wk);
void PMSIVEdit_SetupGraphicDatas(PMSIVEdit *wk, ArcTool *arc);
// Scrolls the edit area up or back down on the lower screen
void PMSIVEdit_ScrollSet(PMSIVEdit *wk, BOOL up);
BOOL PMSIVEdit_ScrollWait(PMSIVEdit *wk);
// Draws the sentence or the words again
void PMSIVEdit_UpdateEditArea(PMSIVEdit *wk);
// The touch rectangle of a word of the sentence
void PMSIVEdit_GetWordArea(PMSIVEdit *wk, TouchRect *rect, u32 index);
u32 PMSIVEdit_GetWordCount(PMSIVEdit *wk);
// Which of the sentence's words a word place of the area is
u16 PMSIVEdit_GetWordIndex(PMSIVEdit *wk, u32 index);
void PMSIVEdit_StopCursor(PMSIVEdit *wk);
void PMSIVEdit_ActiveCursor(PMSIVEdit *wk);
void PMSIVEdit_VisibleCursor(PMSIVEdit *wk, BOOL visible);
void PMSIVEdit_StopArrow(PMSIVEdit *wk);
void PMSIVEdit_ActiveArrow(PMSIVEdit *wk);
void PMSIVEdit_MoveCursor(PMSIVEdit *wk, u32 pos);

#endif // POKEBW2_APP_PMSIV_EDIT_H
