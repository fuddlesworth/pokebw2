#ifndef POKEBW2_APP_WORLDTRADE_H
#define POKEBW2_APP_WORLDTRADE_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Global Trade Station
#define OVERLAY_WORLDTRADE OVERLAY_ID(214)

// worldtrade_data, systemdata, myparty, mybox, wifilist, wifihistory, mystatus, config, record and savedata keep the
// names that worldtrade.c's asserts print. config is the options save block, which TrainerDataSave names here
typedef struct {
    WorldTradeData *worldtrade_data;
    AdventureSave *systemdata;
    PokeParty *myparty;
    BoxSaveAccessor *mybox;
    PokeDexSave *pokedex;
    WifiList *wifilist;
    UnityTowerSurveySave *wifihistory;
    PlayerInfo *mystatus;
    TrainerDataSave *config;
    GameRecords *record;
    BagSave *bag;
    BOOL isNationalDex;
    s32 profileId;
    u32 unk34;
    u32 unk38;
    SaveControl *savedata;
    GameSystem *gsys;
} WorldTradeParam;

extern const GameProcFunctions WORLDTRADE_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WORLDTRADE_H
