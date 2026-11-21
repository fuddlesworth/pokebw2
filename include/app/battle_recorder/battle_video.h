#ifndef POKEBW2_APP_BATTLE_RECORDER_BATTLE_VIDEO_H
#define POKEBW2_APP_BATTLE_RECORDER_BATTLE_VIDEO_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Battle Video proc is in the main program
typedef struct {
    GameData *gameData;
    u32 mode;
} BattleVideoParam;

extern const GameProcFunctions BATTLE_VIDEO_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_VIDEO_H
