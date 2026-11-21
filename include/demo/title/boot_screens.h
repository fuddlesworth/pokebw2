#ifndef POKEBW2_DEMO_TITLE_BOOT_SCREENS_H
#define POKEBW2_DEMO_TITLE_BOOT_SCREENS_H

#include "types.h"
#include "gfl/proc.h"

// The Pokémon Company and Nintendo logos, then the copyright notice, which the game shows when it starts (ov162)

typedef struct {
    // TRUE shows only the copyright notice
    BOOL skipLogos;
} BootScreensParam;

extern const GameProcFunctions BOOT_SCREENS_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BOOT_SCREENS_H
