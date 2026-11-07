#ifndef POKEBW2_APP_WIFIBATTLEMATCH_SUBPROC_H
#define POKEBW2_APP_WIFIBATTLEMATCH_SUBPROC_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 263 (wifibattlematch_subproc.c, a guessed name; the overlay has no string): two procs that overlay 290's
// Wi-Fi and live battle competitions (wifibattlematch_sys.c) run. One picks the team for a battle with overlay 165's
// party list and overlay 207's summary screen, the other sends the team picked to the other player and receives
// theirs, through Wi-Fi (overlay 260) or infrared (overlay 261). Swan has none of these; the names are ours

// Team-selection results, from the actual Wi-Fi and infrared error paths.
enum {
    WIFIBATTLEMATCH_LIST_RESULT_PICKED = 0,
    WIFIBATTLEMATCH_LIST_RESULT_WIFI_ERROR = 1,
    WIFIBATTLEMATCH_LIST_RESULT_IRC_ERROR = 2,
    WIFIBATTLEMATCH_LIST_RESULT_WIFI_TIMEOUT = 3,
};

// What overlay 290 fills in for the team selection
typedef struct {
    Regulation *regulation;
    PokeParty *party;
    u16 *otherName;
    u8 otherGender;
    PokeParty *otherParty;
    GameData *gameData;
    u32 unk18;
    // Gets the Pokémon picked
    PokeParty *selected;
    // 0 once the team is picked, 1 or 3 if the Wi-Fi connection was lost, 2 if the network failed
    u32 result;
} WifiBattleMatchListParam;

// How the parties are exchanged
enum {
    WIFIBATTLEMATCH_EXCHANGE_WIFI,
    WIFIBATTLEMATCH_EXCHANGE_IRC,
};

// Team-exchange results. Unlike team selection, both Wi-Fi disconnection and timeout produce 3.
enum {
    WIFIBATTLEMATCH_EXCHANGE_RESULT_DONE = 0,
    WIFIBATTLEMATCH_EXCHANGE_RESULT_WIFI_ERROR = 1,
    WIFIBATTLEMATCH_EXCHANGE_RESULT_IRC_ERROR = 2,
    WIFIBATTLEMATCH_EXCHANGE_RESULT_WIFI_DISCONNECTED_OR_TIMEOUT = 3,
};

// What overlay 290 fills in for the exchange of the teams
typedef struct {
    // Points to overlay 290's parameters, which start with the GameData
    GameData **gameData;
    // A WIFIBATTLEMATCH_EXCHANGE_*
    u32 mode;
    // 0 once the teams are exchanged, 1 or 3 if the Wi-Fi connection was lost, 2 if the network failed
    u32 result;
    // The team picked, to send
    PokeParty *party;
    // Gets the other player's team
    PokeParty *otherParty;
    WifiBattleMatchNetData *netData;
    u32 *recordSaveIndex;
} WifiBattleMatchExchangeParam;

extern const GameProcFunctions WIFIBATTLEMATCH_LIST_PROC_FUNCTIONS;
extern const GameProcFunctions WIFIBATTLEMATCH_EXCHANGE_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFIBATTLEMATCH_SUBPROC_H
