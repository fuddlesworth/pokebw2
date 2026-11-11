#ifndef POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_LOCAL_H
#define POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_LOCAL_H

#include "types.h"
#include "app/t_download/t_download_graphic.h"
#include "app/ui/frame_list.h"
#include "field/wbt.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "save/extra_block_savedata.h"
#include "struct_decls.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/wordset.h"

// The two details-page-arrow pending animations.
#define T_DOWNLOAD_OBJ_FLAG_PAGE_ARROWS 0x30

// The two list-arrow pending animations.
#define T_DOWNLOAD_OBJ_FLAG_LIST_ARROWS 3

// The pending pressed animation of this button.
#define T_DOWNLOAD_OBJ_FLAG_BUTTON_TAB_DOWN 0x20

// The pending pressed animation of this button.
#define T_DOWNLOAD_OBJ_FLAG_BUTTON_TAB_UP 0x10

// The pending pressed animation of this button.
#define T_DOWNLOAD_OBJ_FLAG_BUTTON_X 8

// The pending pressed animation of this button.
#define T_DOWNLOAD_OBJ_FLAG_BUTTON_RETURN 4

// The pending pressed animation of this button.
#define T_DOWNLOAD_OBJ_FLAG_BUTTON_DOWN 2

// The pending pressed animation of this button.
#define T_DOWNLOAD_OBJ_FLAG_BUTTON_UP 1

// Indices into the message table, rather than message-bank IDs.
enum {
    T_DOWNLOAD_MESSAGE_SELECT_INTRO = 0,
    T_DOWNLOAD_MESSAGE_ENTER_PROMPT = 1,
    T_DOWNLOAD_MESSAGE_ACTIONS_PROMPT = 2,
    T_DOWNLOAD_MESSAGE_DELETE_PROMPT = 3,
    T_DOWNLOAD_MESSAGE_DELETE_CONFIRM = 4,
    T_DOWNLOAD_MESSAGE_RECEIVED_INTRO = 5,
    T_DOWNLOAD_MESSAGE_ALREADY_SAVED = 6,
    T_DOWNLOAD_MESSAGE_LEAVE_RECEIVED = 7,
    T_DOWNLOAD_MESSAGE_QUIT_DELETING = 8,
    T_DOWNLOAD_MESSAGE_SLOTS_FULL = 9,
    T_DOWNLOAD_MESSAGE_DELETED = 10,
    T_DOWNLOAD_MESSAGE_SAVING = 11,
    T_DOWNLOAD_MESSAGE_SAVED = 12,
};

// The third item, when present, is the cancel/return entry.
#define T_DOWNLOAD_MENU_RETURN_ITEM 2

// End marker for the byte-sized BG, window and actor resource tables.
#define T_DOWNLOAD_TABLE_END 0xff

// No completed menu result yet.
#define T_DOWNLOAD_MENU_PENDING -1

// The downloaded-tournament action menus.
enum {
    T_DOWNLOAD_MENU_ENTER = 0,
    T_DOWNLOAD_MENU_RECEIVED = 1,
    T_DOWNLOAD_MENU_YES_NO = 2,
    T_DOWNLOAD_MENU_DETAILS = 3,
};

// The result of translating FrameList_Main for the received screen.
enum {
    T_DOWNLOAD_LIST_NONE = 0,
    T_DOWNLOAD_LIST_MOVED = 1,
    T_DOWNLOAD_LIST_DECIDED = 2,
    T_DOWNLOAD_LIST_ARROW = 3,
    T_DOWNLOAD_LIST_JUMPED = 4,
};

// Button animation offsets: idle, pressed and disabled.
enum {
    T_DOWNLOAD_BUTTON_STATE_IDLE = -1,
    T_DOWNLOAD_BUTTON_STATE_PRESSED = 0,
    T_DOWNLOAD_BUTTON_STATE_DISABLED = 1,
};

// Which arrow pair shows the current range.
enum {
    T_DOWNLOAD_ARROWS_LIST = 0,
    T_DOWNLOAD_ARROWS_PAGES = 1,
};

// The cursor follows received-list rows or saved-slot rows.
enum {
    T_DOWNLOAD_CURSOR_LIST = 0,
    T_DOWNLOAD_CURSOR_SLOTS = 1,
};

// Which array supplies a tournament record.
enum {
    T_DOWNLOAD_SOURCE_SAVED = 0,
    T_DOWNLOAD_SOURCE_RECEIVED = 1,
};

// The three details tabs list banned Pokemon, items and moves.
enum {
    T_DOWNLOAD_TAB_BANNED_POKEMON = 0,
    T_DOWNLOAD_TAB_BANNED_ITEMS = 1,
    T_DOWNLOAD_TAB_BANNED_MOVES = 2,
};

// Entry kinds returned by the tournament record; the third kind is a move.
enum {
    T_DOWNLOAD_ENTRY_SPECIES = 0,
    T_DOWNLOAD_ENTRY_ITEMS = 1,
    T_DOWNLOAD_ENTRY_MOVE = 2,
};

// The string fields that TDownloadSave_LoadString copies.
enum {
    T_DOWNLOAD_STRING_NAME = 0,
    T_DOWNLOAD_STRING_AUTHOR = 1,
    T_DOWNLOAD_STRING_TEXT = 2,
};

// The work of the downloaded tournaments, which t_download.c, t_download_util.c and t_download_save.c share. The
// names are ours

// The screens, each with its own init, main and exit
enum {
    // The tournaments kept in the save's slots
    T_DOWNLOAD_MODE_SAVED,
    // The tournaments received over Wi-Fi
    T_DOWNLOAD_MODE_RECEIVED,
    // The slots, to pick a tournament to enter
    T_DOWNLOAD_MODE_SELECT,
    // The details of a tournament: its banned Pokémon, items and moves, a tab each
    T_DOWNLOAD_MODE_DETAIL,
    T_DOWNLOAD_MODE_COUNT,
};

#define T_DOWNLOAD_WINDOW_COUNT 17
#define T_DOWNLOAD_SAVE_SLOTS 3
#define T_DOWNLOAD_RECEIVED_MAX 12
#define T_DOWNLOAD_TABS 3

// The cell actors' kinds, each with its own array in the work
enum {
    T_DOWNLOAD_ACTOR_OBJ,
    T_DOWNLOAD_ACTOR_BUTTON,
};

#define T_DOWNLOAD_OBJ_COUNT 9
#define T_DOWNLOAD_BUTTON_COUNT 6

// The buttons, the cell actors of the T_DOWNLOAD_ACTOR_BUTTON kind
enum {
    T_DOWNLOAD_BUTTON_UP,
    T_DOWNLOAD_BUTTON_DOWN,
    T_DOWNLOAD_BUTTON_RETURN,
    T_DOWNLOAD_BUTTON_X,
    T_DOWNLOAD_BUTTON_TAB_UP,
    T_DOWNLOAD_BUTTON_TAB_DOWN,
};

// The windows: the first nine are in every screen, the rest in the screens of their own
#define T_DOWNLOAD_WINDOW_TITLE 6
#define T_DOWNLOAD_WINDOW_MESSAGE 8
#define T_DOWNLOAD_WINDOW_DETAIL 9
#define T_DOWNLOAD_WINDOW_DETAIL_COUNT 5
#define T_DOWNLOAD_WINDOW_SLOT 14

// The bits of the work's objFlags. The first six are the buttons whose pressed animation is playing
#define T_DOWNLOAD_OBJ_FLAG_BUTTONS 0x3f
#define T_DOWNLOAD_OBJ_FLAG_NO 0x40
#define T_DOWNLOAD_OBJ_FLAG_YES 0x80
// The yes/no answer, either button
#define T_DOWNLOAD_OBJ_FLAG_RESULT (T_DOWNLOAD_OBJ_FLAG_NO | T_DOWNLOAD_OBJ_FLAG_YES)
#define T_DOWNLOAD_OBJ_FLAG_LIST_MOVED 0x8000
// The background's palette is being faded
#define T_DOWNLOAD_OBJ_FLAG_PALETTE_ANIM 0x10000
// The cursor is shown, as the keys moved it last
#define T_DOWNLOAD_OBJ_FLAG_CURSOR_KEYS 0x20000

// The bits of the work's saveFlags
#define T_DOWNLOAD_SAVE_FLAG_SAVING 0x1
#define T_DOWNLOAD_SAVE_FLAG_SLOT_SHIFT 4

struct TDownloadWork {
    HeapID heapId;
    // The cell resources: the touch bar's characters, palette and cells, then the screen's own
    u32 res[6];
    ClActor *buttons[T_DOWNLOAD_BUTTON_COUNT];
    ClActor *actors[T_DOWNLOAD_OBJ_COUNT];
    TDownloadGraphic *graphic;
    WbtOv326Param2 *param;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData[2];
    PrintWindow windows[T_DOWNLOAD_WINDOW_COUNT];
    PrintStream *stream;
    TCBExManager *tcbEx;
    StrBuf *streamStr;
    KeyCursor *keyCursor;
    AppPrintsysCommon printWait;
    WordSet *wordSet;
    AppTaskMenu *taskMenu;
    AppTaskMenuItem menuItems[3];
    AppTaskMenuRes *taskMenuRes;
    WaitIcon *waitIcon;
    // The step of the screen's sequence, and the screen
    int seq;
    int mode;
    u32 flags;
    u32 objFlags;
    u32 unk154;
    // The screen to go back to from the details, or -1 to end
    int returnMode;
    int tab;
    // The scroll of the background, in 1/256 pixels
    int bgScroll;
    // The windows whose screen is still to be sent, a bit each
    u32 mapFlushMask;
    // Bit 0 while saving, and bit 4 + i while save slot i holds a tournament
    u32 saveFlags;
    // The slot picked in the select screen, and in the saved tournaments
    int slotPos;
    int slotCursor;
    int seqAfterList;
    // The received tournament to save, and the slot to save it in
    int copySrc;
    int copyDest;
    int slotCount;
    int receivedCount;
    int palAnimCount;
    u16 palFadeFrom[16];
    u16 palFadeTo[16];
    u16 palFadeWork[16];
    u16 textPltt[2][16];
    FrameList *list;
    int listCount;
    // The list's item, cursor and scroll, live in [0] and kept in [2] while the details are shown
    int listPos[3];
    int listCursor[3];
    int listScroll[3];
    int lastSlot;
    // For each tab of the details: its items, its pages and the page shown
    int itemCount[T_DOWNLOAD_TABS];
    int pageCount[T_DOWNLOAD_TABS];
    int page[T_DOWNLOAD_TABS];
    s16 *objPos;
    void *unk284;
    WbtDownloadData *record;
    WbtDownloadData *savedData[T_DOWNLOAD_SAVE_SLOTS];
    WbtDownloadData *receivedData[T_DOWNLOAD_RECEIVED_MAX];
};

#endif // POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_LOCAL_H
