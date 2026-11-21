#ifndef POKEBW2_DEMO_TITLE_GAME_START_H
#define POKEBW2_DEMO_TITLE_GAME_START_H

#include "types.h"
#include "gfl/proc.h"

// Starting the game from the start menu and the screens around it (ov162): a new game, which runs the intro and the
// name entries, or a continue, which loads the save. The two continues start the game system with the C-Gear on or off

void GameStart_NewGame(void);
void GameStart_ContinueCGearOff(void);
void GameStart_ContinueCGearOn(void);

extern const GameProcFunctions NEW_GAME_PROC_FUNCTIONS;
extern const GameProcFunctions CONTINUE_PROC_FUNCTIONS;
extern const GameProcFunctions DEBUG_GAME_START_PROC_FUNCTIONS;

#endif // POKEBW2_APP_GAME_START_H
