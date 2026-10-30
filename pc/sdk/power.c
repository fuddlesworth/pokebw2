#include "gfl/ui.h"
#include "nitro/os.h"
#include "twl/pm.h"
#include <SDL3/SDL.h>

static u8 sleep_blocks;

BOOL PM_GetAutoExit(void) {
    // Desktop quit requests arrive through SDL rather than the DSi's power-button service.
    return FALSE;
}
void PM_ReadyToExit(void) {
    OS_ResetSystem(0);
}

void GCTX_HIDBlockSleep(u8 flags) {
    sleep_blocks |= flags;
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_DisableScreenSaver();
}
void GCTX_HIDUnblockSleep(u8 flags) {
    sleep_blocks &= (u8)~flags;
    if (!sleep_blocks && SDL_WasInit(SDL_INIT_VIDEO))
        SDL_EnableScreenSaver();
}
BOOL GCTX_HIDIsSleepBlocked(u8 flags) {
    return (sleep_blocks & flags) != 0;
}
