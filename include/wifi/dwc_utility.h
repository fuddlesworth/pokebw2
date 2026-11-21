#ifndef POKEBW2_APP_DWC_UTILITY_H
#define POKEBW2_APP_DWC_UTILITY_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The Nintendo Wi-Fi Connection settings
#define OVERLAY_DWC_UTILITY OVERLAY_ID(182)

extern const GameProcFunctions DWC_UTILITY_PROC_FUNCTIONS;
// The same, restarting the game when it ends, for the start menu
extern const GameProcFunctions DWC_UTILITY_RESET_PROC_FUNCTIONS;

#endif // POKEBW2_APP_DWC_UTILITY_H
