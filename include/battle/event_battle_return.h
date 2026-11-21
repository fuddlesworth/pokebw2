#ifndef POKEBW2_BATTLE_EVENT_BATTLE_RETURN_H
#define POKEBW2_BATTLE_EVENT_BATTLE_RETURN_H

// Overlay 166's event_battle_return.c, after its embedded name: the process that overlay 12's event_battle.c runs after
// a battle. It copies the battle's party back, spreads Pokérus, runs Pickup, Honey Gather and Natural Cure, changes
// Burmy's form, settles the prize money, registers a caught Pokémon (overlays 297 and 280, as event_pdc_return.c does)
// and plays the evolutions of the Pokémon that leveled up (overlay 284)

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

typedef struct {
    GameData *gameData;
    BtlSetup *setup;
} EventBattleReturnParam;

extern const GameProcFunctions EVENT_BATTLE_RETURN_PROC_FUNCTIONS;

#endif // POKEBW2_APP_EVENT_BATTLE_RETURN_H
