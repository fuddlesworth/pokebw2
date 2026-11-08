#ifndef POKEBW2_APP_BOX2_MAIN_H
#define POKEBW2_APP_BOX2_MAIN_H

#include "types.h"
#include "app/box2.h"
#include "app/name_entry.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "struct_decls.h"
#include "system/app_taskmenu.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The PC box's work and the functions of box2_main.c, which the sequences in box2_seq.c call. Names are ours

// The number of boxes once all are open
#define BOX2_TRAY_MAX 24
// Positions: a tray's 30 Pokémon, then the party's 6, then the boxes of the box list
#define BOX2_TRAY_POKE_MAX 30
#define BOX2_PARTY_POS 30
#define BOX2_BOXLIST_POS 36

// The directions of a box change
enum {
    BOX2_TRAY_SCROLL_L,
    BOX2_TRAY_SCROLL_R,
    BOX2_TRAY_SCROLL_NONE,
};

// The actors of the cursor and the item icon
#define BOX2_ACTOR_CURSOR 4
#define BOX2_ACTOR_ITEM_ICON 12

// The frames the item icon takes to move
#define BOX2_ITEMMOVE_CNT 8

// The frames a Pokémon icon takes to move
#define BOX2_POKEMOVE_CNT 8

// What is picked: nothing, a Pokémon or an item held by the keys (or a range being drawn by them), a range held, or a
// range being drawn by touch on the tray or on the party
enum {
    BOX2_PICK_NONE,
    BOX2_PICK_KEY,
    BOX2_PICK_RANGE_HELD,
    BOX2_PICK_TOUCH_TRAY,
    BOX2_PICK_TOUCH_PARTY,
};

// Why a Pokémon can't be put where it was dropped, or picked, which Box2Bmp_PrintDenyMsg tells for some
enum {
    BOX2_MOVE_ERR_NONE,
    BOX2_MOVE_ERR_BOX_FULL,
    BOX2_MOVE_ERR_MAIL,
    BOX2_MOVE_ERR_LAST_BATTLER,
    // "Eggs cannot be selected!": the Battle Box takes no Eggs, nor does the Dream World
    BOX2_MOVE_ERR_EGG,
    BOX2_MOVE_ERR_BATTLE_BOX_LOCKED,
    // "This Pokémon doesn't seem sleepy now.": a species that can't be sent to the Dream World
    BOX2_MOVE_ERR_NOT_SLEEPY,
    // The Pokémon moved as a group don't fit where they were dropped
    BOX2_MOVE_ERR_GROUP,
};

// Where a button's press animation is: on an actor or on the BG
enum {
    BOX2_BTN_ANM_MODE_OBJ,
    BOX2_BTN_ANM_MODE_BG,
};

// The Pokémon search's criteria, which the boxes' icons are filtered by
#define BOX2_INITIAL_TRAY_COUNT 8
#define BOX2_FIRST_EXPANSION_TRAY_COUNT 16

// How Pokemon or items are picked: menu, single quick move, or range.
enum {
    BOX2_MOVE_MODE_MENU = 0,
    BOX2_MOVE_MODE_SINGLE = 1,
    BOX2_MOVE_MODE_RANGE = 2,
};

// Icon placement context; the right party shifts its x coordinate.
enum {
    BOX2_ICON_POS_TRAY = 0,
    BOX2_ICON_POS_PARTY_LEFT = 1,
    BOX2_ICON_POS_PARTY_RIGHT = 2,
};

// Where to resume after the summary, Bag, name entry, or search.
enum {
    BOX2_SUBRETURN_MENU_TRAY = 0,
    BOX2_SUBRETURN_MENU_PARTY = 1,
    BOX2_SUBRETURN_MOVE_TRAY = 2,
    BOX2_SUBRETURN_MOVE_TRAY_HELD = 3,
    BOX2_SUBRETURN_MOVE_PARTY = 4,
    BOX2_SUBRETURN_MOVE_PARTY_HELD = 5,
};

// Indices into sSubProcs.
enum {
    BOX2_SUBPROC_SUMMARY = 0,
    BOX2_SUBPROC_BAG = 1,
    BOX2_SUBPROC_BOX_NAME = 2,
    BOX2_SUBPROC_SEARCH = 3,
};

// Indices into sYesNoFuncs; each question has its own Yes and No callbacks.
enum {
    BOX2_YESNO_TAKE_ITEM = 0,
    BOX2_YESNO_RELEASE = 1,
    BOX2_YESNO_PUT_ITEM_IN_BAG = 2,
    BOX2_YESNO_QUIT = 3,
    BOX2_YESNO_CONTINUE = 4,
    BOX2_YESNO_TUCK_IN = 5,
};

// Visibility and enabled state of Back, Close, Summary, and Search.
enum {
    BOX2_BUTTON_ENABLED = 0,
    BOX2_BUTTON_HIDDEN = 1,
    BOX2_BUTTON_DISABLED = 2,
};

// BGWinFrame entries created by Box2Frame_Create.
enum {
    BOX2_FRAME_MENU_FIRST = 0,
    BOX2_FRAME_MENU_SECOND = 1,
    BOX2_FRAME_MENU_THIRD = 2,
    BOX2_FRAME_MENU_FOURTH = 3,
    BOX2_FRAME_MENU_FIFTH = 4,
    BOX2_FRAME_MENU_LAST = 5,
    BOX2_FRAME_MESSAGE = 6,
    BOX2_FRAME_MARKINGS = 7,
    BOX2_FRAME_PARTY = 8,
    BOX2_FRAME_BOX_LIST = 9,
    BOX2_FRAME_PARTY_BUTTON = 10,
    BOX2_FRAME_BOX_LIST_BUTTON = 11,
};

// Window indices in sWinData; message variants differ in size.
enum {
    BOX2_WIN_TITLE = 0,
    BOX2_WIN_SPECIES = 1,
    BOX2_WIN_NICKNAME = 2,
    BOX2_WIN_LEVEL = 3,
    BOX2_WIN_GENDER = 4,
    BOX2_WIN_DEX_NUMBER = 5,
    BOX2_WIN_NATURE = 6,
    BOX2_WIN_ABILITY = 7,
    BOX2_WIN_ITEM = 8,
    BOX2_WIN_MOVES = 9,
    BOX2_WIN_MOVES_LABEL = 10,
    BOX2_WIN_NATURE_LABEL = 11,
    BOX2_WIN_ABILITY_LABEL = 12,
    BOX2_WIN_ITEM_LABEL = 13,
    BOX2_WIN_MENU_FIRST = 14,
    BOX2_WIN_MENU_SECOND = 15,
    BOX2_WIN_MENU_THIRD = 16,
    BOX2_WIN_MENU_FOURTH = 17,
    BOX2_WIN_MENU_FIFTH = 18,
    BOX2_WIN_MENU_LAST = 19,
    BOX2_WIN_MARK_CONFIRM = 20,
    BOX2_WIN_MARK_CANCEL = 21,
    BOX2_WIN_PARTY_BUTTON = 22,
    BOX2_WIN_BOX_LIST_BUTTON = 23,
    BOX2_WIN_MESSAGE = 24,
    BOX2_WIN_MESSAGE_TALL_NARROW = 25,
    BOX2_WIN_DEPOSIT_PROMPT = 26,
    BOX2_WIN_MESSAGE_TALL = 27,
};

// Whether a menu row has the forward arrow drawn by DrawButton.
enum {
    BOX2_MENU_ITEM_PLAIN = 0,
    BOX2_MENU_ITEM_ARROW = 1,
};

// Wallpaper category; sScenery*, sMisc*, and sSpecial* menus establish the values.
enum {
    BOX2_WALLPAPER_THEME_MENU = 0,
    BOX2_WALLPAPER_SCENERY_1 = 1,
    BOX2_WALLPAPER_SCENERY_2 = 2,
    BOX2_WALLPAPER_SCENERY_3 = 3,
    BOX2_WALLPAPER_MISC = 4,
    BOX2_WALLPAPER_SPECIAL_1 = 5,
    BOX2_WALLPAPER_SPECIAL_2 = 6,
};

// BoxSaveAccessor_CheckFlags set IDs for extra-wallpaper pages.
enum {
    BOX2_WALLPAPER_UNLOCK_SPECIAL_1 = 1,
    BOX2_WALLPAPER_UNLOCK_SPECIAL_2 = 2,
};

// Held-item filter, indexed by sItemCriterionMsgs.
enum {
    BOX2_SEARCH_ITEM_ANY = 0,
    BOX2_SEARCH_ITEM_WITHOUT = 1,
    BOX2_SEARCH_ITEM_WITH = 2,
};

// Gender filter: zero means any; other values are GENDER_* plus one.
enum {
    BOX2_SEARCH_GENDER_ANY = 0,
    BOX2_SEARCH_GENDER_MALE = 1,
    BOX2_SEARCH_GENDER_FEMALE = 2,
    BOX2_SEARCH_GENDER_UNKNOWN = 3,
};

// Whether range icons share one vector or have individual vectors.
enum {
    BOX2_POKE_MOVE_GROUP = 0,
    BOX2_POKE_MOVE_INDIVIDUAL = 1,
};

// Icon move action; HIDE_AFTER follows the scroll arrow and hides the proxy icon.
enum {
    BOX2_POKE_ICON_MOVE_NONE = 0,
    BOX2_POKE_ICON_MOVE_VISIBLE = 1,
    BOX2_POKE_ICON_MOVE_HIDE_AFTER = 2,
};

// Index of128KB OBJ mapping resources in AppMenuCommon file tables.
enum {
    BOX2_OBJ_MAPPING_128K = 2,
};

// Capacities of the box-owned arrays and frame collection.
#define BOX2_WIN_MAX 28
#define BOX2_FONT_OAM_MAX 10
#define BOX2_POKE_MOVE_MAX 12
#define BOX2_MARK_COUNT 6
#define BOX2_FRAME_COUNT 12
#define BOX2_POKE_ICON_COUNT 66
#define BOX2_ACTOR_MAX 145
#define BOX2_CHAR_RES_MAX 129
#define BOX2_PAL_RES_MAX 11
#define BOX2_CELL_RES_MAX 13

// Text-object groups created by CreateFontOams.
enum {
    BOX2_FONT_OAM_BOX_NAME = 0,
    BOX2_FONT_OAM_BOX_COUNT_FIRST = 1,
    BOX2_FONT_OAM_BOX_COUNT_LAST = 6,
    BOX2_FONT_OAM_TRAY_NAME_FIRST = 7,
    BOX2_FONT_OAM_TRAY_NAME_SECOND = 8,
    BOX2_FONT_OAM_RANGE_COUNT = 9,
};

#define BOX2_BOX_LIST_DRAW_ROWS 6
#define BOX2_BOX_LIST_VISIBLE_ROWS 4

// Displayed Pokerus status, from duration and infection-history checks.
enum {
    BOX2_POKERUS_NONE = 0,
    BOX2_POKERUS_ACTIVE = 1,
    BOX2_POKERUS_CURED = 2,
};

typedef struct {
    u16 species;
    // 1 when the Pokémon must hold no item, 2 when it must hold one
    u8 item;
    u8 marks;
    // The nature and the sex, plus 1, or 0 for any
    u8 nature;
    u8 ability;
    u8 sex;
    u8 active;
} Box2SearchParam;

struct Box2SysWork {
    Box2Param *param;
    GameProcManager *procManager;
    BOOL procMgrResult;
    void *subProcWork;
    u16 subRet;
    u8 subProcType;
    // What a sub proc's return puts back: 0 and 1 the menu, with the party's frame below or at the left; 2 and 3 the
    // move mode's tray and box list, 3 with a Pokémon held; 4 and 5 the party at the right, 5 with a Pokémon held
    u8 subProcReturn;
    u8 tray;
    u8 trayMax;
    u8 pos;
    // The tray of the held Pokémon, or BOX2_GET_NONE
    u8 getTray;
    // A BOX2_PICK_*
    u8 pickState;
    u8 trayScroll;
    // A BOX2_ICON_POS_* placement context for moving icons
    u8 iconPosMode;
    // Set when the move mode or the move started on the party's frame
    u8 fromParty;
    u8 moveMode : 4;
    // Set when BOX2_MOVE_MODE_SINGLE was picked with the menu open, so its Pokemon is picked up
    u8 fromMenu : 2;
    // Set when a BOX2_MOVE_MODE_MENU move ended and goes back to the main state
    u8 moveDone : 2;
    // The cursor's position that subProcReturn 3 and 5 put back
    u8 subProcCursorPos;
    // Set when the party's frame was at the left before the item arrangement brought it in
    u8 partyWasLeft;
    // What Box2Obj_SetButtons shows of the four buttons, kept to show them again
    u8 backButtonState;
    u8 closeButtonState;
    u8 summaryButtonState;
    u8 searchButtonState;
    u8 unk23;
    u32 curRcvPos;
    int nextSeq;
    Box2AppWork *app;
    Box2SearchParam search;
};

// A text object on OAM
typedef struct {
    BmpOamActor *oam;
    GFLBitmap *bitmap;
} Box2FontOam;

// What runs once per frame until it returns FALSE
typedef BOOL (*Box2VFunc)(Box2SysWork *syswk);

typedef struct {
    Box2VFunc func;
    Box2VFunc freq;
    void *work;
    u16 seq;
    u16 cnt;
} Box2IrqWork;

// A button's press animation, on an actor or on the BG
typedef struct {
    // A BOX2_BTN_ANM_MODE_*
    u8 mode : 1;
    u8 id : 7;
    u8 pal1 : 4;
    u8 pal2 : 4;
    u8 seq;
    u8 cnt;
    u8 px;
    u8 py;
    u8 sx;
    u8 sy;
} Box2ButtonAnm;

// A Pokémon icon that moves to a new position
typedef struct {
    // The icon's index into pokeIconId
    u16 iconPos;
    u16 mvPos;
    u16 dfPos;
    // 0 when the entry is unused
    u16 flag;
    u32 mx;
    u32 my;
    s16 vx;
    s16 vy;
    s16 dx;
    s16 dy;
} Box2PokeMoveData;

// The Pokémon that move at once, and where the move started and ends
typedef struct {
    Box2PokeMoveData data[BOX2_POKE_MOVE_MAX];
    u16 cnt;
    u16 mode;
    u32 getPos;
    u32 putPos;
    // Where the Pokémon was dropped
    u32 setPos;
} Box2PokeMoveWork;

// An item of a menu: its message, and 1 for the item that closes the menu
typedef struct {
    u16 msgId;
    u16 type;
} Box2MenuItem;

// An area of the lower screen, with its right edge in it
typedef struct {
    u8 left;
    u8 right;
    u8 top;
    u8 bottom;
} Box2Area;

// The work of the name entry for a box's name
typedef struct {
    NameEntryParam *param;
    StrBuf *name;
} Box2NameInputWork;

// Releasing a Pokémon
typedef struct {
    ClActor *cap;
    // The next box slot whose Pokémon's moves to check
    u16 checkCnt;
    u8 checkFlag;
    u8 scaleCnt;
    f32 scale;
} Box2PokeFreeWork;

// The scroll of the box list by touch
typedef struct {
    s16 cnt;
    s16 dir;
} Box2BoxListDrag;

// The item icon's move
typedef struct {
    u16 putPos;
    u16 setPos;
    u32 mvMode;
    u32 nowX;
    u32 nowY;
    u32 mx;
    u32 my;
    u32 mvX : 1;
    u32 mvY : 1;
    u32 cnt : 30;
} Box2ItemMoveWork;

// What the upper screen shows of a Pokémon
typedef struct {
    BoxPkm *pkm;
    u16 species;
    u16 item;
    u32 pid;
    u8 type1;
    u8 type2;
    u8 ability;
    u8 nature;
    u16 mark;
    u8 level : 7;
    u8 egg : 1;
    u8 sex : 4;
    // 0 for Nidoran and eggs, whose sex isn't shown
    u8 sexPut : 1;
    u8 rare : 1;
    // 1 while infected, 2 once cured
    u8 pokerus : 2;
    u16 moveId[4];
} Box2PokeInfo;

// The cursor's move to a position
typedef struct {
    u8 px;
    u8 py;
    u8 vx;
    u8 vy;
    u32 mx : 1;
    u32 my : 1;
    u32 cnt : 30;
} Box2CursorMoveWork;

typedef struct {
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
    u16 anmMax;
    u16 anmCnt;
    u8 anm;
    u8 prevAnm;
    u8 startPos;
    u8 endPos;
} Box2RangeSelect;

struct Box2AppWork {
    BmpOamSys *bmpOam;
    Box2FontOam fontOam[BOX2_FONT_OAM_MAX];
    u32 keyRepeatWait;
    u32 keyRepeatStart;
    TCB *vtask;
    Box2IrqWork vfunk;
    int vfuncNextSeq;
    PaletteFade *palFade;
    CursorMove *cursorMove;
    BGWinFrame *bgWinFrame;
    Font *font;
    Font *smallFont;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *expandBuf;
    PrintQueue *printQueue;
    u32 unk98;
    PrintWindow windows[BOX2_WIN_MAX];
    u8 flushChar[4];
    u32 cursorChars;
    BOOL nationalDex;
    u16 *regionalDex;
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *yesNoRes;
    AppTaskMenu *yesNoMenu;
    u16 ynID;
    Box2ButtonAnm bawk;
    ArcTool *pokeIconArc;
    u8 pokeIconChar[30][0x200];
    u8 pokeIconPal[30];
    u8 pokeIconId[BOX2_POKE_ICON_COUNT];
    BOOL pokeIconExist[30];
    u8 trayIconChar[BOX2_TRAY_MAX][0x400];
    ClActUnit *clunit;
    ClActor *actors[BOX2_ACTOR_MAX];
    u32 chrRes[BOX2_CHAR_RES_MAX];
    u8 unkA2E0[0x204];
    u32 palRes[BOX2_PAL_RES_MAX];
    u32 cellRes[BOX2_CELL_RES_MAX];
    u16 pokegraSwap;
    u16 unkA546;
    u32 oldCurPos;
    u16 getItem;
    u16 getItemInitPos;
    // Set when the icons were dimmed for a message, to be lit again when the menu opens
    u8 iconsDimmed;
    // Set while the held switch button has already switched the frames, until it is let go
    u8 switchButtonHeld;
    // Set when the menu's message is to be printed once the cursor's move ends
    u8 menuMsgPending;
    // Why the last move check failed, a BOX2_MOVE_ERR_*
    u8 moveErr;
    // The markings being edited
    u32 markings;
    s8 wallPx;
    u8 wallArea;
    // Set while the box jump is open
    u8 boxJumpOpen;
    u8 wallpaperPos;
    // A BOX2_WALLPAPER_* category, or BOX2_WALLPAPER_THEME_MENU
    u8 wallpaperTheme;
    u8 unkA55D;
    // Where the hand puts a Pokémon or an item
    u8 pokePutKey;
    // The box picked in the box list or the box jump
    u8 pickedTray;
    u32 tpx;
    u32 tpy;
    int wipeSeq;
    int wait;
    // The work of the current sequence, such as a Box2PokeFreeWork
    void *subWork;
    int subSeq;
    int msgNextSeq;
    u8 rangeFlags[30];
    u8 rangeWidth;
    u8 rangeHeight;
    Box2RangeSelect rangeSelect;
    // TRUE when the range was drawn or last moved by the keys, when a touch doesn't pick it up
    BOOL rangeByKey;
    // TRUE when a box's name was drawn to the first text object and is yet to be sent to VRAM
    BOOL trayNamePending;
};

void Box2Main_InitVBlank(Box2SysWork *syswk);
void Box2Main_ExitVBlank(Box2SysWork *syswk);
// Sets the function that runs each frame
void Box2Main_VFuncSet(Box2AppWork *app, Box2VFunc func);
// Sets the function that Box2Main_VFuncReqSet makes the next to run
void Box2Main_VFuncReq(Box2AppWork *app, Box2VFunc func);
void Box2Main_VFuncReqSet(Box2AppWork *app);
void Box2Main_InitVramBanks(void);
const BGSysVRAMConfig *Box2Main_GetVramBanks(void);
void Box2Main_InitBg(Box2SysWork *syswk);
void Box2Main_ExitBg(Box2SysWork *syswk);
void Box2Main_LoadBgGraphics(Box2SysWork *syswk);
void Box2Main_InitPaletteFade(Box2SysWork *syswk);
void Box2Main_ExitPaletteFade(Box2SysWork *syswk);
void Box2Main_SetBlendAlpha(BOOL enabled);
void Box2Main_InitMsg(Box2SysWork *syswk);
void Box2Main_ExitMsg(Box2SysWork *syswk);
void Box2Main_InitYesNo(Box2SysWork *syswk);
void Box2Main_ExitYesNo(Box2SysWork *syswk);
void Box2Main_OpenYesNo(Box2SysWork *syswk, u32 pos);
// Runs the press animation of a button; FALSE once it is done
BOOL Box2Main_ButtonAnmMain(Box2SysWork *syswk);
// Starts the press animation of a frame's button
void Box2Main_SetFrameButtonAnm(Box2SysWork *syswk, u32 frame);
void Box2Main_InitSettings(Box2SysWork *syswk);
void Box2Main_PutMessageFrame(Box2SysWork *syswk);
// Shows the previous or next box
void Box2Main_ScrollTray(Box2SysWork *syswk, BOOL right);
void Box2Main_StartBoxListScroll(Box2SysWork *syswk, u32 mv);
void Box2Main_OpenBoxList(Box2SysWork *syswk);
BOOL Box2Main_IsTrayScrollRight(Box2SysWork *syswk, u32 from, u32 to);
// Shows the cursor and leaves touch mode
void Box2Main_ShowCursor(Box2SysWork *syswk);
u32 Box2Main_GetPokeParam(Box2SysWork *syswk, u16 pos, u16 tray, u32 param, void *buf);
void Box2Main_SetPokeParam(Box2SysWork *syswk, u32 pos, u32 tray, u32 param, u32 value);
// The Pokémon at a position of a tray, or of the party when tray is BOX2_GET_NONE
BoxPkm *Box2Main_GetBoxPkm(Box2SysWork *syswk, u32 tray, u32 pos);
void Box2Main_ClearPokeData(Box2SysWork *syswk, u32 tray, u32 pos);
// Moves the Pokémon of the move that just ended in the save data
void Box2Main_PokeDataMove(Box2SysWork *syswk);
// The box that a tray icon of the box list shows
u32 Box2Main_GetScrolledTray(Box2SysWork *syswk, u32 tray);
BOOL Box2Main_BattlePokeCheck(Box2SysWork *syswk, u32 pos);
BOOL Box2Main_PokeItemFormChange(Box2SysWork *syswk, BoxPkm *pkm);
// Recalculates the stats of the party Pokémon at pos, after a forme change
void Box2Main_RecalcPartyStats(Box2SysWork *syswk, u32 pos);
void Box2Main_InitDexData(Box2SysWork *syswk);
void Box2Main_ExitDexData(Box2SysWork *syswk);
BOOL Box2Main_IsSpeciesSendable(Box2SysWork *syswk, u32 pos);
BOOL Box2Main_PokeItemMoveCheck(Box2SysWork *syswk, u32 getPos, u32 putPos);
void Box2Main_PokeFreeCreate(Box2SysWork *syswk);
void Box2Main_PokeFreeExit(Box2SysWork *syswk);
BOOL Box2Main_PokeFreeMoveCheck(Box2SysWork *syswk);
void Box2Main_UpdateChatter(Box2SysWork *syswk);
u8 Box2Main_GetTrayScroll(Box2SysWork *syswk, s8 mv);
// Shows a wallpaper, scrolling from the direction dir, a BOX2_TRAY_SCROLL_*
void Box2Main_WallPaperSet(Box2SysWork *syswk, u32 wallpaper, u32 dir);
void Box2Main_WallPaperChange(Box2SysWork *syswk, u32 wallpaper);
// The wallpaper of a box
u32 Box2Main_GetWallPaperNumber(Box2SysWork *syswk, u32 tray);
BOOL Box2Main_PokeInfoPutCore(Box2SysWork *syswk, u32 tray, u32 pos);
BOOL Box2Main_PokeInfoPut(Box2SysWork *syswk, u32 pos);
void Box2Main_PokeInfoRewrite(Box2SysWork *syswk, u32 pos);
void Box2Main_PokeInfoOff(Box2SysWork *syswk);
void Box2Main_PokeSelectOff(Box2SysWork *syswk);
void Box2Main_MarkingPutMain(Box2SysWork *syswk, u32 mark);
void Box2Main_MarkingPutSub(Box2SysWork *syswk, u32 mark);
// The sub procs: the summary, the bag, the name entry for a box's name and the search
int Box2Main_PokeStatusCall(Box2SysWork *syswk);
int Box2Main_PokeStatusExit(Box2SysWork *syswk);
int Box2Main_BagCall(Box2SysWork *syswk);
int Box2Main_BagExit(Box2SysWork *syswk);
int Box2Main_NameInputCall(Box2SysWork *syswk);
int Box2Main_NameInputExit(Box2SysWork *syswk);
int Box2Main_BoxSearchCall(Box2SysWork *syswk);
int Box2Main_BoxSearchExit(Box2SysWork *syswk);
BOOL Box2Main_VFuncPokeMoveTouchParty(Box2SysWork *syswk);
BOOL Box2Main_VFuncPokeMoveTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyPokeFreeSort(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyInPokeMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncTrayScrollLeft(Box2SysWork *syswk);
BOOL Box2Main_VFuncTrayScrollRight(Box2SysWork *syswk);
BOOL Box2Main_VFuncFrameMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncMarkingFrameMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyFrameMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxListSlideOut(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxListSlideIn(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyOutTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyInTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncCursorMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncCursorMoveFrame(Box2SysWork *syswk);
void Box2Main_HandGetPokeSet(Box2SysWork *syswk);
BOOL Box2Main_VFuncPokeMoveGetKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncPokeMovePutKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyOutPutKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeMenuClose(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeGetTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeMenuOpen(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeFrameMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemIconHide(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangePartyGetTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemIconPutBack(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeGetKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangePutKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeKeyCancel(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeBoxPartyGetTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeMenuCancel(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxListScrollNext(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxListScrollPrevious(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxJumpScrollNext(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxJumpScrollPrevious(Box2SysWork *syswk);
BOOL Box2Main_VFuncRangeMoveTouch(Box2SysWork *syswk);
void Box2Main_ClearRangeFlags(Box2SysWork *syswk);
void Box2Main_SetRangeFlags(Box2SysWork *syswk);
// How many Pokémon the picked range holds
u32 Box2Main_GetRangeCount(Box2AppWork *app);
BOOL Box2Main_RangePutCheck(Box2SysWork *syswk, u32 tray, int pos);
u32 Box2Main_GetRowWidth(Box2SysWork *syswk, u32 pos);
void Box2Main_UpdateRangeIconsTouch(Box2SysWork *syswk);
void Box2Main_UpdateRangeIconsPos(Box2SysWork *syswk, u32 pos);

#endif // POKEBW2_APP_BOX2_MAIN_H
