#ifndef POKEBW2_APP_WIFIBATTLEMATCH_BATTLE_H
#define POKEBW2_APP_WIFIBATTLEMATCH_BATTLE_H

#include "types.h"
#include "battle/battle_proc.h"
#include "battle/btl_setup.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// wifibattlematch_battle.c, in overlay 290 (a guessed name; the file has no string): the proc that
// wifibattlematch_sys.c's battle step runs

// The players of a battle, with what the ranking needs
typedef struct {
    BattlePlayers base;
    u32 competition;
    GameRecords *records;
} WifiBattleMatchPlayers;

// How the battle wrapper ended, separate from BtlSetup.result. ERROR also covers the BTL_RESULT_6 wait timeout.
enum {
    WIFIBATTLEMATCH_BATTLE_RESULT_FINISHED = 0,
    WIFIBATTLEMATCH_BATTLE_RESULT_IRC_ERROR = 1,
    WIFIBATTLEMATCH_BATTLE_RESULT_ERROR = 2,
    WIFIBATTLEMATCH_BATTLE_RESULT_WIFI_TIMEOUT = 3,
};

// What the battle step gets
typedef struct {
    GameData *gameData;
    BtlSetup *setup;
    WifiBattleMatchPlayers *players;
    u32 result;
} WifiBattleMatchBattleParam;

extern const GameProcFunctions WIFIBATTLEMATCH_BATTLE_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFIBATTLEMATCH_BATTLE_H
