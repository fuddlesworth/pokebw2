#ifndef POKEBW2_TOURNAMENT_EVENT_WBT_H
#define POKEBW2_TOURNAMENT_EVENT_WBT_H

#include "types.h"
#include "struct_decls.h"

// The Pokémon World Tournament's events, overlay 22 (event_wbt.c, a descriptive name), which its script plugins start
// through GameEvent_CreateOverlayDelegate

// Picks the Pokémon to enter the tournament with on the party screen, for the WbtSystem
GameEvent *EventWbtPokeSelect_Create(GameSystem *gsys, void *args);
// Shows the tournament's Trainers from a WbtSetup on overlay 320's screen, then frees it
GameEvent *EventWbtList_Create(GameSystem *gsys, void *args);
// The battle of the WbtSystem's current round
GameEvent *EventWbtBattle_Create(GameSystem *gsys, void *args);
// Runs overlay 326's records screen with a WbtOv326Param, then frees it
GameEvent *EventWbtWinRecord_Create(GameSystem *gsys, void *args);
// Runs overlay 326's downloaded tournaments with a WbtOv326Param2, then frees it
GameEvent *EventWbtDownload_Create(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_WBT_H
