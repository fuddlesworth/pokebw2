#ifndef POKEBW2_DWCUTIL_KEYBOARD10_H
#define POKEBW2_DWCUTIL_KEYBOARD10_H

#include "types.h"

// The connection utility's numeric keypad: ten digit keys, two arrow keys and two buttons under them, which slide up
// into place row by row and down again when it closes. A key chosen by touch, by the D-pad and A, or by B, is read
// back as its code (the digit's own, DWC_UTIL_KB10_KEY_LEFT and _RIGHT for the arrows, _CANCEL and _CONFIRM for
// the buttons); DWC_UTIL_KB10_KEY_IDLE until the keypad is up. The digits, the left arrow and the right arrow can each
// be disabled, which makes them buzz. The file is the DWC utility library's keyboard10.o (libdwcutil.TWL.HYB.a), as
// aletheia's map of Black 1's SDK code to that library's objects (layout/symbols_sdk.txt) shows it.

// What DWCi_KB10lGet reports besides a digit's own code
enum {
    DWC_UTIL_KB10_KEY_NONE = 0,
    DWC_UTIL_KB10_KEY_LEFT = 0x10,
    DWC_UTIL_KB10_KEY_RIGHT = 0x11,
    DWC_UTIL_KB10_KEY_CANCEL = 0x12,
    DWC_UTIL_KB10_KEY_CONFIRM = 0x13,
    DWC_UTIL_KB10_KEY_IDLE = 0x1f,
};

void DWCi_KB10lInit(void);
// Slides the keypad down and frees it
void DWCi_KB10lEnd(void);
u8 DWCi_KB10lGet(void);
void DWCi_KB10lSetErase(u8 enabled);
void DWCi_KB10lSetInput(u8 enabled);
void DWCi_KB10lSetShift(u8 enabled);
// Whether the keypad exists, until it has slid down
BOOL DWCi_KB10lIsInit(void);

#endif // POKEBW2_DWCUTIL_KEYBOARD10_H
