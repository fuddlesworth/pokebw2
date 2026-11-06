#ifndef POKEBW2_APP_BAG_H
#define POKEBW2_APP_BAG_H

#include "types.h"
#include "field/player_action.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "save/bag.h"
#include "struct_decls.h"

// The bag, overlay 142 (itemmenu.c and bag_item.c)
#define OVERLAY_BAG OVERLAY_ID(142)

// The modes the bag is opened in
enum {
    BAG_MODE_NORMAL = 0,
    BAG_MODE_UNION_ROOM = 1,
    BAG_MODE_GIVE_ITEM = 2,
    BAG_MODE_COLOSSEUM = 3,
    BAG_MODE_SELL = 4,
    BAG_MODE_SELECT_ITEM = 5,
};

// What the player leaves the bag to do
enum {
    BAG_RESULT_CLOSE = 0,
    BAG_RESULT_BACK = 1,
    BAG_RESULT_GIVE_ITEM = 2,
    BAG_RESULT_SELECT_ITEM = 3,
    BAG_RESULT_USE_ITEM = 5,
    BAG_RESULT_TEACH_TM = 6,
    BAG_RESULT_TOWN_MAP = 7,
    BAG_RESULT_PAL_PAD = 8,
    BAG_RESULT_RIDE_BICYCLE = 10,
    BAG_RESULT_DISMOUNT_BICYCLE = 11,
    BAG_RESULT_EVOLVE_ITEM = 12,
    BAG_RESULT_ESCAPE_ROPE = 14,
    BAG_RESULT_HONEY = 15,
    BAG_RESULT_FISH = 16,
    BAG_RESULT_VS_RECORDER = 17,
    BAG_RESULT_GIVE_MAIL = 18,
    BAG_RESULT_READ_MAIL = 19,
    BAG_RESULT_DOWSING = 20,
    BAG_RESULT_XTRANSCEIVER = 21,
    BAG_RESULT_MEDAL_BOX = 22,
};

// What the bag is started with, 0x4c bytes that BagParam_Create (bag_param.c) allocates
struct BagProcessData {
    GameData *gameData;
    TrainerDataSave *trainerData;
    PlayerInfo *playerInfo;
    // The bag's cursor in each pocket, which BagCursor_GetPocketPosition and the functions after it read and set
    BagCursor *cursor;
    // The Free Space's filter
    u32 freeSpaceFilter;
    BagSave *bag;
    PlayerActionPerms perms;
    u32 mode;
    BOOL isCycling;
    // Whether the Dowsing MCHN is on the lower screen
    BOOL dowsingActive;
    // What the player did, 0 for nothing
    u32 result;
    u32 item;
};

extern const GameProcFunctions BAG_PROC_FUNCTIONS;

// Creates the bag's data for a mode, in the main module, with a copy of the perms when they are given
BagProcessData *BagParam_Create(GameData *gameData, const PlayerActionPerms *perms, u32 mode, HeapID heapId);

#endif // POKEBW2_APP_BAG_H
