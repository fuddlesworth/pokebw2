#ifndef POKEBW2_DWCUTIL_WINDOW_H
#define POKEBW2_DWCUTIL_WINDOW_H

#include "types.h"

// The connection utility's dialog over the main screen: a frame with a message (as DWCi_COMMENTlDispEx shows it) and
// the buttons of one of six layouts, which slides up from the bottom of the screen, with the screen darkened behind it
// when fade is TRUE, and slides down again once it is closed. Layout 5 has no buttons and closes after 120 frames.
// The file is the DWC utility library's window.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.

// What DWCi_WINlGet reports: the button of the layout's row that was chosen, by its place in the row; -2 while the
// dialog is still sliding up and -1 until a button is chosen
enum {
    DWC_UTIL_DIALOG_HIDDEN = -2,
    DWC_UTIL_DIALOG_NONE = -1,
    DWC_UTIL_DIALOG_BUTTON_FIRST,
    DWC_UTIL_DIALOG_BUTTON_SECOND,
};

void DWCi_WINlInitEx(int msgIndex, int layout, BOOL fade, int slot, int value);
// Closes the dialog
void DWCi_WINlEnd(void);
// The button chosen (DWC_UTIL_DIALOG_BUTTON_FIRST or _SECOND), or DWC_UTIL_DIALOG_HIDDEN or _NONE
int DWCi_WINlGet(void);
// TRUE until the dialog has closed
BOOL DWCi_WINlIsInit(void);

// The offset of the dialog's text in its frame, which nothing reads
extern const u16 gDialogTextOffset[2];

#endif // POKEBW2_DWCUTIL_WINDOW_H
