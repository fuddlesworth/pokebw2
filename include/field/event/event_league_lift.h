#ifndef POKEBW2_FIELD_EVENT_EVENT_LEAGUE_LIFT_H
#define POKEBW2_FIELD_EVENT_EVENT_LEAGUE_LIFT_H

// Overlay 12's event_league_lift.c (a descriptive name): the Pokémon League's lift down to the Champion's room.
// EventLeagueLiftWarp_Create is swan's name (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

GameEvent *EventLeagueLiftWarp_Create(GameSystem *gsys, Field *field);

#endif // POKEBW2_FIELD_EVENT_LEAGUE_LIFT_H
