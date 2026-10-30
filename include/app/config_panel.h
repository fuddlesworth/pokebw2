#ifndef POKEBW2_APP_CONFIG_PANEL_H
#define POKEBW2_APP_CONFIG_PANEL_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// config_panel.c: The options screen of the menu. The ROM has no name for the file; "config_panel.c" is a guess

// What the screen is started with. result is set when the screen was left with the touch bar's close icon, which
// closes the menu as well
typedef struct {
    BOOL result;
    GameSystem *gsys;
} ConfigPanelParam;

#ifdef POKEBW2_PC
typedef struct {
    Font *font;
    Config *config;
} ConfigPanelNativeState;
BOOL ConfigPanel_GetNativeState(ConfigPanelNativeState *state);
void ConfigPanel_CloseNative(void);
#endif

extern const GameProcFunctions CONFIG_PANEL_PROC_FUNCTIONS;

#endif // POKEBW2_APP_CONFIG_PANEL_H
