#ifndef POKEBW2_FIELD_WBT_H
#define POKEBW2_FIELD_WBT_H

#include "types.h"
#include "battle/regulation.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "save/extra_block_savedata.h"
#include "struct_decls.h"

// The Pokémon World Tournament: overlay 55's wbt_system.c, wbt_tool.c, wbt_setup.c and wbt_party.c, and the script
// plugins 6 (the entrance, with overlay 56) and 7 (the stadium, with overlay 57) that share overlay 55

// Tournament IDs, in PWT_TOURNAMENT_INFO order; the first Driftveil visit uses a separate ID.
enum {
    WBT_TOURNAMENT_NONE = 0,
    WBT_TOURNAMENT_CHAMPIONS = 1,
    WBT_TOURNAMENT_TYPE_EXPERT = 2,
    WBT_TOURNAMENT_DOWNLOADED = 3,
    WBT_TOURNAMENT_DRIFTVEIL = 4,
    WBT_TOURNAMENT_UNOVA_LEADERS = 5,
    WBT_TOURNAMENT_KANTO_LEADERS = 6,
    WBT_TOURNAMENT_JOHTO_LEADERS = 7,
    WBT_TOURNAMENT_HOENN_LEADERS = 8,
    WBT_TOURNAMENT_SINNOH_LEADERS = 9,
    WBT_TOURNAMENT_WORLD_LEADERS = 10,
    WBT_TOURNAMENT_DRIFTVEIL_FIRST = 11,
    WBT_TOURNAMENT_RENTAL = 12,
    WBT_TOURNAMENT_RENTAL_MASTER = 13,
    WBT_TOURNAMENT_MIX = 14,
    WBT_TOURNAMENT_MIX_MASTER = 15,
    WBT_TOURNAMENT_MAX = 16,
};

// Save records 0 to 16 are the Type Expert Tournament's Pokemon types.
enum {
    WBT_RECORD_CHAMPIONS = 17,
    WBT_RECORD_DRIFTVEIL = 18,
    WBT_RECORD_UNOVA_LEADERS = 19,
    WBT_RECORD_KANTO_LEADERS = 20,
    WBT_RECORD_JOHTO_LEADERS = 21,
    WBT_RECORD_HOENN_LEADERS = 22,
    WBT_RECORD_SINNOH_LEADERS = 23,
    WBT_RECORD_WORLD_LEADERS = 24,
    WBT_RECORD_RENTAL = 25,
    WBT_RECORD_RENTAL_MASTER = 26,
    WBT_RECORD_MIX = 27,
    WBT_RECORD_MIX_MASTER = 28,
    WBT_RECORD_MAX = 29,
};

// Rule groups: the unrestricted and restricted regulation tables, type rules, or a download.
enum {
    WBT_TOURNAMENT_KIND_UNRESTRICTED = 0,
    WBT_TOURNAMENT_KIND_RESTRICTED = 1,
    WBT_TOURNAMENT_KIND_TYPE_EXPERT = 2,
    WBT_TOURNAMENT_KIND_DOWNLOADED = 4,
};

// The four battle styles; MAX is the initial, unselected value.
enum {
    WBT_STYLE_SINGLE = 0,
    WBT_STYLE_DOUBLE = 1,
    WBT_STYLE_TRIPLE = 2,
    WBT_STYLE_ROTATION = 3,
    WBT_STYLE_MAX = 4,
};

// Script stages: preview before the first match, the three matches, then completion.
enum {
    WBT_ROUND_NONE = 0,
    WBT_ROUND_PREVIEW = 1,
    WBT_ROUND_FIRST = 2,
    WBT_ROUND_SEMIFINAL = 3,
    WBT_ROUND_FINAL = 4,
    WBT_ROUND_COMPLETE = 5,
};

// Rental file pools; mixed mode takes three from the second pool before the first.
enum {
    WBT_RENTAL_MODE_FIRST_POOL = 0,
    WBT_RENTAL_MODE_SECOND_POOL = 1,
    WBT_RENTAL_MODE_MIXED_POOLS = 2,
};

// A Trainer of the tournaments, from arc 247
typedef struct {
    u8 type;
    u8 gender;
    u16 objCode;
    u8 trainerClass;
    // Whether the Trainer takes part in tournaments 11, 4, 10, 12, 14, 13 and 15, from bit 0
    u8 inTournament11 : 1;
    u8 inTournament4 : 1;
    u8 inTournament10 : 1;
    u8 inTournament12 : 1;
    u8 inTournament14 : 1;
    u8 inTournament13 : 1;
    u8 inTournament15 : 1;
    u8 unk5_7 : 1;
    // The tournament that only has this Trainer, for tournaments 1 and 5 to 9
    u8 tournament;
    // The type the Type Expert Tournament picks the Trainer for, where TYPE_NULL is any
    u8 expertType;
    u8 rank : 3;
    u8 iv : 5;
    u8 padding9;
    // The music of the battle against the Trainer and of winning it
    u16 bgm;
    u16 winBgm;
    // Nonzero when the Trainer has a special Pokémon
    u16 specialPokemon;
} WbtTrainer;

typedef struct {
    u32 count;
    WbtTrainer trainers[];
} WbtTrainers;

// One of the eight Trainers of a tournament
typedef struct {
    u8 kind : 3;
    u8 gender : 1;
    u8 rank : 3; // The Trainer's rank, which decides a match between ranks that differ
    u8 hasSpecialPokemon : 1;
    u8 type;
    u8 trainerId;
    u8 iv : 6;
    u8 downloadedRound : 2;
    u16 objCode;
    u16 trainerClass : 12;
    // The battle's AI: bits 0 to 2 are AI flags 0 to 2, and bit 3 is AI flag 7
    u16 aiBits : 4;
    StrBuf *name;
    StrBuf *className;
} WbtEntrant;

// WbtEntrant's kind
enum {
    WBT_ENTRANT_KIND_TABLE,
    WBT_ENTRANT_KIND_DOWNLOADED,
    WBT_ENTRANT_KIND_DOWNLOADED_TABLE,
    WBT_ENTRANT_KIND_PLAYER,
};

// The result of a match of the bracket: whether the first Trainer won, and how
typedef struct {
    u8 firstWon;
    u8 score;
} WbtMatch;

struct WbtSystem {
    HeapID heapId;
    u8 padding2[2];
    // Clear the saved opponent slots even when the player wins.
    BOOL clearOpponentSlots;
    WbtTrainers *trainers;
    u32 tournament;
    // The battle style, which starts out as 4, out of range
    u32 style;
    u32 category;
    u32 round;
    // The Type Expert Tournament's type
    u8 type;
    u8 downloadSlot;
    // The map's season, rolled when the tournament starts
    u8 season;
    u8 padding1F;
    u64 seed;
    MATHRandContext32 rand;
    WbtEntrant entrants[8];
    WbtMatch matches[7];
    Regulation regulation;
    // The rule WbtSystem_GetExtraRule makes for the party screen
    RegulationExtraRule extraRule;
    u8 padding18E[2];
    // Whether the downloaded tournament of the slot downloadSlot is in the save and its checksum holds
    BOOL downloadValid;
    // The downloaded tournament, copied from the save: its entrant slots are the kind and rank of the seven Trainers it
    // picks, from the second
    WbtDownloadData download;
    u8 useBattleBox;
    u8 padding13A9[3];
    // The Battle Box's party
    PokeParty *battleBoxParty;
    u8 castSlots[6];
    // The Pokémon the party screen showed the summary of, and how it ended
    u16 listIndex;
    u16 listResult;
    u8 padding13BA[2];
    PokeParty *playerParty;
    PokeParty *opponentParty;
    // All rental candidates before entry, then the original selected team restored when picks reset.
    PokeParty *initialParty;
    u8 picks[4];
    WbtPartyList *partyList;
    u8 unk13D0[4];
    // The opponent's lines when it loses and when it wins, which the battle shows
    StrBuf *trainerLoseMsg;
    StrBuf *trainerWinMsg;
    // One for each round. Tournaments 1 and 10 award their total as Battle Points
    u8 roundRemaining[3];
    u8 padding13DF;
    // The script variable that plugin 6 passes with EventWbtPokeSelect_Create's event
    u16 *resultVar;
    u32 rentalMode;
};

// A tournament's type, from overlay 36's table of tournaments 1 to 15
typedef struct {
    u16 name;
    // Which of the save's 29 counts the tournament's wins go to. The Type Expert Tournament, tournament 2, counts
    // each of the 17 types in 0 to 16 instead
    u8 record;
    u8 category;
    u8 kind;
    // Which of the arcs of Trainers the tournament takes its Trainers from
    u8 trainerArc;
    // The Battle Points for winning, by battle style
    u8 battlePoints[4];
} WbtTournamentInfo;

extern const WbtTournamentInfo PWT_TOURNAMENT_INFO[15];

const WbtTournamentInfo *GetPWTTournamentInfo(s32 tournament);

void LoadPWTTournamentTypeText(HeapID heapId, u32 tournament, StrBuf *strbuf);

// The overworld setup of the tournament's Trainers, which overlay 22 shows
typedef struct {
    StrBuf *name;
    s16 resIndex;
    u8 face;
    u8 gender;
    u8 rank;
    u8 type;
    u8 isPlayer;
    u8 padB;
} WbtSetupEntrant;

typedef struct {
    u32 firstWon;
    u8 message;
    u8 pad5[3];
} WbtSetupMatch;

typedef struct {
    GameSystem *gsys;
    u32 mode;
    WbtSetupEntrant entrants[8];
    WbtSetupMatch firstRound[4];
    WbtSetupMatch secondRound[2];
    StrBuf *tournamentName;
    StrBuf *styleName;
    // Whether the list screen runs through EventFieldSubprocessTransition_Create
    u32 useTransition;
} WbtSetup;

// wbt_system.c
WbtSystem *WbtSystem_Create(HeapID heapId, BOOL clearOpponentSlots);
void WbtSystem_Free(WbtSystem *sys);
void WbtSystem_SetTournament(WbtSystem *sys, u32 tournament);
u32 WbtSystem_GetTournament(WbtSystem *sys);
void WbtSystem_SetStyle(WbtSystem *sys, u32 style);
u32 WbtSystem_GetStyle(WbtSystem *sys);
void WbtSystem_SetType(WbtSystem *sys, u8 type);
u8 WbtSystem_GetType(WbtSystem *sys);
void WbtSystem_SetCategory(WbtSystem *sys, u32 value);
u32 WbtSystem_GetCategory(WbtSystem *sys);
void WbtSystem_SetRound(WbtSystem *sys, u32 round);
u32 WbtSystem_GetRound(WbtSystem *sys);
u64 WbtSystem_GetSeed(WbtSystem *sys);
void WbtSystem_SetDownloadSlot(WbtSystem *sys, u8 value);
u8 WbtSystem_GetDownloadSlot(WbtSystem *sys);
void WbtSystem_SetResultVar(WbtSystem *sys, u16 *var);
u16 *WbtSystem_GetResultVar(WbtSystem *sys);
void WbtSystem_SetUseBattleBox(WbtSystem *sys, u8 value);
void WbtSystem_SetPartyList(WbtSystem *sys, WbtPartyList *list);
WbtPartyList *WbtSystem_GetPartyList(WbtSystem *sys);
void WbtSystem_SetRentalMode(WbtSystem *sys, u32 value);
WbtEntrant *WbtSystem_GetEntrant(WbtSystem *sys, u32 index);
WbtEntrant *WbtSystem_GetPlayerEntrant(WbtSystem *sys);
void WbtSystem_LoadTournamentName(WbtSystem *sys, u32 tournament, StrBuf *strbuf);
PokeParty *WbtSystem_GetPlayerParty(WbtSystem *sys);
PokeParty *WbtSystem_GetOpponentParty(WbtSystem *sys);
// The number of Pokémon each Trainer enters with, by the battle style
u32 WbtSystem_GetEntryCount(WbtSystem *sys);
u32 WbtEntrant_GetRank(WbtEntrant *entrant);
u8 WbtEntrant_GetIv(WbtEntrant *entrant);
u32 WbtEntrant_GetDownloadedRound(WbtEntrant *entrant);
u8 WbtEntrant_GetTrainerId(WbtEntrant *entrant);
u16 WbtEntrant_GetTrainerClass(WbtEntrant *entrant);
u32 WbtEntrant_GetKind(WbtEntrant *entrant);
void WbtEntrant_GetName(const WbtEntrant *entrant, StrBuf *dest);
u32 WbtEntrant_HasSpecialPokemon(WbtEntrant *entrant);
void WbtSystem_SetPickedParty(WbtSystem *sys, GameData *gameData);
u8 WbtSystem_GetRegulationId(WbtSystem *sys);
Regulation *WbtSystem_GetRegulation(WbtSystem *sys);
RegulationExtraRule *WbtSystem_GetExtraRule(WbtSystem *sys);
u32 WbtSystem_GetOpponentSeat(WbtSystem *sys);
void WbtSystem_PlayFirstRounds(WbtSystem *sys, MATHRandContext32 *rand);

// wbt_tool.c
// Load a Trainer's name and Trainer class from message files 409 and 410
void WbtTool_LoadTrainerName(HeapID heapId, u32 message, StrBuf *strbuf);
void WbtTool_LoadTrainerClassName(HeapID heapId, u32 message, StrBuf *strbuf);
WbtTrainers *WbtTrainers_Load(HeapID heapId);
void WbtTrainers_Free(WbtTrainers *trainers);
void WbtTrainers_Get(WbtTrainers *trainers, u32 index, WbtTrainer *dest);
BOOL WbtTrainers_IsValid(WbtTrainers *trainers, u8 index);
u16 WbtTrainers_GetObjCode(WbtTrainers *trainers, u8 index);
int WbtTrainers_GetRank(WbtTrainers *trainers, u8 index);
// The battle styles: single, double, triple and rotation
void WbtStyle_LoadName(int style, StrBuf *strbuf);
u8 WbtStyle_GetPokemonCount(int style);
u32 WbtStyle_GetBattleMode(int style);
int WbtStyle_Clamp(int style);
// The type that most of the party's Pokémon have, or a random one of those that tie
u8 WbtTool_GetPartyMainType(PokeParty *party, u64 seed);
// Whether the tournament is open, by the wins that the save counts
BOOL WbtTournament_IsOpen(GameData *gameData, u32 tournament);
// Whether winning the tournament would open another
BOOL WbtTournament_WouldOpen(GameData *gameData, u32 won, u32 tournament);
// The save's count of the tournament, where type is the Type Expert Tournament's
u32 WbtTournament_GetRecord(u32 tournament, u32 type);
// The tournament whose wins a count of the save counts
u32 WbtTournament_FromRecord(int record);
u32 WbtTournament_GetKind(u32 tournament);
BOOL WbtTournament_IsInCategory(u32 tournament, u32 value);
u16 WbtTournament_GetTrainerArc(u32 tournament);
// The Battle Points for winning the tournament in the battle style
u32 WbtTournament_GetBattlePoints(u32 tournament, int style);
u8 WbtTool_GetDownloadedTrainerClass(u32 index);

// What wbt_setup.c fills in for overlay 326's screens
typedef struct {
    u32 unk0;
    // TRUE when the record screen closes with X, FALSE when it closes with Return or B.
    u16 *closedWithXVar;
    WbtSave *save;
    PlayerInfo *playerInfo;
    // Whether the tournament of each of the save's counts is open
    u8 open[29];
} WbtOv326Param;

typedef struct {
    GameSystem *gsys;
    // Address of the twelve received tournament records; its original integer type is retained.
    u32 downloadBuffer;
    u32 mode;
    // The script variables that get whether the player answered yes, and the slot picked from 1, or 0
    u16 *yesVar;
    u16 *slotVar;
} WbtOv326Param2;

// wbt_setup.c
WbtSetup *WbtSetup_Create(HeapID heapId, GameSystem *gsys);
void WbtSetup_Free(WbtSetup *setup);
void WbtSetup_Load(GameSystem *gsys, WbtSystem *sys, WbtSetup *setup);
WbtOv326Param *WbtSetup_CreateWinRecordParam(HeapID heapId, GameSystem *gsys, u16 *var);
void WbtSetup_FreeWinRecordParam(WbtOv326Param *param);
WbtOv326Param2 *WbtSetup_CreateDownloadParam(HeapID heapId, GameSystem *gsys, u32 mode, u16 *yesVar, u16 *slotVar);
void WbtSetup_FreeDownloadParam(WbtOv326Param2 *param);

// wbt_party.c
// What a party picked from the Battle Subway's Pokémon may not repeat: the species, unless allowSameSpecies is set, the
// held items and the Pokémon files, each a list of 12
typedef struct {
    u32 allowSameSpecies;
    u16 speciesCount;
    u16 itemCount;
    u16 fileCount;
    u16 *species;
    u16 *items;
    // 0xffff for none
    u16 *files;
} WbtPartyFilter;

WbtPartyFilter *WbtPartyFilter_Create(HeapID heapId, u32 tournament);
void WbtPartyFilter_Free(WbtPartyFilter *filter);
void WbtPartyFilter_AddItem(WbtPartyFilter *filter, u16 item);
void WbtPartyFilter_AddSpecies(WbtPartyFilter *filter, u16 species);
void WbtPartyFilter_AddFile(WbtPartyFilter *filter, s16 file);
// Picks count of the files at random, as the filter allows, and adds them to the filter
void WbtParty_PickFiles(u16 *dest, WbtPartyFilter *filter, HeapID heapId, u16 fileCount, const u16 *files, u32 arcId,
                        u8 count, MATHRandContext32 *rand);
void WbtParty_MakeRentalParty(WbtSystem *sys, u32 rentalMode, PlayerInfo *playerInfo, u16 placeName);
void WbtParty_MakeOpponentParty(WbtSystem *sys);

#endif // POKEBW2_FIELD_WBT_H
