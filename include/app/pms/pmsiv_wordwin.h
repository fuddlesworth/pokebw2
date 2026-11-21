#ifndef POKEBW2_APP_PMS_PMSIV_WORDWIN_H
#define POKEBW2_APP_PMS_PMSIV_WORDWIN_H

#include "types.h"
#include "gfl/clact.h"
#include "struct_decls.h"

// The phrase input's word window, the list of a category's words on BG2 with its cursor and scroll bar, of overlay
// 185's pmsiv_wordwin.c. The names are ours, guessed

PMSIVWordWin *PMSIVWordWin_Create(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk);
void PMSIVWordWin_Delete(PMSIVWordWin *wk);
void PMSIVWordWin_SetupGraphicDatas(PMSIVWordWin *wk);
// Draws the first words, or the words from a line on
void PMSIVWordWin_SetupWords(PMSIVWordWin *wk);
void PMSIVWordWin_RedrawWords(PMSIVWordWin *wk, u32 top);
// Shows the window over the categories, and hides it
void PMSIVWordWin_StartFadeIn(PMSIVWordWin *wk);
BOOL PMSIVWordWin_WaitFadeIn(PMSIVWordWin *wk);
void PMSIVWordWin_StartFadeOut(PMSIVWordWin *wk);
BOOL PMSIVWordWin_WaitFadeOut(PMSIVWordWin *wk);
void PMSIVWordWin_VisibleCursor(PMSIVWordWin *wk, BOOL visible);
void PMSIVWordWin_MoveCursor(PMSIVWordWin *wk, u32 pos);
// Scrolls the words by lines
void PMSIVWordWin_StartScroll(PMSIVWordWin *wk, int vector);
BOOL PMSIVWordWin_WaitScroll(PMSIVWordWin *wk);
// The scroll bar's place, and whether it is shown
BOOL PMSIVWordWin_GetScrollBarPos(PMSIVWordWin *wk, ClActorPos *pos);
void PMSIVWordWin_SetScrollBar(PMSIVWordWin *wk, u32 top, u32 scrollMax);
void PMSIVWordWin_SetScrollBarY(PMSIVWordWin *wk, u32 y);
// The line the scroll bar points at
u32 PMSIVWordWin_GetScrollBarLine(PMSIVWordWin *wk, u32 scrollMax);
// Plays the cursor's animation of a word chosen
void PMSIVWordWin_StartCursorDecide(PMSIVWordWin *wk, u32 pos);
BOOL PMSIVWordWin_WaitCursorDecide(PMSIVWordWin *wk);

#endif // POKEBW2_APP_PMSIV_WORDWIN_H
