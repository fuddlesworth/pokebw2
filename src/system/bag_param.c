#include "types.h"
#include "app/bag/bag.h"
#include "field/player/player_action.h"
#include "field/player/player_state.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "system/game_data.h"
#include "system/savedata/save_control.h"

// The bag's parameters. Our names

BagProcessData *BagParam_Create(GameData *gameData, const PlayerActionPerms *perms, u32 mode, HeapID heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    BagProcessData *param = GFL_HeapAllocate(heapId, sizeof(BagProcessData), TRUE, "bag_param.c", 57);

    param->gameData = gameData;
    param->trainerData = getTrainerDataBlkAddress(save);
    param->playerInfo = GetGameDataPlayerInfo(gameData);
    param->cursor = GameData_GetBagCursor(gameData);
    param->bag = GameData_GetBag(gameData);
    param->freeSpaceFilter = BagCursor_GetFreeSpaceFilter(param->cursor);
    if (perms != NULL) {
        MI_CpuCopy8(perms, &param->perms, sizeof(PlayerActionPerms));
    }
    param->mode = mode;
    param->isCycling = FieldPlayerState_GetExState(GameData_GetPlayerState(gameData)) == FLD_PLAYER_EXSTATE_CYCLING;
    param->dowsingActive = GameData_GetLastSubscreen(gameData) == 6;
    return param;
}
