#ifndef POKEBW2_APP_WIFICLUB_H
#define POKEBW2_APP_WIFICLUB_H

#include "types.h"
#include "app/wifi_status.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Wi-Fi Club runs in overlay 203, and its proc table is in overlay 173, which also has to be loaded
#define OVERLAY_WIFICLUB OVERLAY_ID(203)
#define OVERLAY_WIFICLUB_MAIN OVERLAY_ID(173)

// How the match board's connection was made; retained when returning from an application.
enum {
    WIFICLUB_CONNECTION_NONE,
    WIFICLUB_CONNECTION_RECRUIT,
    WIFICLUB_CONNECTION_JOIN,
    WIFICLUB_CONNECTION_REQUEST,
    WIFICLUB_CONNECTION_RESPONSE,
};

// The match board returns these to its event when it leaves or needs to log in again.
enum {
    WIFICLUB_MODE_LEAVE,
    WIFICLUB_MODE_LOGIN,
};

enum {
    WIFICLUB_BATTLE_SINGLE = 7,
    WIFICLUB_BATTLE_SINGLE_FLAT,
    WIFICLUB_BATTLE_DOUBLE,
    WIFICLUB_BATTLE_DOUBLE_FLAT,
    WIFICLUB_BATTLE_TRIPLE,
    WIFICLUB_BATTLE_TRIPLE_FLAT,
    WIFICLUB_BATTLE_ROTATION,
    WIFICLUB_BATTLE_ROTATION_FLAT,
};

typedef struct {
    WIFI_STATUS *buffer;
    GameData *gameData;
    SaveControl *save;
    // What the player chose in the Wi-Fi Club
    u32 mode;
    // The connected friend's zero-based index from DWCRap_GetFriendIndex
    u32 friendListIndex;
    PokeParty *parties[2];
    Regulation *regulation;
    // The Wonder Launcher setting remembered by the battle rules menu
    u8 launcherChoice;
    // The battle the players chose: its mode (single, double, triple, rotation) and its rule (no restrictions, flat)
    u8 battleMode;
    u8 battleRule;
    u8 unk23;
    // Each friend's call counter (WIFI_STATUS_GetCallCounter) when the player last answered a call of that friend
    u8 callCounter[0x20];
    // Whether each machine of the connection uses voice chat, by net ID
    u8 vchat[2];
    // The player's persistent voice chat preference
    u8 voiceChatEnabled;
    // The friend's index in the friend list, plus 1
    u8 friendIndex;
    u8 launcher;
    // WIFICLUB_CONNECTION_*
    u8 connectionResult;
    u8 unk4A[2];
} WifiClubData;

extern const GameProcFunctions WIFICLUB_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFICLUB_H
