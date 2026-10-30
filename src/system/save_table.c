#include "types.h"
#include "field/field_actor.h"
#include "field/unity_tower.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/adventure.h"
#include "save/bag.h"
#include "save/battle_examination.h"
#include "save/battle_rec.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/c_gear_savedata.h"
#include "save/cgear_picture_save.h"
#include "save/chatter.h"
#include "save/cygnus_save_control.h"
#include "save/dream_world.h"
#include "save/encounter.h"
#include "save/event_work.h"
#include "save/extra_block_savedata.h"
#include "save/field_actor_savedata.h"
#include "save/funfest_savedata.h"
#include "save/gimmick_savedata.h"
#include "save/hall_of_fame.h"
#include "save/high_link.h"
#include "save/join_avenue.h"
#include "save/key_data_savedata.h"
#include "save/medal_box.h"
#include "save/misc_savedata.h"
#include "save/musical_dist_save.h"
#include "save/musical_savedata.h"
#include "save/mystery_gift.h"
#include "save/player_savedata.h"
#include "save/pokedex_savedata.h"
#include "save/pokedex_skin_save.h"
#include "save/pokewood.h"
#include "save/records.h"
#include "save/resh_zek.h"
#include "save/save_control.h"
#include "save/save_table.h"
#include "save/survey_savedata.h"
#include "save/symbol_save.h"
#include "save/timesig_savedata.h"
#include "save/tournament_block.h"
#include "save/traded_pokemon.h"
#include "save/trainer_data_savedata.h"
#include "save/trainer_game_info.h"
#include "save/trainer_log.h"
#include "save/trial_house.h"
#include "save/unity_tower_floor_savedata.h"
#include "save/wifi_list.h"
#include "save/worldtrade_data.h"
#include "system/pms_word.h"

// The tables of the saves' blocks: the main save's, and the one block of each extra save. The file has no code and no
// name in the ROM; save_table.c is a guess. That it is a file apart from save_control.c shows in the order of its
// tables, which MWCC only gives when the file defines nothing else.

// Where the saves are on the card, and the magic number in their footers
#define SAVE_MAIN_ADDR 0
#define SAVE_BACKUP_ADDR 0x26000
#define SAVE_SIZE 0x26000
#define SAVE_MAGIC 0x31053527

typedef void (*SaveBlockInit)(void *block);

#ifdef POKEBW2_PC
// Typed initializers keep their public APIs; the save table calls C-compatible void-pointer adapters.
#define PC_SAVE_INIT(name)                                                                                             \
    static void name##_Callback(void *block) {                                                                         \
        name(block);                                                                                                   \
    }
PC_SAVE_INIT(BoxMainSave_InitBlock)
PC_SAVE_INIT(BagSave_Init)
PC_SAVE_INIT(PokeParty_Init)
PC_SAVE_INIT(TrainerDataSave_Init)
PC_SAVE_INIT(PlayerSave_Init)
PC_SAVE_INIT(UnityTower_InitSurveySave)
PC_SAVE_INIT(initWifi)
PC_SAVE_INIT(PalPadFriendList_Init)
PC_SAVE_INIT(CGearSave_Init)
PC_SAVE_INIT(TimeSigSave_Init)
PC_SAVE_INIT(MysteryGiftSaveData_Init)
PC_SAVE_INIT(DreamWorldSave_Init)
PC_SAVE_INIT(initChatotChatter)
PC_SAVE_INIT(initAdventureDataBlk)
PC_SAVE_INIT(initTrainerCardInfoBlk)
PC_SAVE_INIT(PMSWordSave_Init)
PC_SAVE_INIT(MailBox_Clear)
PC_SAVE_INIT(ActorSave_Init)
PC_SAVE_INIT(MusicalSave_Init)
PC_SAVE_INIT(ReshZekSave_Init)
PC_SAVE_INIT(TrainerLogSave_Init)
PC_SAVE_INIT(EventWork_Init)
PC_SAVE_INIT(WorldTradeData_Init)
PC_SAVE_INIT(GimmickSave_Init)
PC_SAVE_INIT(BattleBoxSave_Init)
PC_SAVE_INIT(setEggArea)
PC_SAVE_INIT(StrengthRockSave_Init)
PC_SAVE_INIT(TrainerGameInfo_Init)
PC_SAVE_INIT(probability)
PC_SAVE_INIT(initPokedex)
PC_SAVE_INIT(EncountSave_Init)
PC_SAVE_INIT(BSubwayPlayData_Init)
PC_SAVE_INIT(BSubwayScoreData_Init)
PC_SAVE_INIT(BSubwayWifiData_Init)
PC_SAVE_INIT(RecordSave_Init)
PC_SAVE_INIT(initNpcData)
PC_SAVE_INIT(SurveySave_Init)
PC_SAVE_INIT(UnityTowerSave_Init)
PC_SAVE_INIT(TrialHouseSave_Init)
PC_SAVE_INIT(HallOfFameTeam_Init)
PC_SAVE_INIT(TradedPokemonSave_Init)
PC_SAVE_INIT(clearHollowRivalData)
PC_SAVE_INIT(JoinAvenueSave_Init)
PC_SAVE_INIT(MedalBox_Init)
PC_SAVE_INIT(KeyDataSave_Init)
PC_SAVE_INIT(FestRecords_Init)
PC_SAVE_INIT(PokewoodSave_Init)
PC_SAVE_INIT(initDreamRadarSave)
PC_SAVE_INIT(clearVSPlayerBlk)
PC_SAVE_INIT(CGearPictureSave_Init)
PC_SAVE_INIT(BattleExamination_Init)
PC_SAVE_INIT(HallOfFameSave_Init)
PC_SAVE_INIT(clearPokewoodBlock)
#undef PC_SAVE_INIT
#define SAVE_INIT(name) name##_Callback
#else
#define SAVE_INIT(name) (SaveBlockInit) name
#endif

static const SaveBlockDef SAVEDATA_INIT[SAVE_BLOCK_MAX] = {
    { SAVE_BLOCK_BOX_MAIN, BoxMainSave_GetBlockSize, SAVE_INIT(BoxMainSave_InitBlock) },
    { SAVE_BLOCK_BOX_TRAY + 0, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 1, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 2, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 3, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 4, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 5, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 6, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 7, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 8, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 9, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 10, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 11, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 12, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 13, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 14, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 15, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 16, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 17, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 18, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 19, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 20, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 21, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 22, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 23, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BAG, BagSave_GetBlockSize, SAVE_INIT(BagSave_Init) },
    { SAVE_BLOCK_PARTY, PokeParty_GetSaveDataSize, SAVE_INIT(PokeParty_Init) },
    { SAVE_BLOCK_TRAINER_DATA, getSizeofTrainerData, SAVE_INIT(TrainerDataSave_Init) },
    { SAVE_BLOCK_PLAYER, PlayerSave_GetBlockSize, SAVE_INIT(PlayerSave_Init) },
    { SAVE_BLOCK_UNITY_TOWER_SURVEY, UnityTower_GetSurveySaveSize, SAVE_INIT(UnityTower_InitSurveySave) },
    { SAVE_BLOCK_WIFI_LIST, getSizeofWifiData, SAVE_INIT(initWifi) },
    { SAVE_BLOCK_PAL_PAD, PalPadFriendList_GetSize, SAVE_INIT(PalPadFriendList_Init) },
    { SAVE_BLOCK_CGEAR, CGearSave_GetSize, SAVE_INIT(CGearSave_Init) },
    { SAVE_BLOCK_TIMESIG, TimeSigSave_GetSize, SAVE_INIT(TimeSigSave_Init) },
    { SAVE_BLOCK_MYSTERY_GIFT, MysteryGiftSaveData_GetSize, SAVE_INIT(MysteryGiftSaveData_Init) },
    { SAVE_BLOCK_DREAM_WORLD, DreamWorldSave_GetSize, SAVE_INIT(DreamWorldSave_Init) },
    { SAVE_BLOCK_CHATTER, PerapVoice_GetSize, SAVE_INIT(initChatotChatter) },
    { SAVE_BLOCK_ADVENTURE, getSizeofAdventureDataBlk, SAVE_INIT(initAdventureDataBlk) },
    { SAVE_BLOCK_RECORDS, GameRecords_GetSize, SAVE_INIT(initTrainerCardInfoBlk) },
    { SAVE_BLOCK_PMS, PMSWordSave_GetSize, SAVE_INIT(PMSWordSave_Init) },
    { SAVE_BLOCK_MAIL, MailBox_GetSize, SAVE_INIT(MailBox_Clear) },
    { SAVE_BLOCK_FIELD_ACTORS, ActorSave_GetBlockSize, SAVE_INIT(ActorSave_Init) },
    { SAVE_BLOCK_MUSICAL, MusicalSave_GetSize, SAVE_INIT(MusicalSave_Init) },
    { SAVE_BLOCK_RESH_ZEK, ReshZekSave_GetSize, SAVE_INIT(ReshZekSave_Init) },
    { SAVE_BLOCK_TRAINER_LOG, TrainerLogSave_GetSize, SAVE_INIT(TrainerLogSave_Init) },
    { SAVE_BLOCK_EVENT_WORK, EventWork_GetBlockSize, SAVE_INIT(EventWork_Init) },
    { SAVE_BLOCK_WORLDTRADE, WorldTradeData_GetBlockSize, SAVE_INIT(WorldTradeData_Init) },
    { SAVE_BLOCK_TOURNAMENT, getSizeofTourneyBlock, clearTournamentBlock },
    { SAVE_BLOCK_GIMMICK, GimmickSave_GetBlockSize, SAVE_INIT(GimmickSave_Init) },
    { SAVE_BLOCK_BATTLE_BOX, BattleBoxSave_GetSize, SAVE_INIT(BattleBoxSave_Init) },
    { SAVE_BLOCK_DAY_CARE, getSizeofBreedingBlock, SAVE_INIT(setEggArea) },
    // The only block whose functions are an overlay's, the field's (overlay 12)
    { SAVE_BLOCK_STRENGTH_ROCKS, StrengthRockSave_GetSize, SAVE_INIT(StrengthRockSave_Init) },
    { SAVE_BLOCK_TRAINER_GAME_INFO, TrainerGameInfo_GetSize, SAVE_INIT(TrainerGameInfo_Init) },
    { SAVE_BLOCK_HIGH_LINK, getSizeofProbabilityTable, SAVE_INIT(probability) },
    { SAVE_BLOCK_POKEDEX, getSizeofPokedexData, SAVE_INIT(initPokedex) },
    { SAVE_BLOCK_ENCOUNT, EncountSave_GetBlockSize, SAVE_INIT(EncountSave_Init) },
    { SAVE_BLOCK_BSUBWAY_PLAY, BSubwayPlayData_GetSize, SAVE_INIT(BSubwayPlayData_Init) },
    { SAVE_BLOCK_BSUBWAY_SCORE, BSubwayScoreData_GetSize, SAVE_INIT(BSubwayScoreData_Init) },
    { SAVE_BLOCK_BSUBWAY_3A, BSubwayWifiData_GetSize, SAVE_INIT(BSubwayWifiData_Init) },
    { SAVE_BLOCK_RECORD, RecordSave_GetSize, SAVE_INIT(RecordSave_Init) },
    { SAVE_BLOCK_SYMBOL, getSizeofNpcData, SAVE_INIT(initNpcData) },
    { SAVE_BLOCK_SURVEY, SurveySave_GetSize, SAVE_INIT(SurveySave_Init) },
    { SAVE_BLOCK_UNITY_TOWER_FLOOR, UnityTowerSave_GetBlockSize, SAVE_INIT(UnityTowerSave_Init) },
    { SAVE_BLOCK_TRIAL_HOUSE, TrialHouseSave_GetSize, SAVE_INIT(TrialHouseSave_Init) },
    { SAVE_BLOCK_40, HallOfFameTeam_GetSize, SAVE_INIT(HallOfFameTeam_Init) },
    { SAVE_BLOCK_TRADED_POKEMON, TradedPokemonSave_GetSize, SAVE_INIT(TradedPokemonSave_Init) },
    { SAVE_BLOCK_MISC, getSizeofHollow_RivalData, SAVE_INIT(clearHollowRivalData) },
    { SAVE_BLOCK_JOIN_AVENUE, JoinAvenueSave_GetSize, SAVE_INIT(JoinAvenueSave_Init) },
    { SAVE_BLOCK_MEDAL_BOX, MedalBox_GetBlockSize, SAVE_INIT(MedalBox_Init) },
    { SAVE_BLOCK_KEY_DATA, KeyDataSave_GetSize, SAVE_INIT(KeyDataSave_Init) },
    { SAVE_BLOCK_FUNFEST, FestRecords_GetSize, SAVE_INIT(FestRecords_Init) },
    { SAVE_BLOCK_POKEWOOD, PokewoodSave_GetSize, SAVE_INIT(PokewoodSave_Init) },
    { SAVE_BLOCK_DREAM_RADAR, getDreamRadarSaveSize, SAVE_INIT(initDreamRadarSave) },
};

// The one block of each extra save
static const SaveBlockDef sExtraBattleRec0[] = {
    { 0, getSizeOfVSPlayer, SAVE_INIT(clearVSPlayerBlk) },
};

static const SaveBlockDef sExtraBattleRec1[] = {
    { 0, getSizeOfVSPlayer, SAVE_INIT(clearVSPlayerBlk) },
};

static const SaveBlockDef sExtraBattleRec2[] = {
    { 0, getSizeOfVSPlayer, SAVE_INIT(clearVSPlayerBlk) },
};

static const SaveBlockDef sExtraBattleRec3[] = {
    { 0, getSizeOfVSPlayer, SAVE_INIT(clearVSPlayerBlk) },
};

static const SaveBlockDef sExtraCGearPicture[] = {
    { 0, CGearPictureSave_GetSize, SAVE_INIT(CGearPictureSave_Init) },
};

static const SaveBlockDef sExtraBattleExamination[] = {
    { 0, BattleExamination_GetSize, SAVE_INIT(BattleExamination_Init) },
};

static const SaveBlockDef sExtraMusicalDist[] = {
    { 0, MusicalDist_GetSaveSize, MusicalDist_InitBlock },
};

static const SaveBlockDef sExtraPokedexSkin[] = {
    { 0, PokedexSkinSave_GetSize, PokedexSkinSave_Init },
};

static const SaveBlockDef sExtraHallOfFame[] = {
    { 0, HallOfFameSave_GetSize, SAVE_INIT(HallOfFameSave_Init) },
};

static const SaveBlockDef sExtraPokewood[] = {
    { 0, getSizeOfPokewoodBlock, SAVE_INIT(clearPokewoodBlock) },
};

static const SaveBlockDef sExtraTournament0[] = {
    { 0, WbtDownloadData_GetSize, WbtDownloadData_Clear },
};

static const SaveBlockDef sExtraTournament1[] = {
    { 0, WbtDownloadData_GetSize, WbtDownloadData_Clear },
};

static const SaveBlockDef sExtraTournament2[] = {
    { 0, WbtDownloadData_GetSize, WbtDownloadData_Clear },
};

static const SaveBlockDef sExtraBlock20[] = {
    { 0, func_020103f4, func_020103f8 },
};

const SaveDataParams save_block_data = {
    SAVEDATA_INIT, SAVE_BLOCK_MAX, SAVE_MAIN_ADDR, SAVE_BACKUP_ADDR, SAVE_SIZE, SAVE_MAGIC,
};

// The extra saves follow the main save's two copies. Only the Hall of Fame has a backup copy
const SaveDataParams sv_extra_blk_data[SAVE_EXTRA_ID_MAX] = {
    { sExtraBattleRec0, 1, 0x4c000, 0x4c000, 0x1a00, SAVE_MAGIC },
    { sExtraBattleRec1, 1, 0x4da00, 0x4da00, 0x1a00, SAVE_MAGIC },
    { sExtraBattleRec2, 1, 0x4f400, 0x4f400, 0x1a00, SAVE_MAGIC },
    { sExtraBattleRec3, 1, 0x50e00, 0x50e00, 0x1a00, SAVE_MAGIC },
    { sExtraCGearPicture, 1, 0x52800, 0x52800, 0x2800, SAVE_MAGIC },
    { sExtraBattleExamination, 1, 0x55000, 0x55000, 0x800, SAVE_MAGIC },
    { sExtraMusicalDist, 1, 0x55800, 0x55800, 0x18000, SAVE_MAGIC },
    { sExtraPokedexSkin, 1, 0x6d800, 0x6d800, 0x6800, SAVE_MAGIC },
    { sExtraHallOfFame, 1, 0x74000, 0x75800, 0x1800, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x77000, 0x77000, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x77600, 0x77600, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x77c00, 0x77c00, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x78200, 0x78200, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x78800, 0x78800, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x78e00, 0x78e00, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x79400, 0x79400, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x79a00, 0x79a00, 0x600, SAVE_MAGIC },
    { sExtraTournament0, 1, 0x7a000, 0x7a000, 0x1400, SAVE_MAGIC },
    { sExtraTournament1, 1, 0x7b400, 0x7b400, 0x1400, SAVE_MAGIC },
    { sExtraTournament2, 1, 0x7c800, 0x7c800, 0x1400, SAVE_MAGIC },
    { sExtraBlock20, 1, 0x7dc00, 0x7dc00, 0x400, SAVE_MAGIC },
};
