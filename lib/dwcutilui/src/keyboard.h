#ifndef POKEBW2_DWCUTIL_KEYBOARD_H
#define POKEBW2_DWCUTIL_KEYBOARD_H

#include "types.h"

// The connection utility's keyboard: four rows of character keys in three modes (lower case, shifted and capitals),
// two keys that switch the mode, a space and a backspace key, and two buttons under them, all chosen with the touch
// panel or the pad. The rows slide up into place when it opens and back down when it closes.
// The file is the DWC utility library's keyboard.o: Black 1's decompilation (aletheia) has the same functions, in the
// same order, as libdwcutil.TWL.HYB.a's keyboard.o, and the file's functions have those SDK names, the statics' among
// them (taskStart0 to taskEnd5, touchTriger as the SDK spells it, key, disp).

// What DWCi_KBlGet reports besides a character's own code: no key this frame, the backspace, the cancel and confirm
// buttons under the keys, and the code before the keyboard has read a key
enum {
    DWC_UTIL_KEY_NONE = 0,
    DWC_UTIL_KEY_BACKSPACE = 0x80,
    DWC_UTIL_KEY_CANCEL = 0x82,
    DWC_UTIL_KEY_CONFIRM = 0x83,
    DWC_UTIL_KEY_IDLE = 0xff,
};

void DWCi_KBlInit(void);
// Slides the keyboard down and frees it
void DWCi_KBlEnd(void);
// The code of the key entered this frame: a character's own code, DWC_UTIL_KEY_BACKSPACE, DWC_UTIL_KEY_CANCEL or
// DWC_UTIL_KEY_CONFIRM, or DWC_UTIL_KEY_NONE
u8 DWCi_KBlGet(void);
// Turns the backspace key, and the character and space keys, on or off
void DWCi_KBlSetErase(u8 enabled);
void DWCi_KBlSetInput(u8 enabled);
// Whether the keyboard is still open
BOOL DWCi_KBlIsInit(void);

#endif // POKEBW2_DWCUTIL_KEYBOARD_H
