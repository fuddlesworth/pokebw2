#ifndef POKEBW2_COMMUNICATION_GTS_EVENT_GTSNEGO_H
#define POKEBW2_COMMUNICATION_GTS_EVENT_GTSNEGO_H

// GTS Negotiation, started by the NetConnectGTSNegotiation script command

#include "types.h"
#include "struct_decls.h"

GameEvent *EventGtsNego_Create(GameSystem *gsys, Field *field);
// Takes the field instead of a pointer to arguments
GameEvent *EventGtsNego_CreateFromArgs(GameSystem *gsys, void *field);

#endif // POKEBW2_FIELD_EVENT_GTSNEGO_H
