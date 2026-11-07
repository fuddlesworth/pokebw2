#ifndef POKEBW2_FIELD_SCRCMD_RESORT_SHOP_H
#define POKEBW2_FIELD_SCRCMD_RESORT_SHOP_H

#include "types.h"
#include "struct_decls.h"

// The command modes; selection can list records or people.
enum {
    RESORT_SHOP_MODE_BUY = 0,
    RESORT_SHOP_MODE_SELECT = 1,
    RESORT_SHOP_MODE_REORDER = 2,
};

// Selection submodes. ALT selects the other prompt pair for the same list.
enum {
    RESORT_SHOP_SELECT_RECORD = 0,
    RESORT_SHOP_SELECT_RECORD_ALT = 1,
    RESORT_SHOP_SELECT_PERSON = 2,
    RESORT_SHOP_SELECT_PERSON_ALT = 3,
};

// Reordering submodes.
enum {
    RESORT_SHOP_REORDER_RECORDS = 0,
    RESORT_SHOP_REORDER_PEOPLE = 1,
};

// Camera placement: player area, current shop side, or the fixed positive-X preset.
enum {
    RESORT_SHOP_CAMERA_PLAYER_AREA = 0,
    RESORT_SHOP_CAMERA_SHOP_SIDE = 1,
    RESORT_SHOP_CAMERA_FIXED = 2,
};

// Script outputs; RESULT is the entry kind, or whether a reordered list changed.
enum {
    RESORT_SHOP_OUT_RESULT = 0,
    RESORT_SHOP_OUT_ITEM = 1,
    RESORT_SHOP_OUT_COUNT = 2,
    RESORT_SHOP_OUT_ENTRY_ID = 3,
    RESORT_SHOP_OUT_PRICE = 4,
};

// Established goods kinds: ordinary bag items and an item drawn from the raffle table.
enum {
    RESORT_SHOP_GOODS_ITEM = 0,
    RESORT_SHOP_GOODS_RANDOM_ITEM = 11,
};

// The selection was cancelled rather than accepted.
#define RESORT_SHOP_SELECTION_CANCEL 0xff

// Hides the item/model icon.
#define RESORT_SHOP_ICON_NONE 0xffff

// The script command of the Join Avenue's shops, overlay 60
BOOL Scrcmd_ResortShop_Open(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_RESORT_SHOP_H
