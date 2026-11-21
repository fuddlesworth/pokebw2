#ifndef POKEBW2_FIELD_EVENT_EVENT_CGEAR_SHUTDOWN_H
#define POKEBW2_FIELD_EVENT_EVENT_CGEAR_SHUTDOWN_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "system/game_event.h"

struct CGearShutdownData {
    Field *field;
    FieldSubscreen *subscreen;
};

GameEvent *EventCGearShutdown_Create(GameSystem *gsys);
GameEventReturnCode EventCGearShutdown_Callback(GameEvent *event, u32 *state, void *data);

#endif // POKEBW2_FIELD_EVENT_CGEAR_SHUTDOWN_H
