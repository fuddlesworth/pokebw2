#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_BTN_DATA_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_BTN_DATA_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Battle Recorder's menus and their buttons (br_btn_data.c)

// The menus. Five belong to the Battle Recorder in the bag, five to the Global Link's Battle Videos and two to its
// musical photos
enum {
    BR_MENUID_BROWSE_TOP,
    BR_MENUID_BROWSE_RECORD = 1,
    BR_MENUID_BROWSE_OTHER = 2,
    BR_MENUID_BROWSE_DELETE = 3,
    BR_MENUID_BROWSE_DELETE_OTHER = 4,
    BR_MENUID_GBV_TOP = 5,
    BR_MENUID_GBV_BROWSE = 6,
    BR_MENUID_GBV_LIST = 7,
    BR_MENUID_GBV_UPLOAD_CONFIRM = 8,
    BR_MENUID_GBV_EXIT_CONFIRM = 9,
    BR_MENUID_MUSICAL_TOP = 10,
    BR_MENUID_MUSICAL_EXIT_CONFIRM = 11,
    BR_MENUID_MAX = 12,
};

// The parameters of a button
enum {
    BR_BTN_DATA_PARAM_X,
    BR_BTN_DATA_PARAM_Y,
    // The menu the button is in
    BR_BTN_DATA_PARAM_MENUID,
    BR_BTN_DATA_PARAM_MSGID,
    BR_BTN_DATA_PARAM_ANMSEQ,
    // BR_BTN_TYPE_*
    BR_BTN_DATA_PARAM_TYPE,
    // The menu or screen the button opens
    BR_BTN_DATA_PARAM_DATA1,
    // A message the button shows, or the mode of its screen
    BR_BTN_DATA_PARAM_DATA2,
    // Whether the button can be pressed
    BR_BTN_DATA_PARAM_VALID,
    // What a button that can't be pressed does: 1 shows the message BR_BTN_DATA_PARAM_UNVALID_DATA
    BR_BTN_DATA_PARAM_UNVALID_TYPE,
    BR_BTN_DATA_PARAM_UNVALID_DATA,
    // BR_BTN_VALID_TYPE_*
    BR_BTN_DATA_PARAM_VALID_TYPE,
    BR_BTN_DATA_PARAM_MAX,
};

// What a button does when pressed
enum {
    // Opens the menu BR_BTN_DATA_PARAM_DATA1
    BR_BTN_TYPE_SELECT,
    // Opens the menu BR_BTN_DATA_PARAM_DATA1 and shows the message BR_BTN_DATA_PARAM_DATA2
    BR_BTN_TYPE_MENU,
    // Returns to the menu before
    BR_BTN_TYPE_RETURN,
    // Leaves the Battle Recorder
    BR_BTN_TYPE_EXIT,
    // Starts the screen BR_BTN_DATA_PARAM_DATA1
    BR_BTN_TYPE_CHANGE_DISPLAY,
};

// What decides whether a button can be pressed: nothing, a saved video, or a saved musical photo
enum {
    BR_BTN_VALID_TYPE_NONE,
    BR_BTN_VALID_TYPE_MINE,
    BR_BTN_VALID_TYPE_OTHER1,
    BR_BTN_VALID_TYPE_OTHER2,
    BR_BTN_VALID_TYPE_OTHER3,
    BR_BTN_VALID_TYPE_MUSICAL,
};

typedef struct {
    u16 param[BR_BTN_DATA_PARAM_MAX];
} BrBtnData;

typedef struct {
    BrRecordInfo *recordInfo;
} BrBtnDataSetup;

BrBtnDataSys *BrBtnData_Init(const BrBtnDataSetup *cp_setup, HeapID heapId);
void BrBtnData_Exit(BrBtnDataSys *p_wk);
const BrBtnData *BrBtnData_GetData(const BrBtnDataSys *cp_wk, int menuID, u16 btnID);
// The number of buttons of a menu, and the most any menu has
u16 BrBtnData_GetNum(const BrBtnDataSys *cp_wk, int menuID);
u32 BrBtnData_GetMaxNum(const BrBtnDataSys *cp_wk);
u16 BrBtnData_GetParam(const BrBtnData *cp_data, int paramID);
// The button's label, with the player's name for a saved video
StrBuf *BrBtnData_CreateStr(const BrBtnDataSys *cp_wk, const BrBtnData *cp_data, MsgData *msg, HeapID heapId);

enum {
    BR_BTN_INVALID_TYPE_NONE = 0,
    BR_BTN_INVALID_TYPE_INFO = 1,
};

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_BTN_DATA_H
