#ifndef POKEBW2_TOURNAMENT_EVENT_WIFI_BSUBWAY_H
#define POKEBW2_TOURNAMENT_EVENT_WIFI_BSUBWAY_H

// The Wi-Fi Battle Subway, started by a script command in overlay 50

#include "types.h"
#include "struct_decls.h"

typedef struct {
    u32 mode;
    u16 *result;
} EventWifiBSubwayArgs;

GameEvent *EventWifiBSubway_Create(GameSystem *gsys, u32 mode, u16 *result);
GameEvent *EventWifiBSubway_CreateFromArgs(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_WIFI_BSUBWAY_H
