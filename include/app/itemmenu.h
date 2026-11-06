#ifndef POKEBW2_APP_ITEMMENU_H
#define POKEBW2_APP_ITEMMENU_H

#include "types.h"
#include "app/bag.h"
#include "app/bag_item.h"
#include "gfl/bmp.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "save/bag.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/app_taskmenu.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The bag's work, in overlay 142 (itemmenu.c). itemmenu_disp.c draws it

// Forces the next list draw, outside the range of any scroll position
#define ITEMMENU_DRAWN_SCROLL_INVALID 0xffff

// The quantity picker: tossing items or selling them
enum {
    ITEMMENU_QUANTITY_TOSS = 1,
    ITEMMENU_QUANTITY_SELL = 2,
};

// The TM panel change performed by the VBlank task
enum {
    ITEMMENU_TM_INFO_NONE = 0,
    ITEMMENU_TM_INFO_SHOW = 1,
    ITEMMENU_TM_INFO_HIDE = 2,
};

// The palette of the row under the cursor
enum {
    ITEMMENU_CURSOR_PALETTE_OFF = 0,
    ITEMMENU_CURSOR_PALETTE_ON = 1,
    ITEMMENU_CURSOR_PALETTE_PRESS = 2,
};

// The actors in buttons
enum {
    ITEMMENU_BUTTON_POCKET_LEFT = 0,
    ITEMMENU_BUTTON_POCKET_RIGHT = 1,
    ITEMMENU_BUTTON_REGISTER = 2,
    ITEMMENU_BUTTON_CLOSE = 3,
    ITEMMENU_BUTTON_BACK = 4,
    ITEMMENU_BUTTON_QUANTITY_UP = 5,
    ITEMMENU_BUTTON_QUANTITY_DOWN = 6,
};

// The filter menu's extra actions; the others are BAG_POCKET_* values
enum {
    ITEMMENU_FILTER_ACTION_ALL = 5,
    ITEMMENU_FILTER_ACTION_CANCEL = 6,
};

// The list cursor resource's animation sequences
enum {
    ITEMMENU_LIST_ANM_SCROLL_BAR = 0,
    ITEMMENU_LIST_ANM_CURSOR = 1,
    ITEMMENU_LIST_ANM_MOVE_CURSOR = 2,
};

// The touch bar resource's animation sequences
enum {
    ITEMMENU_TOUCHBAR_ANM_CLOSE = 0,
    ITEMMENU_TOUCHBAR_ANM_BACK = 1,
    ITEMMENU_TOUCHBAR_ANM_QUANTITY_DOWN = 2,
    ITEMMENU_TOUCHBAR_ANM_QUANTITY_UP = 3,
    ITEMMENU_TOUCHBAR_ANM_POCKET_LEFT = 4,
    ITEMMENU_TOUCHBAR_ANM_POCKET_RIGHT = 5,
    ITEMMENU_TOUCHBAR_ANM_REGISTER_OFF = 6,
    ITEMMENU_TOUCHBAR_ANM_REGISTER_ON = 7,
    ITEMMENU_TOUCHBAR_ANM_CLOSE_PRESS = 8,
    ITEMMENU_TOUCHBAR_ANM_BACK_PRESS = 9,
    ITEMMENU_TOUCHBAR_ANM_QUANTITY_DOWN_PRESS = 10,
    ITEMMENU_TOUCHBAR_ANM_QUANTITY_UP_PRESS = 11,
    ITEMMENU_TOUCHBAR_ANM_POCKET_LEFT_PRESS = 12,
    ITEMMENU_TOUCHBAR_ANM_POCKET_RIGHT_PRESS = 13,
    ITEMMENU_TOUCHBAR_ANM_CLOSE_DISABLED = 14,
    ITEMMENU_TOUCHBAR_ANM_BACK_DISABLED = 15,
    ITEMMENU_TOUCHBAR_ANM_POCKET_LEFT_DISABLED = 18,
    ITEMMENU_TOUCHBAR_ANM_POCKET_RIGHT_DISABLED = 19,
    ITEMMENU_TOUCHBAR_ANM_REGISTER_OFF_DISABLED = 21,
    ITEMMENU_TOUCHBAR_ANM_REGISTER_ON_DISABLED = 22,
};

// The sort button resource's animation sequences
enum {
    ITEMMENU_SORT_ANM_READY = 0,
    ITEMMENU_SORT_ANM_PRESS = 2,
    ITEMMENU_SORT_ANM_DISABLED = 4,
};

// The filter button resource's animation sequences
enum {
    ITEMMENU_FILTER_ANM_READY = 0,
    ITEMMENU_FILTER_ANM_PRESS = 1,
    ITEMMENU_FILTER_ANM_DISABLED = 2,
};

// The number of rows the item list shows
#define ITEMMENU_LIST_ROWS 6
// The rows of OBJ the list draws its items with, one more above and below the rows it shows
#define ITEMMENU_ROW_OBJS 8

typedef void (*ItemMenuState)(ItemMenuWork *work);

struct ItemMenuWork {
    GameData *gameData;
    TrainerDataSave *trainerData;
    PlayerInfo *playerInfo;
    PlayerActionPerms *perms;
    u32 mode;
    BagCursor *cursor;
    BagSave *bag;
    BOOL isCycling;
    BOOL dowsingActive;
    // A copy of the pocket, while its items are moved around
    BagItem items[310];
    ItemMenuState state;
    PrintWindow msgWindow;
    PrintWindow pocketNameWindow;
    PrintQueue *printQueue;
    PrintStream *printStream;
    // Whether the message was resumed after a pause
    BOOL streamResumed;
    KeyCursor *keyCursor;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    StrBuf *expandBuf;
    StrBuf *tempBuf;
    Font *font;
    u8 unk538[0x14];
    u16 heapId;
    u32 selIconPlt;
    u32 selIconChr;
    u32 selIconCel;
    u32 listCursorPlt;
    u32 unk560;
    u32 buttonPlt;
    u32 listCursorChr;
    u32 buttonChr;
    u32 filterButtonChr;
    u32 listCursorCel;
    u32 rowNameCel;
    u32 buttonCel;
    u32 filterButtonCel;
    u32 rowMarkPlt;
    u32 rowMarkChr;
    u32 rowMarkCel;
    u32 sortButtonPlt;
    u32 sortButtonChr;
    u32 rowNamePlt;
    u32 sortButtonCel;
    u32 pocketTabPlt;
    u32 pocketTabChr;
    u32 pocketTabCel;
    u32 typeIconChr[17];
    u32 typeIconPlt;
    u32 typeIconCel;
    u32 categoryIconChr[3];
    u32 rowNameChr[ITEMMENU_ROW_OBJS];
    GFLBitmap *rowNameBitmaps[ITEMMENU_ROW_OBJS];
    ClActor *rowNames[ITEMMENU_ROW_OBJS];
    u32 rowMarkTypes[ITEMMENU_ROW_OBJS];
    ClActor *rowMarks[ITEMMENU_ROW_OBJS];
    ButtonMan *buttonMan;
    TCB *vblankTask;
    ClActUnit *actorUnit;
    ClActor *selIcon;
    ClActor *listCursor;
    ClActor *scrollBar;
    ClActor *pocketTabs[6];
    ClActor *categoryIcons[3];
    ClActor *typeIcons[17];
    ClActor *buttons[7];
    ClActor *filterButton;
    ClActor *sortButton;
    TCBExManager *tcbManager;
    PrintWindow tmInfoWindow;
    PrintWindow itemNameWindow;
    PrintWindow countWindow;
    PrintWindow descWindow;
    PrintWindow pocketLabelWindow;
    PrintWindow quantityWindow;
    PrintWindow moneyWindow;
    PrintWindow moneyValueWindow;
    PrintWindow priceWindow;
    PrintWindow menuTitleWindow;
    AppTaskMenu *taskMenu;
    AppTaskMenuItem menuItems[APP_TASKMENU_ITEM_MAX];
    AppTaskMenuRes *taskMenuRes;
    u8 unk804[8];
    u32 quantityMode;
    s32 quantityRepeat;
    s32 quantity;
    u32 bg0Chars;
    u32 bg4Chars;
    u32 bg5Chars;
    u32 bg1Chars;
    u32 bg3Chars;
    s32 pocket;
    // The cursor's row on the screen, and the list's scroll, one less than the index of the top row
    s32 cursorRow;
    s32 scroll;
    // The scroll the list was last drawn at
    s32 drawnScroll;
    // Whether the list shows the copy in items, while items are moved around
    BOOL movingItem;
    u8 unk840[8];
    u32 itemMenuActions[5];
    u32 sortMenuActions[5];
    u32 freeSpaceMenuActions[7];
    u32 cursorImageChars;
    u16 iconsDirty;
    u16 listDirty;
    u32 tmInfoRequest;
    u32 result;
    u32 menuAction;
    s32 item;
    BlinkPalAnm *paletteAnim;
    PaletteFade *paletteFade;
    BOOL buttonsActive;
    BOOL touchHeld;
    // Whether the item being moved was dragged
    BOOL touchMoved;
    u16 buttonAnim;
    u16 buttonAnimTimer;
    ItemMenuState buttonAnimNext;
    u32 savedRepeatWait;
    u32 savedRepeatStart;
    BagItemList itemList;
    u32 sortType;
    u32 freeSpaceFilter;
};

// The slot of an item of the list
BagItem *ItemMenu_GetSlot(ItemMenuWork *work, u32 index);
// What a count of an item sells for
s32 ItemMenu_GetSellPrice(u32 item, s32 count, HeapID heapId);
// The index of the item under the cursor
s32 ItemMenu_GetCursorIndex(ItemMenuWork *work);
// The number of items the list shows
s32 ItemMenu_GetItemCount(ItemMenuWork *work);
void ItemMenu_SetItemName(ItemMenuWork *work, u32 index, u32 item);
void ItemMenu_SetPocketName(ItemMenuWork *work, u32 index, u32 pocket);
BOOL ItemMenu_CanRegister(ItemMenuWork *work);
// Whether an item is registered for the Y button
BOOL ItemMenu_IsItemRegistered(ItemMenuWork *work, u32 item);
s32 ItemMenu_CountFilterActions(ItemMenuWork *work);

#endif // POKEBW2_APP_ITEMMENU_H
