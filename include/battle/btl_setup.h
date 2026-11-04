#ifndef POKEBW2_BATTLE_BTL_SETUP_H
#define POKEBW2_BATTLE_BTL_SETUP_H

#include "types.h"
#include "battle/regulation.h"
#include "gfl/heap.h"
#include "nitro/math.h"
#include "save/bag.h"
#include "save/config.h"
#include "struct_decls.h"
#include "system/pms_data.h"

// The background, place and time of a battle, which the battle view copies as one struct
typedef struct {
    u32 bgType;
    u32 terrain;
    u8 weather;
    u8 season;
    u16 zoneId;
    u8 hour;
    u8 minute;
} BtlFieldEnv;

// The battle's surroundings, which GetFieldEffectData returns
struct BtlFieldSituation {
    BtlFieldEnv env;
    // The battle's music, and the music of a win
    u16 bgm;
    u16 winBgm;
    NetHandle *netHandle;
    // The adapters' mode (BtlAdapter_InitSystem, GetCommMode), 0 for a battle alone
    u8 commMode;
    // The player's client
    u8 playerClientId;
    // The kind of multi battle, which GetMultiMode returns: 0 for a battle of one player a side
    u8 multiMode;
    // Whether the battle plays a recording back (BtlSetup_InitForRecord, IsRecPlayMode)
    u8 recPlay;
};

// A trainer the battle is set up with
struct BtlSetupTrainer {
    u32 trainerId;
    u32 trainerClass;
    u32 aiFlags;
    u16 items[4];
    StrBuf *name;
    // The phrases the trainer says on winning and on losing
    PMSData winWords;
    PMSData loseWords;
};

// A trainer as a link battle sends it, with the name as characters
typedef struct {
    u32 trainerId;
    u32 trainerClass;
    u32 aiFlags;
    u16 items[4];
    u8 unk14[4];
    PMSData winWords;
    PMSData loseWords;
    u16 name[0x20];
    u32 nameLength;
} BtlCommTrainerData;

// What BtlSetup.studioEnding is: how a Pokéstar Studios movie ended, which the audience reacts to
enum {
    STUDIO_ENDING_GOOD,
    STUDIO_ENDING_BAD,
    STUDIO_ENDING_STRANGE,
};

// What BtlSetup.flags holds, which BtlSetup_SetFlag sets and BtlSetup_CheckFlag and BtlSetup_IsBattleType read
enum {
    // A fishing encounter (BtlSetup_CreateFishing): the Dive Ball's bonus, and the player's reflection after it
    BTL_SETUP_FLAG_FISHING = 1 << 0,
    // The player has met Amanita, from the event flag that picks whose PC a caught Pokémon is sent to
    // (event_battle_return.c)
    BTL_SETUP_FLAG_MET_AMANITA = 1 << 1,
    // A legendary Pokémon's encounter, as the scripts ask for: its message has no "wild"
    BTL_SETUP_FLAG_LEGENDARY = 1 << 2,
    // A roaming Pokémon's: its own message, and an AI that flees
    BTL_SETUP_FLAG_ROAMING = 1 << 3,
    // Shows the message of a Pokémon seen in a dream
    BTL_SETUP_FLAG_DREAM = 1 << 4,
    // An encounter in dark grass, which applies its capture modifier
    BTL_SETUP_FLAG_DARK_GRASS = 1 << 5,
    // The party and the boxes are full, so a Poké Ball can't be thrown
    BTL_SETUP_FLAG_BOXES_FULL = 1 << 6,
    // Picks the third of the wild encounter messages, which reads as the first does
    BTL_SETUP_FLAG_80 = 1 << 7,
    // A trainer battle that can be lost without blacking out and without losing money
    BTL_SETUP_FLAG_NO_BLACKOUT = 1 << 8,
    // No experience and no bonus money, the foes are named as the player's Pokémon are, and their faints have no
    // message
    BTL_SETUP_FLAG_200 = 1 << 9,
    // From the regulation's REGULATION_PARAM_IDLE_EFFECTS: the battle view's idle effects while a message waits
    BTL_SETUP_FLAG_MSG_IDLE_EFFECTS = 1 << 10,
    // And its idle effects that start after 300 frames rather than 900
    BTL_SETUP_FLAG_SHORT_IDLE_WAIT = 1 << 11,
    // A link battle keeps the players' text speed and battle scene settings rather than fast text without effects
    BTL_SETUP_FLAG_PLAYER_CONFIG = 1 << 12,
    // One of N's Pokémon (FieldEncount_SetupNPoke), with its own encounter message
    BTL_SETUP_FLAG_N_POKEMON = 1 << 13,
    // An encounter in rustling grass, a dust cloud or rippling water (ENCOUNT_MODE_EFFECT), which a Funfest mission
    // counts (GameBeaconSys_SendDefeatMission)
    BTL_SETUP_FLAG_EFFECT_ENCOUNT = 1 << 14,
    // A Pokémon disguised as an item, as the scripts' Foongus and Amoonguss are, which a Funfest mission counts
    BTL_SETUP_FLAG_DISGUISED = 1 << 15,
    // Poké Balls can't be thrown, because of Ghetsis's device
    BTL_SETUP_FLAG_NO_BALLS = 1 << 16,
};

struct BtlSetup {
    u32 battleType;
    u32 battleStyle;
    BtlFieldSituation fieldSituation;
    // The parties of the four clients
    PokeParty *party[4];
    // The players' infos, by client
    PlayerInfo *playerInfo[4];
    // The effect of each client's Chatter recording (GetClientChatterEffect)
    u8 chatterEffects[4];
    // The trainers of the four clients
    BtlSetupTrainer *trainers[4];
    // The trainers' names, by client
    StrBuf *trainerNames[4];
    // The opponent's lines when it loses and when it wins, for a trainer ID from 0x800 (a Pokémon World Tournament
    // entrant's, wbt_battle.c), which BtlString_LoadTrainerWinLoseMsg shows
    StrBuf *trainerLoseMsg;
    StrBuf *trainerWinMsg;
    GameData *gameData;
    Config *config;
    BagSave *bag;
    BagCursor *bagCursor;
    PokeDexSave *pokedex;
    GameRecords *records;
    CommPlayerSupport *commSupport;
    // The regulation's time limits of the battle and of each turn's command, in seconds
    u16 gameTimeLimit;
    u16 commandTimeLimit;
    // The Shooter items that can't be used, and whether the Shooter can be, from the regulation
    // (BtlSetup_ApplyRegulation)
    RegulationShooterSettings shooterSettings;
    // The player's badges
    u8 badgeCount;
    u8 unk99[3];
    // BtlSetup_SetFlag sets these
    u32 flags;
    // The clients BtlNet_Init links in a battle over the network (BtlSetup_SetNetImpl sets 0xff)
    u16 netClientMask;
    u16 unkA2;
    // The money the player wins, or negative loses
    u32 moneyChange;
    // How the battle ended, a BTL_RESULT_*
    u32 result;
    // The slot of the caught Pokémon in the second party, or 6 if none was caught
    u8 capturedSlot;
    u8 commonVersion;
    u8 unkAE;
    u8 unkAF;
    void *recBuffer;
    u32 recSize;
    // The battle's random state, which the main module copies back
    MATHRandContext32 rand;
    // What the battle stores for the Battle Institute's score (BtlMain_StoreResultStats): the HP left in the
    // player's party, the turns, the player's switches, the hits of no effect, super effective and not very
    // effective by the player's Pokémon and by the foes', whether the player won, the foes' and the player's
    // fainted Pokémon, and the kinds of moves the player's Pokémon used
    u16 playerHp;
    u8 turnCount;
    u8 switchCount;
    u8 playerImmuneHitCount;
    u8 playerSuperEffectiveHitCount;
    u8 playerNotVeryEffectiveHitCount;
    u8 foeImmuneHitCount;
    u8 foeNotVeryEffectiveHitCount;
    u8 won;
    u8 foeFaintedCount;
    u8 playerFaintedCount;
    u8 usedMoveKindCount;
    // Set when a link battle's check of the commands found an error (SetCmdCheckError)
    u8 cmdCheckError : 1;
    // Whether a played recording was read to its end on every client (BtlMain_IsRecReadComplete)
    u8 recReadComplete : 1;
    // Shows the other players' names as the hidden trainer name
    u8 hideOpponentNames : 1;
    // 1 for a Pokéstar Studios movie's battle and 2 for one loaded from its record (GetStudioMode)
    u8 studioMode : 2;
    // The bag can't be used, as in the Infinity Dungeon (IsBagEnabled)
    u8 bagDisabled : 1;
    u8 unkDD_6 : 1;
    // A Pokéstar Studios movie not yet watched, which can't be skipped (IsStudioSkipDisabled)
    u8 studioSkipDisabled : 1;
    // Whether the battle doesn't register the foes in the Pokédex (RegisterDexMon)
    u8 noPokedexRegistration : 1;
    u8 unkDE_1 : 7;
    // The Pokéstar Studios movie (GetStudioMovieId)
    u8 studioMovie;
    // Pokéstar Studios: 1 when the studio gives the cast, 0 when the player's own Pokémon act
    u8 studioCast;
    // Whether each party slot's Pokémon appeared in the battle
    u8 appeared[6];
    // The state of each client's Pokémon at the end, by party slot: 0 well, 1 with a status condition, 2 fainted
    u8 monStatus[4][6];
    u8 unkFF;
    // Each client's remaining HP, in percent of its party's total
    u32 hpPercent[4];
    // The choice made in each scene of a Pokéstar Studios movie, which its playback shows again
    u8 studioChoices[0x14];
    // How a Pokéstar Studios movie ended, which picks its ending music
    u32 studioEnding;
    // The size of a Pokéstar Studios movie's audience, which event_pokewood.c draws from the grade
    u8 audienceCount;
    // The player's grade at the studio (PokewoodSave_GetLevelOfPoints)
    u8 studioGrade;
    // One bit per member of the audience still in it
    u32 audience;
    // The Pokéstar Studios movie's score
    s32 studioScore;
    // Set when the player quits the playback of a recording (SetRecPlayQuit)
    u32 recPlayQuit;
    // The music of a Pokéstar Studios movie's good and strange endings (GetStudioEndingBGM0 and 2)
    u16 studioGoodEndingBgm;
    u16 studioStrangeEndingBgm;
    // Added to the defeated mon's level for the experience it gives
    s8 levelDiff;
};

BtlSetup *BtlSetup_Create(HeapID heapId);
u32 BtlSetup_CheckFlag(BtlSetup *setup, u32 flag);
void BtlSetup_Reset(BtlSetup *setup);
// Wild battles, against the party, alone or beside the trainer
void BtlSetup_SetWildNormal(BtlSetup *setup, GameData *gameData, PokeParty *party, BtlFieldStatus *status,
                            BOOL doubleBattle, HeapID heapId);
void BtlSetup_SetWildMulti(BtlSetup *setup, GameData *gameData, PokeParty *party, u32 trainerId, BtlFieldStatus *status,
                           HeapID heapId);
void BtlSetup_SetFlag(BtlSetup *setup, u32 flag);
void BtlSetup_Free(BtlSetup *setup);
PokeParty *BtlSetup_GetParty(BtlSetup *setup, u32 index);
// Battles over the network: commMode is the adapters' mode (BtlAdapter_InitSystem, 0 for a battle alone) and
// clientId the player's client, its net ID where it isn't given
void BtlSetup_SetNet1v1Double(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 commMode, HeapID heapId);
void BtlSetup_SetNet1v1Single(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 commMode, HeapID heapId);
void BtlSetup_SetNetRotation(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 commMode, HeapID heapId);
void BtlSetup_SetNetTriple(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 commMode, HeapID heapId);
void BtlSetup_SetNetMultiVsNet(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 commMode, u8 clientId,
                               HeapID heapId);
// Battles against trainers, in the surroundings that SaveBtlFieldStatus saved
void BtlSetup_SetTrainer1v1Single(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 trainerId,
                                  HeapID heapId);
void BtlSetup_SetTrainer1v1Double(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 trainerId,
                                  HeapID heapId);
void BtlSetup_SetTrainer2v2(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 partnerId, u32 trainerId,
                            u32 trainerId2, HeapID heapId);
void BtlSetup_SetTrainer1v2(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 trainerId, u32 trainerId2,
                            HeapID heapId);
void BtlSetup_SetTrainer3v3(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 trainerId, HeapID heapId);
void BtlSetup_SetTrainerRotation(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 trainerId,
                                 HeapID heapId);
// A multi battle of two linked players against trainers
void BtlSetup_SetNetMultiVsAI(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 commMode, u8 clientId,
                              u32 trainerId, u32 trainerId2, HeapID heapId);
void BtlSetup_PostProcessTrialHouse(BtlSetup *setup);
// The capture demonstration, between the two parties
void BtlSetup_SetCaptureDemo(BtlSetup *setup, GameData *gameData, PokeParty *party, PokeParty *enemyParty,
                             BtlFieldStatus *status, HeapID heapId);
// Changes the levels of the parties for the challenge mode of the zone, which the keys of Unova Link unlock
void adjustPkmLvForChallengeKeys(BtlSetup *setup, GameData *gameData, u16 zoneId);
// Frees what the setup holds and clears it
void BtlSetup_FreeContents(BtlSetup *setup);
void BtlSetup_SetParty(BtlSetup *setup, PokeParty *party, u32 client);
void BtlSetup_ApplyRegulation(BtlSetup *setup, Regulation *regulation, HeapID heapId);
// Gives a battle's trainer a name buffer and empty words
BtlSetupTrainer *TrainerBattleSetup_Create(HeapID heapId);
// A field status with the default background
void ResetBtlFieldStatus(BtlFieldStatus *status);
void BtlSetup_InitForRecord(BtlSetup *setup, GameData *gameData, HeapID heapId);
void BtlSetup_FreeForRecord(BtlSetup *setup);
void BtlSetup_AllocRecBuffer(BtlSetup *setup, HeapID heapId);
void BattleRec_Init(HeapID heapId);
void freeVSPlayerBlkClearPtr(void);

#endif // POKEBW2_BATTLE_BTL_SETUP_H
