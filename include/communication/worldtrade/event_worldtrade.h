#ifndef POKEBW2_COMMUNICATION_WORLDTRADE_EVENT_WORLDTRADE_H
#define POKEBW2_COMMUNICATION_WORLDTRADE_EVENT_WORLDTRADE_H

// The Global Trade Station, started by the NetConnectGTS script command

#include "types.h"
#include "struct_decls.h"

typedef struct {
    Field *field;
    u32 unused;
} EventWorldTradeArgs;

GameEvent *EventWorldTrade_Create(GameSystem *gsys, Field *field, u32 unused);
GameEvent *EventWorldTrade_CreateFromArgs(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_WORLDTRADE_H
