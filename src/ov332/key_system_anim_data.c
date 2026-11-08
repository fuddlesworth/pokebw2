#include "app/key_system_flow.h"
#include "text/system/ability_handlers_2.h"

// The key animations' data: the palette of each key, where the key, the plate and the key of an enabled setting
// move, and the names of the keys in each version's order. A data-only unit between key_system_net.c and
// key_system_flow.c; MWCC sorts its objects by size, and the order of the twelve positions among themselves comes
// from the declaration order through its heapsort (several orders give the ROM's; this is one). The ROM doesn't
// name the unit; the name is ours.
const KeySystemPos sKeyOutStart = { 184, 104 };
const KeySystemPos sShowInStart = { 128, -32 };
const KeySystemPos sKeyOutEnd = { 184, 122 };
const u8 sKeyPalettes[KEY_SYSTEM_KEY_COUNT] = { 2, 1, 1, 1, 2 };
const u16 sKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { AbilityHandlers2_Text_EasyMode, AbilityHandlers2_Text_ChallengeMode, AbilityHandlers2_Text_BlackCity,
      AbilityHandlers2_Text_IronChamber, AbilityHandlers2_Text_IcebergChamber },
    { AbilityHandlers2_Text_EasyMode, AbilityHandlers2_Text_ChallengeMode, AbilityHandlers2_Text_WhiteForest,
      AbilityHandlers2_Text_IronChamber, AbilityHandlers2_Text_IcebergChamber },
};
const u16 sReceivedKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { AbilityHandlers2_Text_EasyMode, AbilityHandlers2_Text_ChallengeMode, AbilityHandlers2_Text_BlackCity,
      AbilityHandlers2_Text_IronChamber, AbilityHandlers2_Text_IcebergChamber },
    { AbilityHandlers2_Text_EasyMode, AbilityHandlers2_Text_ChallengeMode, AbilityHandlers2_Text_WhiteForest,
      AbilityHandlers2_Text_IronChamber, AbilityHandlers2_Text_IcebergChamber },
};
const KeySystemPos sKeyInStart = { 184, -32 };
const KeySystemPos sKeyInEnd = { 184, 104 };
const KeySystemPos sShowInEnd = { 128, 104 };
const KeySystemPos sPlateUpStart = { 128, 208 };
const KeySystemPos sShowOutStart = { 128, 104 };
const KeySystemPos sPlateUpEnd = { 128, 114 };
const KeySystemPos sPlateDownEnd = { 128, 208 };
const KeySystemPos sShowOutEnd = { 128, 288 };
const KeySystemPos sPlateDownStart = { 128, 114 };
