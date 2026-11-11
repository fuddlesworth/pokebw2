#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "field/battle_facility.h"
#include "field/bsubway_scr.h"
#include "field/wbt.h"
#include "field/wbt_battle.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "text/system/world_tournament_party.h"

// The exclusion lists and their unused file marker.
#define WBT_PARTY_FILTER_CAPACITY 12
#define WBT_PARTY_FILE_NONE 0xffff
#define WBT_RENTAL_MIXED_SECOND_POOL_COUNT 3

typedef struct {
    u8 available;
    u16 file;
    BSubwayPokemonData data;
} WbtPartyChoice;

typedef struct {
    u16 count;
    WbtPartyChoice *choices;
} WbtPartyChoices;

// The first and last files of the rental Pokémon, by the group and the tournament
static u16 sRentalFiles[2][2][2] = {
    { { 1, 132 }, { 209, 558 } },
    { { 133, 208 }, { 559, 908 } },
};

static u16 WbtParty_Random(MATHRandContext32 *rand);
static BOOL WbtPartyFilter_Excludes(const WbtPartyFilter *filter, u16 species, u16 item, s16 file);
static WbtPartyChoices *WbtPartyChoices_Load(HeapID heapId, u32 arcId, u16 count, const u16 *files);
static void WbtPartyChoices_Free(WbtPartyChoices *choices);
static u16 WbtPartyChoices_Filter(WbtPartyChoices *choices, WbtPartyFilter *filter);
static u16 WbtPartyChoices_GetAvailable(WbtPartyChoices *choices, u32 index);
static void WbtPartyChoices_Take(WbtPartyChoices *choices, WbtPartyFilter *filter, u16 index);
static void WbtPartyChoices_Pick(u16 *dest, WbtPartyChoices *choices, u32 count, WbtPartyFilter *filter,
                                 MATHRandContext32 *rand, HeapID heapId);
static void WbtParty_GetRentalFileRange(u32 tournament, u32 group, u16 *first, u16 *last);
static u16 *WbtParty_CreateRentalFiles(u32 tournament, u32 group, HeapID heapId, u16 *count);
static void WbtParty_MakeParty(WbtSystem *sys, PokeParty *party, u32 id, u8 iv, u8 count, const u16 *species);

// A random number below 0x10000, from rand, or from GFL_RandomLC without it
static u16 WbtParty_Random(MATHRandContext32 *rand) {
    if (rand == NULL) {
        return GFL_RandomLC(0xffffffff) / 0xffff;
    }
    return MATH_Rand32(rand, 0xffffffff) / 0xffff;
}

WbtPartyFilter *WbtPartyFilter_Create(HeapID heapId, u32 tournament) {
    WbtPartyFilter *filter = GFL_HeapAllocate(heapId, sizeof(WbtPartyFilter), TRUE, "wbt_party.c", 127);

    filter->allowSameSpecies = FALSE;
    if (tournament != WBT_TOURNAMENT_RENTAL && WbtTournament_GetKind(tournament) == WBT_TOURNAMENT_KIND_UNRESTRICTED) {
        filter->allowSameSpecies = TRUE;
    }
    filter->speciesCount = WBT_PARTY_FILTER_CAPACITY;
    filter->itemCount = WBT_PARTY_FILTER_CAPACITY;
    filter->fileCount = WBT_PARTY_FILTER_CAPACITY;
    filter->species = GFL_HeapAllocate(heapId, WBT_PARTY_FILTER_CAPACITY * sizeof(u16), TRUE, "wbt_party.c", 136);
    filter->items = GFL_HeapAllocate(heapId, WBT_PARTY_FILTER_CAPACITY * sizeof(u16), TRUE, "wbt_party.c", 137);
    filter->files = GFL_HeapAllocate(heapId, WBT_PARTY_FILTER_CAPACITY * sizeof(u16), FALSE, "wbt_party.c", 138);
    MIi_CpuClear16(WBT_PARTY_FILE_NONE, filter->files, WBT_PARTY_FILTER_CAPACITY * sizeof(u16));
    return filter;
}

void WbtPartyFilter_Free(WbtPartyFilter *filter) {
    GFL_HeapFree(filter->species);
    GFL_HeapFree(filter->items);
    GFL_HeapFree(filter->files);
    GFL_HeapFree(filter);
}

void WbtPartyFilter_AddItem(WbtPartyFilter *filter, u16 item) {
    int i;

    for (i = 0; i < filter->itemCount; i++) {
        if (filter->items[i] == 0) {
            filter->items[i] = item;
            return;
        }
    }
}

void WbtPartyFilter_AddSpecies(WbtPartyFilter *filter, u16 species) {
    int i;

    for (i = 0; i < filter->speciesCount; i++) {
        if (filter->species[i] == 0) {
            filter->species[i] = species;
            return;
        }
    }
}

void WbtPartyFilter_AddFile(WbtPartyFilter *filter, s16 file) {
    int i;

    for (i = 0; i < filter->fileCount; i++) {
        if (filter->files[i] == WBT_PARTY_FILE_NONE) {
            filter->files[i] = file;
            return;
        }
    }
}

// Whether the filter keeps out the Pokémon
static BOOL WbtPartyFilter_Excludes(const WbtPartyFilter *filter, u16 species, u16 item, s16 file) {
    int i;

    if (filter == NULL) {
        return TRUE;
    }
    if (filter->allowSameSpecies == FALSE) {
        for (i = 0; i < filter->speciesCount; i++) {
            if (species != 0 && species == filter->species[i]) {
                return TRUE;
            }
        }
    }
    for (i = 0; i < filter->itemCount; i++) {
        if (item != 0 && item == filter->items[i]) {
            return TRUE;
        }
    }
    for (i = 0; i < filter->fileCount; i++) {
        if (file != WBT_PARTY_FILE_NONE && file == filter->files[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

static WbtPartyChoices *WbtPartyChoices_Load(HeapID heapId, u32 arcId, u16 count, const u16 *files) {
    WbtPartyChoices *choices = GFL_HeapAllocate(heapId, sizeof(WbtPartyChoices), TRUE, "wbt_party.c", 274);
    ArcTool *handle;
    int i;

    choices->count = count;
    choices->choices = GFL_HeapAllocate(heapId, count * sizeof(WbtPartyChoice), TRUE, "wbt_party.c", 276);
    handle = GFL_ArcSysCreateFileHandle(arcId, HEAPID_TAIL(heapId));
    for (i = 0; i < count; i++) {
        WbtPartyChoice *choice = &choices->choices[i];

        choice->available = TRUE;
        choice->file = files[i];
        GFL_ArcToolRead(handle, files[i], &choice->data);
    }
    GFL_ArcToolFree(handle);
    return choices;
}

static void WbtPartyChoices_Free(WbtPartyChoices *choices) {
    GFL_HeapFree(choices->choices);
    GFL_HeapFree(choices);
}

// Takes the choices the filter keeps out away, and counts those left
static u16 WbtPartyChoices_Filter(WbtPartyChoices *choices, WbtPartyFilter *filter) {
    int count = 0;
    int i;

    for (i = 0; i < choices->count; i++) {
        WbtPartyChoice *choice = &choices->choices[i];

        if (WbtPartyFilter_Excludes(filter, choice->data.species, choice->data.item, choice->file) == TRUE) {
            choice->available = FALSE;
        }
        if (choice->available) {
            count++;
        }
    }
    return count;
}

// The index of the choice that is the index-th of those left
static u16 WbtPartyChoices_GetAvailable(WbtPartyChoices *choices, u32 index) {
    u32 i;

    for (i = 0; i < choices->count; i++) {
        if (choices->choices[i].available) {
            if (index == 0) {
                break;
            }
            index--;
        }
    }
    return i;
}

static void WbtPartyChoices_Take(WbtPartyChoices *choices, WbtPartyFilter *filter, u16 index) {
    WbtPartyChoice *choice;

    choices->choices[index].available = FALSE;
    choice = &choices->choices[index];
    WbtPartyFilter_AddSpecies(filter, choice->data.species);
    WbtPartyFilter_AddItem(filter, choice->data.item);
}

static void WbtPartyChoices_Pick(u16 *dest, WbtPartyChoices *choices, u32 count, WbtPartyFilter *filter,
                                 MATHRandContext32 *rand, HeapID heapId) {
    u16 picked[6];
    u16 index;
    int i;
    int j;

    i = 0;
    MI_CpuFill8(picked, 0, sizeof(picked));
    for (; i != count; i++) {
        u32 left = WbtPartyChoices_Filter(choices, filter);

        index = WbtPartyChoices_GetAvailable(choices, WbtParty_Random(rand) % left);
        WbtPartyChoices_Take(choices, filter, index);
        picked[i] = index;
    }
    for (j = 0; j < i; j++) {
        dest[j] = choices->choices[picked[j]].file;
    }
}

static void WbtParty_GetRentalFileRange(u32 tournament, u32 group, u16 *first, u16 *last) {
    int master;

    switch (tournament) {
    case WBT_TOURNAMENT_RENTAL:
    default:
        master = 0;
        break;
    case WBT_TOURNAMENT_RENTAL_MASTER:
        master = 1;
        break;
    }
    *first = sRentalFiles[group][master][0];
    *last = sRentalFiles[group][master][1];
}

static u16 *WbtParty_CreateRentalFiles(u32 tournament, u32 group, HeapID heapId, u16 *count) {
    u16 first;
    u16 last;
    u16 n;
    u16 *files;
    int i;

    WbtParty_GetRentalFileRange(tournament, group, &first, &last);
    n = last - first + 1;
    files = GFL_HeapAllocate(heapId, n * sizeof(u16), TRUE, "wbt_party.c", 505);
    for (i = 0; i < n; i++) {
        files[i] = first + i;
    }
    *count = n;
    return files;
}

void WbtParty_PickFiles(u16 *dest, WbtPartyFilter *filter, HeapID heapId, u16 fileCount, const u16 *files, u32 arcId,
                        u8 count, MATHRandContext32 *rand) {
    WbtPartyChoices *choices;

    OS_GetTick();
    choices = WbtPartyChoices_Load(heapId, arcId, fileCount, files);
    WbtPartyChoices_Pick(dest, choices, count, filter, rand, heapId);
    WbtPartyChoices_Free(choices);
    OS_GetTick();
}

static void WbtParty_MakeParty(WbtSystem *sys, PokeParty *party, u32 id, u8 iv, u8 count, const u16 *species) {
    HeapID heapId = HEAPID_TAIL(sys->heapId);
    WbtPartyFilter *filter = WbtPartyFilter_Create(heapId, sys->tournament);
    u16 fileCount;
    u16 picked[6];
    u16 *files;
    BSubwayPokemon *pkms;
    int i;

    if (species != NULL) {
        for (i = 0; i < count; i++) {
            WbtPartyFilter_AddSpecies(filter, species[i]);
        }
    }
    MI_CpuFill8(picked, 0, sizeof(picked));
    switch (sys->rentalMode) {
    case WBT_RENTAL_MODE_FIRST_POOL:
    default:
        files = WbtParty_CreateRentalFiles(sys->tournament, 0, heapId, &fileCount);
        WbtParty_PickFiles(picked, filter, heapId, fileCount, files, 257, count, NULL);
        GFL_HeapFree(files);
        break;
    case WBT_RENTAL_MODE_SECOND_POOL:
        files = WbtParty_CreateRentalFiles(sys->tournament, 1, heapId, &fileCount);
        WbtParty_PickFiles(picked, filter, heapId, fileCount, files, 257, count, NULL);
        GFL_HeapFree(files);
        break;
    case WBT_RENTAL_MODE_MIXED_POOLS:
        files = WbtParty_CreateRentalFiles(sys->tournament, 1, heapId, &fileCount);
        WbtParty_PickFiles(picked, filter, heapId, fileCount, files, 257, WBT_RENTAL_MIXED_SECOND_POOL_COUNT, NULL);
        GFL_HeapFree(files);
        if (count > WBT_RENTAL_MIXED_SECOND_POOL_COUNT) {
            files = WbtParty_CreateRentalFiles(sys->tournament, 0, heapId, &fileCount);
            WbtParty_PickFiles(&picked[WBT_RENTAL_MIXED_SECOND_POOL_COUNT], filter, heapId, fileCount, files, 257,
                               count - WBT_RENTAL_MIXED_SECOND_POOL_COUNT, NULL);
            GFL_HeapFree(files);
        }
        break;
    }
    WbtPartyFilter_Free(filter);
    pkms = GFL_HeapAllocate(HEAPID_TAIL(heapId), 6 * sizeof(BSubwayPokemon), FALSE, "wbt_party.c", 626);
    for (i = 0; i < count; i++) {
        BtlInst_MakeRomPokemon(&pkms[i], 257, picked[i], id, 0, iv, i, 0, heapId);
    }
    BtlInst_MakeParty(party, pkms, 50, count, heapId);
    GFL_HeapFree(pkms);
}

// Makes the rental party
void WbtParty_MakeRentalParty(WbtSystem *sys, u32 rentalMode, PlayerInfo *playerInfo, u16 placeName) {
    HeapID heapId;
    StrBuf *name;
    MsgData *msgData;
    PartyPkm *pkm;
    int i;

    WbtParty_MakeParty(sys, sys->initialParty, 41122, 20, 6, NULL);
    heapId = HEAPID_TAIL(sys->heapId);
    name = GFL_StrBufCreate(14, heapId);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WORLD_TOURNAMENT_PARTY, heapId);
    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, WorldTournamentParty_Text_Empty, name);
        GFL_MsgDataFree(msgData);
    }
    for (i = 0; i < 6; i++) {
        pkm = PokeParty_GetPkm(sys->initialParty, i);
        PokeParty_SetupMetData(pkm, 0, playerInfo, placeName, heapId);
        PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME, (u32)name);
        PokeParty_SetParam(pkm, PKM_PARAM_OT_GENDER, 0);
        PokeParty_SetParam(pkm, PKM_PARAM_ID, 41122);
    }
    GFL_StrBufFree(name);
}

// Makes the party of the player's opponent, without the species of the player's party
void WbtParty_MakeOpponentParty(WbtSystem *sys) {
    u32 count = WbtSystem_GetEntryCount(sys);
    int n = PokeParty_GetPkmCount(sys->playerParty);
    u8 iv = WbtEntrant_GetIv(WbtBattle_GetOpponent(sys));
    u16 species[6];
    int i;

    MI_CpuFill8(species, 0, sizeof(species));
    for (i = 0; i < n; i++) {
        species[i] = PokeParty_GetParam(PokeParty_GetPkm(sys->playerParty, i), PKM_PARAM_SPECIES, NULL);
    }
    WbtParty_MakeParty(sys, sys->opponentParty, 12345, iv, count, species);
}
