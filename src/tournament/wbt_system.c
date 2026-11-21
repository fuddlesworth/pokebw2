#include "types.h"
#include "constants/moves.h"
#include "constants/types.h"
#include "field/wbt.h"
#include "field/wbt_makematch.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "pml/poke_party.h"

enum {
    WBT_TYPE_DAMAGE_IMMUNE = 0,
    WBT_TYPE_DAMAGE_RESISTED = 2,
    WBT_TYPE_DAMAGE_NEUTRAL = 4,
    WBT_TYPE_DAMAGE_SUPER_EFFECTIVE = 8,
};

enum {
    WBT_TYPE_EFFECT_IMMUNE = 0,
    WBT_TYPE_EFFECT_RESISTED = 1,
    WBT_TYPE_EFFECT_NEUTRAL = 2,
    WBT_TYPE_EFFECT_SUPER_EFFECTIVE = 3,
};

enum {
    WBT_MATCH_TOP_RANKS = 0,
    WBT_MATCH_LARGE_RANK_WIN = 1,
    WBT_MATCH_SMALL_RANK_WIN = 2,
    WBT_MATCH_EQUAL_RANKS = 3,
    WBT_MATCH_UPSET = 4,
};

static void WbtEntrant_Init(WbtEntrant *entrant, HeapID heapId);
static void WbtEntrant_Free(WbtEntrant *entrant);
static void WbtMatch_Play(WbtMatch *match, MATHRandContext32 *rand, WbtEntrant *first, WbtEntrant *second);
static u8 WbtMatch_GetTypeEffectiveness(u32 attackType, u32 defenseType);

WbtSystem *WbtSystem_Create(HeapID heapId, BOOL clearOpponentSlots) {
    int i;
    WbtSystem *sys = GFL_HeapAllocate(heapId, sizeof(WbtSystem), TRUE, "wbt_system.c", 51);

    sys->heapId = heapId;
    sys->clearOpponentSlots = clearOpponentSlots;
    sys->tournament = WBT_TOURNAMENT_NONE;
    sys->style = WBT_STYLE_MAX;
    sys->round = WBT_ROUND_NONE;
    sys->seed = ((u64)GFL_RandomMT() << 32) + GFL_RandomMT();
    MATH_InitRand32(&sys->rand, sys->seed);
    sys->season = GFL_RandomLC(4);
    for (i = 0; i < 8; i++) {
        WbtEntrant_Init(&sys->entrants[i], heapId);
    }
    sys->playerParty = PokeParty_Create(sys->heapId);
    sys->opponentParty = PokeParty_Create(sys->heapId);
    sys->initialParty = PokeParty_Create(sys->heapId);
    sys->battleBoxParty = NULL;
    sys->trainers = WbtTrainers_Load(sys->heapId);
    return sys;
}

void WbtSystem_Free(WbtSystem *sys) {
    int i;

    WbtTrainers_Free(sys->trainers);
    GFL_HeapFree(sys->initialParty);
    GFL_HeapFree(sys->playerParty);
    GFL_HeapFree(sys->opponentParty);
    if (sys->battleBoxParty != NULL) {
        GFL_HeapFree(sys->battleBoxParty);
    }
    for (i = 0; i < 8; i++) {
        WbtEntrant_Free(&sys->entrants[i]);
    }
    GFL_HeapFree(sys);
}

void WbtSystem_SetTournament(WbtSystem *sys, u32 tournament) {
    sys->tournament = tournament;
}

u32 WbtSystem_GetTournament(WbtSystem *sys) {
    return sys->tournament;
}

void WbtSystem_SetStyle(WbtSystem *sys, u32 style) {
    sys->style = style;
}

u32 WbtSystem_GetStyle(WbtSystem *sys) {
    return sys->style;
}

void WbtSystem_SetType(WbtSystem *sys, u8 type) {
    sys->type = type;
}

u8 WbtSystem_GetType(WbtSystem *sys) {
    return sys->type;
}

void WbtSystem_SetCategory(WbtSystem *sys, u32 value) {
    sys->category = value;
}

u32 WbtSystem_GetCategory(WbtSystem *sys) {
    return sys->category;
}

void WbtSystem_SetRound(WbtSystem *sys, u32 round) {
    sys->round = round;
}

u32 WbtSystem_GetRound(WbtSystem *sys) {
    return sys->round;
}

u64 WbtSystem_GetSeed(WbtSystem *sys) {
    return sys->seed;
}

void WbtSystem_SetDownloadSlot(WbtSystem *sys, u8 value) {
    sys->downloadSlot = value;
}

u8 WbtSystem_GetDownloadSlot(WbtSystem *sys) {
    return sys->downloadSlot;
}

void WbtSystem_SetResultVar(WbtSystem *sys, u16 *var) {
    sys->resultVar = var;
}

u16 *WbtSystem_GetResultVar(WbtSystem *sys) {
    return sys->resultVar;
}

void WbtSystem_SetUseBattleBox(WbtSystem *sys, u8 value) {
    sys->useBattleBox = value;
}

void WbtSystem_SetPartyList(WbtSystem *sys, WbtPartyList *list) {
    sys->partyList = list;
}

WbtPartyList *WbtSystem_GetPartyList(WbtSystem *sys) {
    return sys->partyList;
}

void WbtSystem_SetRentalMode(WbtSystem *sys, u32 value) {
    sys->rentalMode = value;
}

WbtEntrant *WbtSystem_GetEntrant(WbtSystem *sys, u32 index) {
    return &sys->entrants[index];
}

// The player's entrant
WbtEntrant *WbtSystem_GetPlayerEntrant(WbtSystem *sys) {
    int i;

    for (i = 0; i < 8; i++) {
        if (sys->entrants[i].kind == WBT_ENTRANT_KIND_PLAYER) {
            return &sys->entrants[i];
        }
    }
    return sys->entrants;
}

void WbtSystem_LoadTournamentName(WbtSystem *sys, u32 tournament, StrBuf *strbuf) {
    if (tournament == WBT_TOURNAMENT_DOWNLOADED) {
        GFL_StrBufLoadFixedString(strbuf, sys->download.name, NELEMS(sys->download.name));
    } else {
        LoadPWTTournamentTypeText(sys->heapId, tournament, strbuf);
    }
}

PokeParty *WbtSystem_GetPlayerParty(WbtSystem *sys) {
    return sys->playerParty;
}

PokeParty *WbtSystem_GetOpponentParty(WbtSystem *sys) {
    return sys->opponentParty;
}

u32 WbtSystem_GetEntryCount(WbtSystem *sys) {
    return WbtStyle_GetPokemonCount(sys->style);
}

static void WbtEntrant_Init(WbtEntrant *entrant, HeapID heapId) {
    entrant->kind = WBT_ENTRANT_KIND_TABLE;
    entrant->gender = 0;
    entrant->rank = 1;
    entrant->hasSpecialPokemon = 0;
    entrant->type = 0;
    entrant->trainerId = 0;
    entrant->iv = 0;
    entrant->downloadedRound = 0;
    entrant->objCode = 0xe7;
    entrant->trainerClass = 0;
    entrant->aiBits = 1;
    entrant->name = GFL_StrBufCreate(16, heapId);
    entrant->className = GFL_StrBufCreate(0x16, heapId);
}

static void WbtEntrant_Free(WbtEntrant *entrant) {
    GFL_StrBufFree(entrant->name);
    GFL_StrBufFree(entrant->className);
}

u32 WbtEntrant_GetRank(WbtEntrant *entrant) {
    return entrant->rank;
}

u8 WbtEntrant_GetIv(WbtEntrant *entrant) {
    return entrant->iv;
}

u32 WbtEntrant_GetDownloadedRound(WbtEntrant *entrant) {
    return entrant->downloadedRound;
}

u8 WbtEntrant_GetTrainerId(WbtEntrant *entrant) {
    return entrant->trainerId;
}

u16 WbtEntrant_GetTrainerClass(WbtEntrant *entrant) {
    return entrant->trainerClass;
}

u32 WbtEntrant_GetKind(WbtEntrant *entrant) {
    return entrant->kind;
}

void WbtEntrant_GetName(const WbtEntrant *entrant, StrBuf *dest) {
    GFL_StrBufCopy(dest, entrant->name);
}

u32 WbtEntrant_HasSpecialPokemon(WbtEntrant *entrant) {
    return entrant->hasSpecialPokemon;
}

// Copies the Pokémon picked for the tournament into its parties
void WbtSystem_SetPickedParty(WbtSystem *sys, GameData *gameData) {
    PokeParty *party = PokeParty_Create(HEAPID_TAIL(sys->heapId));
    PokeParty *src;
    PartyPkm *copy;
    HeapID heapId;
    u32 count;
    u32 n;
    u32 i;
    u8 slot;

    PokeParty_Init(party);
    src = WbtMakeMatch_GetParty(sys, gameData);
    heapId = HEAPID_TAIL(sys->heapId);
    copy = GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), FALSE, "wbt_system.c", 468);
    n = 0;
    PokeParty_ClearPkm(copy);
    count = WbtSystem_GetEntryCount(sys);
    if (WbtTournament_GetKind(sys->tournament) == WBT_TOURNAMENT_KIND_DOWNLOADED) {
        for (i = 0; i < NELEMS(sys->castSlots); i++) {
            if (sys->castSlots[i] != 0) {
                n++;
            }
        }
        count = n;
    }
    for (i = 0; i < count; i++) {
        slot = sys->castSlots[i];
        if (slot == 0) {
            slot = 1;
        }
        copyPartyPkm(PokeParty_GetPkm(src, slot - 1), copy);
        PokeParty_AddPkm(party, copy);
    }
    PokeParty_Copy(party, sys->playerParty);
    PokeParty_Copy(party, sys->initialParty);
    GFL_HeapFree(party);
    GFL_HeapFree(copy);
}

static const u8 sUnrestrictedRegulations[] = { 29, 30, 31, 32 };
static const u8 sRestrictedRegulations[] = { 33, 34, 35, 36 };

// The tournament's regulation, in arc 106
u8 WbtSystem_GetRegulationId(WbtSystem *sys) {
    const u8 *regulations;

    if (sys->tournament == WBT_TOURNAMENT_DRIFTVEIL_FIRST) {
        return 37;
    }
    switch (WbtTournament_GetKind(sys->tournament)) {
    case WBT_TOURNAMENT_KIND_UNRESTRICTED:
    case 3:
    case WBT_TOURNAMENT_KIND_DOWNLOADED:
    default:
        regulations = sUnrestrictedRegulations;
        break;
    case WBT_TOURNAMENT_KIND_RESTRICTED:
    case WBT_TOURNAMENT_KIND_TYPE_EXPERT:
        regulations = sRestrictedRegulations;
        break;
    }
    return regulations[sys->style];
}

Regulation *WbtSystem_GetRegulation(WbtSystem *sys) {
    if (sys->tournament == WBT_TOURNAMENT_DOWNLOADED) {
        WbtDownloadRules *downloaded = &sys->download.rules;
        int i;

        MI_CpuFill8(&sys->regulation, 0, sizeof(sys->regulation));
        sys->regulation.minCount = downloaded->minCount;
        sys->regulation.maxCount = downloaded->maxCount;
        sys->regulation.level = downloaded->level;
        sys->regulation.levelMode = downloaded->levelMode;
        sys->regulation.maxLevelTotal = downloaded->maxLevelTotal;
        sys->regulation.allowSameSpecies = downloaded->allowSameSpecies;
        sys->regulation.allowSameItem = downloaded->allowSameItem;
        for (i = 0; i < 0x52; i++) {
            sys->regulation.bannedSpecies[i] = downloaded->bannedSpecies[i];
        }
        for (i = 0; i < 0x4c; i++) {
            sys->regulation.bannedItems[i] = downloaded->bannedItems[i];
        }
        sys->regulation.battleType = downloaded->battleType;
    } else {
        PokeRegulation_Load(WbtSystem_GetRegulationId(sys), &sys->regulation);
        if (sys->tournament == WBT_TOURNAMENT_DRIFTVEIL_FIRST || sys->tournament == WBT_TOURNAMENT_DRIFTVEIL) {
            sys->regulation.level = 25;
        }
    }
    return &sys->regulation;
}

RegulationExtraRule *WbtSystem_GetExtraRule(WbtSystem *sys) {
    u8 type = TYPE_NULL;
    u16 move = MOVE_NONE;

    switch (WbtTournament_GetKind(sys->tournament)) {
    case WBT_TOURNAMENT_KIND_UNRESTRICTED:
        break;
    case WBT_TOURNAMENT_KIND_RESTRICTED:
        move = MOVE_SKY_DROP;
        break;
    case WBT_TOURNAMENT_KIND_TYPE_EXPERT:
        type = sys->type;
        move = MOVE_SKY_DROP;
        break;
    case 3:
        break;
    case WBT_TOURNAMENT_KIND_DOWNLOADED:
        type = sys->download.rules.requiredType;
        move = sys->download.rules.bannedMove;
        break;
    }
    sys->extraRule.type = type;
    sys->extraRule.banMoves = FALSE;
    sys->extraRule.move = move;
    return &sys->extraRule;
}

// The score of a match, by how it was won and how the Trainers' types matched up
static const u8 sScores[5][3] = {
    { 3, 3, 4 }, { 0, 0, 0 }, { 1, 1, 1 }, { 2, 2, 4 }, { 3, 3, 4 },
};

// The opponent of each seat in the first round, and which match of the one before each seat's next opponent comes from
static const u32 sFirstOpponents[] = { 1, 0, 3, 2, 5, 4, 7, 6 };
static const u32 sSecondMatches[] = { 1, 1, 0, 0, 3, 3, 2, 2 };
static const u32 sThirdMatches[] = { 1, 1, 1, 1, 0, 0, 0, 0 };

// The seat of the player's opponent in the current round
u32 WbtSystem_GetOpponentSeat(WbtSystem *sys) {
    u8 firstWinners[4];
    u8 secondWinners[2];
    WbtMatch *matches;
    u32 player;
    u32 opponent;
    int i;
    int seat;

    matches = sys->matches;
    player = 0;
    opponent = 0;
    for (i = 0; i < 4; i++) {
        seat = i * 2 + (matches[i].firstWon == TRUE ? 0 : 1);
        firstWinners[i] = seat;
    }
    for (i = 0; i < 2; i++) {
        seat = i * 2 + (matches[i + 4].firstWon == TRUE ? 0 : 1);
        secondWinners[i] = firstWinners[seat];
    }
    for (i = 0; i < 8; i++) {
        if (sys->entrants[i].kind == WBT_ENTRANT_KIND_PLAYER) {
            player = i;
            break;
        }
    }
    switch (WbtSystem_GetRound(sys)) {
    case WBT_ROUND_PREVIEW:
    case WBT_ROUND_FIRST:
        opponent = sFirstOpponents[player];
        break;
    case WBT_ROUND_SEMIFINAL:
        opponent = firstWinners[sSecondMatches[player]];
        break;
    case WBT_ROUND_FINAL:
        opponent = secondWinners[sThirdMatches[player]];
        break;
    case WBT_ROUND_COMPLETE:
        break;
    }
    return opponent;
}

static void WbtMatch_Play(WbtMatch *match, MATHRandContext32 *rand, WbtEntrant *first, WbtEntrant *second) {
    int firstRank = first->rank;
    int secondRank = second->rank;
    u8 firstEffect = WbtMatch_GetTypeEffectiveness(first->type, second->type);
    u8 secondEffect = WbtMatch_GetTypeEffectiveness(second->type, first->type);
    int kind;
    int sign;

    if (firstRank == secondRank) {
        if (firstEffect == secondEffect) {
            match->firstWon = MATH_Rand32(rand, 2) == 0;
        } else {
            match->firstWon = firstEffect > secondEffect;
            if (MATH_Rand32(rand, 10) >= 7) {
                match->firstWon = match->firstWon == FALSE;
            }
        }
    } else {
        match->firstWon = firstRank > secondRank;
    }
    if (first->kind == WBT_ENTRANT_KIND_PLAYER) {
        match->firstWon = TRUE;
    } else if (second->kind == WBT_ENTRANT_KIND_PLAYER) {
        match->firstWon = FALSE;
    }
    if (firstRank >= 4 && secondRank >= 4) {
        kind = WBT_MATCH_TOP_RANKS;
    } else if ((firstRank - secondRank >= 2 && match->firstWon == TRUE) ||
               (secondRank - firstRank >= 2 && match->firstWon == FALSE)) {
        kind = WBT_MATCH_LARGE_RANK_WIN;
    } else if ((firstRank - secondRank == 1 && match->firstWon == TRUE) ||
               (secondRank - firstRank == 1 && match->firstWon == FALSE)) {
        kind = WBT_MATCH_SMALL_RANK_WIN;
    } else {
        kind = firstRank == secondRank ? WBT_MATCH_EQUAL_RANKS : WBT_MATCH_UPSET;
    }
    if (firstEffect > secondEffect) {
        sign = -1;
    } else if (firstEffect < secondEffect) {
        sign = 1;
    } else {
        sign = 0;
    }
    if (match->firstWon == FALSE) {
        sign *= -1;
    }
    match->score = sScores[kind][sign + 1];
}

// Plays the first two rounds of the bracket
void WbtSystem_PlayFirstRounds(WbtSystem *sys, MATHRandContext32 *rand) {
    WbtEntrant *entrants[2];
    WbtMatch *match;
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        WbtMatch_Play(&sys->matches[i], rand, &sys->entrants[i * 2], &sys->entrants[i * 2 + 1]);
    }
    for (i = 0; i < 2; i++) {
        match = &sys->matches[i + 4];
        for (j = 0; j < 2; j++) {
            if (sys->matches[i * 2 + j].firstWon) {
                entrants[j] = &sys->entrants[i * 4 + j * 2];
            } else {
                entrants[j] = &sys->entrants[i * 4 + j * 2 + 1];
            }
        }
        WbtMatch_Play(match, rand, entrants[0], entrants[1]);
    }
}

// The type chart, in damage multiples of 4
// clang-format off
static const u8 sTypeChart[17][17] = {
    { 4, 4, 4, 4, 4, 2, 4, 0, 2, 4, 4, 4, 4, 4, 4, 4, 4 },
    { 8, 4, 2, 2, 4, 8, 2, 0, 8, 4, 4, 4, 4, 2, 8, 4, 8 },
    { 4, 8, 4, 4, 4, 2, 8, 4, 2, 4, 4, 8, 2, 4, 4, 4, 4 },
    { 4, 4, 4, 2, 2, 2, 4, 2, 0, 4, 4, 8, 4, 4, 4, 4, 4 },
    { 4, 4, 0, 8, 4, 8, 2, 4, 8, 8, 4, 2, 8, 4, 4, 4, 4 },
    { 4, 2, 8, 4, 2, 4, 8, 4, 2, 8, 4, 4, 4, 4, 8, 4, 4 },
    { 4, 2, 2, 2, 4, 4, 4, 2, 2, 2, 4, 8, 4, 8, 4, 4, 8 },
    { 0, 4, 4, 4, 4, 4, 4, 8, 2, 4, 4, 4, 4, 8, 4, 4, 2 },
    { 4, 4, 4, 4, 4, 8, 4, 4, 2, 2, 2, 4, 2, 4, 8, 4, 4 },
    { 4, 4, 4, 4, 4, 2, 8, 4, 8, 2, 2, 8, 4, 4, 8, 2, 4 },
    { 4, 4, 4, 4, 8, 8, 4, 4, 4, 8, 2, 2, 4, 4, 4, 2, 4 },
    { 4, 4, 2, 2, 8, 8, 2, 4, 2, 2, 8, 2, 4, 4, 4, 2, 4 },
    { 4, 4, 8, 4, 0, 4, 4, 4, 4, 4, 8, 2, 2, 4, 4, 2, 4 },
    { 4, 8, 4, 8, 4, 4, 4, 4, 2, 4, 4, 4, 4, 2, 4, 4, 0 },
    { 4, 4, 8, 4, 8, 4, 4, 4, 2, 2, 2, 8, 4, 4, 2, 8, 4 },
    { 4, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 8, 4 },
    { 4, 2, 4, 4, 4, 4, 4, 8, 2, 4, 4, 4, 4, 8, 4, 4, 2 },
};

// clang-format on

// How well one type attacks another: 0 not at all, 1 not very well, 2 normally and 3 super effectively. Type 17 is
// no type
static u8 WbtMatch_GetTypeEffectiveness(u32 attackType, u32 defenseType) {
    if (attackType == TYPE_NULL || defenseType == TYPE_NULL) {
        return WBT_TYPE_EFFECT_NEUTRAL;
    }
    switch (sTypeChart[attackType][defenseType]) {
    case WBT_TYPE_DAMAGE_IMMUNE:
        return WBT_TYPE_EFFECT_IMMUNE;
    case WBT_TYPE_DAMAGE_RESISTED:
        return WBT_TYPE_EFFECT_RESISTED;
    case WBT_TYPE_DAMAGE_NEUTRAL:
        return WBT_TYPE_EFFECT_NEUTRAL;
    case WBT_TYPE_DAMAGE_SUPER_EFFECTIVE:
        return WBT_TYPE_EFFECT_SUPER_EFFECTIVE;
    }
    return WBT_TYPE_EFFECT_IMMUNE;
}
