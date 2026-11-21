#ifndef POKEBW2_BATTLE_BATTLE_PROC_H
#define POKEBW2_BATTLE_BATTLE_PROC_H

#include "types.h"
#include "app/battle_recorder/btl_rec_sel.h"
#include "battle/select/vs_multi_list.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 10 starts a battle, overlay 167 is the battle system and overlay 213 selects the party for a Wi-Fi battle
#define OVERLAY_BATTLE OVERLAY_ID(10)
#define OVERLAY_BATTLE_MAIN OVERLAY_ID(167)
#define OVERLAY_BATTLE_SELECT OVERLAY_ID(213)

typedef struct {
    PokeParty *party;
    PlayerInfo *info;
    u8 padding08;
    // BATTLE_REC_VERSION, which the other player's game must share to record the battle
    u8 battleRecordVersion;
    // Selected party slots; reused by overlay 305 for its COMM_BTL_DEMO_POKE_* health states
    u8 order[6];
} BattlePlayer;

// The players of a battle, and the result that the battle proc leaves
typedef struct {
    BattlePlayer players[4];
    u32 result;
    // What overlay 305's screen before and after the battle shows, COMM_BTL_DEMO_TYPE_*
    u32 demoType;
    u32 rule;
    // The battle video's counter, which overlay 306's screen passes on (BattleRecHeader.counter)
    u32 counter;
} BattlePlayers;

// What overlay 10's battle proc runs: the party select, the battle and overlay 306's screen after it
typedef struct {
    GameData *gameData;
    BtlSetup *setup;
    BattlePlayers *players;
    u32 rule;
    // The battle video's counter, passed to overlay 306's screen (BattleRecHeader.counter)
    u32 counter;
    // Set, a network error is marked shown when the proc ends
    u32 markNetErrShown;
    BtlRecSelParam ov306Param;
} BattleParam;

// The battle proc's work
typedef struct {
    GameProcManager *manager;
    // What overlay 171 gets
    VsMultiListParam ov171Param;
    BOOL ov167Loaded;
    BOOL commandsSet;
    BOOL vsPlayerLoaded;
} BattleProcWork;

extern const GameProcFunctions EVENT_MAKE_BATTLE_PROC_FUNCTIONS;
// The battle system's proc
extern const GameProcFunctions BATTLE_PROC_FUNCTIONS;
extern const GameProcFunctions CommBtlDemoProc;

BOOL BattleProc_Init(GameProc *proc, u32 *state, void *param, void *work);
BOOL BattleProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
BOOL BattleProc_Main(GameProc *proc, u32 *state, void *param, void *work);

#endif // POKEBW2_BATTLE_BATTLE_PROC_H
