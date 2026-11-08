#include "types.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/script_text_banks.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_actor_drawdata.h"
#include "field/field_camera.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/game_beacon_set.h"
#include "field/resort.h"
#include "field/scrcmd_resort_shop.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "pml/item.h"
#include "save/bag.h"
#include "save/join_avenue.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/bmp_menu.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/nsbtx_to_clwk.h"
#include "system/printsys.h"
#include "system/resort_binary.h"
#include "system/text_speed.h"
#include "system/vm.h"
#include "system/wordset.h"
#include "text/script/scrcmd_resort_shop.h"

// The Join Avenue's shops (plugin 8's command 1000 while this overlay takes overlay 59's place). The command opens a
// menu over the field: mode 0 is a shop's items, mode 1 the four records, and mode 2 the order of the records (sub
// mode 0) or of the avenue's people (sub mode 1)

// The outer callback lifecycle.
enum {
    RESORT_SHOP_STATE_FADE_IN = 0,
    RESORT_SHOP_STATE_UPDATE = 1,
    RESORT_SHOP_STATE_FADE_OUT = 2,
    RESORT_SHOP_STATE_FREE = 3,
};

// The initial work state is shared by the three mode-specific state machines.
enum {
    RESORT_SHOP_WORK_INITIAL = 0,
};

// The buying state machine.
enum {
    RESORT_BUY_STATE_LIST = 0,
    RESORT_BUY_STATE_UNUSED = 1,
    RESORT_BUY_STATE_RESTORE_LIST = 2,
    RESORT_BUY_STATE_CONFIRM = 3,
    RESORT_BUY_STATE_CONFIRM_WITHOUT_COUNT = 4,
    RESORT_BUY_STATE_WAIT_CONFIRM = 5,
    RESORT_BUY_STATE_BUY = 6,
    RESORT_BUY_STATE_GIVE_RANDOM_ITEM = 7,
    RESORT_BUY_STATE_MESSAGE = 8,
    RESORT_BUY_STATE_FINISH = 9,
};

// The record/person selection state machine.
enum {
    RESORT_SELECT_STATE_LIST = 0,
    RESORT_SELECT_STATE_UNUSED = 1,
    RESORT_SELECT_STATE_RESTORE_LIST = 2,
    RESORT_SELECT_STATE_CONFIRM = 3,
    RESORT_SELECT_STATE_CONFIRM_DUPLICATE = 4,
    RESORT_SELECT_STATE_WAIT_CONFIRM = 5,
    RESORT_SELECT_STATE_ACCEPT = 6,
    RESORT_SELECT_STATE_MESSAGE = 7,
    RESORT_SELECT_STATE_FINISH = 8,
};

// The reordering state machine.
enum {
    RESORT_REORDER_STATE_PROMPT = 0,
    RESORT_REORDER_STATE_LIST = 1,
    RESORT_REORDER_STATE_UNUSED = 2,
    RESORT_REORDER_STATE_RESTORE_LIST = 3,
    RESORT_REORDER_STATE_SWAP = 4,
    RESORT_REORDER_STATE_CONFIRM = 5,
    RESORT_REORDER_STATE_CONFIRM_DUPLICATE = 6,
    RESORT_REORDER_STATE_WAIT_CONFIRM = 7,
    RESORT_REORDER_STATE_MESSAGE = 8,
    RESORT_REORDER_STATE_FINISH = 9,
};

// The fade-in sequence.
enum {
    RESORT_SHOP_FADE_IN_BLACKEN = 0,
    RESORT_SHOP_FADE_IN_WAIT_BLACK = 1,
    RESORT_SHOP_FADE_IN_POSITION = 2,
    RESORT_SHOP_FADE_IN_CREATE = 3,
    RESORT_SHOP_FADE_IN_WAIT_LIST = 4,
    RESORT_SHOP_FADE_IN_REVEAL = 5,
    RESORT_SHOP_FADE_IN_WAIT_REVEAL = 6,
};

// The fade-out sequence.
enum {
    RESORT_SHOP_FADE_OUT_BLACKEN = 0,
    RESORT_SHOP_FADE_OUT_WAIT_BLACK = 1,
    RESORT_SHOP_FADE_OUT_RESTORE = 2,
    RESORT_SHOP_FADE_OUT_WAIT_REVEAL = 3,
};

// The windows with a known role; slot 4 is created but unused.
enum {
    RESORT_SHOP_WINDOW_MONEY_LABEL = 0,
    RESORT_SHOP_WINDOW_MONEY = 1,
    RESORT_SHOP_WINDOW_LIST = 2,
    RESORT_SHOP_WINDOW_BAG_COUNT = 3,
    RESORT_SHOP_WINDOW_DESCRIPTION = 5,
    RESORT_SHOP_WINDOW_MESSAGE = 6,
};

// A row of the menu. For the people, item is the object code
typedef struct {
    u16 id;
    u16 kind;
    u16 item;
    u16 count;
    u32 price;
    StrBuf *label;
    StrBuf *name;
    StrBuf *description;
    void *person;
} ResortShopEntry;

// Cell actor resources, as the ClActSys_LoadChars family returns them
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
} ResortShopClActRes;

typedef BOOL (*ResortShopUpdateFunc)(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
typedef void (*ResortShopFunc)(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
typedef void (*ResortShopListFunc)(ResortShopWork *wk, u8 mode, u8 subMode);
typedef BOOL (*ResortShopFadeFunc)(ResortShop *shop, Field *field, int *seq);

struct ResortShopWork {
    u8 state;
    u8 nextState;
    u16 unk2;
    HeapID heapId;
    Font *font;
    TrainerCardSave *trainerCard;
    BagSave *bag;
    GameRecords *records;
    ResortShopEntry entries[17];
    u16 entryCount;
    BmpWin *windows[7];
    ListMenuOption *options;
    BmpMenu *confirmDialog;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *priceFormat;
    StrBuf *message;
    StrBuf *boughtLabel;
    BmpMenuList *list;
    PrintWindow printWindow;
    PrintQueue *printQueue;
    TCBExManager *tcbManager;
    PrintStream *printStream;
    KeyCursor *keyCursor;
    ClActUnit *clactUnit;
    ClActor *actors[6];
    ResortShopClActRes res[3];
    ArcTool *iconArc;
    ResortShopEntry *selected;
    u16 listShown;
    ResortShopUpdateFunc update;
    ResortPeople *people;
    ResortNPC *npc;
    ResortSys *sys;
    ResortShopData *shops;
    const u16 *row;
    ResortBinary *table;
    JoinAvenueShop *flags;
    JoinAvenueOccupants *occupants;
    ResortPersonData **datas;
    ResortPerson *person;
    JoinAvenueInfo *info;
    u16 *vars[5];
    u16 cameraMode;
    // Added to the messages' IDs
    u16 msgBase;
    ResortShopFunc create;
    ResortShopFunc destroy;
    ResortShopListFunc swapEntries;
    ResortShopListFunc applyOrder;
    BmpMenuListCursorCallback cursorCallback;
    BmpMenuListPrintCallback printCallback;
    u16 mode;
    u16 msgs[3];
    BOOL restrictedGoodsUnlocked;
    u8 moveFrom;
    u8 moveTo;
    BOOL moving;
    u16 moveTop;
    u16 moveCursor;
    BOOL moved;
    u16 initialOrder[8];
    u16 order[8];
};

struct ResortShop {
    u8 state;
    u8 unk1;
    u8 mode;
    u8 subMode;
    BOOL useBoundary;
    ResortShopWork work;
    int seq;
    ResortShopFadeFunc fadeIn;
    ResortShopFadeFunc fadeOut;
    GameSystem *gsys;
};

static inline ResortShopEntry *ResortShop_GetEntry(ResortShopWork *wk, u32 index) {
    return &wk->entries[index];
}

typedef struct {
    u32 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u16 palette;
    u16 unkA;
} ResortShopWindowSetup;

// An area of the avenue in grid coordinates, and the camera setup for it
typedef struct {
    s16 xMin;
    s16 xMax;
    s16 zMin;
    s16 zMax;
    u32 camera;
} ResortShopArea;

typedef struct {
    u16 yaw;
    fx32 dx;
    fx32 dz;
} ResortShopCamera;

typedef struct {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
} ResortShopRect;

// Reading the layouts before their definitions keeps the unused entries in MWCC's
// shared constant pool, as in the original. This inline accessor emits no code.
const ResortShopWindowSetup sUnk021e8a6c;
const ResortShopWindowSetup sMoneyLabelWindowSetup;
const ResortShopWindowSetup sListWindowSetup;
const ResortShopWindowSetup sBagCountWindowSetup;
const ResortShopWindowSetup sUnk021e8a60;
const ResortShopWindowSetup sMessageWindowSetup;
const ResortShopWindowSetup sDescriptionWindowSetup;
const ResortShopWindowSetup sUnk021e8ab4;
const ResortShopWindowSetup sMoneyWindowSetup;
const ResortShopWindowSetup sWindow4;
const u8 sUnk021e8acc[16];

static inline u32 ResortShop_GetLayoutBG(u32 index) {
    switch (index) {
    case 0:
        return sUnk021e8a6c.bg;
    case 1:
        return sMoneyLabelWindowSetup.bg;
    case 2:
        return sListWindowSetup.bg;
    case 3:
        return sBagCountWindowSetup.bg;
    case 4:
        return sUnk021e8a60.bg;
    case 5:
        return sMessageWindowSetup.bg;
    case 6:
        return sDescriptionWindowSetup.bg;
    case 7:
        return sUnk021e8ab4.bg;
    case 8:
        return sMoneyWindowSetup.bg;
    case 9:
        return sWindow4.bg;
    default:
        return sUnk021e8acc[0];
    }
}

static const ConfirmDialogSetup sConfirmDialogSetup = { 1, 24, 13, 11, 25 };

const ResortShopWindowSetup sUnk021e8a6c = { 1, 1, 18, 30, 6, 11, 469 };

const ResortShopWindowSetup sListWindowSetup = { 1, 12, 1, 19, 16, 2, 54 };

const ResortShopWindowSetup sMessageWindowSetup = { 1, 1, 19, 30, 4, 11, 631 };

const ResortShopWindowSetup sWindow4 = { 1, 18, 15, 13, 2, 11, 447 };

const ResortShopWindowSetup sMoneyWindowSetup = { 1, 1, 3, 9, 2, 11, 38 };

const ResortShopWindowSetup sUnk021e8ab4 = { 1, 2, 2, 8, 2, 11, 38 };

const ResortShopWindowSetup sDescriptionWindowSetup = { 1, 5, 18, 27, 6, 11, 469 };

const ResortShopWindowSetup sMoneyLabelWindowSetup = { 1, 1, 1, 9, 2, 11, 20 };

const ResortShopWindowSetup sUnk021e8a60 = { 1, 5, 2, 5, 2, 11, 38 };

const ResortShopWindowSetup sBagCountWindowSetup = { 1, 1, 15, 15, 2, 11, 415 };

const u8 sUnk021e8acc[] = { 2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0 };

static const ResortShopRect sListRects[] = {
    { 0, 12, 32, 12 },
    { 0, 12, 32, 6 },
};

static const ResortShopArea sAreas[] = {
    { 17, 32, 0, 65, 3 },
    { 0, 17, 0, 80, 1 },
    { 17, 32, 65, 80, 1 },
};

static const ResortShopCamera sCameras[] = {
    { 0x8000, FX32_CONST(-70), 0 },
    { 0, FX32_CONST(70), 0 },
    { 0x3fff, 0, FX32_CONST(-70) },
    { 0xc000, 0, FX32_CONST(70) },
};

// clang-format off
static const ClActorSetup sActorSetups[] = {
    { 172, 22, 0, 0, 1 },
    { 172, 92, 0, 0, 0 },
    { 172, 132, 1, 0, 0 },
    { 224, 128, 2, 0, 0 },
    { 21, 172, 0, 0, 1 },
    { 18, 168, 0, 0, 1 },
    { 18, 168, 1, 0, 1 },
};

// clang-format on

static const ResortShopWindowSetup *sWindowSetups[] = {
    &sMoneyLabelWindowSetup,  &sMoneyWindowSetup,   &sListWindowSetup, &sBagCountWindowSetup, &sWindow4,
    &sDescriptionWindowSetup, &sMessageWindowSetup,
};

static BOOL ResortShop_Run(VM *vm, void *env);
static void ResortShop_LoadPriceStrings(ResortShopWork *wk);
static void ResortShop_Create(ResortShop *shop);
static BOOL ResortShop_ShowList(ResortShop *shop);
static void ResortShop_Destroy(ResortShop *shop);
static BOOL ResortShop_UpdateShop(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static BOOL ResortShop_UpdateRecords(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static BOOL ResortShop_UpdateOrder(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_MakeItemEntries(ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_MakeRecordEntries(ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_MakePersonEntries(ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_SwapRecordEntries(ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_SwapPersonEntries(ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_FreeEntries(ResortShopWork *wk);
static u8 ResortShop_InitMessages(ResortShopWork *wk, u8 mode);
static void ResortShop_FreeMessages(ResortShopWork *wk);
static void ResortShop_LoadBG(ResortShopWork *wk, u32 mode);
static void ResortShop_ClearBG(ResortShopWork *wk);
static void ResortShop_LoadCursorResources(ResortShopWork *wk);
static void ResortShop_LoadItemIconResources(ResortShopWork *wk);
static void ResortShop_ShowItemIcon(ResortShopWork *wk, u16 item);
static void ResortShop_ShowPersonModel(ResortShopWork *wk, u16 objCode);
static void ResortShop_CreateActors(ResortShopWork *wk, const ClActorSetup *iconSetup);
static void ResortShop_CreateClActUnit(ResortShopWork *wk);
static void ResortShop_CreateItemActors(ResortShopWork *wk);
static void ResortShop_CreatePersonActors(ResortShopWork *wk);
static void ResortShop_FreeActors(ResortShopWork *wk);
static void ResortShop_CreateWindows(ResortShopWork *wk);
static void ResortShop_PrintMoneyLabel(ResortShopWork *wk);
static void ResortShop_FreeWindows(ResortShopWork *wk);
static void ResortShop_PrintMoney(ResortShopWork *wk);
static void ResortShop_PrintBagCount(ResortShopWork *wk, u16 item);
static void ResortShop_PrintDescription(ResortShopWork *wk, StrBuf *strbuf);
static void ResortShop_CreateList(ResortShopWork *wk, BmpMenuListCursorCallback cursorCallback,
                                  BmpMenuListPrintCallback printCallback);
static void ResortShop_PrintPrice(BmpMenuList *list, s32 value, u8 y);
static void ResortShop_FreeList(ResortShopWork *wk);
static BOOL ResortShop_PrintStreamCallback(u32 event);
static void ResortShop_PrintItemMessage(ResortShopWork *wk, u32 msgId, u16 item, u16 count);
static void ResortShop_PrintMessage(ResortShopWork *wk, u32 msgId);
static void ResortShop_PrintMessageNoWait(ResortShopWork *wk, u32 msgId);
static void ResortShop_DrawMessageFrame(BmpWin *window);
static void ResortShop_PrintMessageInFrame(ResortShopWork *wk, u32 msgId);
static void ResortShop_PrintEntryMessage(ResortShopWork *wk, u32 msgId, ResortShopEntry *entry);
static void ResortShop_ClearListScreen(u32 which);
static ResortPeople *ResortShop_GetPeople(Field *field);
static ResortNPC *ResortShop_GetNPC(Field *field);
static void ResortShop_CreateItemList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_DestroyItemList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_ItemCursorCallback(BmpMenuList *list, s32 value, u8 init);
static void ResortShop_CreateRecordList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_DestroyRecordList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_CreatePersonList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_DestroyPersonList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode);
static StrBuf *ResortShop_MakePersonDescription(ResortShopWork *wk, JoinAvenuePerson *person);
static void ResortShop_MakePersonEntry(ResortShopWork *wk, ResortPersonData *data, u32 index, ResortShopEntry *entry);
static void ResortShop_ModelCursorCallback(BmpMenuList *list, s32 value, u8 init);
static void ResortShop_OrderCursorCallback(BmpMenuList *list, s32 value, u8 init);
static void ResortShop_UpdateMoveArrow(ResortShopWork *wk);
static void ResortShop_ApplyRecordOrder(ResortShopWork *wk, u8 mode, u8 subMode);
static void ResortShop_ApplyPersonOrder(ResortShopWork *wk, u8 mode, u8 subMode);
static u8 ResortShop_GetTextureIndex(u32 index);
static void ResortShop_GetModelTexture(ArcTool *handle, u16 objCode, u8 *texture, u16 *fileId);
static u32 ResortShop_GetDiscountedPrice(ResortShopWork *wk, u32 price);
static void ResortShop_SetCamera(ResortShop *shop, Field *field, u16 cameraMode);
static BOOL ResortShop_FadeIn(ResortShop *shop, Field *field, int *seq);
static BOOL ResortShop_FadeOut(ResortShop *shop, Field *field, int *seq);

BOOL Scrcmd_ResortShop_Open(VM *vm, FieldScriptEnv *env) {
    u16 mode = ScriptReadAny(vm, env);
    u16 subMode = ScriptReadAny(vm, env);
    u16 cameraMode = ScriptReadAny(vm, env);
    u16 *resultOut = ScriptReadVar(vm, env);
    u16 *itemOut = ScriptReadVar(vm, env);
    u16 *countOut = ScriptReadVar(vm, env);
    u16 *entryIdOut = ScriptReadVar(vm, env);
    u16 *priceOut = ScriptReadVar(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    void **ptr = ScriptWork_GetUserHeapPtr(FieldScriptEnv_GetScriptWork(env));
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);
    ResortSys *sys = ResortSys_Get(field);
    ResortShop *shop = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ResortShop), TRUE, "scrcmd_resort_shop.c", 375);

    *ptr = shop;
    shop->work.font = MsgBGPrinter_GetFont(msgBGSys);
    shop->work.trainerCard = getTrainerCardDataBlkAddress(gameData);
    shop->work.heapId = HEAPID_FIELDMAP;
    shop->work.bag = GameData_GetBag(gameData);
    shop->work.records = GameData_GetRecords(gameData);
    shop->work.vars[RESORT_SHOP_OUT_RESULT] = resultOut;
    shop->work.vars[RESORT_SHOP_OUT_ITEM] = itemOut;
    shop->work.vars[RESORT_SHOP_OUT_COUNT] = countOut;
    shop->work.vars[RESORT_SHOP_OUT_ENTRY_ID] = entryIdOut;
    shop->work.vars[RESORT_SHOP_OUT_PRICE] = priceOut;
    shop->work.shops = ResortSys_GetShopData(sys);
    shop->work.table = ResortSys_GetRaffles(sys);
    shop->work.occupants = ResortSys_GetOccupants(sys);
    shop->work.datas = ResortSys_GetDatas(sys);
    shop->work.info = ResortSys_GetInfo(sys);
    shop->work.sys = sys;
    shop->work.cameraMode = cameraMode;
    shop->work.people = ResortShop_GetPeople(field);
    shop->work.npc = ResortShop_GetNPC(field);
    shop->gsys = gsys;
    shop->work.person = ResortSys_GetScriptPerson(env, shop->work.people);
    if (JoinAvenue_GetParam(shop->work.info, JOIN_AVE_INFO_UNLOCKED_6, 0) != 0) {
        shop->work.restrictedGoodsUnlocked = TRUE;
    } else {
        shop->work.restrictedGoodsUnlocked = FALSE;
    }
    shop->fadeIn = ResortShop_FadeIn;
    shop->fadeOut = ResortShop_FadeOut;
    shop->work.msgBase = ResortPerson_GetParam(shop->work.person, JOIN_AVE_PARAM_GENDER, NULL);
    *shop->work.vars[RESORT_SHOP_OUT_RESULT] = 0;
    shop->work.mode = mode;
    switch (mode) {
    case RESORT_SHOP_MODE_BUY:
    default:
        GameBeacon_BroadcastJoinAvenueShopping();
        shop->mode = mode;
        shop->subMode = 0;
        shop->work.update = ResortShop_UpdateShop;
        shop->work.create = ResortShop_CreateItemList;
        shop->work.destroy = ResortShop_DestroyItemList;
        shop->work.cursorCallback = ResortShop_ItemCursorCallback;
        shop->work.printCallback = ResortShop_PrintPrice;
        shop->work.flags = JoinAvenuePerson_GetShop(ResortPerson_GetJoinAvenuePerson(shop->work.person));
        shop->work.row =
            ResortShopData_GetPersonShop(shop->work.shops, ResortPerson_GetJoinAvenuePerson(shop->work.person));
        break;
    case RESORT_SHOP_MODE_SELECT:
        shop->mode = mode;
        shop->subMode = subMode;
        shop->work.update = ResortShop_UpdateRecords;
        switch (subMode) {
        case RESORT_SHOP_SELECT_RECORD:
        default:
            shop->work.create = ResortShop_CreateRecordList;
            shop->work.destroy = ResortShop_DestroyRecordList;
            shop->work.msgs[1] = 52;
            shop->work.msgs[2] = 53;
            shop->work.cursorCallback = ResortShop_ModelCursorCallback;
            shop->work.printCallback = NULL;
            break;
        case RESORT_SHOP_SELECT_RECORD_ALT:
            shop->work.create = ResortShop_CreateRecordList;
            shop->work.destroy = ResortShop_DestroyRecordList;
            shop->work.msgs[1] = 50;
            shop->work.msgs[2] = 51;
            shop->work.cursorCallback = ResortShop_ModelCursorCallback;
            shop->work.printCallback = NULL;
            break;
        case RESORT_SHOP_SELECT_PERSON:
            shop->work.create = ResortShop_CreatePersonList;
            shop->work.destroy = ResortShop_DestroyPersonList;
            shop->work.msgs[1] = 54;
            shop->work.msgs[2] = 55;
            shop->work.cursorCallback = ResortShop_ModelCursorCallback;
            shop->work.printCallback = NULL;
            break;
        case RESORT_SHOP_SELECT_PERSON_ALT:
            shop->work.create = ResortShop_CreatePersonList;
            shop->work.destroy = ResortShop_DestroyPersonList;
            shop->work.msgs[1] = 56;
            shop->work.msgs[2] = 57;
            shop->work.cursorCallback = ResortShop_ModelCursorCallback;
            shop->work.printCallback = NULL;
            break;
        }
        break;
    case RESORT_SHOP_MODE_REORDER:
        shop->mode = mode;
        shop->subMode = subMode;
        shop->work.update = ResortShop_UpdateOrder;
        switch (subMode) {
        case RESORT_SHOP_REORDER_RECORDS:
            shop->work.create = ResortShop_CreateRecordList;
            shop->work.destroy = ResortShop_DestroyRecordList;
            shop->work.swapEntries = ResortShop_SwapRecordEntries;
            shop->work.applyOrder = ResortShop_ApplyRecordOrder;
            shop->work.msgs[0] = 67;
            shop->work.msgs[1] = 59;
            shop->work.msgs[2] = 68;
            shop->work.cursorCallback = ResortShop_OrderCursorCallback;
            shop->work.printCallback = NULL;
            break;
        case RESORT_SHOP_REORDER_PEOPLE:
            shop->work.create = ResortShop_CreatePersonList;
            shop->work.destroy = ResortShop_DestroyPersonList;
            shop->work.swapEntries = ResortShop_SwapPersonEntries;
            shop->work.applyOrder = ResortShop_ApplyPersonOrder;
            shop->work.msgs[0] = 58;
            shop->work.msgs[1] = 59;
            shop->work.msgs[2] = 60;
            shop->work.cursorCallback = ResortShop_OrderCursorCallback;
            shop->work.printCallback = NULL;
            break;
        }
        break;
    }
    VM_SetNativeCallback(vm, ResortShop_Run);
    return TRUE;
}

static BOOL ResortShop_Run(VM *vm, void *env) {
    void **ptr = ScriptWork_GetUserHeapPtr(FieldScriptEnv_GetScriptWork(env));
    ResortShop *shop = *ptr;
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    Field_GetCameraSystem(field);
    switch (shop->state) {
    case RESORT_SHOP_STATE_FADE_IN:
        if (shop->fadeIn(shop, field, &shop->seq)) {
            setAlphaBlend_wrapper(FALSE);
            shop->work.state = RESORT_SHOP_WORK_INITIAL;
            shop->state++;
        }
        break;
    case RESORT_SHOP_STATE_UPDATE:
        if (shop->work.update(gsys, &shop->work, shop->mode, shop->subMode)) {
            shop->state++;
        }
        break;
    case RESORT_SHOP_STATE_FADE_OUT:
        if (shop->fadeOut(shop, field, &shop->seq)) {
            setAlphaBlend_wrapper(TRUE);
            shop->state++;
        }
        break;
    case RESORT_SHOP_STATE_FREE:
        GFL_HeapFree(*ptr);
        return TRUE;
    }
    return FALSE;
}

static void ResortShop_Init(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    GetGameDataPlayerInfo(GSYS_GetGameData(gsys));
    ResortShop_InitMessages(wk, mode);
    ResortShop_LoadPriceStrings(wk);
    ResortShop_CreateWindows(wk);
    ResortShop_CreateClActUnit(wk);
}

static void ResortShop_LoadPriceStrings(ResortShopWork *wk) {
    wk->priceFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty);
    wk->boughtLabel = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_2);
}

static u32 ResortShop_GetCash(ResortShopWork *wk) {
    return getCash(wk->trainerCard);
}

// Buying the chosen item: the answers when it was bought already, does not fit in the bag or costs too much, and else
// the question
static void ResortShop_TryBuy(ResortShopWork *wk) {
    u32 cash = ResortShop_GetCash(wk);

    if (JoinAvenueShop_GetFlag(wk->flags, wk->selected->id) == TRUE) {
        if (wk->selected->kind == RESORT_SHOP_GOODS_ITEM) {
            ResortShop_PrintMessage(wk, wk->msgBase + 17);
        } else {
            ResortShop_PrintMessage(wk, wk->msgBase + 21);
        }
        wk->state = RESORT_BUY_STATE_MESSAGE;
        wk->nextState = RESORT_BUY_STATE_RESTORE_LIST;
        return;
    }
    if (wk->selected->kind == RESORT_SHOP_GOODS_ITEM &&
        !BagSave_CheckAvailItemSpace(wk->bag, wk->selected->item, wk->selected->count, wk->heapId)) {
        ResortShop_PrintMessage(wk, wk->msgBase + 10);
        wk->state = RESORT_BUY_STATE_MESSAGE;
        wk->nextState = RESORT_BUY_STATE_RESTORE_LIST;
        return;
    }
    if (cash < wk->selected->price) {
        ResortShop_PrintMessage(wk, wk->msgBase);
        wk->state = RESORT_BUY_STATE_MESSAGE;
        wk->nextState = RESORT_BUY_STATE_RESTORE_LIST;
        return;
    }
    if (wk->selected->kind == RESORT_SHOP_GOODS_ITEM) {
        ResortShop_PrintEntryMessage(wk, wk->msgBase + 4, wk->selected);
    } else {
        ResortShop_PrintEntryMessage(wk, wk->msgBase + 6, wk->selected);
    }
    wk->state = RESORT_BUY_STATE_MESSAGE;
    wk->nextState = RESORT_BUY_STATE_CONFIRM;
}

static void ResortShop_UpdateItemList(ResortShopWork *wk) {
    s32 result = BmpMenuList_Update(wk->list);

    if (result == BMPMENULIST_NULL) {
        return;
    }
    if (result == BMPMENULIST_CANCEL) {
        wk->selected = NULL;
        wk->state = RESORT_BUY_STATE_FINISH;
        return;
    }
    wk->selected = &wk->entries[result];
    ResortShop_TryBuy(wk);
}

static void ResortShop_UpdateRecordList(ResortShopWork *wk) {
    s32 result = BmpMenuList_Update(wk->list);

    if (result == BMPMENULIST_NULL) {
        return;
    }
    if (result == BMPMENULIST_CANCEL) {
        wk->selected = NULL;
        wk->state = RESORT_SELECT_STATE_FINISH;
        return;
    }
    wk->selected = &wk->entries[result];
    ResortShop_PrintEntryMessage(wk, wk->msgs[1], wk->selected);
    wk->state = RESORT_SELECT_STATE_MESSAGE;
    wk->nextState = RESORT_SELECT_STATE_CONFIRM;
}

// The list of mode 2, where the first choice picks an entry up and the second puts it down
static void ResortShop_UpdateOrderList(ResortShopWork *wk) {
    s32 result = BmpMenuList_Update(wk->list);

    if (result == BMPMENULIST_NULL) {
        return;
    }
    if (result == BMPMENULIST_CANCEL) {
        if (!wk->moving) {
            wk->selected = NULL;
            wk->state = RESORT_REORDER_STATE_FINISH;
            return;
        }
        wk->moving = FALSE;
        ClActor_SetVisible(wk->actors[5], FALSE);
        return;
    }
    if (wk->moving) {
        wk->moveTo = result;
        if (wk->moveTo != wk->moveFrom) {
            wk->moved = TRUE;
            wk->state = RESORT_REORDER_STATE_SWAP;
        }
        return;
    }
    wk->moveFrom = result;
    wk->moving = TRUE;
    wk->selected = &wk->entries[result];
    BmpMenuList_GetPos(wk->list, &wk->moveTop, &wk->moveCursor);
    ResortShop_UpdateMoveArrow(wk);
}

// Prints the message, returning the next state once it is done
static u8 ResortShop_UpdateMessage(ResortShopWork *wk) {
    u32 state = PrintStream_GetState(wk->printStream);

    if (wk->keyCursor != NULL) {
        KeyCursor_Update(wk->keyCursor, wk->printStream, wk->windows[RESORT_SHOP_WINDOW_MESSAGE]);
    }
    if (state == PRINT_STREAM_DONE) {
        PrintStream_Delete(wk->printStream);
        return wk->nextState;
    }
    if (state == PRINT_STREAM_RUNNING) {
        if ((GCTX_HIDGetHeldKeys() & PAD_BUTTON_A) || (GCTX_HIDGetHeldKeys() & PAD_BUTTON_B)) {
            PrintStream_SkipWait(wk->printStream, 0);
        }
    } else if (state == PRINT_STREAM_PAUSED) {
        BOOL next = FALSE;

        if (wk->keyCursor != NULL) {
            if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
                next = TRUE;
            }
        } else if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) ||
                   (GCTX_HIDGetPressedKeys() & PAD_PLUS_KEY_MASK)) {
            next = TRUE;
        }
        if (next) {
            PrintStream_Continue(wk->printStream);
            GFL_SndSEPlay(1351);
        }
    }
    return wk->state;
}

static void ResortShop_Pay(ResortShopWork *wk, u32 price) {
    subCashFromTotal(wk->trainerCard, price);
    RecordAdd(wk->records, 22, price);
    RecordAddOne(wk->records, 21);
}

static void ResortShop_Create(ResortShop *shop) {
    ResortShopWork *wk = &shop->work;

    ResortShop_Init(shop->gsys, wk, shop->mode, shop->subMode);
    ResortShop_LoadBG(wk, shop->mode);
    if (shop->mode == RESORT_SHOP_MODE_BUY) {
        ResortShop_PrintMoneyLabel(wk);
        ResortShop_PrintMoney(wk);
    }
    wk->create(shop->gsys, wk, shop->mode, shop->subMode);
}

// Shows the list's window once everything is printed
static BOOL ResortShop_ShowList(ResortShop *shop) {
    ResortShopWork *wk = &shop->work;

    if (!PrintQueue_IsEmpty(wk->printQueue)) {
        return FALSE;
    }
    if (wk->listShown == 0) {
        BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        wk->listShown++;
        return TRUE;
    }
    return FALSE;
}

static void ResortShop_Destroy(ResortShop *shop) {
    ResortShopWork *wk = &shop->work;

    wk->destroy(shop->gsys, wk, shop->mode, shop->subMode);
    ResortShop_FreeList(wk);
    ResortShop_FreeActors(wk);
    ResortShop_FreeWindows(wk);
    ResortShop_ClearBG(wk);
    ResortShop_FreeEntries(wk);
    ResortShop_FreeMessages(wk);
}

// Mode 0, a shop
static BOOL ResortShop_UpdateShop(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    u32 result;

    switch (wk->state) {
    case RESORT_BUY_STATE_LIST:
        ResortShop_UpdateItemList(wk);
        break;
    case RESORT_BUY_STATE_UNUSED:
        break;
    case RESORT_BUY_STATE_RESTORE_LIST:
        ResortShop_ClearListScreen(0);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_DESCRIPTION]);
        GFL_BGSysQueueScrLoad(1);
        ClActor_SetVisible(wk->actors[3], FALSE);
        BmpMenuList_Redraw(wk->list);
        wk->state = RESORT_BUY_STATE_LIST;
        break;
    case RESORT_BUY_STATE_CONFIRM:
        ResortShop_ClearListScreen(1);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        ClActor_SetVisible(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        if (wk->selected->kind == RESORT_SHOP_GOODS_ITEM) {
            ResortShop_PrintBagCount(wk, wk->selected->item);
        }
        wk->state = RESORT_BUY_STATE_WAIT_CONFIRM;
        break;
    case RESORT_BUY_STATE_CONFIRM_WITHOUT_COUNT:
        ResortShop_ClearListScreen(1);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        ClActor_SetVisible(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = RESORT_BUY_STATE_WAIT_CONFIRM;
        break;
    case RESORT_BUY_STATE_WAIT_CONFIRM:
        result = ConfirmDialog_Update(wk->confirmDialog);
        if (result != BMPMENU_NULL) {
            if (result == CONFIRM_DIALOG_RESULT_YES) {
                wk->state = RESORT_BUY_STATE_BUY;
                ResortShop_ClearListScreen(1);
                BmpMenuList_Redraw(wk->list);
                BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
                GFL_BGSysQueueScrLoad(1);
            } else if (result == BMPMENU_CANCEL) {
                wk->state = RESORT_BUY_STATE_RESTORE_LIST;
            }
        }
        break;
    case RESORT_BUY_STATE_BUY:
        if (wk->selected->kind == RESORT_SHOP_GOODS_ITEM) {
            GFL_SndSEPlay(1621);
            ResortShop_PrintItemMessage(wk, wk->msgBase + 8, wk->selected->item, wk->selected->count);
            ResortShop_Pay(wk, wk->selected->price);
            ResortShop_PrintMoney(wk);
            BagSave_AddItem(wk->bag, wk->selected->item, wk->selected->count, wk->heapId);
            JoinAvenueShop_SetFlag(wk->flags, wk->selected->id, TRUE);
            wk->state = RESORT_BUY_STATE_MESSAGE;
            wk->nextState = RESORT_BUY_STATE_RESTORE_LIST;
        } else if (wk->selected->kind == RESORT_SHOP_GOODS_RANDOM_ITEM) {
            GFL_SndSEPlay(1621);
            ResortShop_PrintEntryMessage(wk, wk->msgBase + 61, wk->selected);
            ResortShop_Pay(wk, wk->selected->price);
            ResortShop_PrintMoney(wk);
            JoinAvenueShop_SetFlag(wk->flags, wk->selected->id, TRUE);
            wk->state = RESORT_BUY_STATE_MESSAGE;
            wk->nextState = RESORT_BUY_STATE_GIVE_RANDOM_ITEM;
        } else {
            ResortShop_PrintMessage(wk, wk->msgBase + 15);
            wk->state = RESORT_BUY_STATE_MESSAGE;
            wk->nextState = RESORT_BUY_STATE_FINISH;
        }
        break;
    case RESORT_BUY_STATE_GIVE_RANDOM_ITEM: {
        u16 row = wk->selected->item;
        ResortPersonData *data = ResortPerson_GetData(wk->person);
        u32 prize = ResortBinary_Get(
            wk->table, row,
            join_ave_raffle_shop(wk->table, row,
                                 ResortData_Roll(data, wk->selected->id + 5, ResortBinary_Get(wk->table, row, 0))) *
                    2 +
                3);

        GFL_WordSetSetStrbuf(wk->wordSet, 2, wk->selected->name, 2, 1, 2);
        if (BagSave_CheckAvailItemSpace(wk->bag, prize, 1, wk->heapId)) {
            ResortShop_PrintItemMessage(wk, wk->msgBase + 63, prize, 1);
            BagSave_AddItem(wk->bag, prize, 1, wk->heapId);
        } else {
            ResortShop_PrintItemMessage(wk, wk->msgBase + 65, prize, 1);
        }
        wk->state = RESORT_BUY_STATE_MESSAGE;
        wk->nextState = RESORT_BUY_STATE_RESTORE_LIST;
        break;
    }
    case RESORT_BUY_STATE_MESSAGE:
        wk->state = ResortShop_UpdateMessage(wk);
        break;
    case RESORT_BUY_STATE_FINISH:
        if (wk->selected != NULL) {
            *wk->vars[RESORT_SHOP_OUT_RESULT] = wk->selected->kind;
            *wk->vars[RESORT_SHOP_OUT_ITEM] = wk->selected->item;
            *wk->vars[RESORT_SHOP_OUT_COUNT] = wk->selected->count;
            *wk->vars[RESORT_SHOP_OUT_ENTRY_ID] = wk->selected->id;
            *wk->vars[RESORT_SHOP_OUT_PRICE] = wk->selected->price / 10;
        } else {
            *wk->vars[RESORT_SHOP_OUT_RESULT] = RESORT_SHOP_SELECTION_CANCEL;
            *wk->vars[RESORT_SHOP_OUT_ITEM] = 0;
            *wk->vars[RESORT_SHOP_OUT_COUNT] = 0;
            *wk->vars[RESORT_SHOP_OUT_ENTRY_ID] = 0;
            *wk->vars[RESORT_SHOP_OUT_PRICE] = 0;
        }
        return TRUE;
    }
    PrintQueue_Main(wk->printQueue);
    PrintWindow_Flush(&wk->printWindow, wk->printQueue);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    return FALSE;
}

// Mode 1, the records
static BOOL ResortShop_UpdateRecords(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    u32 result;

    switch (wk->state) {
    case RESORT_SELECT_STATE_LIST:
        ResortShop_UpdateRecordList(wk);
        break;
    case RESORT_SELECT_STATE_UNUSED:
        break;
    case RESORT_SELECT_STATE_RESTORE_LIST:
        ResortShop_ClearListScreen(0);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_DESCRIPTION]);
        GFL_BGSysQueueScrLoad(1);
        ClActor_SetVisible(wk->actors[3], FALSE);
        BmpMenuList_Redraw(wk->list);
        wk->state = RESORT_SELECT_STATE_LIST;
        break;
    case RESORT_SELECT_STATE_CONFIRM:
        ResortShop_ClearListScreen(1);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        ClActor_SetVisible(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = RESORT_SELECT_STATE_WAIT_CONFIRM;
        break;
    case RESORT_SELECT_STATE_CONFIRM_DUPLICATE:
        ResortShop_ClearListScreen(1);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        ClActor_SetVisible(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = RESORT_SELECT_STATE_WAIT_CONFIRM;
        break;
    case RESORT_SELECT_STATE_WAIT_CONFIRM:
        result = ConfirmDialog_Update(wk->confirmDialog);
        if (result != BMPMENU_NULL) {
            if (result == CONFIRM_DIALOG_RESULT_YES) {
                wk->state = RESORT_SELECT_STATE_ACCEPT;
                ResortShop_ClearListScreen(1);
                BmpMenuList_Redraw(wk->list);
                BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
                GFL_BGSysQueueScrLoad(1);
            } else if (result == BMPMENU_CANCEL) {
                wk->state = RESORT_SELECT_STATE_RESTORE_LIST;
            }
        }
        break;
    case RESORT_SELECT_STATE_ACCEPT: {
        StrBuf *strbuf = GFL_StrBufCreate(64, wk->heapId);
        u16 name[8];
        u8 gender;

        joinAveTextHandler(wk->selected->person, JOIN_AVE_PARAM_NAME, name);
        GFL_StrBufLoadString(strbuf, name);
        gender = joinAveTextHandler(wk->selected->person, JOIN_AVE_PARAM_GENDER, NULL);
        GFL_WordSetSetStrbuf(wk->wordSet, 2, strbuf, gender, 1, 2);
        GFL_StrBufFree(strbuf);
        ResortShop_PrintMessage(wk, wk->msgs[2]);
        wk->state = RESORT_SELECT_STATE_MESSAGE;
        wk->nextState = RESORT_SELECT_STATE_FINISH;
        break;
    }
    case RESORT_SELECT_STATE_MESSAGE:
        wk->state = ResortShop_UpdateMessage(wk);
        break;
    case RESORT_SELECT_STATE_FINISH:
        if (wk->selected != NULL) {
            *wk->vars[RESORT_SHOP_OUT_RESULT] = wk->selected->kind;
            *wk->vars[RESORT_SHOP_OUT_ITEM] = 0;
            *wk->vars[RESORT_SHOP_OUT_COUNT] = 0;
            *wk->vars[RESORT_SHOP_OUT_ENTRY_ID] = wk->selected->id;
            *wk->vars[RESORT_SHOP_OUT_PRICE] = wk->selected->price;
        } else {
            *wk->vars[RESORT_SHOP_OUT_RESULT] = RESORT_SHOP_SELECTION_CANCEL;
            *wk->vars[RESORT_SHOP_OUT_ITEM] = 0;
            *wk->vars[RESORT_SHOP_OUT_COUNT] = 0;
            *wk->vars[RESORT_SHOP_OUT_ENTRY_ID] = 0;
            *wk->vars[RESORT_SHOP_OUT_PRICE] = 0;
        }
        return TRUE;
    }
    PrintQueue_Main(wk->printQueue);
    PrintWindow_Flush(&wk->printWindow, wk->printQueue);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    return FALSE;
}

// Mode 2, the order of the records or the people
static BOOL ResortShop_UpdateOrder(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    u32 result;

    switch (wk->state) {
    case RESORT_REORDER_STATE_PROMPT:
        ResortShop_PrintMessageNoWait(wk, wk->msgs[0]);
        wk->state = RESORT_REORDER_STATE_MESSAGE;
        wk->nextState = RESORT_REORDER_STATE_RESTORE_LIST;
        break;
    case RESORT_REORDER_STATE_LIST:
        ResortShop_UpdateOrderList(wk);
        break;
    case RESORT_REORDER_STATE_UNUSED:
        break;
    case RESORT_REORDER_STATE_RESTORE_LIST:
        ResortShop_ClearListScreen(0);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_DESCRIPTION]);
        GFL_BGSysQueueScrLoad(1);
        ClActor_SetVisible(wk->actors[3], FALSE);
        BmpMenuList_Redraw(wk->list);
        wk->state = RESORT_REORDER_STATE_LIST;
        break;
    case RESORT_REORDER_STATE_SWAP:
        wk->swapEntries(wk, mode, subMode);
        wk->moving = FALSE;
        ClActor_SetVisible(wk->actors[5], FALSE);
        ResortShop_FreeList(wk);
        ResortShop_CreateList(wk, wk->cursorCallback, wk->printCallback);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        GFL_BGSysQueueScrLoad(1);
        ResortShop_PrintMessage(wk, wk->msgs[2]);
        wk->state = RESORT_REORDER_STATE_MESSAGE;
        wk->nextState = RESORT_REORDER_STATE_RESTORE_LIST;
        break;
    case RESORT_REORDER_STATE_CONFIRM:
        ResortShop_ClearListScreen(1);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        ClActor_SetVisible(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = RESORT_REORDER_STATE_WAIT_CONFIRM;
        break;
    case RESORT_REORDER_STATE_CONFIRM_DUPLICATE:
        ResortShop_ClearListScreen(1);
        BmpWin_FlushMap(wk->windows[RESORT_SHOP_WINDOW_LIST]);
        ClActor_SetVisible(wk->actors[3], FALSE);
        wk->confirmDialog = ShopUI_CreateConfirmDialog(&sConfirmDialogSetup, 1, 9, 0, wk->heapId);
        wk->state = RESORT_REORDER_STATE_WAIT_CONFIRM;
        break;
    case RESORT_REORDER_STATE_WAIT_CONFIRM:
        result = ConfirmDialog_Update(wk->confirmDialog);
        if (result != BMPMENU_NULL) {
            if (result == CONFIRM_DIALOG_RESULT_YES) {
                wk->state = RESORT_REORDER_STATE_SWAP;
            } else if (result == BMPMENU_CANCEL) {
                wk->state = RESORT_REORDER_STATE_RESTORE_LIST;
            }
        }
        break;
    case RESORT_REORDER_STATE_MESSAGE:
        wk->state = ResortShop_UpdateMessage(wk);
        break;
    case RESORT_REORDER_STATE_FINISH:
        wk->applyOrder(wk, mode, subMode);
        *wk->vars[RESORT_SHOP_OUT_RESULT] =
            GFL_STD_MemCmp(wk->order, wk->initialOrder, sizeof(wk->order)) != 0 ? TRUE : FALSE;
        return TRUE;
    }
    PrintQueue_Main(wk->printQueue);
    PrintWindow_Flush(&wk->printWindow, wk->printQueue);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    GFL_TCBExMgrUpdate(wk->tcbManager);
    return FALSE;
}

// The shop's items
static void ResortShop_MakeItemEntries(ResortShopWork *wk, u8 mode, u8 subMode) {
    int i;
    int count;
    MsgData *itemNames;
    MsgData *scriptMsgData;
    MsgData *itemDescriptions;
    int n = 0;

    itemNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_NAMES, wk->heapId);
    scriptMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, wk->heapId);
    itemDescriptions = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ITEM_DESCRIPTIONS, wk->heapId);
    count = ResortShopData_GetGoodsCount(wk->shops, wk->row);
    wk->options = ListMenuCore_CreateOptionList(count + 1, wk->heapId);
    for (i = 0; i < count; i++) {
        const u16 *item = ResortShopData_GetGoods(wk->shops, wk->row, i);
        u16 price = ResortShopData_GetGoodsParam(item, 4);

        if (ResortShopData_GetGoodsParam(item, 0) != 0 && !wk->restrictedGoodsUnlocked) {
            continue;
        }
        wk->entries[n].id = i;
        wk->entries[n].kind = ResortShopData_GetGoodsParam(item, 5);
        wk->entries[n].price = ResortShop_GetDiscountedPrice(wk, price * 10);
        wk->entries[n].item = ResortShopData_GetGoodsParam(item, 6);
        wk->entries[n].count = ResortShopData_GetGoodsParam(item, 7);
        if (ResortShopData_GetGoodsParam(item, 1) != 0) {
            u16 nameId = ResortShopData_GetGoodsParam(item, 2);
            u16 descriptionId = ResortShopData_GetGoodsParam(item, 3);

            wk->entries[n].label = GFL_MsgDataLoadStrbufNew(scriptMsgData, nameId);
            wk->entries[n].name = GFL_MsgDataLoadStrbufNew(scriptMsgData, nameId);
            wk->entries[n].description = GFL_MsgDataLoadStrbufNew(scriptMsgData, descriptionId);
        } else {
            wk->entries[n].label = GFL_MsgDataLoadStrbufNew(itemNames, wk->entries[n].item);
            wk->entries[n].name = GFL_MsgDataLoadStrbufNew(itemNames, wk->entries[n].item);
            wk->entries[n].description = GFL_MsgDataLoadStrbufNew(itemDescriptions, wk->entries[n].item);
        }
        ListMenuCore_AppendStrBufOption(&wk->options[n], wk->entries[n].label, n, wk->heapId);
        n++;
    }
    wk->entryCount = n;
    ListMenuCore_AppendMsgOption(&wk->options[n], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
    GFL_MsgDataFree(itemDescriptions);
    GFL_MsgDataFree(scriptMsgData);
    GFL_MsgDataFree(itemNames);
}

// The four records
static void ResortShop_MakeRecordEntries(ResortShopWork *wk, u8 mode, u8 subMode) {
    int i;
    int count;
    int n;
    u16 name[8];

    count = JoinAvenueOccupants_CountRecords(wk->occupants);
    wk->options = ListMenuCore_CreateOptionList(count + 1, wk->heapId);
    n = 0;
    for (i = 0; i < 4; i++) {
        void *record = JoinAvenueOccupants_GetRecord(wk->occupants, i);
        u16 year;
        u16 month;
        u16 day;
        StrBuf *labelFormat;
        StrBuf *descriptionFormat;
        u8 gender;
        StrBuf *nameBuf;
        StrBuf *label;
        StrBuf *description;

        wk->initialOrder[i] = i;
        wk->order[i] = i;
        if (JoinAvenueRecord_IsEmpty(record)) {
            continue;
        }
        wk->entries[n].id = i;
        wk->entries[n].kind = 0;
        wk->entries[n].price = 0;
        wk->entries[n].item = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_TRAINER_TYPE, NULL);
        wk->entries[n].count = 0;
        wk->entries[n].person = record;
        year = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_YEAR, NULL);
        month = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_MONTH, NULL);
        day = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_DAY, NULL);
        labelFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_10);
        descriptionFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, n + 37);
        gender = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_GENDER, NULL);
        JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_NAME, name);
        nameBuf = GFL_StrBufCreate(8, wk->heapId);
        GFL_StrBufLoadString(nameBuf, name);
        label = GFL_StrBufCreate(64, wk->heapId);
        description = GFL_StrBufCreate(128, wk->heapId);
        WordSetNumber(wk->wordSet, 1, n + 1, 1, NUM_PAD_NONE, 1);
        GFL_WordSetSetStrbuf(wk->wordSet, 0, nameBuf, gender, 1, 2);
        GFL_WordSetFormatStrbuf(wk->wordSet, label, labelFormat);
        WordSetNumber(wk->wordSet, 4, year, 2, NUM_PAD_ZERO, 1);
        WordSetNumber(wk->wordSet, 5, month, 2, NUM_PAD_NONE, 1);
        WordSetNumber(wk->wordSet, 6, day, 2, NUM_PAD_NONE, 1);
        GFL_WordSetFormatStrbuf(wk->wordSet, description, descriptionFormat);
        wk->entries[n].label = label;
        wk->entries[n].name = GFL_StrBufClone(nameBuf, wk->heapId);
        wk->entries[n].description = description;
        GFL_StrBufFree(nameBuf);
        GFL_StrBufFree(descriptionFormat);
        GFL_StrBufFree(labelFormat);
        ListMenuCore_AppendStrBufOption(&wk->options[n], wk->entries[n].label, n, wk->heapId);
        n++;
    }
    wk->entryCount = count;
    ListMenuCore_AppendMsgOption(&wk->options[n], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
}

// The avenue's people, from the last
static void ResortShop_MakePersonEntries(ResortShopWork *wk, u8 mode, u8 subMode) {
    int count = JoinAvenueOccupants_CountPeople(wk->occupants);
    int n;
    int i;

    wk->options = ListMenuCore_CreateOptionList(count + 1, wk->heapId);
    n = 0;
    for (i = 7; i >= 0; i--) {
        JoinAvenuePerson *person = JoinAvenueOccupants_GetPerson(wk->occupants, i);
        ResortPersonData *data;

        if (JoinAvenuePerson_IsEmpty(person)) {
            continue;
        }
        data = ResortDatas_FindByPerson(wk->datas, person);
        wk->initialOrder[n] = i;
        wk->order[n] = i;
        wk->entries[n].id = i;
        wk->entries[n].kind = 0;
        wk->entries[n].price = 0;
        wk->entries[n].item = joinAveTextHandler(person, JOIN_AVE_PARAM_TRAINER_TYPE, NULL);
        wk->entries[n].count = 0;
        wk->entries[n].person = person;
        ResortShop_MakePersonEntry(wk, data, n, &wk->entries[n]);
        ListMenuCore_AppendStrBufOption(&wk->options[n], wk->entries[n].label, n, wk->heapId);
        n++;
    }
    wk->entryCount = count;
    ListMenuCore_AppendMsgOption(&wk->options[n], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
}

// Swaps the moved record with the one it was put on, and remakes the list
static void ResortShop_SwapRecordEntries(ResortShopWork *wk, u8 mode, u8 subMode) {
    ResortShopEntry entry;
    int i;
    u16 name[8];

    entry = wk->entries[wk->moveFrom];
    wk->entries[wk->moveFrom] = wk->entries[wk->moveTo];
    wk->entries[wk->moveTo] = entry;
    wk->selected = NULL;
    ListMenuCore_FreeOptionList(wk->options);
    wk->options = ListMenuCore_CreateOptionList(wk->entryCount + 1, wk->heapId);
    for (i = 0; i < wk->entryCount; i++) {
        void *record = wk->entries[i].person;
        StrBuf *labelFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_10);
        StrBuf *descriptionFormat = GFL_MsgDataLoadStrbufNew(wk->msgData, i + 37);
        u8 gender = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_GENDER, NULL);
        u16 year = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_YEAR, NULL);
        u16 month = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_MONTH, NULL);
        u16 day = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_DAY, NULL);
        StrBuf *nameBuf;

        JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_NAME, name);
        nameBuf = GFL_StrBufCreate(8, wk->heapId);
        GFL_StrBufLoadString(nameBuf, name);
        WordSetNumber(wk->wordSet, 1, i + 1, 1, NUM_PAD_NONE, 1);
        GFL_WordSetSetStrbuf(wk->wordSet, 0, nameBuf, gender, 1, 2);
        WordSetNumber(wk->wordSet, 4, year, 2, NUM_PAD_ZERO, 1);
        WordSetNumber(wk->wordSet, 5, month, 2, NUM_PAD_NONE, 1);
        WordSetNumber(wk->wordSet, 6, day, 2, NUM_PAD_NONE, 1);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->entries[i].label, labelFormat);
        // The copy goes into the new name, rather than from it
        GFL_StrBufCopy(nameBuf, wk->entries[i].name);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->entries[i].description, descriptionFormat);
        GFL_StrBufFree(nameBuf);
        GFL_StrBufFree(descriptionFormat);
        GFL_StrBufFree(labelFormat);
        ListMenuCore_AppendStrBufOption(&wk->options[i], wk->entries[i].label, i, wk->heapId);
    }
    ListMenuCore_AppendMsgOption(&wk->options[i], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
}

// The same for the people
static void ResortShop_SwapPersonEntries(ResortShopWork *wk, u8 mode, u8 subMode) {
    ResortShopEntry entry;
    int i;

    entry = wk->entries[wk->moveFrom];
    wk->entries[wk->moveFrom] = wk->entries[wk->moveTo];
    wk->entries[wk->moveTo] = entry;
    wk->selected = NULL;
    ListMenuCore_FreeOptionList(wk->options);
    wk->options = ListMenuCore_CreateOptionList(wk->entryCount + 1, wk->heapId);
    for (i = 0; i < wk->entryCount; i++) {
        ResortShop_MakePersonEntry(wk, ResortDatas_FindByPerson(wk->datas, wk->entries[i].person), i, &wk->entries[i]);
        ListMenuCore_AppendStrBufOption(&wk->options[i], wk->entries[i].label, i, wk->heapId);
    }
    ListMenuCore_AppendMsgOption(&wk->options[i], wk->msgData, 12, BMPMENULIST_CANCEL, wk->heapId);
}

static void ResortShop_FreeEntries(ResortShopWork *wk) {
    int i;

    ListMenuCore_FreeOptionList(wk->options);
    for (i = 0; i < 17; i++) {
        if (wk->entries[i].description != NULL) {
            GFL_StrBufFree(wk->entries[i].description);
            wk->entries[i].description = NULL;
        }
        if (wk->entries[i].name != NULL) {
            GFL_StrBufFree(wk->entries[i].name);
            wk->entries[i].name = NULL;
        }
        if (wk->entries[i].label != NULL) {
            GFL_StrBufFree(wk->entries[i].label);
            wk->entries[i].label = NULL;
        }
    }
}

static u8 ResortShop_InitMessages(ResortShopWork *wk, u8 mode) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_SCRCMD_RESORT_SHOP, wk->heapId);
    wk->wordSet = GFL_WordSetSystemCreate(8, 64, wk->heapId);
    wk->message = GFL_StrBufCreate(200, wk->heapId);
    wk->tcbManager = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 32, 32);
    return mode;
}

static void ResortShop_FreeMessages(ResortShopWork *wk) {
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_WordSetSystemFree(wk->wordSet);
    if (wk->keyCursor != NULL) {
        KeyCursor_Free(wk->keyCursor);
        wk->keyCursor = NULL;
    }
    GFL_StrBufFree(wk->message);
    GFL_StrBufFree(wk->boughtLabel);
    GFL_StrBufFree(wk->priceFormat);
    GFL_MsgDataFree(wk->msgData);
}

static void ResortShop_LoadBG(ResortShopWork *wk, u32 mode) {
    ArcTool *arc;

    G2_BlendNone();
    arc = GFL_ArcSysCreateFileHandle(ARCID_SCRCMD_SHOP, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 0, 0, 0x60, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 2, 100, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 2, 2, 0, 100, 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    LoadSysMsgBox(1, 228, 9, 0, wk->heapId);
    GFL_BGSysClearScr(1);
    GFL_BGSysSetBGPriority(2, 1);
    if (mode == RESORT_SHOP_MODE_SELECT || mode == RESORT_SHOP_MODE_REORDER) {
        GFL_BGSysFillScrArea(2, 0, 0, 0, 11, 17, 16);
        GFL_BGSysLoadScr(2);
    }
}

static void ResortShop_ClearBG(ResortShopWork *wk) {
    GFL_BGSysClearScr(1);
    GFL_BGSysClearScr(2);
    GFL_BGSysSetBGPriority(2, 0);
    setAlphaBlend_wrapper(TRUE);
}

static void ResortShop_LoadCursorResources(ResortShopWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_SCRCMD_SHOP, wk->heapId);

    wk->res[0].palette = ClActSys_LoadPlttRange(arc, 11, 0, 0, 0, 2, wk->heapId);
    wk->res[0].chars = ClActSys_LoadChars(arc, 8, 0, 0, wk->heapId);
    wk->res[1].chars = ClActSys_LoadChars(arc, 5, 0, 0, wk->heapId);
    wk->res[0].cellAnims = ClActSys_LoadCellAnims(arc, 9, 10, wk->heapId);
    wk->res[1].cellAnims = ClActSys_LoadCellAnims(arc, 6, 7, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void ResortShop_LoadItemIconResources(ResortShopWork *wk) {
    wk->res[2].palette = ClActSys_LoadPlttRange(
        wk->iconArc, GetItemGraphicsDatID(ITEM_MASTER_BALL, ITEM_FILE_ICON_PLTT), 0, 64, 0, 1, wk->heapId);
    wk->res[2].chars =
        ClActSys_LoadChars(wk->iconArc, GetItemGraphicsDatID(ITEM_MASTER_BALL, ITEM_FILE_ICON_CHAR), 0, 0, wk->heapId);
    wk->res[2].cellAnims = ClActSys_LoadCellAnims(wk->iconArc, 1, 0, wk->heapId);
}

// Shows an item's icon, or hides the icon for anything past the last item
static void ResortShop_ShowItemIcon(ResortShopWork *wk, u16 item) {
    NNSG2dCharacterData *character;
    NNSG2dPaletteData *palette;
    void *nclr;
    void *ncgr;

    if (item <= ITEM_REVEAL_GLASS) {
        nclr = GFL_G2DIOReadNCLRArc(wk->iconArc, GetItemGraphicsDatID(item, ITEM_FILE_ICON_PLTT), &palette, wk->heapId);
        ncgr = GFL_G2DIOReadOBJNCGRArc(wk->iconArc, GetItemGraphicsDatID(item, ITEM_FILE_ICON_CHAR), FALSE, &character,
                                       wk->heapId);
        ClActSys_ReplacePlttData(wk->res[2].palette, palette, 1);
        ClActSys_ReplaceCharData(wk->res[2].chars, character);
        GFL_HeapFree(nclr);
        GFL_HeapFree(ncgr);
        ClActor_SetVisible(wk->actors[4], TRUE);
    } else {
        ClActor_SetVisible(wk->actors[4], FALSE);
    }
}

// Shows a person's model as the icon, or hides it for 0xffff
static void ResortShop_ShowPersonModel(ResortShopWork *wk, u16 objCode) {
    ArcTool *arc;
    u8 texture;
    u16 fileId;

    if (objCode != RESORT_SHOP_ICON_NONE) {
        arc = GFL_ArcSysCreateFileHandle(ARCID_MMODEL_TBL, wk->heapId);
        ResortShop_GetModelTexture(arc, objCode, &texture, &fileId);
        GFL_ArcToolFree(arc);
        NsbtxToClwk_LoadTexture(wk->actors[4], ARCID_MMODEL_GRA, fileId, texture, 4, 4, 0, 0, wk->heapId);
        ClActor_SetVisible(wk->actors[4], TRUE);
    } else {
        ClActor_SetVisible(wk->actors[4], FALSE);
    }
}

static void ResortShop_CreateActors(ResortShopWork *wk, const ClActorSetup *iconSetup) {
    wk->actors[0] = ClActor_Create(wk->clactUnit, wk->res[0].chars, wk->res[0].palette, wk->res[0].cellAnims,
                                   &sActorSetups[0], 0, wk->heapId);
    wk->actors[1] = ClActor_Create(wk->clactUnit, wk->res[1].chars, wk->res[0].palette, wk->res[1].cellAnims,
                                   &sActorSetups[1], 0, wk->heapId);
    wk->actors[2] = ClActor_Create(wk->clactUnit, wk->res[1].chars, wk->res[0].palette, wk->res[1].cellAnims,
                                   &sActorSetups[2], 0, wk->heapId);
    wk->actors[3] = ClActor_Create(wk->clactUnit, wk->res[1].chars, wk->res[0].palette, wk->res[1].cellAnims,
                                   &sActorSetups[3], 0, wk->heapId);
    ClActor_SetAutoAnim(wk->actors[3], TRUE);
    wk->actors[4] = ClActor_Create(wk->clactUnit, wk->res[2].chars, wk->res[2].palette, wk->res[2].cellAnims, iconSetup,
                                   0, wk->heapId);
    ClActor_SetPalette(wk->actors[4], 0, 1);
    wk->actors[5] = ClActor_Create(wk->clactUnit, wk->res[0].chars, wk->res[0].palette, wk->res[0].cellAnims,
                                   &sActorSetups[6], 0, wk->heapId);
    ClActor_SetVisible(wk->actors[5], FALSE);
    ClActor_SetVisible(wk->actors[1], FALSE);
    ClActor_SetVisible(wk->actors[2], FALSE);
    ClActor_SetVisible(wk->actors[3], FALSE);
}

static void ResortShop_CreateClActUnit(ResortShopWork *wk) {
    wk->clactUnit = ClActUnit_Create(6, 1, wk->heapId);
    ClActUnit_SetActive(wk->clactUnit, TRUE);
    ResortShop_LoadCursorResources(wk);
}

static void ResortShop_CreateItemActors(ResortShopWork *wk) {
    wk->iconArc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, wk->heapId);
    ResortShop_LoadItemIconResources(wk);
    ResortShop_CreateActors(wk, &sActorSetups[4]);
}

static void ResortShop_CreatePersonActors(ResortShopWork *wk) {
    wk->iconArc = GFL_ArcSysCreateFileHandle(ARCID_WIFI_UNIONCHAR, wk->heapId);
    wk->res[2].palette = ClActSys_LoadPlttRange(wk->iconArc, 0, 0, 64, 2, 1, wk->heapId);
    wk->res[2].chars = ClActSys_LoadChars(wk->iconArc, 49, 0, 0, wk->heapId);
    wk->res[2].cellAnims = ClActSys_LoadCellAnims(wk->iconArc, 65, 66, wk->heapId);
    ResortShop_CreateActors(wk, &sActorSetups[5]);
}

static void ResortShop_FreeActors(ResortShopWork *wk) {
    ClActSys_FreeCellAnims(wk->res[2].cellAnims);
    ClActSys_FreeCellAnims(wk->res[1].cellAnims);
    ClActSys_FreeCellAnims(wk->res[0].cellAnims);
    ClActSys_FreeChars(wk->res[2].chars);
    ClActSys_FreeChars(wk->res[1].chars);
    ClActSys_FreeChars(wk->res[0].chars);
    ClActSys_FreePltt(wk->res[2].palette);
    ClActSys_FreePltt(wk->res[0].palette);
    GFL_ArcToolFree(wk->iconArc);
    ClActUnit_Delete(wk->clactUnit);
}

static void ResortShop_CreateWindows(ResortShopWork *wk) {
    int i;

    for (i = 0; i < 7; i++) {
        const ResortShopWindowSetup *setup = sWindowSetups[i];

        wk->windows[i] =
            BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, 1);
    }
    wk->printWindow.window = wk->windows[RESORT_SHOP_WINDOW_LIST];
    wk->printWindow.flushPending = FALSE;
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_MONEY]);
}

static void ResortShop_PrintMoneyLabel(ResortShopWork *wk) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Money);

    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_MONEY_LABEL]), 0, 0, strbuf,
                                   wk->font, PRINT_COLOR(15, 2, 15));
    GFL_StrBufFree(strbuf);
    BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_MONEY_LABEL]);
}

static void ResortShop_FreeWindows(ResortShopWork *wk) {
    int i;

    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    for (i = 0; i < 7; i++) {
        BmpWin_Free(wk->windows[i]);
    }
}

// The player's money
static void ResortShop_PrintMoney(ResortShopWork *wk) {
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_6);
    StrBuf *strbuf = GFL_StrBufCreate(10, wk->heapId);
    GFLBitmap *bitmap;

    WordSetNumber(wk->wordSet, 0, getCash(wk->trainerCard), 7, 1, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    bitmap = BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_MONEY]);
    GFL_BitmapFill(bitmap, 0);
    GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, strbuf, wk->font, PRINT_COLOR(15, 2, 15));
    BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_MONEY]);
    GFL_StrBufFree(strbuf);
    GFL_StrBufFree(format);
}

// How many of the item the bag holds
static void ResortShop_PrintBagCount(ResortShopWork *wk, u16 item) {
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Bag);
    StrBuf *strbuf = GFL_StrBufCreate(20, wk->heapId);
    GFLBitmap *bitmap;

    BmpWin_DrawFrame(wk->windows[RESORT_SHOP_WINDOW_BAG_COUNT], WINFRAME_TRANSFER_NONE, 228, 9);
    WordSetNumber(wk->wordSet, 0, BagSave_GetItemCountByID(wk->bag, item, wk->heapId), 3, NUM_PAD_NONE, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    bitmap = BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_BAG_COUNT]);
    GFL_BitmapFill(bitmap, 15);
    GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, strbuf, wk->font, PRINT_COLOR(1, 2, 15));
    BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_BAG_COUNT]);
    GFL_StrBufFree(strbuf);
    GFL_StrBufFree(format);
}

// The description of the entry at the cursor
static void ResortShop_PrintDescription(ResortShopWork *wk, StrBuf *strbuf) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_DESCRIPTION]);

    GFL_BitmapFill(bitmap, 0);
    if (strbuf != NULL) {
        GFL_TextRendererDrawToBitmapEx(bitmap, 0, 0, strbuf, wk->font, PRINT_COLOR(15, 2, 0));
    }
    BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_DESCRIPTION]);
}

static void ResortShop_CreateList(ResortShopWork *wk, BmpMenuListCursorCallback cursorCallback,
                                  BmpMenuListPrintCallback printCallback) {
    BmpMenuListHeader header;

    header.options = wk->options;
    header.cursorCallback = cursorCallback;
    header.printCallback = printCallback;
    header.count = ListMenuCore_GetFirstFreeIndex(wk->options);
    header.maxShown = 7;
    header.labelX = 0;
    header.itemX = 0;
    header.cursorX = 0;
    header.y = 8;
    header.fgColor = 12;
    header.bgColor = 0;
    header.shadowColor = 13;
    header.letterSpacing = 0;
    header.lineSpacing = 0;
    header.pageSkip = BMPMENULIST_SKIP_LR_KEY;
    header.fontId = 0;
    header.cursorDisplay = BMPMENULIST_CURSOR_HIDE;
    header.work = wk;
    header.fontSizeX = 12;
    header.fontSizeY = 16;
    header.msgData = NULL;
    header.printWindow = &wk->printWindow;
    header.queue = wk->printQueue;
    header.font = wk->font;
    header.wait = 0;
    wk->list = BmpMenuList_Create(&header, 0, 0, wk->heapId);
}

// Prints straight to the bitmap in the first row, and through the print queue below it
static void ResortShop_PrintToRow(PrintQueue *queue, GFLBitmap *bitmap, int x, int y, const StrBuf *strbuf, Font *font,
                                  u16 color) {
    if (y <= 8) {
        GFL_TextRendererDrawToBitmapEx(bitmap, x, y, strbuf, font, color);
    } else {
        PrintQueue_PrintColor(queue, bitmap, x, y, strbuf, font, color);
    }
}

// Prints an item's price, or that it was bought, at the right of its row
static void ResortShop_PrintPrice(BmpMenuList *list, s32 value, u8 y) {
    ResortShopWork *wk = BmpMenuList_GetWork(list);
    s32 width;
    u32 windowWidth;

    if (value == BMPMENULIST_CANCEL) {
        return;
    }
    if (JoinAvenueShop_GetFlag(wk->flags, wk->entries[value].id)) {
        width = GFL_FontGetBlockWidth(wk->boughtLabel, wk->font, 0);
        windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_LIST]));
        ResortShop_PrintToRow(wk->printQueue, BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_LIST]),
                              windowWidth - width, y, wk->boughtLabel, wk->font, PRINT_COLOR(12, 13, 0));
        return;
    }
    WordSetNumber(wk->wordSet, 1, wk->entries[value].price, 5, 1, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->message, wk->priceFormat);
    width = GFL_FontGetBlockWidth(wk->message, wk->font, 0);
    windowWidth = GFL_BitmapGetWidth(BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_LIST]));
    ResortShop_PrintToRow(wk->printQueue, BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_LIST]), windowWidth - width,
                          y, wk->message, wk->font, PRINT_COLOR(12, 13, 0));
}

static void ResortShop_FreeList(ResortShopWork *wk) {
    u16 listTop;
    u16 cursorRow;

    BmpMenuList_Free(wk->list, &listTop, &cursorRow);
}

static BOOL ResortShop_PrintStreamCallback(u32 event) {
    return FALSE;
}

// Puts an item's name and pocket in the word set, and prints a message
static void ResortShop_PrintItemMessage(ResortShopWork *wk, u32 msgId, u16 item, u16 count) {
    u32 pocket = BagSave_GetActualItemPocket(wk->bag, item);

    loadItemText(wk->wordSet, 0, item, count != 1, FALSE);
    loadBagPocketNameToStrbuf(wk->wordSet, 1, pocket);
    ResortShop_PrintMessage(wk, msgId);
}

// Prints a message in a new frame, waiting for a button at its end
static void ResortShop_PrintMessage(ResortShopWork *wk, u32 msgId) {
    StrBuf *strbuf;
    BmpWin *window;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_MESSAGE]), 15);
    BmpWin_DrawFrame(wk->windows[RESORT_SHOP_WINDOW_MESSAGE], WINFRAME_TRANSFER_NOW, 228, 9);
    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->message, strbuf);
    GFL_StrBufFree(strbuf);
    wk->printStream = PrintStream_CreateWithCallback(wk->windows[RESORT_SHOP_WINDOW_MESSAGE], 0, 0, wk->message,
                                                     wk->font, TextSpeed_GetCurrentWait(), wk->tcbManager, 0,
                                                     wk->heapId, 0xffff, ResortShop_PrintStreamCallback);
    if (wk->keyCursor != NULL) {
        KeyCursor_Free(wk->keyCursor);
        wk->keyCursor = NULL;
    }
    wk->keyCursor = KeyCursor_Create(15, TRUE, FALSE, wk->heapId);
    BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_MESSAGE]);
}

// The same without waiting for a button
static void ResortShop_PrintMessageNoWait(ResortShopWork *wk, u32 msgId) {
    StrBuf *strbuf;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[RESORT_SHOP_WINDOW_MESSAGE]), 15);
    BmpWin_DrawFrame(wk->windows[RESORT_SHOP_WINDOW_MESSAGE], WINFRAME_TRANSFER_NOW, 228, 9);
    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->message, strbuf);
    GFL_StrBufFree(strbuf);
    wk->printStream = PrintStream_CreateWithCallback(wk->windows[RESORT_SHOP_WINDOW_MESSAGE], 0, 0, wk->message,
                                                     wk->font, TextSpeed_GetCurrentWait(), wk->tcbManager, 0,
                                                     wk->heapId, 0xffff, ResortShop_PrintStreamCallback);
    BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_MESSAGE]);
}

static void ResortShop_DrawMessageFrame(BmpWin *window) {
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    BmpWin_DrawFrame(window, WINFRAME_TRANSFER_NOW, 228, 9);
}

// ResortShop_PrintMessage in the frame that is there already
static void ResortShop_PrintMessageInFrame(ResortShopWork *wk, u32 msgId) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);

    GFL_WordSetFormatStrbuf(wk->wordSet, wk->message, strbuf);
    GFL_StrBufFree(strbuf);
    wk->printStream =
        PrintStream_CreateWithCallback(wk->windows[RESORT_SHOP_WINDOW_MESSAGE], 0, 0, wk->message, wk->font,
                                       TextSpeed_GetCurrentWait(), wk->tcbManager, 0, wk->heapId, 0xffff, NULL);
    if (wk->keyCursor != NULL) {
        KeyCursor_Free(wk->keyCursor);
        wk->keyCursor = NULL;
    }
    wk->keyCursor = KeyCursor_Create(15, TRUE, FALSE, wk->heapId);
    BmpWin_TransferNow(wk->windows[RESORT_SHOP_WINDOW_MESSAGE]);
}

// A message about an entry, with its name, count and price in the word set
static void ResortShop_PrintEntryMessage(ResortShopWork *wk, u32 msgId, ResortShopEntry *entry) {
    ResortShop_DrawMessageFrame(wk->windows[RESORT_SHOP_WINDOW_MESSAGE]);
    if (entry->kind == RESORT_SHOP_GOODS_ITEM && wk->mode == RESORT_SHOP_MODE_BUY) {
        loadItemNameToStrbuf(wk->wordSet, 0, entry->item);
    } else {
        GFL_WordSetSetStrbuf(wk->wordSet, 0, entry->name, 2, 1, 2);
    }
    WordSetNumber(wk->wordSet, 1, entry->count, 2, NUM_PAD_NONE, 1);
    WordSetNumber(wk->wordSet, 2, entry->price, 7, NUM_PAD_NONE, 1);
    ResortShop_PrintMessageInFrame(wk, msgId);
}

static void ResortShop_ClearListScreen(u32 which) {
    GFL_BGSysFillScrArea(1, 0, sListRects[which].x, sListRects[which].y, sListRects[which].width,
                         sListRects[which].height, 0);
}

static ResortPeople *ResortShop_GetPeople(Field *field) {
    ResortPeople *people = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        people = ResortField0_GetPeople(field);
    } else if (Field_CheckGimmickWorkPassword(field, 1)) {
        people = ResortField_GetPeople(field);
    }
    return people;
}

static ResortNPC *ResortShop_GetNPC(Field *field) {
    void *npc = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        npc = ResortField0_GetNPC(field);
    } else if (Field_CheckGimmickWorkPassword(field, 1)) {
        npc = ResortField_GetNPC(field);
    }
    return npc;
}

static void ResortShop_CreateItemList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    ResortShop_CreateItemActors(wk);
    ResortShop_MakeItemEntries(wk, mode, subMode);
    ResortShop_CreateList(wk, wk->cursorCallback, wk->printCallback);
}

static void ResortShop_DestroyItemList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
}

// Moves the cursor's arrow, and shows the entry's description and icon
static void ResortShop_ItemCursorCallback(BmpMenuList *list, s32 value, u8 init) {
    ResortShopWork *wk = BmpMenuList_GetWork(list);
    u16 top;
    u16 cursor;
    ClActorPos pos;

    BmpMenuList_GetPos(list, &top, &cursor);
    pos.x = 172;
    pos.y = cursor * 16 + 22;
    ClActor_SetSurfacePos(wk->actors[0], &pos, 0);
    if (value < NELEMS(wk->entries)) {
        ResortShop_PrintDescription(wk, wk->entries[value].description);
        if (wk->entries[value].kind == RESORT_SHOP_GOODS_ITEM) {
            ResortShop_ShowItemIcon(wk, wk->entries[value].item);
        } else {
            ResortShop_ShowItemIcon(wk, RESORT_SHOP_ICON_NONE);
        }
    } else {
        ResortShop_PrintDescription(wk, NULL);
        ResortShop_ShowItemIcon(wk, RESORT_SHOP_ICON_NONE);
    }
}

static void ResortShop_CreateRecordList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    ResortShop_CreatePersonActors(wk);
    ResortShop_MakeRecordEntries(wk, mode, subMode);
    ResortShop_CreateList(wk, wk->cursorCallback, wk->printCallback);
}

static void ResortShop_DestroyRecordList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
}

static void ResortShop_CreatePersonList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
    ResortShop_CreatePersonActors(wk);
    ResortShop_MakePersonEntries(wk, mode, subMode);
    ResortShop_CreateList(wk, wk->cursorCallback, wk->printCallback);
}

static void ResortShop_DestroyPersonList(GameSystem *gsys, ResortShopWork *wk, u8 mode, u8 subMode) {
}

// A person's shop, its level and the date, as the description of the person
static StrBuf *ResortShop_MakePersonDescription(ResortShopWork *wk, JoinAvenuePerson *person) {
    void *flags = JoinAvenuePerson_GetShop(person);
    StrBuf *strbuf = GFL_StrBufCreate(128, wk->heapId);
    u16 shop = ResortShopData_GetShopParam(ResortShopData_GetPersonShop(wk->shops, person), 0);
    MsgData *msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(wk->heapId));
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, shop + 42);
    u16 year = joinAveTextHandler(person, JOIN_AVE_PARAM_YEAR, NULL);
    u16 month = joinAveTextHandler(person, JOIN_AVE_PARAM_MONTH, NULL);
    u16 day = joinAveTextHandler(person, JOIN_AVE_PARAM_DAY, NULL);
    u32 level = JoinAvenueShop_GetField(flags, 1);
    StrBuf *shopName = GFL_MsgDataLoadStrbufNew(msgData, shop + 280);

    GFL_WordSetSetStrbuf(wk->wordSet, 1, shopName, 2, 1, 2);
    WordSetNumber(wk->wordSet, 2, level + 1, 2, NUM_PAD_NONE, 1);
    WordSetNumber(wk->wordSet, 4, year, 2, NUM_PAD_ZERO, 1);
    WordSetNumber(wk->wordSet, 5, month, 2, NUM_PAD_NONE, 1);
    WordSetNumber(wk->wordSet, 6, day, 2, NUM_PAD_NONE, 1);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    GFL_WordSetFormatStrbuf(wk->wordSet, strbuf, format);
    GFL_StrBufFree(shopName);
    GFL_StrBufFree(format);
    GFL_MsgDataFree(msgData);
    return strbuf;
}

// Makes a person's entry's strings, or remakes them
static void ResortShop_MakePersonEntry(ResortShopWork *wk, ResortPersonData *data, u32 index, ResortShopEntry *entry) {
    JoinAvenuePerson *person = ResortData_GetPerson(data);
    u16 flag = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(person), 0);
    StrBuf *label = GFL_StrBufCreate(64, wk->heapId);
    StrBuf *name = ResortSys_FormatShopText(wk->shops, data, flag, wk->heapId);
    StrBuf *description = ResortShop_MakePersonDescription(wk, person);
    StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, ScrcmdResortShop_Text_Empty_11);
    u8 gender = joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL);

    WordSetNumber(wk->wordSet, 1, index + 1, 1, NUM_PAD_NONE, 1);
    GFL_WordSetSetStrbuf(wk->wordSet, 0, name, gender, 1, 2);
    GFL_WordSetFormatStrbuf(wk->wordSet, label, format);
    GFL_StrBufFree(format);
    if (entry->label == NULL) {
        entry->label = label;
        entry->name = name;
        entry->description = description;
    } else {
        GFL_StrBufCopy(entry->label, label);
        GFL_StrBufCopy(entry->name, name);
        GFL_StrBufCopy(entry->description, description);
        GFL_StrBufFree(label);
        GFL_StrBufFree(name);
        GFL_StrBufFree(description);
    }
}

// Moves the cursor's arrow, and shows the entry's description and model
static void ResortShop_ModelCursorCallback(BmpMenuList *list, s32 value, u8 init) {
    ResortShopWork *wk = BmpMenuList_GetWork(list);
    u16 top;
    u16 cursor;
    ClActorPos pos;

    BmpMenuList_GetPos(list, &top, &cursor);
    pos.x = 172;
    pos.y = cursor * 16 + 22;
    ClActor_SetSurfacePos(wk->actors[0], &pos, 0);
    if (value < NELEMS(wk->entries)) {
        ResortShop_PrintDescription(wk, wk->entries[value].description);
        ResortShop_ShowPersonModel(wk, wk->entries[value].item);
    } else {
        ResortShop_PrintDescription(wk, NULL);
        ResortShop_ShowPersonModel(wk, RESORT_SHOP_ICON_NONE);
    }
}

// The same, and moves the arrow of the entry being moved
static void ResortShop_OrderCursorCallback(BmpMenuList *list, s32 value, u8 init) {
    ResortShopWork *wk = BmpMenuList_GetWork(list);
    u16 top;
    u16 cursor;
    ClActorPos pos;

    BmpMenuList_GetPos(list, &top, &cursor);
    pos.x = 172;
    pos.y = cursor * 16 + 22;
    ClActor_SetSurfacePos(wk->actors[0], &pos, 0);
    if (value < NELEMS(wk->entries)) {
        ResortShop_PrintDescription(wk, wk->entries[value].description);
        ResortShop_ShowPersonModel(wk, wk->entries[value].item);
    } else {
        ResortShop_PrintDescription(wk, NULL);
        ResortShop_ShowPersonModel(wk, RESORT_SHOP_ICON_NONE);
    }
    if (wk->moving) {
        ResortShop_UpdateMoveArrow(wk);
    } else {
        ClActor_SetVisible(wk->actors[5], FALSE);
    }
}

// Puts the arrow of the entry being moved at its row, showing it while the row is on the screen
static void ResortShop_UpdateMoveArrow(ResortShopWork *wk) {
    u16 top;
    u16 cursor;
    ClActorPos pos;
    u32 rowHeight;
    int row;
    int y;

    BmpMenuList_GetPos(wk->list, &top, &cursor);
    rowHeight = BmpMenuList_GetParam(wk->list, BMPMENULIST_PARAM_ROW_HEIGHT);
    row = wk->moveCursor + wk->moveTop - top;
    y = row * rowHeight;
    pos.x = 172;
    pos.y = y + 22;
    ClActor_SetSurfacePos(wk->actors[5], &pos, 0);
    if (pos.y == 6) {
        ClActor_SetSequence(wk->actors[5], 3);
    } else if (pos.y == 134) {
        ClActor_SetSequence(wk->actors[5], 2);
    } else {
        ClActor_SetSequence(wk->actors[5], 1);
    }
    if (pos.y > 0 && pos.y < 192) {
        ClActor_SetVisible(wk->actors[5], TRUE);
    } else {
        ClActor_SetVisible(wk->actors[5], FALSE);
    }
}

// Puts the records in the new order, keeping fields 0, 1 and 31 in their places
static void ResortShop_ApplyRecordOrder(ResortShopWork *wk, u8 mode, u8 subMode) {
    int pass;
    int i;
    u16 tmp;
    u8 record[0x58];

    if (!wk->moved) {
        return;
    }
    for (pass = 0; pass < wk->entryCount; pass++) {
        for (i = 0; i < wk->entryCount; i++) {
            if (i != wk->entries[i].id) {
                u16 otherId = wk->entries[wk->entries[i].id].id;
                void *recordA = JoinAvenueOccupants_GetRecord(wk->occupants, wk->entries[i].id);
                u32 rowA = JoinAvenueRecord_GetParam(recordA, JOIN_AVE_PARAM_ROW, NULL);
                u16 slotA = JoinAvenueRecord_GetParam(recordA, JOIN_AVE_PARAM_SLOT, NULL);
                u16 shopKindA = JoinAvenueRecord_GetParam(recordA, JOIN_AVE_PARAM_SHOP_KIND, NULL);
                void *recordB = JoinAvenueOccupants_GetRecord(wk->occupants, otherId);
                u32 rowB = JoinAvenueRecord_GetParam(recordB, JOIN_AVE_PARAM_ROW, NULL);
                u16 slotB = JoinAvenueRecord_GetParam(recordB, JOIN_AVE_PARAM_SLOT, NULL);
                u16 shopKindB = JoinAvenueRecord_GetParam(recordB, JOIN_AVE_PARAM_SHOP_KIND, NULL);

                JoinAvenueRecord_SetParam(recordA, JOIN_AVE_PARAM_SLOT, slotB);
                JoinAvenueRecord_SetParam(recordA, JOIN_AVE_PARAM_ROW, rowB);
                JoinAvenueRecord_SetParam(recordA, JOIN_AVE_PARAM_SHOP_KIND, shopKindB);
                JoinAvenueRecord_SetParam(recordB, JOIN_AVE_PARAM_SLOT, slotA);
                JoinAvenueRecord_SetParam(recordB, JOIN_AVE_PARAM_ROW, rowA);
                JoinAvenueRecord_SetParam(recordB, JOIN_AVE_PARAM_SHOP_KIND, shopKindA);
                MI_CpuCopy8(recordA, record, sizeof(record));
                MI_CpuCopy8(recordB, recordA, sizeof(record));
                MI_CpuCopy8(record, recordB, sizeof(record));
                tmp = wk->order[i];
                wk->order[i] = wk->order[wk->entries[i].id];
                wk->order[wk->entries[i].id] = tmp;
                tmp = wk->entries[wk->entries[i].id].id;
                wk->entries[wk->entries[i].id].id = wk->entries[i].id;
                wk->entries[i].id = tmp;
            }
        }
    }
}

// Puts the people in the new order, keeping fields 0 and 1 in their places. The list runs from the last person
static void ResortShop_ApplyPersonOrder(ResortShopWork *wk, u8 mode, u8 subMode) {
    int pass;
    int i;
    u16 tmp;
    u8 person[0xc4];

    if (!wk->moved) {
        return;
    }
    for (pass = 0; pass < wk->entryCount; pass++) {
        for (i = 0; i < wk->entryCount; i++) {
            int j = wk->entryCount - 1 - wk->entries[i].id;

            if (i != j) {
                u16 otherId = ResortShop_GetEntry(wk, j)->id;
                JoinAvenuePerson *personA = JoinAvenueOccupants_GetPerson(wk->occupants, wk->entries[i].id);
                u32 rowA = joinAveTextHandler(personA, JOIN_AVE_PARAM_ROW, NULL);
                u16 slotA = joinAveTextHandler(personA, JOIN_AVE_PARAM_SLOT, NULL);
                JoinAvenuePerson *personB = JoinAvenueOccupants_GetPerson(wk->occupants, otherId);
                u32 rowB = joinAveTextHandler(personB, JOIN_AVE_PARAM_ROW, NULL);
                u16 slotB = joinAveTextHandler(personB, JOIN_AVE_PARAM_SLOT, NULL);

                JoinAvenuePerson_SetParam(personA, JOIN_AVE_PARAM_SLOT, slotB);
                JoinAvenuePerson_SetParam(personA, JOIN_AVE_PARAM_ROW, rowB);
                JoinAvenuePerson_SetParam(personB, JOIN_AVE_PARAM_SLOT, slotA);
                JoinAvenuePerson_SetParam(personB, JOIN_AVE_PARAM_ROW, rowA);
                MI_CpuCopy8(personA, person, sizeof(person));
                MI_CpuCopy8(personB, personA, sizeof(person));
                MI_CpuCopy8(person, personB, sizeof(person));
                tmp = wk->order[i];
                wk->order[i] = wk->order[j];
                wk->order[j] = tmp;
                tmp = wk->entries[j].id;
                wk->entries[j].id = wk->entries[i].id;
                wk->entries[i].id = tmp;
            }
        }
    }
}

static u8 ResortShop_GetTextureIndex(u32 index) {
    return FIELD_BILLBOARD_ANIM_SETS[index].animations[1]->face;
}

static void ResortShop_GetModelTexture(ArcTool *handle, u16 objCode, u8 *texture, u16 *fileId) {
    FieldActorConfig record;
    u32 offset = GetIndexOfObjID(objCode) * sizeof(FieldActorConfig) + 4;

    GFL_ArcToolReadRange(handle, 0, offset, sizeof(record), &record);
    *texture = ResortShop_GetTextureIndex(record.spriteControllerType);
    *fileId = record.rscIndices.res1;
}

// A price with the avenue's discount, which grows with its rank up to 39 and is 40% from rank 40, 75% more with
// ResortSys_GetMaxLevelWeek, and rounded to 10
static u32 ResortShop_GetDiscountedPrice(ResortShopWork *wk, u32 price) {
    fx32 rate;
    u32 rank;
    u32 discount;

    if (ResortSys_GetMaxLevelWeek(wk->sys)) {
        rate = FX32_CONST(1.75);
    } else {
        rate = FX32_ONE;
    }
    rank = JoinAvenue_GetParam(wk->info, JOIN_AVE_INFO_RANK, 0);
    if (rank > 40) {
        rank = 40;
    }
    if (rank != 0 && rank < 40) {
        rank--;
    } else {
        rank = 40;
    }
    discount = price * rank / 100;
    price -= FX_Whole(FX_Mul(FX32_CONST(discount), rate));
    discount = price % 10;
    if (discount != 0) {
        if (discount >= 5) {
            price += 10 - discount;
        } else {
            price -= discount;
        }
    }
    return price;
}

// Points the camera at the shop, from the side of the avenue the player is on or the person's side
static void ResortShop_SetCamera(ResortShop *shop, Field *field, u16 cameraMode) {
    FieldCamera *camera = Field_GetCameraSystem(field);
    int camIndex = 0;
    VecFx32 target;

    shop->useBoundary = FieldCamera_IsUseBoundaryEnable(camera);
    FieldCamera_SetUseBoundaryEnable(camera, FALSE);
    FieldCamera_ClearBind(camera);
    switch (cameraMode) {
    case RESORT_SHOP_CAMERA_SHOP_SIDE: {
        JoinAvenuePerson *person = ResortPerson_GetJoinAvenuePerson(shop->work.person);

        for (camIndex = 0; camIndex < 8; camIndex++) {
            if (JoinAvenueOccupants_GetPerson(shop->work.occupants, camIndex) == person) {
                break;
            }
        }
        if (camIndex % 2) {
            camIndex = 2;
        } else {
            camIndex = 3;
        }
        break;
    }
    case RESORT_SHOP_CAMERA_PLAYER_AREA:
    default: {
        FieldPlayer *player = Field_GetPlayer(field);
        u32 dir = FieldPlayer_GetFaceDir(player);
        s16 x;
        s16 y;
        s16 z;
        u32 i;

        GetPlayerGPosPlusDir(player, dir, &x, &y, &z);
        for (i = 0; i < NELEMS(sAreas); i++) {
            if (sAreas[i].xMin <= x && x < sAreas[i].xMax && sAreas[i].zMin <= z && z < sAreas[i].zMax) {
                camIndex = sAreas[i].camera;
                break;
            }
        }
        break;
    }
    case RESORT_SHOP_CAMERA_FIXED:
        camIndex = 1;
        break;
    }
    FieldCamera_CoordsSetYaw(camera, sCameras[camIndex].yaw);
    FieldCamera_CoordsGetTarget(camera, &target);
    target.x += sCameras[camIndex].dx;
    target.z += sCameras[camIndex].dz;
    FieldCamera_CoordsSetTarget(camera, &target);
}

static BOOL ResortShop_FadeIn(ResortShop *shop, Field *field, int *seq) {
    switch (*seq) {
    case RESORT_SHOP_FADE_IN_BLACKEN:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 1);
        (*seq)++;
        break;
    case RESORT_SHOP_FADE_IN_WAIT_BLACK:
        if (!GFL_FadeIsRunning()) {
            (*seq)++;
        }
        break;
    case RESORT_SHOP_FADE_IN_POSITION:
        ResortShop_SetCamera(shop, field, shop->work.cameraMode);
        ResortSys_HideOverlappingActors(shop->work.people, shop->work.npc, field, shop->work.person,
                                        RESORT_HIDE_WALKERS);
        (*seq)++;
        break;
    case RESORT_SHOP_FADE_IN_CREATE:
        ResortShop_Create(shop);
        (*seq)++;
        break;
    case RESORT_SHOP_FADE_IN_WAIT_LIST:
        if (ResortShop_ShowList(shop)) {
            (*seq)++;
        }
        break;
    case RESORT_SHOP_FADE_IN_REVEAL:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
        (*seq)++;
        break;
    case RESORT_SHOP_FADE_IN_WAIT_REVEAL:
        if (!GFL_FadeIsRunning()) {
            *seq = RESORT_SHOP_FADE_IN_BLACKEN;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL ResortShop_FadeOut(ResortShop *shop, Field *field, int *seq) {
    FieldCamera *camera = Field_GetCameraSystem(field);

    switch (*seq) {
    case RESORT_SHOP_FADE_OUT_BLACKEN:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 1);
        (*seq)++;
        break;
    case RESORT_SHOP_FADE_OUT_WAIT_BLACK:
        if (!GFL_FadeIsRunning()) {
            (*seq)++;
        }
        break;
    case RESORT_SHOP_FADE_OUT_RESTORE:
        ResortPeople_ApplyVisibility(shop->work.people);
        ResortNPC_SetVisible(shop->work.npc, 1);
        ResortShop_Destroy(shop);
        FieldCamera_LoadDefaults(camera);
        FieldCamera_ResetBind(camera);
        FieldCamera_SetUseBoundaryEnable(camera, shop->useBoundary);
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
        (*seq)++;
        break;
    case RESORT_SHOP_FADE_OUT_WAIT_REVEAL:
        if (!GFL_FadeIsRunning()) {
            *seq = RESORT_SHOP_FADE_OUT_BLACKEN;
            return TRUE;
        }
        break;
    }
    return FALSE;
}
