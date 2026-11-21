#ifndef POKEBW2_APP_UI_ERROR_WINDOW_SCREEN_H
#define POKEBW2_APP_UI_ERROR_WINDOW_SCREEN_H

#include "types.h"
#include "gfl/overlay.h"

// An empty error window on the main screen, shown until the game is reset (error_window_screen.c). Nothing in the game
// loads the overlay
#define OVERLAY_ERROR_WINDOW_SCREEN OVERLAY_ID(184)

// Shows the window and never returns
void ErrorWindowScreen_Show(void);

#endif // POKEBW2_APP_ERROR_WINDOW_SCREEN_H
