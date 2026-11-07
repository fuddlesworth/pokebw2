#ifndef POKEBW2_APP_WIFI_LOGIN_H
#define POKEBW2_APP_WIFI_LOGIN_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"
// The login's messages, for the callbacks of WifiLoginParam: WifiLoginMessage_PrintFrom prints one and
// WifiLoginMessage_IsDone waits for it
#include "app/wifilogin_disp.h"
#include "app/wifilogin_message.h"

#define OVERLAY_WIFILOGIN OVERLAY_ID(190)

// The callback of the login, called each frame once the connection is made, with the login's message work and the
// data of WifiLoginParam. It returns a WIFILOGIN_CALLBACK_*
typedef int (*WifiLoginCallback)(void *msgWork, void *data);

enum {
    // The callback is done
    WIFILOGIN_CALLBACK_DONE,
    // Log out with an error
    WIFILOGIN_CALLBACK_ERROR,
    WIFILOGIN_CALLBACK_RUNNING,
};

// The game service IDs that the login passes to the network library
enum {
    WIFILOGIN_SERVICE_NONE = 0,
    WIFILOGIN_SERVICE_MYSTERY_GIFT = 9,
    WIFILOGIN_SERVICE_WIFI_CLUB = 10,
    WIFILOGIN_SERVICE_GTS = 30,
    WIFILOGIN_SERVICE_BATTLE_RECORDER = 33,
    WIFILOGIN_SERVICE_WIFI_BATTLE_MATCH = 36,
    WIFILOGIN_SERVICE_BATTLE_SUBWAY = 44,
    WIFILOGIN_SERVICE_GAME_SYNC = 49,
    WIFILOGIN_SERVICE_GAME_SYNC_ACCOUNT = 50,
    WIFILOGIN_SERVICE_GTS_NEGOTIATION = 51,
    WIFILOGIN_SERVICE_RANDOM_MATCHUP = 52,
    WIFILOGIN_SERVICE_TOURNAMENT_DOWNLOAD = 54,
    WIFILOGIN_SERVICE_MEMORY_LINK = 56,
};

// How login starts: normally, after an error, or without profile checks and saving
enum {
    WIFILOGIN_MODE_NORMAL = 0,
    WIFILOGIN_MODE_RETRY = 1,
    WIFILOGIN_MODE_NO_SAVE = 2,
};

// The result returned through WifiLoginParam.result
enum {
    WIFILOGIN_RESULT_OK = 0,
    WIFILOGIN_RESULT_FAILED = 1,
};

typedef struct {
    GameData *gameData;
    // The display's style, 0 for the Dream World account's and 1 for Game Sync's; with 0 the login prints a title on
    // the top screen and ignores the touch screen
    u32 displayStyle;
    // The style of the messages and menus, as displayStyle numbers them
    u32 messageStyle;
    // The game service ID, which the Wi-Fi library and the beacons identify the game with
    u32 serviceId;
    void *buffer;
    // A WIFILOGIN_MODE_*: retry after an error, or skip profile checks and saving without a save file
    u32 mode;
    // Set to leave the music alone
    u32 keepBgm;
    // A WIFILOGIN_RESULT_*
    u32 result;
    // Called each frame once logged in, with callbackData
    WifiLoginCallback callback;
    void *callbackData;
} WifiLoginParam;

typedef struct {
    GameData *gameData;
    // WIFILOGIN_DISP_STYLE_* and WIFILOGIN_MESSAGE_STYLE_*
    u32 displayStyle;
    u32 messageStyle;
    // Nonzero to leave the music alone
    u32 keepBgm;
    // The wipes' colors: bit 3 for white when fading in and bit 4 when fading out. 6 when 0
    u32 wipeFlags;
    u32 unk14;
    u32 unk18;
} WifiLogoutParam;

extern const GameProcFunctions WIFILOGIN_PROC_FUNCTIONS;
extern const GameProcFunctions WIFILOGOUT_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFI_LOGIN_H
