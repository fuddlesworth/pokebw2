#ifndef POKEBW2_WIFI_EVENT_WIFICLUB_H
#define POKEBW2_WIFI_EVENT_WIFICLUB_H

// The Wi-Fi Club, started by the NetConnectWiFiClub script command

#include "types.h"
#include "struct_decls.h"

typedef struct {
    Field *field;
    BOOL useTransitions;
} EventWifiClubArgs;

GameEvent *EventWifiClub_Create(GameSystem *gsys, Field *field, BOOL useTransitions);
GameEvent *EventWifiClub_CreateFromArgs(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_WIFICLUB_H
