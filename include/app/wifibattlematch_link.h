#ifndef POKEBW2_APP_WIFIBATTLEMATCH_LINK_H
#define POKEBW2_APP_WIFIBATTLEMATCH_LINK_H

#include "types.h"
#include "app/wifibattlematch.h"
#include "gfl/heap.h"
#include "gfl/proc.h"

enum {
    WBM_LINK_RESULT_INITIAL = 0,
    WBM_LINK_RESULT_BATTLE_FINISHED = 1,
    WBM_LINK_RESULT_CANCELLED = 2,
    WBM_LINK_RESULT_RESULT_SHOWN = 3,
};

enum {
    WBM_MENU_NO_SELECTION = -1,
    WBM_MENU_YES = 0,
    WBM_MENU_NO = 1,
};

enum {
    WBM_RATING_INDEX_DOUBLE = 0,
    WBM_RATING_INDEX_ROTATION = 1,
    WBM_RATING_INDEX_SHOOTER = 2,
    WBM_RATING_INDEX_SINGLE = 3,
    WBM_RATING_INDEX_TRIPLE = 4,
};

enum {
    WBM_REGISTRATION_NONE = 0,
    WBM_REGISTRATION_TEAM_REQUIRED = 1,
    WBM_REGISTRATION_COMPLETE = 2,
    WBM_REGISTRATION_PERIOD_ENDED = 3,
    WBM_REGISTRATION_WITHDRAWN = 4,
};

enum {
    WBM_PERIOD_RESULT_ACTIVE = 0,
    WBM_PERIOD_RESULT_EXPIRED = 1,
    WBM_PERIOD_RESULT_NOT_STARTED = 2,
    WBM_PERIOD_RESULT_SERVER_ERROR = 3,
};

enum {
    WBM_UNREGISTER_RESULT_DONE = 0,
    WBM_UNREGISTER_RESULT_NOT_CHANGED = 1,
    WBM_UNREGISTER_RESULT_NOT_REGISTERED = 2,
    WBM_UNREGISTER_RESULT_SERVER_ERROR = 3,
};

enum {
    WBM_TEAM_CHECK_RESULT_VALID = 0,
    WBM_TEAM_CHECK_RESULT_INVALID = 1,
    WBM_TEAM_CHECK_RESULT_REQUEST_FAILED = 2,
    WBM_TEAM_CHECK_RESULT_SERVER_ERROR = 3,
};

// The modes of the random matchups (WifiBattleMatchLinkParam.mode); the 2 is overlay 290's own
enum {
    LINK_MODE_RATING,
    LINK_MODE_FREE,
    LINK_MODE_QUIT = 3,
};

// Overlay 260's wifibattlematch_link.c, wifibattlematch_link_random.c and wifibattlematch_link_competition.c (all
// descriptive names: the ROM names only the next file, wifibattlematch_net.c): the link of the Wi-Fi battle
// competitions, which overlay 290 chains. wifibattlematch_link.c is the proc that overlay 290 runs, which runs one of
// the other two: the random matchups of the field, or the Wi-Fi competition of the title screen's menu. The files are
// three because each has a table of procs, which wifibattlematch_link.c reaches by its symbol, and a string of its
// own for its assert

// The heap of the link, and the one that the exchange of the match results gives the Wi-Fi library
#define HEAPID_WIFIBATTLEMATCH_LINK 0x57
#define HEAPID_WIFIBATTLEMATCH_LINK_DWC 0x86

// What the link leaves for overlay 290 to do next (WifiBattleMatchLinkParam.action); overlay 290 also knows a 4,
// which logs out like the 0
enum {
    WIFIBATTLEMATCH_LINK_ACTION_LOGOUT,
    WIFIBATTLEMATCH_LINK_ACTION_TEAM_LIST,
    WIFIBATTLEMATCH_LINK_ACTION_RESULT,
    WIFIBATTLEMATCH_LINK_ACTION_RELOGIN,
};

// What the link writes to the server after a battle: overlay 290's report, and the word that
// WifiBattleMatchLinkParam.recordSaveIndex points to
typedef struct {
    WifiBattleMatchReport report;
    u32 recordSaveIndex;
} WifiBattleMatchLinkReportBuf;

// The account response: its profile ID, registration flags and competition registration state
typedef struct {
    s32 profileId;
    // Forwarded unchanged in the registration request; 1 clears registered.
    u8 unk4;
    u8 registrationState;
    u8 registered;
    u8 unk7;
} WifiBattleMatchLinkAccount;

// What the server says of the ratings, as 5 ratings, 5 pairs of results in the order that the server has the battle
// types in, and the data of the Wi-Fi competition's ranking
typedef struct {
    u32 rating[5];
    // NetStats.unk14, cheatsCounter and unk1C; the link does not read these words.
    u32 unk14[3];
    u32 disconnectsCounter;
    struct {
        u32 losses;
        u32 wins;
    } results[5];
    u32 initialProfileId;
    u32 currentProfileId;
    u32 recordSaveIndex;
    u32 recordId;
} WifiBattleMatchLinkRatings;

typedef struct {
    u32 cheatsCounter;
    u32 disconnectsCounter;
    u32 wins;
    u32 losses;
    u32 rating;
    u32 recordSaveIndex;
    u8 partyData[0x534];
} WifiBattleMatchLinkRanking;

// What overlay 290 gives the link
typedef struct {
    u32 mode;
    // What the link leaves for overlay 290 to do next, a WIFIBATTLEMATCH_LINK_ACTION_* or 4
    u32 action;
    WifiBattleMatchParam *param;
    u32 result;
    RndMatchData *record;
    WifiBattleMatchPlayer *player;
    WifiBattleMatchPlayer *otherPlayer;
    void *loginBuffer;
    u32 *recordRequestCooldown;
    WifiBattleMatchReport *report;
    u32 *recordSaveIndex;
    WifiBattleMatchBattleResult *battleResult;
    WifiBattleMatchLinkAccount *account;
    WifiBattleMatchLinkRanking *ranking;
    WifiBattleMatchLinkRatings *ratings;
    WifiBattleMatchNetData *netData;
    BOOL *opponentNameRejected;
    u32 unk44;
} WifiBattleMatchLinkParam;

// The proc of the link, which overlay 290 chains
extern const GameProcFunctions WIFIBATTLEMATCH_LINK_PROC_FUNCTIONS;
// The procs that it runs: the random matchups, and the Wi-Fi competition
extern const GameProcFunctions WIFIBATTLEMATCH_LINK_RANDOM_PROC_FUNCTIONS;
extern const GameProcFunctions WIFIBATTLEMATCH_LINK_COMPETITION_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFIBATTLEMATCH_LINK_H
