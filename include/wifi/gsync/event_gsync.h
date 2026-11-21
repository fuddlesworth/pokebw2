#ifndef POKEBW2_WIFI_GSYNC_EVENT_GSYNC_H
#define POKEBW2_WIFI_GSYNC_EVENT_GSYNC_H

// Game Sync, started by the C-Gear

#include "types.h"
#include "struct_decls.h"

GameEvent *EventGameSync_Create(GameSystem *gsys);
GameEvent *EventGameSync_CreateFromArgs(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_GSYNC_H
