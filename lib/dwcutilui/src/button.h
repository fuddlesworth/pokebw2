#ifndef POKEBW2_DWCUTIL_BUTTON_H
#define POKEBW2_DWCUTIL_BUTTON_H

#include "types.h"

// The settings utility's row of selectable objects, one per Wi-Fi option
// variant. The caller shows them with DWCi_BTNlInit, reads the selection
// back through DWCi_BTNlGet, and hides them with
// DWCi_BTNlEnd while DWCi_BTNlIsEnd reports the exit.

// What DWCi_BTNlGet reports, and what DWCi_BTNlSet and DWCi_BTNlSetForce take: the row is still sliding in, no button
// is pressed, or the one of the two under the row that is, the cancel button that B presses and the confirm one that A
// presses
enum {
    DWC_UTIL_BUTTON_HIDDEN = -2,
    DWC_UTIL_BUTTON_NONE = -1,
    DWC_UTIL_BUTTON_CANCEL,
    DWC_UTIL_BUTTON_CONFIRM,
};

void DWCi_BTNlInit(int variant);
void DWCi_BTNlEnd(void);
int DWCi_BTNlGet(void);
void DWCi_BTNlSet(int state);
void DWCi_BTNlSetForce(int state);
BOOL DWCi_BTNlIsEnd(void);
void DWCi_BTNlEnable(void);
void DWCi_BTNlDisable(void);

#endif // POKEBW2_DWCUTIL_BUTTON_H
