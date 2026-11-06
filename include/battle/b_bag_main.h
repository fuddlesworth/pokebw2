#ifndef POKEBW2_BATTLE_B_BAG_MAIN_H
#define POKEBW2_BATTLE_B_BAG_MAIN_H

#include "types.h"
#include "battle/b_app_tool.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "save/bag.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The battle bag in overlay 286 (b_bag_main.c, the ROM's embedded string, and the other b_bag_*.c files, named
// after it and Diamond and Pearl's): the bag shown during a battle to use or throw an item, the Wonder Launcher,
// and the catching demo. It sets done when it ends. Every name here is ours; swan has none for this overlay.

// What the bag is opened for (BBagParam.mode)
enum {
    BBAG_MODE_NORMAL,
    // The Wonder Launcher's items, in a link battle whose regulation allows it
    BBAG_MODE_SHOOTER,
    // The catching demonstration
    BBAG_MODE_CAPTURE_DEMO,
    // Opens on the Poké Balls, for the Entree Forest capture (pdc_main.c)
    BBAG_MODE_BALLS,
};

// Why a Poké Ball can't be thrown (BBagParam.ballError), each with its message
enum {
    BBAG_BALL_OK,
    // The party and the boxes are full (BTL_SETUP_FLAG_BOXES_FULL)
    BBAG_BALL_BOXES_FULL,
    // Two wild Pokémon are left
    BBAG_BALL_TWO_TARGETS,
    // Items can't be used now: "impossible to aim without being focused"
    BBAG_BALL_NOT_FOCUSED,
    // The wild Pokémon is out of sight, as in the middle of Fly or Dig
    BBAG_BALL_HIDDEN,
    // Ghetsis's device blocks them (BTL_SETUP_FLAG_NO_BALLS)
    BBAG_BALL_BLOCKED,
};

// The page displayed by the bag
enum {
    BBAG_PAGE_POCKETS = 0,
    BBAG_PAGE_ITEMS = 1,
    BBAG_PAGE_USE = 2,
    BBAG_PAGE_MAX = 3,
};

// The battle pockets, in display order; the Launcher uses the first pocket
enum {
    BBAG_POCKET_HP_PP = 0,
    BBAG_POCKET_STATUS = 1,
    BBAG_POCKET_BALLS = 2,
    BBAG_POCKET_BATTLE_ITEMS = 3,
    BBAG_POCKET_MAX = 4,
    BBAG_POCKET_NONE = BBAG_POCKET_MAX,
    BBAG_POCKET_SHOOTER = BBAG_POCKET_HP_PP,
};

#define BBAG_ITEMS_PER_PAGE 6
#define BBAG_ITEMS_PER_POCKET 36

// The task dispatch table, followed by the exit that frees its work
enum {
    BBAG_STATE_INIT = 0,
    BBAG_STATE_INIT_LAUNCHER = 1,
    BBAG_STATE_START_WIPE = 2,
    BBAG_STATE_WAIT_WIPE = 3,
    BBAG_STATE_POCKET_PAGE = 4,
    BBAG_STATE_ITEM_PAGE = 5,
    BBAG_STATE_USE_PAGE = 6,
    BBAG_STATE_TO_POCKET_PAGE = 7,
    BBAG_STATE_TO_ITEM_PAGE = 8,
    BBAG_STATE_TO_USE_PAGE = 9,
    BBAG_STATE_WAIT_PAGE_PRINT = 10,
    BBAG_STATE_STEP_PAGE = 11,
    BBAG_STATE_WAIT_STEP_PAGE = 12,
    BBAG_STATE_CLOSE_MESSAGE = 13,
    BBAG_STATE_PRINT_MESSAGE = 14,
    BBAG_STATE_WAIT_MESSAGE_KEY = 15,
    BBAG_STATE_WAIT_BUTTON_ANM = 16,
    BBAG_STATE_DEMO = 17,
    BBAG_STATE_START_EXIT = 18,
    BBAG_STATE_WAIT_FADE_OUT = 19,
    BBAG_STATE_EXIT = 20,
};

// The catching demonstration: each touch is followed by its release
enum {
    BBAG_DEMO_WAIT_POCKET_TOUCH = 0,
    BBAG_DEMO_WAIT_POCKET_RELEASE = 1,
    BBAG_DEMO_WAIT_ITEM_TOUCH = 2,
    BBAG_DEMO_WAIT_ITEM_RELEASE = 3,
    BBAG_DEMO_WAIT_USE_TOUCH = 4,
    BBAG_DEMO_WAIT_USE_RELEASE = 5,
};

// What the battle passes to the bag, 0x44 bytes, inside BtlvCore
typedef struct {
    BagSave *bag;               // 0x00
    Font *font;                 // 0x04
    u32 mode;                   // 0x08  a BBAG_MODE_*
    HeapID heapId;              // 0x0c
    const u8 *shooterDisabled;  // 0x10  ShooterItem_IsEnabled's bit field
    u8 shooterEnergy;           // 0x14
    u8 shooterSpent;            // 0x15
    u16 item;                   // 0x16  out, 0 for none
    u8 cost;                    // 0x18  out, the item's Launcher cost in mode 1, else 0
    u8 done;                    // 0x19
    BagCursor *bagCursor;       // 0x1c  the bag's saved cursor
    BOOL quit;                  // 0x20  1 leaves with the fade
    BOOL abort;                 // 0x24  1 leaves at once
    u8 ballError;               // 0x28  a BBAG_BALL_*: why a Poké Ball can't be thrown
    u8 isWild;                  // 0x29
    u8 pocket;                  // 0x2a  out, 4 on cancel
    u8 *usingKeys;              // 0x2c
    s16 rows[BBAG_POCKET_MAX];  // 0x30  by pocket, the cursor's row on the item page
    s16 pages[BBAG_POCKET_MAX]; // 0x38  by pocket
    BOOL playSound;             // 0x40  1 plays the sound effects
} BBagParam;

// The windows; each list buffer holds six name/count pairs
enum {
    BBAG_WINDOW_POCKET_HP_PP = 0,
    BBAG_WINDOW_POCKET_STATUS = 1,
    BBAG_WINDOW_POCKET_BALLS = 2,
    BBAG_WINDOW_POCKET_BATTLE_ITEMS = 3,
    BBAG_WINDOW_LAST_ITEM_LABEL = 4,
    BBAG_WINDOW_LIST_FIRST = 5,
    BBAG_WINDOW_LIST_SECOND = 17,
    BBAG_WINDOW_POCKET_NAME = 29,
    BBAG_WINDOW_PAGE_NUMBER = 30,
    BBAG_WINDOW_ITEM_NAME = 31,
    BBAG_WINDOW_ITEM_COUNT = 32,
    BBAG_WINDOW_ITEM_DESCRIPTION = 33,
    BBAG_WINDOW_USE = 34,
    BBAG_WINDOW_MAX = 35,
};

// The first of each kind of actor in BBagWork's actors
enum {
    BBAG_ACTOR_ITEM = 0,       // 0-5 the page's items
    BBAG_ACTOR_SHOWN_ITEM = 6, // the last used item on the pockets' page, or the chosen item
    BBAG_ACTOR_COST = 7,       // 8 per item slot: the cost, then 7 gauge pips (mode 1)
    BBAG_ACTOR_ENERGY = 55,    // 8: the energy left and spent
    BBAG_ACTOR_MAX = 63,
};

// The bag's work, 0x5a4 bytes, shared by the overlay's files
struct BBagWork {
    BBagParam *param;                                      // 0x000
    TCBExManager *tcbExMgr;                                // 0x004
    PaletteFade *paletteFade;                              // 0x008
    MsgData *msgData;                                      // 0x00c
    WordSet *wordSet;                                      // 0x010
    StrBuf *strBuf;                                        // 0x014
    PrintQueue *printQueue;                                // 0x018
    PrintStream *printStream;                              // 0x01c
    BOOL streamAdvanced;                                   // 0x020
    BmpWin *msgWin;                                        // 0x024
    PrintWindow windows[BBAG_WINDOW_MAX];                  // 0x028
    u8 unk140;                                             // 0x140  never accessed
    u8 listBuf;                                            // 0x141  0: list windows 5-16, 1: 17-28; flipped per draw
    CursorMove *cursorMove;                                // 0x144
    BAppCursor *cursor;                                    // 0x148
    BtlvFingerCursor *fingerCursor;                        // 0x14c  the catching demo's
    BagItem items[BBAG_POCKET_MAX][BBAG_ITEMS_PER_POCKET]; // 0x150  by battle pocket
    u8 unk390[0x90];                                       // 0x390  never accessed, the size of one more pocket
    ClActUnit *actorUnit;                                  // 0x420
    ClActor *actors[BBAG_ACTOR_MAX];                       // 0x424
    BGWinFrame *buttons;                                   // 0x520
    u8 animSeq;                                            // 0x524
    u8 animCount;                                          // 0x525
    u8 animButton;                                         // 0x526
    u8 animActive;                                         // 0x527
    u8 unk528[8];                                          // 0x528  never accessed
    u8 seq;                                                // 0x530  a BBAG_STATE_*
    u8 nextSeq;                                            // 0x531  taken after the button animation or the message
    u8 page;                                               // 0x532  a BBAG_PAGE_*
    u8 pocket;                                             // 0x533
    s8 pageStep;                                           // 0x534  -1 or 1
    u8 itemCount[BBAG_POCKET_MAX];                         // 0x535
    u8 lastPage[BBAG_POCKET_MAX];                          // 0x539
    u8 demoSeq;                                            // 0x53d
    u8 unk53E[2];                                          // 0x53e  never accessed
    u16 lastItem;                                          // 0x540
    u8 lastPocket;                                         // 0x542
    u32 charRes[9];                                        // 0x544
    u32 plttRes[9];                                        // 0x568
    u32 cellRes[3];                                        // 0x58c
    BOOL cursorVisible;                                    // 0x598
    const u8 *flushList;                                   // 0x59c  window indices ending in 0xff
    BOOL initialized;                                      // 0x5a0
}; // 0x5a4

void BBagMain_Start(BBagParam *param);

#endif // POKEBW2_BATTLE_B_BAG_MAIN_H
