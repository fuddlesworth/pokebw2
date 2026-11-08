#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_RES_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_RES_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "struct_decls.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The Battle Recorder's resources (br_res.c): the font, the messages, the BG and OBJ graphics, and the color the
// player chose for it

// The color of the Global Link's modes, which the player can't choose
#define BR_RES_COLOR_GLOBAL 1

// The cell actor resources that BrRes_LoadOBJ loads. The buttons are for the main screen and then the sub screen
enum {
    BR_RES_OBJ_BROWSE_BTN_M,
    BR_RES_OBJ_BROWSE_BTN_S,
    BR_RES_OBJ_MUSICAL_BTN_M,
    BR_RES_OBJ_MUSICAL_BTN_S,
    BR_RES_OBJ_SIDEBAR_M,
    BR_RES_OBJ_SIDEBAR_S,
    BR_RES_OBJ_MAX = 14,
};

// The graphics resources of a cell actor, as they are loaded
typedef struct {
    u32 cell;
    u32 chr;
    u32 plt;
} BrResObjData;

// isUseColor is FALSE in the Global Link's modes, which have their own color
BrRes *BrRes_Init(u32 color, BOOL isUseColor, HeapID heapId);
void BrRes_Exit(BrRes *p_wk);
void BrRes_LoadBG(BrRes *p_wk, u32 bgID, HeapID heapId);
void BrRes_UnloadBG(BrRes *p_wk, u32 bgID);
void BrRes_LoadOBJ(BrRes *p_wk, u32 objID, HeapID heapId);
void BrRes_UnloadOBJ(BrRes *p_wk, u32 objID);
// Whether the resource is loaded, and if it is, its data
BOOL BrRes_GetObjData(const BrRes *p_wk, u32 objID, BrResObjData *p_data);
// The palettes of the player's color, for CLACT_VRAM_MAIN or CLACT_VRAM_SUB
void BrRes_LoadCommonPltt(BrRes *p_wk, u32 vramType, HeapID heapId);
void BrRes_UnloadCommonPltt(BrRes *p_wk, u32 vramType);
Font *BrRes_GetFont(const BrRes *p_wk);
MsgData *BrRes_GetMsgData(const BrRes *p_wk);
WordSet *BrRes_GetWordSet(const BrRes *p_wk);
// The fade color of the player's color
u16 BrRes_GetFadeColor(const BrRes *p_wk);
void BrRes_SetColor(BrRes *p_wk, u32 color);
u32 BrRes_GetColor(const BrRes *p_wk);
void BrRes_LoadColorPlttToFade(BrRes *p_wk, BrFade *fade, HeapID heapId);

#define BR_POKESEARCH_BG_LETTER 0x14

#define BR_POKESEARCH_BG_LIST 0x15

#define BR_RES_BG_MUSICAL_SEND 0x17

#define BR_RES_BG_MUSICAL_LOOK 0x16

#define BR_RES_OBJ_BTLSUBWAY_BP 13

enum {
    BR_RES_BG_COMMON_M = 0,
    BR_RES_BG_COMMON_S = 1,
    BR_RES_BG_TEXT = 2,
    BR_RES_BG_MENU_BROWSE = 3,
    BR_RES_BG_MENU_ONLINE = 4,
    BR_RES_BG_RECORD_CONTROLS = 5,
    BR_RES_BG_RECORD_INFO = 6,
    BR_RES_BG_RECORD_INFO_SMALL_TEAM = 7,
    BR_RES_BG_PROFILE = 8,
    BR_RES_BG_BTLSUBWAY_MENU = 9,
    BR_RES_BG_BTLSUBWAY_SINGLE = 10,
    BR_RES_BG_BTLSUBWAY_DOUBLE = 11,
    BR_RES_BG_BTLSUBWAY_MULTI = 12,
    BR_RES_BG_BTLSUBWAY_WIFI = 13,
    BR_RES_BG_BTLSUBWAY_TOP = 14,
    BR_RES_BG_RNDMATCH_MENU = 15,
    BR_RES_BG_RNDMATCH_RATING = 16,
    BR_RES_BG_RNDMATCH_RECORDS = 17,
    BR_RES_BG_BV_LIST = 24,
    BR_RES_BG_BV_SEARCH_MAIN = 25,
    BR_RES_BG_BV_SEARCH_MENU = 26,
    BR_RES_BG_BV_SEARCH_MODE = 27,
    BR_RES_BG_BV_SEARCH_AREA = 28,
    BR_RES_BG_CODEIN = 29,
    BR_RES_BG_CONFIRM = 30,
};

enum {
    BR_RES_OBJ_CONTROLS_S = 6,
    BR_RES_OBJ_SCROLL_ARROW_M = 7,
    BR_RES_OBJ_SCROLL_ARROW_S = 8,
    BR_RES_OBJ_BALL_M = 9,
    BR_RES_OBJ_BALL_S = 10,
    BR_RES_OBJ_CODEIN_CODE = 11,
    BR_RES_OBJ_CODEIN_CURSOR = 12,
};

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_RES_H
