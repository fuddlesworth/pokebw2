#ifndef POKEBW2_APP_PMS_PMSI_INITIAL_DATA_H
#define POKEBW2_APP_PMS_PMSI_INITIAL_DATA_H

#include "types.h"
#include "gfl/str.h"

// The initials that the phrase input sorts words by, A to Z and the others, and where their buttons are, of overlay
// 185. The ROM doesn't name this file; pmsi_initial_data.c is a guess after its neighbors. The names are ours, guessed

u32 PMSIInitial_GetCount(void);
// The initial's letter
void PMSIInitial_GetString(u32 initial, StrBuf *buf);
u16 PMSIInitial_GetCode(u32 initial);
// Where the initial's button is
void PMSIInitial_GetPos(u32 initial, u32 *x, u32 *y);
// The initials next to it, for the cursor, or a button above or below the grid
#define PMSI_INITIAL_POS_BUTTON_0 0xfc
#define PMSI_INITIAL_POS_BUTTON_1 0xfd
#define PMSI_INITIAL_POS_BUTTON_2 0xfe
u32 PMSIInitial_GetUp(u32 initial);
u32 PMSIInitial_GetDown(u32 initial);
u32 PMSIInitial_GetLeft(u32 initial);
u32 PMSIInitial_GetRight(u32 initial);
u32 PMSIInitial_GetBottom(u32 initial);

#endif // POKEBW2_APP_PMSI_INITIAL_DATA_H
