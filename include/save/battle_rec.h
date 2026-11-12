#ifndef POKEBW2_SAVE_BATTLE_REC_H
#define POKEBW2_SAVE_BATTLE_REC_H

#include "types.h"
#include "battle/btl_setup.h"
#include "gfl/heap.h"
#include "nitro/math.h"
#include "save/gds_profile.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/pms_data.h"
#include <stddef.h>

// battle_rec.c: the battle video that is loaded, which the battle recorder plays and the extra save data keeps

// Indices of sBattleRecModes, returned for BATTLE_REC_HEADER_MODE; not the encoded header.mode bits.
enum {
    BATTLE_REC_MODE_COLOSSEUM_SINGLE_NO_RESTRICTIONS = 0,
    BATTLE_REC_MODE_COLOSSEUM_SINGLE_FLAT = 1,
    BATTLE_REC_MODE_COLOSSEUM_SINGLE_NO_RESTRICTIONS_LAUNCHER = 2,
    BATTLE_REC_MODE_COLOSSEUM_SINGLE_FLAT_LAUNCHER = 3,
    BATTLE_REC_MODE_COLOSSEUM_DOUBLE_NO_RESTRICTIONS = 4,
    BATTLE_REC_MODE_COLOSSEUM_DOUBLE_FLAT = 5,
    BATTLE_REC_MODE_COLOSSEUM_DOUBLE_NO_RESTRICTIONS_LAUNCHER = 6,
    BATTLE_REC_MODE_COLOSSEUM_DOUBLE_FLAT_LAUNCHER = 7,
    BATTLE_REC_MODE_COLOSSEUM_TRIPLE_NO_RESTRICTIONS = 8,
    BATTLE_REC_MODE_COLOSSEUM_TRIPLE_FLAT = 9,
    BATTLE_REC_MODE_COLOSSEUM_TRIPLE_NO_RESTRICTIONS_LAUNCHER = 10,
    BATTLE_REC_MODE_COLOSSEUM_TRIPLE_FLAT_LAUNCHER = 11,
    BATTLE_REC_MODE_COLOSSEUM_ROTATION_NO_RESTRICTIONS = 12,
    BATTLE_REC_MODE_COLOSSEUM_ROTATION_FLAT = 13,
    BATTLE_REC_MODE_COLOSSEUM_ROTATION_NO_RESTRICTIONS_LAUNCHER = 14,
    BATTLE_REC_MODE_COLOSSEUM_ROTATION_FLAT_LAUNCHER = 15,
    BATTLE_REC_MODE_COLOSSEUM_MULTI_NO_RESTRICTIONS = 16,
    BATTLE_REC_MODE_COLOSSEUM_MULTI_FLAT = 17,
    BATTLE_REC_MODE_COLOSSEUM_MULTI_NO_RESTRICTIONS_LAUNCHER = 18,
    BATTLE_REC_MODE_COLOSSEUM_MULTI_FLAT_LAUNCHER = 19,
    BATTLE_REC_MODE_BATTLE_SUBWAY_SINGLE = 20,
    BATTLE_REC_MODE_BATTLE_SUBWAY_DOUBLE = 21,
    BATTLE_REC_MODE_BATTLE_SUBWAY_MULTI = 22,
    BATTLE_REC_MODE_RANDOM_MATCHUP_FREE_SINGLE = 23,
    BATTLE_REC_MODE_RANDOM_MATCHUP_FREE_DOUBLE = 24,
    BATTLE_REC_MODE_RANDOM_MATCHUP_FREE_TRIPLE = 25,
    BATTLE_REC_MODE_RANDOM_MATCHUP_FREE_ROTATION = 26,
    BATTLE_REC_MODE_RANDOM_MATCHUP_FREE_LAUNCHER = 27,
    BATTLE_REC_MODE_RANDOM_MATCHUP_RATING_SINGLE = 28,
    BATTLE_REC_MODE_RANDOM_MATCHUP_RATING_DOUBLE = 29,
    BATTLE_REC_MODE_RANDOM_MATCHUP_RATING_TRIPLE = 30,
    BATTLE_REC_MODE_RANDOM_MATCHUP_RATING_ROTATION = 31,
    BATTLE_REC_MODE_RANDOM_MATCHUP_RATING_LAUNCHER = 32,
    BATTLE_REC_MODE_COMPETITION_SINGLE_LAUNCHER_OFF = 33,
    BATTLE_REC_MODE_COMPETITION_SINGLE_LAUNCHER_ON = 34,
    BATTLE_REC_MODE_COMPETITION_DOUBLE_LAUNCHER_OFF = 35,
    BATTLE_REC_MODE_COMPETITION_DOUBLE_LAUNCHER_ON = 36,
    BATTLE_REC_MODE_COMPETITION_TRIPLE_LAUNCHER_OFF = 37,
    BATTLE_REC_MODE_COMPETITION_TRIPLE_LAUNCHER_ON = 38,
    BATTLE_REC_MODE_COMPETITION_ROTATION_LAUNCHER_OFF = 39,
    BATTLE_REC_MODE_COMPETITION_ROTATION_LAUNCHER_ON = 40,
};

// Slot-load outputs: distinct from the extra-save loader inputs and from save-write results.
enum {
    BATTLE_REC_LOAD_RESULT_EMPTY = 0,
    BATTLE_REC_LOAD_RESULT_VALID = 1,
    BATTLE_REC_LOAD_RESULT_INVALID = 2,
    BATTLE_REC_LOAD_RESULT_LOAD_FAILED = 3,
};

// A Pokémon as a recording keeps it: the fields of its blocks that the battle uses, unencrypted
typedef struct {
    u32 pid;
    u16 partyDecrypted : 1;
    u16 boxDecrypted : 1;
    u16 badEgg : 1;
    u16 nature : 8;
    u16 nPoke : 1;
    u16 species;
    u16 item;
    u32 id;
    u32 exp;
    u8 friendship;
    u8 ability;
    u8 hpEV;
    u8 atkEV;
    u8 defEV;
    u8 speEV;
    u8 spaEV;
    u8 spdEV;
    u16 moves[4];
    u8 pp[4];
    u8 ppUps[4];
    u32 hpIV : 5;
    u32 atkIV : 5;
    u32 defIV : 5;
    u32 speIV : 5;
    u32 spaIV : 5;
    u32 spdIV : 5;
    u32 isEgg : 1;
    u32 isNicknamed : 1;
    u8 fatefulEncounter : 1;
    u8 gender : 2;
    u8 form : 5;
    u16 nickname[11];
    u16 otName[8];
    u8 ball;
    u8 language;
    u32 statusCond;
    u8 level;
    u8 pokestarFame;
    u16 nowHP;
    u16 maxHP;
    u16 atk;
    u16 def;
    u16 spe;
    u16 spa;
    u16 spd;
} BattleRecPkm;

// A party as a recording keeps it
typedef struct {
    u16 capacity;
    u16 count;
    BattleRecPkm pkm[6];
} BattleRecParty;

// A BtlSetupTrainer as a recording keeps it, with the name as characters
typedef struct {
    u32 aiFlags;
    u16 trainerId;
    u16 trainerClass;
    u16 items[4];
    u16 name[16];
    PMSData winWords;
    PMSData loseWords;
} BattleRecTrainer;

enum {
    BATTLE_REC_CLIENT_NONE,
    BATTLE_REC_CLIENT_PLAYER,
    BATTLE_REC_CLIENT_TRAINER,
};

// A client of the battle: a player, whose info the setup has, or a trainer, BATTLE_REC_CLIENT_*
typedef struct {
    u16 type;
    // The setup carries this Chatter effect as u8; the record keeps its original u16 storage.
    u16 chatterEffect;
    union {
        PlayerInfo player;
        BattleRecTrainer trainer;
    } info;
} BattleRecClient;

// The setup's parameters, which both forms keep at their start
typedef struct {
    MATHRandContext32 rand;
    BtlFieldEnv env;
    u8 config[4];
    u16 bgm;
    // The music that plays when the battle is won
    u16 winBgm;
    // The still-opaque BtlSetup.unkA2 value, separate from the trainer's win words. The recorder copies it to
    // and from that field, and a link battle's sync data carries it, but no code gives either a value.
    u16 unk30;
    u8 battleType : 5;
    u8 playerClientId : 3;
    u8 battleStyle : 4;
    u8 multiMode : 3;
    u8 launcherEnabled : 1;
} BattleRecSetup;

// The magic number of a video's header and body
#define BATTLE_REC_MAGIC 0xe281
// The size of the extra save block that holds a video, which is larger than the video
#define BATTLE_REC_BLOCK_SIZE 0x1a00
// The version that a video's header carries
#define BATTLE_REC_VERSION 100
// The battle modes that a header's mode can be, the size of sBattleRecModes
#define BATTLE_REC_MODE_MAX 41
// The Pokémon that a header lists
#define BATTLE_REC_HEADER_PKM_MAX 12

// The header of a battle video, which is what a list of videos shows
struct BattleRecHeader {
    u16 species[BATTLE_REC_HEADER_PKM_MAX];
    // The form in the low 7 bits and the gender in the top one
    u8 formGender[BATTLE_REC_HEADER_PKM_MAX];
    u16 counter;
    u16 mode;
    u16 magic;
    // Whether the video was saved as a secure one
    u8 secure;
    u8 version;
    // Never accessed: the header is cleared, copied and CRC'd whole. Aletheia's BattleRecHeader calls these
    // bytes unusedBytes
    u8 unk2C[0xc];
    // The video's number, which the server gives it and the CRC leaves out
    u64 dataNumber;
    // getCRC16 of everything before dataNumber
    u16 crc;
    u16 padding;
};

// The size that the header's CRC covers
#define BATTLE_REC_HEADER_CRC_SIZE offsetof(BattleRecHeader, dataNumber)

// The body of a battle video, after its profile and header
typedef struct {
    BattleRecSetup setup;
    u32 dataSize;
    u8 data[0xc00];
    BattleRecParty parties[4];
    BattleRecClient clients[4];
    u16 magic;
    u16 padding;
    // getCRC16 of everything before it, and the key that the body is encrypted with
    u16 crc;
    u16 padding2;
} BattleRecBody;

// The size that the body's CRC covers, and that is encrypted
#define BATTLE_REC_BODY_CRC_SIZE offsetof(BattleRecBody, crc)

// The battle video that is loaded, gBattleRecVideo (battle_rec.c).
typedef struct {
    u8 profile[0x80];
    BattleRecHeader header;
    BattleRecBody body;
} BattleRecVideo;

extern BattleRecVideo *gBattleRecVideo;

// The values of a header that BattleRec_GetHeaderValue gives
enum {
    BATTLE_REC_HEADER_SPECIES,
    BATTLE_REC_HEADER_FORM,
    BATTLE_REC_HEADER_COUNTER,
    BATTLE_REC_HEADER_MODE,
    BATTLE_REC_HEADER_DATA_NUMBER,
    BATTLE_REC_HEADER_SECURE,
    BATTLE_REC_HEADER_GENDER,
    BATTLE_REC_HEADER_VERSION,
};

// Allocates the loaded video, freeing the one there is, and frees it
void BattleRec_Init(HeapID heapId);
void freeVSPlayerBlkClearPtr(void);
// Frees a video of BattleRec_LoadVideo
void BattleRec_FreeVideo(BattleRecVideo *video);
u32 getSizeOfVSPlayer(void);
void clearVSPlayerBlk(BattleRecVideo *video);
// Whether the loaded video's body has anything in it
BOOL BattleRec_HasVideo(void);
// Decrypt and encrypt the loaded video's body
void BattleRec_DecryptBody(void);
void BattleRec_EncryptBody(void);
// Loads the saved video of a slot (0 is the player's own, 1 to 3 the downloaded ones). result is 1 if it loaded,
// 0 if there is none, 2 if its CRCs are wrong and 3 if the load failed
void BattleRec_LoadToCurrent(SaveControl *save, HeapID heapId, u32 *result, u32 slot);
// The same into a video of its own, which BattleRec_FreeVideo frees
BattleRecVideo *BattleRec_LoadVideo(SaveControl *save, HeapID heapId, u32 *result, u32 slot);
// Saves the loaded video to a slot as it is, a step each call: 0 while it runs, then the save's result (2 or 3)
u32 BattleRec_SaveSlot(GameData *gameData, BattleRecVideo *video, u32 slot, u16 *seq, HeapID heapId);
// The clients of a battle mode and the Pokémon of each
void BattleRec_GetModeCounts(u16 mode, s32 *clientCount, s32 *pkmCount);
// decryptData, which encrypts as well
void decryptVSPlayer(void *data, u32 size, u32 seed);
// The profile of the player who recorded the loaded video. The name is swan's
GdsProfile *getVSPlayerAllocation(void);
// The body and the header of the loaded video
BattleRecBody *BattleRec_GetBody(void);
BattleRecHeader *BattleRec_GetHeader(void);
// The header of a video
BattleRecHeader *BattleRec_GetVideoHeader(BattleRecVideo *video);
// Makes the loaded video of a received profile, header and body, and decrypts its body
void BattleRec_StoreDownloaded(const GdsProfile *profile, const BattleRecHeader *header, const BattleRecBody *body,
                               SaveControl *save);
// A value from the header, BATTLE_REC_HEADER_*, such as the video's number
u64 BattleRec_GetHeaderValue(BattleRecHeader *header, u32 id, u32 index);
// Sets the CRC of a body
void BattleRec_UpdateBodyCRC(BattleRecBody *body);
// BattleRec_IsSlotValid, BattleRec_FinishAndSaveSlot, BattleRec_IsVersionSupported, BattleRec_LoadToolOverlay and
// BattleRec_UnloadToolOverlay of this file are declared in save/save_control.h

#endif // POKEBW2_SAVE_BATTLE_REC_H
