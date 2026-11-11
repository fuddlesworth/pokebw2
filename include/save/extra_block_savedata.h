#ifndef POKEBW2_SAVE_EXTRA_BLOCK_SAVEDATA_H
#define POKEBW2_SAVE_EXTRA_BLOCK_SAVEDATA_H

#include "types.h"

// Two extra save blocks that are only cleared here (extra_block_savedata.c, a descriptive name): one of 0xc8 bytes,
// and a downloaded tournament of the Pokémon World Tournament. The tournament is also what the download receives
// (overlay 327), what the screen of the downloaded tournaments shows and saves (overlay 326), and what the tournament
// copies to play it (WbtSystem's download)

// The versions a downloaded tournament is for
enum {
    WBT_DOWNLOAD_KIND_WHITE2,
    WBT_DOWNLOAD_KIND_BLACK2,
    WBT_DOWNLOAD_KIND_BOTH,
};

// The state of a downloaded tournament that can be used
#define WBT_DOWNLOAD_STATE_VALID 2

#define WBT_DOWNLOAD_NAME_LENGTH 37
#define WBT_DOWNLOAD_AUTHOR_LENGTH 75
#define WBT_DOWNLOAD_TEXT_LENGTH 111
#define WBT_DOWNLOAD_SPECIES_BYTES 82
#define WBT_DOWNLOAD_ITEM_BYTES 76
#define WBT_DOWNLOAD_ROUNDS 4
#define WBT_DOWNLOAD_ENTRANT_SLOTS 7

// A downloaded tournament's rules, which WbtSystem_GetRegulation copies
typedef struct {
    u8 minCount;
    u8 maxCount;
    u8 level;
    u8 levelMode;
    u16 maxLevelTotal;
    u8 allowSameSpecies;
    u8 allowSameItem;
    // Bitsets of the banned species and items
    u8 bannedSpecies[WBT_DOWNLOAD_SPECIES_BYTES];
    u8 bannedItems[WBT_DOWNLOAD_ITEM_BYTES];
    u8 unkA6;
    u8 battleType;
    // Set for a World Championships tournament, which has their music and its own battle background
    u8 isWcs;
    u8 requiredType;
    u16 bannedMove;
} WbtDownloadRules;

// A Pokémon of a Trainer of a downloaded tournament, which becomes a party Pokémon
typedef struct {
    u16 species;
    u16 nickname[11];
    u16 item;
    // Whether PML_GenPID makes the Pokémon shiny
    u8 shiny : 1;
    u8 unk1A_1 : 7;
    // The ability, by its number
    u8 ability;
    u8 evs[6];
    u16 moves[4];
    // Two bits per move
    u8 ppUps;
    u8 nature;
    struct {
        u32 hp : 5;
        u32 attack : 5;
        u32 defense : 5;
        u32 speed : 5;
        u32 spAttack : 5;
        u32 spDefense : 5;
        u32 sex : 2;
    } ivs;
    u8 form;
    u8 ball;
    u8 level;
    u8 happiness;
} WbtDownloadedPokemon;

// A Trainer of a downloaded tournament: up to six Pokémon, and four lines of 74 characters that the Trainer says,
// each unless its message set is 0 a message of that set of the Trainers' messages instead
typedef struct {
    WbtDownloadedPokemon pokemon[6];
    // The Trainer's name and Trainer class name
    u16 name[16];
    u16 className[0x16];
    u16 lines[4][0x4a];
    u8 classIndex;
    u8 messageSets[6];
    u8 padding3DB;
    // WbtEntrant's aiBits
    u16 aiBits;
    u8 unk3DE[2];
} WbtDownloadedTrainer;

// An entrant slot of a downloaded tournament: its kind (overlay 135's WBT_SLOT_*), the entrant's rank, and the index
// of the Trainer in its table or of the downloaded Trainer
typedef struct {
    u8 type;
    u8 rank;
    u16 index;
} WbtDownloadEntrantSlot;

// A downloaded tournament
typedef struct {
    // A WBT_DOWNLOAD_KIND_*
    u32 kind;
    u8 unk4[3];
    // WBT_DOWNLOAD_STATE_VALID for one that can be used
    u8 state;
    // 0 for none
    u16 id;
    u16 name[WBT_DOWNLOAD_NAME_LENGTH];
    u16 author[WBT_DOWNLOAD_AUTHOR_LENGTH];
    u16 text[WBT_DOWNLOAD_TEXT_LENGTH];
    WbtDownloadRules rules;
    // The Trainer of each round
    WbtDownloadedTrainer trainers[WBT_DOWNLOAD_ROUNDS];
    WbtDownloadEntrantSlot entrantSlots[WBT_DOWNLOAD_ENTRANT_SLOTS];
    // getCRC16 of everything before it
    u32 crc;
} WbtDownloadData; // size: 0x1214

// The size of the block of 0xc8 bytes and its initial state
u32 ExtraBlock20_GetSize(void);
void ExtraBlock20_Clear(void *block);
// The size of a downloaded tournament and its initial state
u32 WbtDownloadData_GetSize(void);
void WbtDownloadData_Clear(void *block);

#endif // POKEBW2_SAVE_EXTRA_BLOCK_SAVEDATA_H
