#include "types.h"
#include "app/itemmenu.h"
#include "app/itemmenu_disp.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "pml/item.h"
#include "pml/move.h"
#include "save/bag.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/blink_palanm.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/scroll_bar.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/btl_main.h"

// The bag's screens: BGs, windows, OBJ, the drawing of the item list and of the item under the cursor, and the menus'
// windows. The ROM doesn't name this file; it is named after itemmenu.c for what it holds

// The row marks, and the two values that hide the mark or the whole row
enum {
    ITEMMENU_ROW_MARK_REGISTERED = 0,
    ITEMMENU_ROW_MARK_REGISTRABLE = 1,
    ITEMMENU_ROW_MARK_BALL = 3,
    ITEMMENU_ROW_MARK_MAIL = 4,
    ITEMMENU_ROW_MARK_HELD_EFFECT = 5,
    ITEMMENU_ROW_MARK_TM = 6,
    ITEMMENU_ROW_MARK_HM = 7,
    ITEMMENU_ROW_MARK_NONE = 8,
    ITEMMENU_ROW_MARK_EMPTY = 9,
};

// The pocket tab table's selected and unselected columns
enum {
    ITEMMENU_TAB_SELECTED = 0,
    ITEMMENU_TAB_UNSELECTED = 1,
};

// The pocket tab resource's animation sequences
enum {
    ITEMMENU_TAB_ANM_KEY_ITEMS = 0,
    ITEMMENU_TAB_ANM_TMS_HMS = 1,
    ITEMMENU_TAB_ANM_BERRIES = 2,
    ITEMMENU_TAB_ANM_MEDICINE = 3,
    ITEMMENU_TAB_ANM_ITEMS = 4,
    ITEMMENU_TAB_ANM_KEY_ITEMS_SELECTED = 5,
    ITEMMENU_TAB_ANM_TMS_HMS_SELECTED = 6,
    ITEMMENU_TAB_ANM_BERRIES_SELECTED = 7,
    ITEMMENU_TAB_ANM_MEDICINE_SELECTED = 8,
    ITEMMENU_TAB_ANM_ITEMS_SELECTED = 9,
    ITEMMENU_TAB_ANM_FREE_SPACE = 10,
    ITEMMENU_TAB_ANM_FREE_SPACE_SELECTED = 11,
};

// The colors of the text
#define ITEMMENU_TEXT_COLOR PRINT_COLOR(15, 14, 0)
#define ITEMMENU_TITLE_COLOR PRINT_COLOR(15, 2, 0)
#define ITEMMENU_MENU_COLOR PRINT_COLOR(14, 15, 0)

static void ItemMenuDisp_InitBGs(void);
static void ItemMenuDisp_LoadBagPicture(ItemMenuWork *work, ArcTool *arc);
static void ItemMenuDisp_LoadPocketTabRes(ItemMenuWork *work, ArcTool *arc);
static void ItemMenuDisp_DrawMenuTitle(ItemMenuWork *work);
static void ItemMenuDisp_FreeSelIcon(ItemMenuWork *work);
static void ItemMenuDisp_ShowSelIcon(ItemMenuWork *work, u32 item);
static BOOL ItemMenuDisp_IsFieldMoveTM(u16 item);
static void ItemMenuDisp_CreateButtons(ItemMenuWork *work);
static void ItemMenuDisp_DrawTMInfo(ItemMenuWork *work, u32 move);
static void ItemMenuDisp_Print(ItemMenuWork *work, PrintWindow *window, StrBuf *strbuf, u16 x, s16 y, u16 color);
static void ItemMenuDisp_TransferScreen(PrintWindow *window);
static BOOL ItemMenuDisp_ShowsCount(u8 pocket);

// Each data table is declared just before the function that first uses it, which gives the original's .rodata order

static void ItemMenuDisp_InitBGs(void) {
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearBG(0);
        GFL_BGSysLoadScr(0);
        GFL_BGSysSetBGEnabled(0, FALSE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysClearBG(1);
        GFL_BGSysLoadScr(1);
        GFL_BGSysSetBGEnabled(1, FALSE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysClearBG(2);
        GFL_BGSysLoadScr(2);
        GFL_BGSysSetBGEnabled(2, FALSE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x18000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(3, 0, 1, 0);
        GFL_BGSysClearBG(3);
        GFL_BGSysLoadScr(3);
        GFL_BGSysSetBGEnabled(3, FALSE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(4, FALSE);
        GFL_BGSysLoadScr(4);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(5, FALSE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x10000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(6, FALSE);
        GFL_BGSysFillChar(6, 0, 1, 0);
        GFL_BGSysClearScr(6);
    }
}

void ItemMenuDisp_ShowBGs(void) {
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
}

// Loads the bag's picture for the player's gender
static void ItemMenuDisp_LoadBagPicture(ItemMenuWork *work, ArcTool *arc) {
    u32 gender;
    u32 palette;
    u32 chars;
    u32 screen;

    gender = getTrainerGender(work->playerInfo);
    if (gender == GENDER_MALE) {
        palette = 4;
        chars = 3;
        screen = 5;
    } else if (gender == GENDER_FEMALE) {
        palette = 1;
        chars = 0;
        screen = 2;
    }
    GFL_G2DIOLoadArcNCLRDefault(arc, palette, PALTYPE_MAIN_BG, 0, 0x20, work->heapId);
    work->bg0Chars = GFL_BGSysLoadArcNCGRDynamic(arc, chars, 0, 0, FALSE, work->heapId);
    GFL_G2DIOLoadNSCRSync(arc, screen, 0, 0, CHAR_POS(work->bg0Chars), 0, FALSE, work->heapId);
}

// Loads the pocket tabs' OBJ for the player's gender
static void ItemMenuDisp_LoadPocketTabRes(ItemMenuWork *work, ArcTool *arc) {
    u32 gender;
    u32 palette;
    u32 chars;
    u32 cells;
    u32 anims;

    gender = getTrainerGender(work->playerInfo);
    if (gender == GENDER_MALE) {
        palette = 16;
        chars = 15;
        cells = 14;
        anims = 13;
    } else if (gender == GENDER_FEMALE) {
        palette = 12;
        chars = 11;
        cells = 10;
        anims = 9;
    }
    work->pocketTabPlt = ClActSys_LoadPlttRange(arc, palette, CLACT_VRAM_MAIN, 0xc0, 0, 5, work->heapId);
    work->pocketTabChr = ClActSys_LoadChars(arc, chars, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->pocketTabCel = ClActSys_LoadCellAnims(arc, cells, anims, work->heapId);
}

static const ClActSysSetup sItemMenuClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 100, 100, 100, 100, 16, 16 };

static BGSysVRAMConfig sItemMenuVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// Sets up the screens and loads the bag's graphics
void ItemMenuDisp_Init(ItemMenuWork *work) {
    ClActorSetup categorySetup;
    ClActorSetup typeSetup;
    ClActorSetup sortSetup;
    ArcTool *arc;
    ArcTool *uiArc;
    u32 i;

    G2_BlendNone();
    GFL_BGSysCreate(work->heapId);
    BmpWin_InitAllocator(work->heapId);
    GFL_TextRndInitColorIndexLUT();
    GFL_BGSysSetVRAMBanks(&sItemMenuVRAMConfig);
    ClActSys_Create(&sItemMenuClActSetup, &sItemMenuVRAMConfig, work->heapId);
    {
        BGSysLCDConfig lcd = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcd);
    }
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    ItemMenuDisp_InitBGs();
    GFL_TextRndResetColorIndexLUT();
    work->cursorImageChars = LoadCursorImageEndOfHeap(3, 13, 0, work->heapId);

    arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMMENU, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 8, PALTYPE_SUB_BG, 0, 0, work->heapId);
    if (getTrainerGender(work->playerInfo) == GENDER_MALE) {
        GFL_G2DIOLoadArcNCLR(arc, 8, PALTYPE_SUB_BG, 0x20, 0, 0x20, work->heapId);
    }
    work->bg4Chars = GFL_BGSysLoadArcNCGRDynamic(arc, 6, 4, 0, FALSE, work->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 7, 4, 0, CHAR_POS(work->bg4Chars), 0, FALSE, work->heapId);
    ItemMenuDisp_LoadBagPicture(work, arc);
    work->bg5Chars = GFL_BGSysLoadArcNCGRDynamic(arc, 17, 5, 0, FALSE, work->heapId);
    if (work->pocket == BAG_POCKET_FREE_SPACE) {
        GFL_G2DIOLoadNSCRSync(arc, 24, 5, 0, CHAR_POS(work->bg5Chars), 0, FALSE, work->heapId);
    } else {
        GFL_G2DIOLoadNSCRSync(arc, 23, 5, 0, CHAR_POS(work->bg5Chars), 0, FALSE, work->heapId);
    }
    ItemMenuDisp_LoadPocketTabRes(work, arc);
    work->bg3Chars = GFL_BGSysLoadArcNCGRDynamic(arc, 33, 3, 0, FALSE, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 34, PALTYPE_MAIN_BG, 0x60, 0x20, work->heapId);
    GFL_ArcToolFree(arc);

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x180, 0x20, work->heapId);
    work->actorUnit = ClActUnit_Create(54, 0, work->heapId);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);

    arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 27, PALTYPE_MAIN_BG, 0x100, 0x20, work->heapId);
    work->bg1Chars = GFL_BGSysLoadArcNCGRDynamic(arc, 28, 1, 0, FALSE, work->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 29, 1, 0, CHAR_POS(work->bg1Chars) + 0x8000, 0, FALSE, work->heapId);
    GFL_BGSysLoadScr(1);
    work->buttonPlt = ClActSys_LoadPlttRange(arc, 19, CLACT_VRAM_MAIN, 0x160, 0, 3, work->heapId);
    work->buttonChr = ClActSys_LoadChars(arc, 20, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->buttonCel = ClActSys_LoadCellAnims(arc, 23, 26, work->heapId);
    work->filterButtonChr = ClActSys_LoadChars(arc, 175, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->filterButtonCel = ClActSys_LoadCellAnims(arc, 174, 173, work->heapId);
    GFL_ArcToolFree(arc);
    ItemMenuDisp_CreateButtons(work);

    uiArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), work->heapId);
    work->typeIconPlt =
        ClActSys_LoadPlttRange(uiArc, AppMenuCommon_GetTypeIconPaletteFile(), CLACT_VRAM_SUB, 0x80, 0, 3, work->heapId);
    work->typeIconCel = ClActSys_LoadCellAnims(uiArc, AppMenuCommon_GetTypeIconCellFile(2),
                                               AppMenuCommon_GetTypeIconAnimFile(2), work->heapId);
    for (i = 0; i < NELEMS(work->categoryIconChr); i++) {
        work->categoryIconChr[i] =
            ClActSys_LoadChars(uiArc, AppMenuCommon_GetCategoryIconCharFile(i), FALSE, CLACT_VRAM_SUB, work->heapId);
    }
    for (i = 0; i < NELEMS(work->typeIconChr); i++) {
        work->typeIconChr[i] =
            ClActSys_LoadChars(uiArc, AppMenuCommon_GetTypeIconCharFile(i), FALSE, CLACT_VRAM_SUB, work->heapId);
    }
    GFL_ArcToolFree(uiArc);

    categorySetup.x = 80;
    categorySetup.y = 184;
    categorySetup.priority = 1;
    categorySetup.bgPriority = 1;
    categorySetup.sequence = 0;
    for (i = 0; i < NELEMS(work->categoryIcons); i++) {
        work->categoryIcons[i] = ClActor_Create(work->actorUnit, work->categoryIconChr[i], work->typeIconPlt,
                                                work->typeIconCel, &categorySetup, CLACT_SURFACE_SUB, work->heapId);
        ClActor_SetPalette(work->categoryIcons[i], AppMenuCommon_GetCategoryIconPaletteIndex(i), 1);
        ClActor_SetVisible(work->categoryIcons[i], FALSE);
        ClActor_SetDrawAnim(work->categoryIcons[i], FALSE);
    }
    typeSetup.x = 82;
    typeSetup.y = 165;
    typeSetup.priority = 1;
    typeSetup.bgPriority = 1;
    typeSetup.sequence = 0;
    for (i = 0; i < NELEMS(work->typeIcons); i++) {
        work->typeIcons[i] = ClActor_Create(work->actorUnit, work->typeIconChr[i], work->typeIconPlt, work->typeIconCel,
                                            &typeSetup, CLACT_SURFACE_SUB, work->heapId);
        ClActor_SetPalette(work->typeIcons[i], AppMenuCommon_GetTypeIconPaletteIndex(i), 1);
        ClActor_SetVisible(work->typeIcons[i], FALSE);
        ClActor_SetDrawAnim(work->typeIcons[i], FALSE);
    }

    arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMMENU, work->heapId);
    work->sortButtonPlt = ClActSys_LoadPlttRange(arc, 39, CLACT_VRAM_MAIN, 0x80, 0, 2, work->heapId);
    work->sortButtonChr = ClActSys_LoadChars(arc, 38, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->sortButtonCel = ClActSys_LoadCellAnims(arc, 37, 36, work->heapId);
    GFL_ArcToolFree(arc);
    sortSetup.priority = 10;
    sortSetup.bgPriority = 1;
    sortSetup.x = 146;
    sortSetup.y = 176;
    sortSetup.sequence = ITEMMENU_SORT_ANM_READY;
    work->sortButton = ClActor_Create(work->actorUnit, work->sortButtonChr, work->sortButtonPlt, work->sortButtonCel,
                                      &sortSetup, CLACT_SURFACE_MAIN, work->heapId);
    ClActor_SetAutoAnim(work->sortButton, TRUE);
    ClActor_SetVisible(work->sortButton, TRUE);
    ClActor_SetDrawAnim(work->sortButton, FALSE);
    ItemMenuDisp_UpdateSortButton(work);
}

void ItemMenuDisp_FreeBGChars(ItemMenuWork *work) {
    GFL_BGSysFreeCharMemory(4, CHAR_POS(work->bg4Chars), CHAR_SIZE(work->bg4Chars));
    GFL_BGSysFreeCharMemory(5, CHAR_POS(work->bg5Chars), CHAR_SIZE(work->bg5Chars));
    GFL_BGSysFreeCharMemory(3, CHAR_POS(work->bg3Chars), CHAR_SIZE(work->bg3Chars));
    GFL_BGSysFreeCharMemory(0, CHAR_POS(work->bg0Chars), CHAR_SIZE(work->bg0Chars));
    GFL_BGSysFreeCharMemory(1, CHAR_POS(work->bg1Chars), CHAR_SIZE(work->bg1Chars));
}

// Shows or hides the TM's move data on the lower screen
void ItemMenuDisp_ShowTMInfoBGs(ItemMenuWork *work, BOOL show) {
    GFL_BGSysSetBGEnabled(5, show);
    GFL_BGSysSetBGEnabled(6, show);
}

// Shows or hides the TM panel, and the move's category and type
void ItemMenuDisp_ShowTMIcons(ItemMenuWork *work, BOOL show) {
    u16 move;

    if (show == TRUE) {
        work->tmInfoRequest = ITEMMENU_TM_INFO_SHOW;
    } else {
        work->tmInfoRequest = ITEMMENU_TM_INFO_HIDE;
    }
    if (work->selIcon != NULL) {
        ClActor_SetVisible(work->selIcon, show);
    }
    move = PML_ItemGetTMMoveID(ItemMenu_GetSlot(work, ItemMenu_GetCursorIndex(work))->item);
    if (move != MOVE_NONE) {
        ClActor_SetVisible(work->categoryIcons[PML_MoveGetCategory(move)], show);
        ClActor_SetVisible(work->typeIcons[PML_MoveGetType(move)], show);
    }
}

// Draws the item under the cursor on the lower screen
void ItemMenuDisp_DrawItemInfo(ItemMenuWork *work) {
    BagItem *slot;
    u32 i;
    u32 pocket;
    u16 move;
    u8 hm;
    u32 x;

    for (i = 0; i < NELEMS(work->categoryIcons); i++) {
        ClActor_SetVisible(work->categoryIcons[i], FALSE);
    }
    for (i = 0; i < NELEMS(work->typeIcons); i++) {
        ClActor_SetVisible(work->typeIcons[i], FALSE);
    }
    slot = ItemMenu_GetSlot(work, ItemMenu_GetCursorIndex(work));
    if (slot == NULL || slot->item == ITEM_NONE) {
        ItemMenuDisp_ShowTMIcons(work, FALSE);
        return;
    }
    ItemMenuDisp_ShowTMIcons(work, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(work->pocketLabelWindow.window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->itemNameWindow.window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->countWindow.window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->descWindow.window), 0);
    move = PML_ItemGetTMMoveID(slot->item);
    pocket = BagSave_GetExistingItemPocket(work->bag, slot->item);
    if (move == MOVE_NONE) {
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_97, work->strbuf);
        ItemMenu_SetItemName(work, 0, slot->item);
        GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
        ItemMenuDisp_Print(work, &work->itemNameWindow, work->expandBuf, 0, 4, ITEMMENU_TEXT_COLOR);
        if (ItemMenuDisp_ShowsCount(pocket)) {
            GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_X, work->strbuf);
            WordSetNumber(work->wordSet, 0, slot->count, 3, 0, TRUE);
            GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
            ItemMenuDisp_Print(work, &work->countWindow, work->expandBuf, 0, 4, ITEMMENU_TEXT_COLOR);
        } else {
            BmpWin_FlushChar(work->countWindow.window);
        }
        BmpWin_ClearScreen(work->tmInfoWindow.window);
    } else {
        hm = PML_ItemGetHMID(slot->item);
        if (hm == ITEM_TM_HM_NONE) {
            GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_No, work->strbuf);
            WordSetNumber(work->wordSet, 0, PML_ItemGetTMBitMask(slot->item) + 1, 2, 2, TRUE);
        } else {
            GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Hidden, work->strbuf);
            WordSetNumber(work->wordSet, 0, hm + 1, 2, 2, TRUE);
        }
        loadMoveNameToStrbuf(work->wordSet, 1, move);
        GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
        ItemMenuDisp_Print(work, &work->itemNameWindow, work->expandBuf, 0, 4, ITEMMENU_TEXT_COLOR);
        ItemMenuDisp_DrawTMInfo(work, move);
        BmpWin_FlushChar(work->countWindow.window);
    }
    if (work->pocket == BAG_POCKET_FREE_SPACE) {
        ItemMenu_SetPocketName(work, 0, pocket);
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_100, work->strbuf);
        GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
        x = (96 - GFL_FontGetBlockWidth(work->expandBuf, work->font, 0)) / 2;
        ItemMenuDisp_Print(work, &work->pocketLabelWindow, work->expandBuf, x, 0, ITEMMENU_TEXT_COLOR);
    } else {
        BmpWin_ClearScreen(work->pocketLabelWindow.window);
        BmpWin_FlushChar(work->pocketLabelWindow.window);
    }
    setItemDescriptionTextToStrbuf(work->strbuf, slot->item, work->heapId);
    ItemMenuDisp_Print(work, &work->descWindow, work->strbuf, 0, 4, ITEMMENU_TEXT_COLOR);
    ItemMenuDisp_TransferScreen(&work->pocketLabelWindow);
    ItemMenuDisp_TransferScreen(&work->itemNameWindow);
    ItemMenuDisp_TransferScreen(&work->countWindow);
    ItemMenuDisp_TransferScreen(&work->descWindow);
    ItemMenuDisp_ShowSelIcon(work, slot->item);
}

// Hides the item's data on the lower screen
void ItemMenuDisp_HideItemInfo(ItemMenuWork *work) {
    u32 i;

    ItemMenuDisp_ShowTMIcons(work, FALSE);
    BmpWin_ClearScreen(work->tmInfoWindow.window);
    for (i = 0; i < NELEMS(work->categoryIcons); i++) {
        ClActor_SetVisible(work->categoryIcons[i], FALSE);
    }
    for (i = 0; i < NELEMS(work->typeIcons); i++) {
        ClActor_SetVisible(work->typeIcons[i], FALSE);
    }
}

void ItemMenuDisp_Exit(ItemMenuWork *work) {
    s32 i;

    BmpWin_Free(work->pocketLabelWindow.window);
    BmpWin_Free(work->itemNameWindow.window);
    BmpWin_Free(work->descWindow.window);
    BmpWin_Free(work->countWindow.window);
    BmpWin_Free(work->tmInfoWindow.window);
    BmpWin_Free(work->quantityWindow.window);
    BmpWin_Free(work->priceWindow.window);
    BmpWin_Free(work->menuTitleWindow.window);
    ClActor_Delete(work->scrollBar);
    if (work->selIcon != NULL) {
        ClActor_Delete(work->selIcon);
        ClActSys_FreeChars(work->selIconChr);
        ClActSys_FreePltt(work->selIconPlt);
    }
    ClActSys_FreeCellAnims(work->selIconCel);
    for (i = 0; i < ITEMMENU_ROW_OBJS; i++) {
        GFL_BitmapFree(work->rowNameBitmaps[i]);
    }
    ClActUnit_Delete(work->actorUnit);
    ClActSys_Delete();
}

// Draws the title of the menu
static void ItemMenuDisp_DrawMenuTitle(ItemMenuWork *work) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->menuTitleWindow.window), 0);
    if (ItemMenu_CountFilterActions(work) > 2 && work->pocket == BAG_POCKET_FREE_SPACE) {
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_CannotFind, work->strbuf);
    } else {
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_ItsEmpty, work->strbuf);
    }
    ItemMenuDisp_Print(work, &work->menuTitleWindow, work->strbuf, 0, 4, ITEMMENU_TEXT_COLOR);
}

// Creates the windows
void ItemMenuDisp_CreateWindows(ItemMenuWork *work) {
    ArcTool *arc;

    work->menuTitleWindow.window = BmpWin_CreateDynamic(3, 18, 9, 12, 3, 3, TRUE);
    ItemMenuDisp_DrawMenuTitle(work);
    BmpWin_ClearScreen(work->menuTitleWindow.window);
    work->quantityWindow.window = BmpWin_CreateDynamic(3, 17, 13, 11, 2, 3, TRUE);
    work->priceWindow.window = BmpWin_CreateDynamic(3, 17, 15, 11, 2, 3, TRUE);
    work->tmInfoWindow.window = BmpWin_CreateDynamic(6, 0, 19, 32, 5, 0, TRUE);
    work->itemNameWindow.window = BmpWin_CreateDynamic(6, 7, 5, 18, 3, 0, TRUE);
    work->countWindow.window = BmpWin_CreateDynamic(6, 20, 8, 6, 3, 0, TRUE);
    work->descWindow.window = BmpWin_CreateDynamic(6, 2, 12, 29, 7, 0, TRUE);
    work->pocketLabelWindow.window = BmpWin_CreateDynamic(6, 10, 1, 12, 2, 0, TRUE);
    BmpWin_FlushMap(work->pocketLabelWindow.window);
    BmpWin_FlushMap(work->itemNameWindow.window);
    BmpWin_FlushMap(work->countWindow.window);
    BmpWin_FlushMap(work->descWindow.window);
    BmpWin_FlushMap(work->tmInfoWindow.window);

    arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, work->heapId);
    work->selIconCel = ClActSys_LoadCellAnims(arc, 1, 0, work->heapId);
    GFL_ArcToolFree(arc);

    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Type, work->strbuf);
    ItemMenuDisp_Print(work, &work->tmInfoWindow, work->strbuf, 8, 4, ITEMMENU_TEXT_COLOR);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Category, work->strbuf);
    ItemMenuDisp_Print(work, &work->tmInfoWindow, work->strbuf, 8, 24, ITEMMENU_TEXT_COLOR);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Power, work->strbuf);
    ItemMenuDisp_Print(work, &work->tmInfoWindow, work->strbuf, 112, 4, ITEMMENU_TEXT_COLOR);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Accuracy, work->strbuf);
    ItemMenuDisp_Print(work, &work->tmInfoWindow, work->strbuf, 112, 24, ITEMMENU_TEXT_COLOR);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Pp, work->strbuf);
    ItemMenuDisp_Print(work, &work->tmInfoWindow, work->strbuf, 208, 4, ITEMMENU_TEXT_COLOR);
    if (GCTX_HIDIsUsingTouch() == FALSE) {
        ItemMenuDisp_DrawItemInfo(work);
    }
}

static void ItemMenuDisp_FreeSelIcon(ItemMenuWork *work) {
    if (work->selIcon != NULL) {
        ClActor_Delete(work->selIcon);
        ClActSys_FreeChars(work->selIconChr);
        ClActSys_FreePltt(work->selIconPlt);
        work->selIcon = NULL;
    }
}

// Shows the icon of the item under the cursor
static void ItemMenuDisp_ShowSelIcon(ItemMenuWork *work, u32 item) {
    ArcTool *arc;
    ClActorSetup setup;

    if (work->selIcon != NULL) {
        ItemMenuDisp_FreeSelIcon(work);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, work->heapId);
    work->selIconPlt = ClActSys_LoadPlttRange(arc, GetItemGraphicsDatID(item, ITEM_FILE_ICON_PLTT), CLACT_VRAM_SUB, 0,
                                              0, 1, work->heapId);
    work->selIconChr =
        ClActSys_LoadChars(arc, GetItemGraphicsDatID(item, ITEM_FILE_ICON_CHAR), FALSE, CLACT_VRAM_SUB, work->heapId);
    GFL_ArcToolFree(arc);
    setup.x = 132;
    setup.y = 80;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    work->selIcon = ClActor_Create(work->actorUnit, work->selIconChr, work->selIconPlt, work->selIconCel, &setup,
                                   CLACT_SURFACE_SUB, work->heapId);
    ClActor_SetAutoAnim(work->selIcon, TRUE);
    ClActor_SetVisible(work->selIcon, TRUE);
    ClActor_SetDrawAnim(work->selIcon, FALSE);
}

void ItemMenuDisp_Update(ItemMenuWork *work) {
    ClActSys_Draw();
    if (work->taskMenu != NULL) {
        AppTaskMenu_Update(work->taskMenu);
    }
}

// Loads the list's OBJ
void ItemMenuDisp_LoadListRes(ItemMenuWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMMENU, work->heapId);
    s32 i;

    work->rowMarkPlt = ClActSys_LoadPlttRange(arc, 32, CLACT_VRAM_MAIN, 0x1c0, 0, 1, work->heapId);
    work->rowMarkChr = ClActSys_LoadChars(arc, 31, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->rowMarkCel = ClActSys_LoadCellAnims(arc, 30, 29, work->heapId);
    work->listCursorPlt = ClActSys_LoadPlttRange(arc, 28, CLACT_VRAM_MAIN, 0x60, 0, 1, work->heapId);
    work->listCursorChr = ClActSys_LoadChars(arc, 27, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->listCursorCel = ClActSys_LoadCellAnims(arc, 26, 25, work->heapId);
    for (i = 0; i < ITEMMENU_ROW_OBJS; i++) {
        work->rowNameBitmaps[i] = GFL_BitmapCreate(12, 2, 32, work->heapId);
        work->rowNameChr[i] = ClActSys_LoadChars(arc, 20, FALSE, CLACT_VRAM_MAIN, work->heapId);
    }
    getTrainerGender(work->playerInfo);
    work->rowNamePlt = ClActSys_LoadPlttRange(arc, 21, CLACT_VRAM_MAIN, 0, 0, 3, work->heapId);
    work->rowNameCel = ClActSys_LoadCellAnims(arc, 19, 18, work->heapId);
    GFL_ArcToolFree(arc);
}

// Creates the list's actors
void ItemMenuDisp_CreateListActors(ItemMenuWork *work) {
    ClActorSetup scrollSetup;
    ClActorSetup cursorSetup;
    ClActorSetup rowSetup;
    s32 i;

    scrollSetup.x = 248;
    scrollSetup.y = 26;
    scrollSetup.priority = 1;
    scrollSetup.bgPriority = 2;
    scrollSetup.sequence = ITEMMENU_LIST_ANM_SCROLL_BAR;
    work->scrollBar = ClActor_Create(work->actorUnit, work->listCursorChr, work->listCursorPlt, work->listCursorCel,
                                     &scrollSetup, CLACT_SURFACE_MAIN, work->heapId);
    ClActor_SetVisible(work->scrollBar, TRUE);
    ClActor_SetAutoAnim(work->scrollBar, TRUE);
    ClActor_SetDrawAnim(work->scrollBar, FALSE);
    ItemMenuDisp_UpdateScrollBar(work);

    cursorSetup.x = 140;
    cursorSetup.y = 48;
    cursorSetup.priority = 1;
    cursorSetup.bgPriority = 2;
    cursorSetup.sequence = ITEMMENU_LIST_ANM_CURSOR;
    work->listCursor = ClActor_Create(work->actorUnit, work->listCursorChr, work->listCursorPlt, work->listCursorCel,
                                      &cursorSetup, CLACT_SURFACE_MAIN, work->heapId);
    ClActor_SetDrawAnim(work->listCursor, FALSE);
    ClActor_SetVisible(work->listCursor, TRUE);
    ClActor_SetAutoAnim(work->listCursor, TRUE);

    for (i = 0; i < ITEMMENU_ROW_OBJS; i++) {
        rowSetup.x = 140;
        rowSetup.y = i * 24;
        rowSetup.priority = 10;
        rowSetup.bgPriority = 3;
        rowSetup.sequence = 0;
        work->rowNames[i] = ClActor_Create(work->actorUnit, work->rowNameChr[i], work->rowNamePlt, work->rowNameCel,
                                           &rowSetup, CLACT_SURFACE_MAIN, work->heapId);
        rowSetup.x -= 24;
        rowSetup.sequence = ITEMMENU_ROW_MARK_REGISTRABLE;
        work->rowMarks[i] = ClActor_Create(work->actorUnit, work->rowMarkChr, work->rowMarkPlt, work->rowMarkCel,
                                           &rowSetup, CLACT_SURFACE_MAIN, work->heapId);
        ClActor_SetVisible(work->rowNames[i], FALSE);
        ClActor_SetVisible(work->rowMarks[i], FALSE);
        ClActor_SetDrawAnim(work->rowNames[i], FALSE);
        ClActor_SetDrawAnim(work->rowMarks[i], FALSE);
    }
}

// Whether a TM or HM teaches a move that is used in the field
static BOOL ItemMenuDisp_IsFieldMoveTM(u16 item) {
    if (PML_ItemIsTMHM(item) == TRUE) {
        switch (PML_ItemGetTMMoveID(item)) {
        case MOVE_CUT:
        case MOVE_FLY:
        case MOVE_SURF:
        case MOVE_STRENGTH:
        case MOVE_WATERFALL:
        case MOVE_DIVE:
            return TRUE;
        }
    }
    return FALSE;
}

// Whether each row of the list is dimmed: the rows half off the screen
static const u8 sRowDimmed[] = { TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE };

// Draws the list's rows
void ItemMenuDisp_DrawList(ItemMenuWork *work) {
    s32 i;
    ArcTool *arc;
    BagItem *slot;
    void *data;
    s32 kind;
    u16 color;
    u16 move;

    if (ItemMenu_GetItemCount(work) == 0) {
        ItemMenuDisp_DrawMenuTitle(work);
        BmpWin_ClearScreen(work->menuTitleWindow.window);
        ItemMenuDisp_TransferScreen(&work->menuTitleWindow);
    } else {
        BmpWin *window = work->menuTitleWindow.window;

        BmpWin_ClearScreen(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    }
    if (work->drawnScroll == work->scroll) {
        return;
    }
    arc = PML_ItemArcHandleCreate(work->heapId);
    for (i = 0; i < ITEMMENU_ROW_OBJS; i++) {
        work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_EMPTY;
        if (work->scroll + i < 0) {
            continue;
        }
        slot = ItemMenu_GetSlot(work, work->scroll + i);
        if (slot == NULL || slot->item == ITEM_NONE) {
            continue;
        }
        data = PML_ItemArcHandleReadFile(arc, slot->item, work->heapId);
        kind = PML_ItemGetParam(data, ITEM_PARAM_KIND);
        if (sRowDimmed[i] == FALSE) {
            if (ItemMenuDisp_IsFieldMoveTM(slot->item) == TRUE) {
                color = PRINT_COLOR(6, 5, 13);
            } else {
                color = PRINT_COLOR(2, 1, 13);
            }
        } else if (ItemMenuDisp_IsFieldMoveTM(slot->item) == TRUE) {
            color = PRINT_COLOR(8, 7, 13);
        } else {
            color = PRINT_COLOR(4, 3, 13);
        }
        GFL_BitmapFill(work->rowNameBitmaps[i], 13);
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_96, work->strbuf);
        move = PML_ItemGetTMMoveID(slot->item);
        if (move == MOVE_NONE) {
            ItemMenu_SetItemName(work, 0, slot->item);
        } else {
            loadMoveNameToStrbuf(work->wordSet, 0, move);
        }
        GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
        GFL_TextRendererDrawToBitmapEx(work->rowNameBitmaps[i], 0, 0, work->expandBuf, work->font, color);
        if (work->pocket == BAG_POCKET_MEDICINE) {
            work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_NONE;
        } else if (work->pocket == BAG_POCKET_BERRIES) {
            work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_NONE;
        } else if (kind == ITEM_KIND_BALL) {
            work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_BALL;
        } else if (kind == ITEM_KIND_MAIL) {
            work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_MAIL;
        } else if (kind == ITEM_KIND_HELD_EFFECT) {
            work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_HELD_EFFECT;
        } else if (PML_ItemGetHMID(slot->item) != ITEM_TM_HM_NONE) {
            work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_HM;
        } else if (PML_ItemGetTMBitMask(slot->item) != ITEM_TM_HM_NONE) {
            work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_TM;
        } else if (PML_ItemGetParam(data, ITEM_PARAM_REGISTRABLE)) {
            if (ItemMenu_IsItemRegistered(work, slot->item) == TRUE) {
                work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_REGISTERED;
            } else {
                work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_REGISTRABLE;
            }
        } else {
            work->rowMarkTypes[i] = ITEMMENU_ROW_MARK_NONE;
        }
        GFL_HeapFree(data);
    }
    GFL_ArcToolFree(arc);
    work->iconsDirty = TRUE;
}

// Moves the list's cursor to its row
void ItemMenuDisp_UpdateListCursor(ItemMenuWork *work) {
    ClActorPos pos;

    ItemMenuDisp_SetListCursorPalette(work, ITEMMENU_CURSOR_PALETTE_OFF);
    ClActor_GetSurfacePos(work->listCursor, &pos, 0xffff);
    pos.y = (work->cursorRow + 1) * 24;
    ClActor_SetSurfacePos(work->listCursor, &pos, 0xffff);
    if (GCTX_HIDIsUsingTouch() == FALSE) {
        ItemMenuDisp_SetListCursorPalette(work, ITEMMENU_CURSOR_PALETTE_ON);
    }
    BlinkPalAnm_InitAnime(work->paletteAnim);
    ClActor_SetVisible(work->scrollBar, ItemMenu_GetItemCount(work) >= ITEMMENU_LIST_ROWS + 1);
    ClActor_SetAnimFrame(work->listCursor, 0);
    ClActor_SetAnimFrame(work->scrollBar, 0);
}

// Copies the rows' names to their OBJ and shows their marks
void ItemMenuDisp_UpdateListRows(ItemMenuWork *work) {
    u8 *pixels;
    u32 chars;
    s32 i;

    for (i = 0; i < ITEMMENU_ROW_OBJS; i++) {
        chars = ClActSys_GetCharVramAddr(work->rowNameChr[i], FALSE);
        pixels = GFL_BitmapGetPixelData(work->rowNameBitmaps[i]);
        DC_FlushRange(pixels, GFL_BitmapCalcPixelDataSize(work->rowNameBitmaps[i]));
        GX_LoadOBJ(pixels + 0x100, chars + 0x80, 0x80);
        GX_LoadOBJ(pixels + 0x280, chars + 0x100, 0x80);
        GX_LoadOBJ(pixels, chars + 0x300, 0x100);
        GX_LoadOBJ(pixels + 0x180, chars + 0x400, 0x100);
        if (work->rowMarkTypes[i] != ITEMMENU_ROW_MARK_EMPTY) {
            if (work->rowMarkTypes[i] != ITEMMENU_ROW_MARK_NONE) {
                ClActor_SetSequence(work->rowMarks[i], work->rowMarkTypes[i]);
                ClActor_SetVisible(work->rowMarks[i], TRUE);
            } else {
                ClActor_SetVisible(work->rowMarks[i], FALSE);
            }
            ClActor_SetVisible(work->rowNames[i], TRUE);
        } else {
            ClActor_SetVisible(work->rowMarks[i], FALSE);
            ClActor_SetVisible(work->rowNames[i], FALSE);
        }
    }
}

// Moves the scroll bar's thumb to the touch
void ItemMenuDisp_TouchScrollBar(ItemMenuWork *work) {
    u32 x;
    u32 y;
    ClActorPos pos;

    if (GFL_HIDGetTouchHeldPoint(&x, &y) == TRUE) {
        if (y < 26) {
            y = 26;
        } else if (y > 142) {
            y = 142;
        }
        ClActor_GetSurfacePos(work->scrollBar, &pos, 0xffff);
        pos.y = y;
        ClActor_SetSurfacePos(work->scrollBar, &pos, 0xffff);
    }
}

// Moves the scroll bar's thumb to the list's scroll
void ItemMenuDisp_UpdateScrollBar(ItemMenuWork *work) {
    s32 count = ItemMenu_GetItemCount(work);
    ClActorPos pos;

    if (count >= ITEMMENU_LIST_ROWS + 1) {
        ClActor_GetSurfacePos(work->scrollBar, &pos, 0xffff);
        pos.y = ScrollBar_GetPos(count - ITEMMENU_LIST_ROWS, work->scroll + 1, 26, 142, 0);
        ClActor_SetSurfacePos(work->scrollBar, &pos, 0xffff);
    }
}

// Creates the buttons of the lower screen
static void ItemMenuDisp_CreateButtons(ItemMenuWork *work) {
    ClActorSetup setup;
    u8 i;

    setup.priority = 10;
    setup.bgPriority = 1;
    {
        u8 sequences[] = { ITEMMENU_TOUCHBAR_ANM_POCKET_LEFT, ITEMMENU_TOUCHBAR_ANM_POCKET_RIGHT,
                           ITEMMENU_TOUCHBAR_ANM_REGISTER_OFF, ITEMMENU_TOUCHBAR_ANM_CLOSE,
                           ITEMMENU_TOUCHBAR_ANM_BACK };
        u8 xs[] = { 0, 120, 173, 192, 224 };

        for (i = 0; i < 5; i++) {
            setup.x = xs[i];
            setup.y = 168;
            setup.sequence = sequences[i];
            if (i == ITEMMENU_BUTTON_REGISTER) {
                setup.y += 4;
            }
            work->buttons[i] = ClActor_Create(work->actorUnit, work->buttonChr, work->buttonPlt, work->buttonCel,
                                              &setup, CLACT_SURFACE_MAIN, work->heapId);
            ClActor_SetVisible(work->buttons[i], TRUE);
            ClActor_SetDrawAnim(work->buttons[i], FALSE);
        }
    }
    setup.x = 146;
    setup.y = 168;
    setup.sequence = ITEMMENU_FILTER_ANM_READY;
    work->filterButton = ClActor_Create(work->actorUnit, work->filterButtonChr, work->buttonPlt, work->filterButtonCel,
                                        &setup, CLACT_SURFACE_MAIN, work->heapId);
    ClActor_SetVisible(work->filterButton, FALSE);
    ClActor_SetAutoAnim(work->filterButton, TRUE);
    ClActor_SetDrawAnim(work->filterButton, FALSE);
    if (ItemMenu_CanRegister(work) == FALSE) {
        ClActor_SetVisible(work->buttons[ITEMMENU_BUTTON_REGISTER], FALSE);
    }
    if (work->mode == BAG_MODE_GIVE_ITEM) {
        ClActor_SetVisible(work->buttons[ITEMMENU_BUTTON_CLOSE], FALSE);
    }
    {
        u8 sequences[] = { ITEMMENU_TOUCHBAR_ANM_QUANTITY_UP, ITEMMENU_TOUCHBAR_ANM_QUANTITY_DOWN };
        u8 ys[] = { 97, 119 };
        u8 button;

        for (i = 0; i < 2; i++) {
            setup.x = 232;
            setup.y = ys[i];
            setup.sequence = sequences[i];
            setup.bgPriority = 0;
            button = i + ITEMMENU_BUTTON_QUANTITY_UP;
            work->buttons[button] = ClActor_Create(work->actorUnit, work->buttonChr, work->buttonPlt, work->buttonCel,
                                                   &setup, CLACT_SURFACE_MAIN, work->heapId);
            ClActor_SetAutoAnim(work->buttons[button], TRUE);
            ClActor_SetVisible(work->buttons[button], FALSE);
            ClActor_SetDrawAnim(work->buttons[button], FALSE);
        }
    }
}

// The priority of each pocket's tab, and its animation sequences when picked and not
static const u8 sPocketTabPriorities[] = { 14, 12, 10, 12, 16, 16 };
static const u8 sPocketTabSequences[][2] = { { ITEMMENU_TAB_ANM_ITEMS_SELECTED, ITEMMENU_TAB_ANM_ITEMS },
                                             { ITEMMENU_TAB_ANM_MEDICINE_SELECTED, ITEMMENU_TAB_ANM_MEDICINE },
                                             { ITEMMENU_TAB_ANM_TMS_HMS_SELECTED, ITEMMENU_TAB_ANM_TMS_HMS },
                                             { ITEMMENU_TAB_ANM_BERRIES_SELECTED, ITEMMENU_TAB_ANM_BERRIES },
                                             { ITEMMENU_TAB_ANM_KEY_ITEMS_SELECTED, ITEMMENU_TAB_ANM_KEY_ITEMS },
                                             { ITEMMENU_TAB_ANM_FREE_SPACE_SELECTED, ITEMMENU_TAB_ANM_FREE_SPACE } };

// Where the pocket tabs slide to
static ClActorPos sPocketTabPos[] = { { 0, 80 }, { 0, 80 }, { 0, 80 }, { 0, 80 }, { 0, 80 }, { 0, 80 } };

// Creates the pocket tabs, left of where they slide in to
void ItemMenuDisp_CreatePocketTabs(ItemMenuWork *work) {
    ClActorSetup setup;
    u32 i;

    for (i = 0; i < NELEMS(work->pocketTabs); i++) {
        setup.priority = sPocketTabPriorities[i];
        setup.bgPriority = 1;
        setup.x = sPocketTabPos[i].x - 16;
        setup.y = sPocketTabPos[i].y;
        setup.sequence = sPocketTabSequences[i][ITEMMENU_TAB_UNSELECTED];
        work->pocketTabs[i] = ClActor_Create(work->actorUnit, work->pocketTabChr, work->pocketTabPlt,
                                             work->pocketTabCel, &setup, CLACT_SURFACE_MAIN, work->heapId);
        ClActor_SetAutoAnim(work->pocketTabs[i], TRUE);
    }
}

// Shows the pocket's tab as picked
void ItemMenuDisp_SetPocketTab(ItemMenuWork *work, u32 pocket) {
    u32 i;

    for (i = 0; i < NELEMS(work->pocketTabs); i++) {
        if (i == pocket) {
            ClActor_SetSequence(work->pocketTabs[i], sPocketTabSequences[i][ITEMMENU_TAB_SELECTED]);
        } else {
            ClActor_SetSequence(work->pocketTabs[i], sPocketTabSequences[i][ITEMMENU_TAB_UNSELECTED]);
        }
    }
}

void ItemMenuDisp_ClearMsgWindow(ItemMenuWork *work) {
    BmpWin_ClearFrame(work->msgWindow.window, WINFRAME_TRANSFER_VBLANK);
}

// Opens the item menu with the messages of its entries
void ItemMenuDisp_OpenItemMenu(ItemMenuWork *work, u32 *msgIds, s32 count) {
    AppTaskMenuInit init;
    int i;

    init.heapId = work->heapId;
    init.itemCount = count;
    init.items = work->menuItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 14;
    init.height = 3;
    for (i = 0; i < count; i++) {
        work->menuItems[i].str = GFL_StrBufCreate(100, work->heapId);
        GFL_MsgDataLoadStrbuf(work->msgData, msgIds[i], work->menuItems[i].str);
        work->menuItems[i].color = ITEMMENU_MENU_COLOR;
        work->menuItems[i].type = 0;
    }
    work->menuItems[i - 1].type = APP_TASKMENU_ITEM_RETURN;
    work->taskMenu = AppTaskMenu_Create(&init, work->taskMenuRes);
    for (i = 0; i < count; i++) {
        GFL_StrBufFree(work->menuItems[i].str);
    }
    ItemMenuDisp_SetButtonsActive(work, FALSE);
}

// Opens the sort menu with the messages of its entries
void ItemMenuDisp_OpenSortMenu(ItemMenuWork *work, u32 *msgIds, s32 count) {
    AppTaskMenuInit init;
    int i;

    init.heapId = work->heapId;
    init.itemCount = count;
    init.items = work->menuItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 14;
    init.height = 3;
    for (i = 0; i < count; i++) {
        work->menuItems[i].str = GFL_StrBufCreate(100, work->heapId);
        GFL_MsgDataLoadStrbuf(work->msgData, msgIds[i], work->menuItems[i].str);
        work->menuItems[i].color = ITEMMENU_MENU_COLOR;
        work->menuItems[i].type = 0;
    }
    work->menuItems[i - 1].type = APP_TASKMENU_ITEM_RETURN;
    work->taskMenu = AppTaskMenu_Create(&init, work->taskMenuRes);
    for (i = 0; i < count; i++) {
        GFL_StrBufFree(work->menuItems[i].str);
    }
    ItemMenuDisp_SetButtonsActive(work, FALSE);
}

// Opens the Free Space's filter menu
void ItemMenuDisp_OpenFilterMenu(ItemMenuWork *work, u8 count) {
    AppTaskMenuInit init;
    u32 action;
    s32 i;

    init.heapId = work->heapId;
    init.itemCount = count;
    init.items = work->menuItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 14;
    init.height = 3;
    for (i = 0; i < count; i++) {
        action = work->freeSpaceMenuActions[i];
        if (action == ITEMMENU_FILTER_ACTION_ALL) {
            work->menuItems[i].str = GFL_StrBufCreate(100, work->heapId);
            GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_All, work->menuItems[i].str);
            work->menuItems[i].color = ITEMMENU_MENU_COLOR;
            work->menuItems[i].type = 0;
        } else if (action == ITEMMENU_FILTER_ACTION_CANCEL) {
            work->menuItems[i].str = GFL_StrBufCreate(100, work->heapId);
            GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Cancel, work->menuItems[i].str);
            work->menuItems[i].color = ITEMMENU_MENU_COLOR;
            work->menuItems[i].type = APP_TASKMENU_ITEM_RETURN;
        } else {
            work->menuItems[i].str = GFL_StrBufCreate(100, work->heapId);
            GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_100, work->strbuf);
            loadBagPocketNameToStrbuf(work->wordSet, 0, action);
            GFL_WordSetFormatStrbuf(work->wordSet, work->menuItems[i].str, work->strbuf);
            work->menuItems[i].color = ITEMMENU_MENU_COLOR;
            work->menuItems[i].type = 0;
        }
    }
    work->taskMenu = AppTaskMenu_Create(&init, work->taskMenuRes);
    for (i = 0; i < count; i++) {
        GFL_StrBufFree(work->menuItems[i].str);
    }
    ItemMenuDisp_SetButtonsActive(work, FALSE);
}

// Loads the lower screen's frame for a pocket
void ItemMenuDisp_LoadPocketFrame(ItemMenuWork *work, u32 pocket) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMMENU, work->heapId);

    if (pocket == BAG_POCKET_FREE_SPACE) {
        GFL_G2DIOLoadNSCRAsync(arc, 24, 5, 0, CHAR_POS(work->bg5Chars), 0, FALSE, work->heapId);
    } else {
        GFL_G2DIOLoadNSCRAsync(arc, 23, 5, 0, CHAR_POS(work->bg5Chars), 0, FALSE, work->heapId);
    }
    GFL_ArcToolFree(arc);
}

// Draws a TM's move data
static void ItemMenuDisp_DrawTMInfo(ItemMenuWork *work, u32 move) {
    PrintWindow *window = &work->tmInfoWindow;
    u8 pp;
    s32 power;
    s32 accuracy;

    ClActor_SetVisible(work->categoryIcons[PML_MoveGetCategory(move)], TRUE);
    ClActor_SetVisible(work->typeIcons[PML_MoveGetType(move)], TRUE);
    pp = PML_MoveGetMaxPP(move, 0);
    power = PML_MoveGetParam(move, MOVE_PARAM_POWER);
    accuracy = PML_MoveGetParam(move, MOVE_PARAM_ACCURACY);
    GFL_BitmapFillArea(BmpWin_GetBitmap(window->window), 176, 0, 32, 40, 0);
    GFL_BitmapFillArea(BmpWin_GetBitmap(window->window), 232, 0, 24, 24, 0);
    if (power <= 1) {
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_15, work->strbuf);
        ItemMenuDisp_Print(work, window, work->strbuf, 176, 4, ITEMMENU_TEXT_COLOR);
    } else {
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_63, work->strbuf);
        WordSetNumber(work->wordSet, 0, power, 3, 0, TRUE);
        GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
        ItemMenuDisp_Print(work, window, work->expandBuf, 176, 4, ITEMMENU_TEXT_COLOR);
    }
    if (accuracy == 0 || PML_MoveIsAlwaysHit(move) == TRUE) {
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_15, work->strbuf);
        ItemMenuDisp_Print(work, window, work->strbuf, 176, 24, ITEMMENU_TEXT_COLOR);
    } else {
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_63, work->strbuf);
        WordSetNumber(work->wordSet, 0, accuracy, 3, 0, TRUE);
        GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
        ItemMenuDisp_Print(work, window, work->expandBuf, 176, 24, ITEMMENU_TEXT_COLOR);
    }
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_62, work->strbuf);
    WordSetNumber(work->wordSet, 0, pp, 2, 0, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
    ItemMenuDisp_Print(work, window, work->expandBuf, 232, 4, ITEMMENU_TEXT_COLOR);
    ItemMenuDisp_TransferScreen(window);
}

// Shows the message in expandBuf, printed a character at a time if stream is TRUE
void ItemMenuDisp_ShowMessage(ItemMenuWork *work, BOOL stream) {
    PrintWindow *window;
    if (work->msgWindow.window == NULL) {
        work->msgWindow.window = BmpWin_CreateDynamic(3, 1, 1, 30, 4, 12, TRUE);
    }
    window = &work->msgWindow;
    GFL_BitmapFill(BmpWin_GetBitmap(window->window), 15);
    BmpWin_FlushChar(window->window);
    if (stream == TRUE) {
        work->printStream = PrintStream_Create(window->window, 0, 0, work->expandBuf, work->font,
                                               TextSpeed_GetCurrentWait(), work->tcbManager, 2, work->heapId, 15);
    } else {
        ItemMenuDisp_Print(work, window, work->expandBuf, 0, 0, PRINT_COLOR(1, 2, 15));
    }
    BmpWin_DrawFrame(window->window, WINFRAME_TRANSFER_VBLANK, work->cursorImageChars, 13);
    ItemMenuDisp_TransferScreen(window);
}

void ItemMenuDisp_ShowMessageNow(ItemMenuWork *work) {
    ItemMenuDisp_ShowMessage(work, FALSE);
}

// Says what the player does with the item
void ItemMenuDisp_SetItemMenuMessage(ItemMenuWork *work, u32 item) {
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Selected, work->strbuf);
    ItemMenu_SetItemName(work, 0, item);
    GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
}

// Whether the message is printed and read
BOOL ItemMenuDisp_IsMessageDone(ItemMenuWork *work) {
    if (work->printStream != NULL) {
        u32 state = PrintStream_GetState(work->printStream);

        KeyCursor_Update(work->keyCursor, work->printStream, work->msgWindow.window);
        switch (state) {
        case PRINT_STREAM_DONE:
            PrintStream_Delete(work->printStream);
            work->printStream = NULL;
            work->streamResumed = FALSE;
            break;
        case PRINT_STREAM_PAUSED:
            if (work->streamResumed == FALSE &&
                ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed())) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
                PrintStream_Continue(work->printStream);
                work->streamResumed = TRUE;
            }
            break;
        case PRINT_STREAM_RUNNING:
            if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchHeld()) {
                PrintStream_SkipWait(work->printStream, 0);
            }
            work->streamResumed = FALSE;
            break;
        }
        return FALSE;
    }
    return TRUE;
}

void ItemMenuDisp_CreatePocketWindows(ItemMenuWork *work) {
    work->pocketNameWindow.window = BmpWin_CreateDynamic(2, 3, 21, 12, 3, 12, TRUE);
    work->moneyWindow.window = BmpWin_CreateDynamic(2, 1, 21, 8, 3, 12, TRUE);
    work->moneyValueWindow.window = BmpWin_CreateDynamic(2, 9, 21, 9, 3, 12, TRUE);
    ItemMenuDisp_DrawPocketName(work, work->pocket);
    ItemMenuDisp_SetPocketTab(work, work->pocket);
}

void ItemMenuDisp_FreePocketWindows(ItemMenuWork *work) {
    BmpWin_Free(work->pocketNameWindow.window);
    BmpWin_Free(work->moneyValueWindow.window);
    BmpWin_Free(work->moneyWindow.window);
}

// Draws the pocket's name
void ItemMenuDisp_DrawPocketName(ItemMenuWork *work, u32 pocket) {
    u32 x;

    GFL_BitmapFill(BmpWin_GetBitmap(work->pocketNameWindow.window), 0);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_100, work->strbuf);
    loadBagPocketNameToStrbuf(work->wordSet, 0, pocket);
    GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
    x = (96 - GFL_FontGetBlockWidth(work->expandBuf, work->font, 0)) / 2;
    ItemMenuDisp_Print(work, &work->pocketNameWindow, work->expandBuf, x, 4, ITEMMENU_TITLE_COLOR);
    ItemMenuDisp_TransferScreen(&work->pocketNameWindow);
}

// Draws the player's money
void ItemMenuDisp_DrawMoney(ItemMenuWork *work) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->moneyValueWindow.window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->moneyWindow.window), 0);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Money, work->strbuf);
    ItemMenuDisp_Print(work, &work->moneyWindow, work->strbuf, 0, 4, ITEMMENU_TITLE_COLOR);
    ItemMenuDisp_TransferScreen(&work->moneyWindow);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_54, work->strbuf);
    WordSetNumber(work->wordSet, 0, getCash(getTrainerCardDataBlkAddress(work->gameData)), 7, 1, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
    ItemMenuDisp_Print(work, &work->moneyValueWindow, work->expandBuf, 0, 4, ITEMMENU_TITLE_COLOR);
    ItemMenuDisp_TransferScreen(&work->moneyValueWindow);
}

// Shows the money in place of the pocket's name and arrows
void ItemMenuDisp_ShowMoney(ItemMenuWork *work) {
    ClActor_SetVisible(work->buttons[ITEMMENU_BUTTON_POCKET_LEFT], FALSE);
    ClActor_SetVisible(work->buttons[ITEMMENU_BUTTON_POCKET_RIGHT], FALSE);
    BmpWin_ClearScreen(work->pocketNameWindow.window);
    ItemMenuDisp_DrawMoney(work);
}

// Shows the pocket's name and arrows again
void ItemMenuDisp_HideMoney(ItemMenuWork *work) {
    ClActor_SetVisible(work->buttons[ITEMMENU_BUTTON_POCKET_LEFT], TRUE);
    ClActor_SetVisible(work->buttons[ITEMMENU_BUTTON_POCKET_RIGHT], TRUE);
    BmpWin_ClearScreen(work->moneyValueWindow.window);
    BmpWin_ClearScreen(work->moneyWindow.window);
    ItemMenuDisp_DrawPocketName(work, work->pocket);
}

// Opens the yes and no menu
void ItemMenuDisp_OpenYesNoMenu(ItemMenuWork *work) {
    AppTaskMenuInit init;

    init.heapId = work->heapId;
    init.itemCount = 2;
    init.items = work->menuItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 12;
    init.width = 8;
    init.height = 3;
    work->menuItems[0].str = GFL_StrBufCreate(100, work->heapId);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Yes, work->menuItems[0].str);
    work->menuItems[0].color = ITEMMENU_MENU_COLOR;
    work->menuItems[0].type = 0;
    work->menuItems[1].str = GFL_StrBufCreate(100, work->heapId);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_No_2, work->menuItems[1].str);
    work->menuItems[1].color = ITEMMENU_MENU_COLOR;
    work->menuItems[1].type = 0;
    work->taskMenu = AppTaskMenu_Create(&init, work->taskMenuRes);
    GFL_StrBufFree(work->menuItems[0].str);
    GFL_StrBufFree(work->menuItems[1].str);
    ItemMenuDisp_SetBackButtonActive(work, FALSE);
    ItemMenuDisp_SetButtonsActive(work, FALSE);
}

void ItemMenuDisp_CloseMenu(ItemMenuWork *work) {
    AppTaskMenu_Free(work->taskMenu);
    work->taskMenu = NULL;
}

// Draws the quantity picker's frame
void ItemMenuDisp_DrawQuantityFrame(ItemMenuWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMMENU, work->heapId);
    void *file = GFL_ArcToolReadHeapNewLZ(arc, 35, FALSE, work->heapId);
    NNSG2dScreenData *screen;
    s32 i = 0;

    if (NNS_G2dGetUnpackedScreenData(file, &screen)) {
        u16 *tiles = (u16 *)screen->rawData;
        s32 count = (screen->screenWidth / 8) * (screen->screenHeight / 8);

        for (; i < count; i++) {
            tiles[i] += (u16)work->bg3Chars | (3 << 12);
        }
        GFL_BGSysLoadScrAreaAll(3, screen->rawData, 16, 12, screen->screenWidth / 8, screen->screenHeight / 8);
        GFL_BGSysQueueScrLoad(3);
    }
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

// Draws the quantity, and the price when selling
void ItemMenuDisp_DrawQuantity(ItemMenuWork *work, s32 quantity) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->quantityWindow.window), 5);
    GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_X, work->strbuf);
    WordSetNumber(work->wordSet, 0, quantity, 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
    ItemMenuDisp_Print(work, &work->quantityWindow, work->expandBuf, 0, 0, PRINT_COLOR(15, 14, 5));
    ItemMenuDisp_TransferScreen(&work->quantityWindow);
    if (work->quantityMode == ITEMMENU_QUANTITY_SELL) {
        PrintWindow *window = &work->priceWindow;
        s32 price = ItemMenu_GetSellPrice(work->item, work->quantity, work->heapId);
        GFL_BitmapFill(BmpWin_GetBitmap(window->window), 5);
        GFL_MsgDataLoadStrbuf(work->msgData, BtlMain_Text_Empty_56, work->strbuf);
        WordSetNumber(work->wordSet, 0, price, 7, 0, TRUE);
        GFL_WordSetFormatStrbuf(work->wordSet, work->expandBuf, work->strbuf);
        ItemMenuDisp_Print(work, window, work->expandBuf, 0, 0, PRINT_COLOR(15, 14, 5));
        ItemMenuDisp_TransferScreen(window);
    }
}

// The Y button shortcut of a pocket
u32 ItemMenuDisp_GetPocketShortcut(s32 pocket) {
    switch (pocket) {
    case BAG_POCKET_ITEMS:
        return SHORTCUT_BAG_ITEMS;
    case BAG_POCKET_BERRIES:
        return SHORTCUT_BAG_BERRIES;
    case BAG_POCKET_MEDICINE:
        return SHORTCUT_BAG_MEDICINE;
    case BAG_POCKET_TMS_HMS:
        return SHORTCUT_BAG_TMS_HMS;
    case BAG_POCKET_KEY_ITEMS:
        return SHORTCUT_BAG_KEY_ITEMS;
    case BAG_POCKET_FREE_SPACE:
        return SHORTCUT_BAG_FREE_SPACE;
    }
}

// Sets the palette of the row under the list's cursor
void ItemMenuDisp_SetListCursorPalette(ItemMenuWork *work, u32 palette) {
    ClActorPos cursorPos;
    ClActorPos rowPos;
    u32 i;

    ClActor_GetSurfacePos(work->listCursor, &cursorPos, CLACT_SURFACE_MAIN);
    for (i = 0; i < ITEMMENU_ROW_OBJS; i++) {
        ClActor_GetSurfacePos(work->rowNames[i], &rowPos, CLACT_SURFACE_MAIN);
        if (rowPos.y == cursorPos.y) {
            break;
        }
    }
    ClActor_SetPalette(work->rowNames[i], (u8)palette, 0);
}

// Lights the lower screen up for the list, or dims it behind a menu
void ItemMenuDisp_SetButtonsActive(ItemMenuWork *work, BOOL active) {
    BOOL registered;

    if (work->buttonsActive == active) {
        return;
    }
    work->buttonsActive = active;
    registered = GameData_IsShortcutRegistered(work->gameData, ItemMenuDisp_GetPocketShortcut(work->pocket));
    if (active == TRUE) {
        PaletteFade_StartFade(work->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0x47cf, 0, 0, 0, 0,
                              GFL_VBlankGetTCBMgr());
        PaletteFade_StartFade(work->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 1, 0, 0, 0, 0, GFL_VBlankGetTCBMgr());
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2, 0);
        ItemMenuDisp_UpdateSortButton(work);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_POCKET_LEFT], ITEMMENU_TOUCHBAR_ANM_POCKET_LEFT);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_POCKET_RIGHT], ITEMMENU_TOUCHBAR_ANM_POCKET_RIGHT);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_CLOSE], ITEMMENU_TOUCHBAR_ANM_CLOSE);
        if (registered == TRUE) {
            ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_REGISTER], ITEMMENU_TOUCHBAR_ANM_REGISTER_ON);
        } else {
            ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_REGISTER], ITEMMENU_TOUCHBAR_ANM_REGISTER_OFF);
        }
        ItemMenuDisp_SetBackButtonActive(work, TRUE);
    } else {
        PaletteFade_LoadFromVRAM(work->paletteFade, PALFADE_VRAM_MAIN_OBJ, 0, 0x200);
        PaletteFade_StartFade(work->paletteFade, 1 << PALFADE_BUFFER_MAIN_OBJ, 0x47cf, 0, 8, 8, 0,
                              GFL_VBlankGetTCBMgr());
        PaletteFade_StartFade(work->paletteFade, 1 << PALFADE_BUFFER_MAIN_BG, 1, 0, 8, 8, 0, GFL_VBlankGetTCBMgr());
        if (work->mode == BAG_MODE_SELL && ClActor_IsVisible(work->buttons[ITEMMENU_BUTTON_POCKET_LEFT]) == FALSE) {
            G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, -8);
        } else {
            G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2, -8);
        }
        ClActor_SetSequence(work->sortButton, ITEMMENU_SORT_ANM_DISABLED);
        ClActor_SetSequence(work->filterButton, ITEMMENU_FILTER_ANM_DISABLED);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_POCKET_LEFT], ITEMMENU_TOUCHBAR_ANM_POCKET_LEFT_DISABLED);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_POCKET_RIGHT], ITEMMENU_TOUCHBAR_ANM_POCKET_RIGHT_DISABLED);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_CLOSE], ITEMMENU_TOUCHBAR_ANM_CLOSE_DISABLED);
        if (registered == TRUE) {
            ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_REGISTER], ITEMMENU_TOUCHBAR_ANM_REGISTER_ON_DISABLED);
        } else {
            ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_REGISTER], ITEMMENU_TOUCHBAR_ANM_REGISTER_OFF_DISABLED);
        }
    }
}

// Lights the back button up, or dims it
void ItemMenuDisp_SetBackButtonActive(ItemMenuWork *work, BOOL active) {
    if (active == TRUE) {
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_BACK], ITEMMENU_TOUCHBAR_ANM_BACK);
    } else {
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_BACK], ITEMMENU_TOUCHBAR_ANM_BACK_DISABLED);
    }
}

// Shows the sort button, or the Free Space's filter button
void ItemMenuDisp_UpdateSortButton(ItemMenuWork *work) {
    if (work->pocket != BAG_POCKET_FREE_SPACE) {
        ClActor_SetVisible(work->sortButton, TRUE);
        ClActor_SetVisible(work->filterButton, FALSE);
        if (ItemMenu_GetItemCount(work) <= 1) {
            ClActor_SetSequence(work->sortButton, ITEMMENU_SORT_ANM_DISABLED);
        } else {
            ClActor_SetSequence(work->sortButton, ITEMMENU_SORT_ANM_READY);
        }
    } else {
        ClActor_SetVisible(work->filterButton, TRUE);
        ClActor_SetVisible(work->sortButton, FALSE);
        if (ItemMenu_CountFilterActions(work) > 2) {
            ClActor_SetSequence(work->filterButton, ITEMMENU_FILTER_ANM_READY);
        } else {
            ClActor_SetSequence(work->filterButton, ITEMMENU_FILTER_ANM_DISABLED);
        }
    }
}

// Lights the buttons up, or dims them while an item is moved
void ItemMenuDisp_SetMoveButtons(ItemMenuWork *work, BOOL active) {
    BOOL registered = GameData_IsShortcutRegistered(work->gameData, ItemMenuDisp_GetPocketShortcut(work->pocket));

    if (active == TRUE) {
        ItemMenuDisp_UpdateSortButton(work);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_POCKET_LEFT], ITEMMENU_TOUCHBAR_ANM_POCKET_LEFT);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_POCKET_RIGHT], ITEMMENU_TOUCHBAR_ANM_POCKET_RIGHT);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_CLOSE], ITEMMENU_TOUCHBAR_ANM_CLOSE);
        if (registered == TRUE) {
            ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_REGISTER], ITEMMENU_TOUCHBAR_ANM_REGISTER_ON);
        } else {
            ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_REGISTER], ITEMMENU_TOUCHBAR_ANM_REGISTER_OFF);
        }
    } else {
        ClActor_SetSequence(work->sortButton, ITEMMENU_SORT_ANM_DISABLED);
        ClActor_SetSequence(work->filterButton, ITEMMENU_FILTER_ANM_DISABLED);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_POCKET_LEFT], ITEMMENU_TOUCHBAR_ANM_POCKET_LEFT_DISABLED);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_POCKET_RIGHT], ITEMMENU_TOUCHBAR_ANM_POCKET_RIGHT_DISABLED);
        ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_CLOSE], ITEMMENU_TOUCHBAR_ANM_CLOSE_DISABLED);
        if (registered == TRUE) {
            ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_REGISTER], ITEMMENU_TOUCHBAR_ANM_REGISTER_ON_DISABLED);
        } else {
            ClActor_SetSequence(work->buttons[ITEMMENU_BUTTON_REGISTER], ITEMMENU_TOUCHBAR_ANM_REGISTER_OFF_DISABLED);
        }
    }
}

// Slides the pocket tabs in. Returns whether they are still moving
BOOL ItemMenuDisp_SlidePocketTabs(ItemMenuWork *work) {
    ClActorPos pos;
    u32 i;

    for (i = 0; i < NELEMS(work->pocketTabs); i++) {
        ClActor_GetSurfacePos(work->pocketTabs[i], &pos, 0xffff);
        if (pos.x == sPocketTabPos[i].x) {
            return FALSE;
        }
        pos.x += 4;
        ClActor_SetSurfacePos(work->pocketTabs[i], &pos, 0xffff);
    }
    return TRUE;
}

static void ItemMenuDisp_Print(ItemMenuWork *work, PrintWindow *window, StrBuf *strbuf, u16 x, s16 y, u16 color) {
    PrintWindow_Print(window, work->printQueue, x, y, strbuf, work->font, color);
}

static void ItemMenuDisp_TransferScreen(PrintWindow *window) {
    BmpWin_FlushMap(window->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window->window));
}

// Prints the queue's text and sends the windows that are done to VRAM
void ItemMenuDisp_FlushWindows(ItemMenuWork *work) {
    PrintQueue_Main(work->printQueue);
    PrintWindow_Flush(&work->tmInfoWindow, work->printQueue);
    PrintWindow_Flush(&work->pocketLabelWindow, work->printQueue);
    PrintWindow_Flush(&work->itemNameWindow, work->printQueue);
    PrintWindow_Flush(&work->countWindow, work->printQueue);
    PrintWindow_Flush(&work->descWindow, work->printQueue);
    PrintWindow_Flush(&work->quantityWindow, work->printQueue);
    PrintWindow_Flush(&work->moneyWindow, work->printQueue);
    PrintWindow_Flush(&work->moneyValueWindow, work->printQueue);
    PrintWindow_Flush(&work->priceWindow, work->printQueue);
    PrintWindow_Flush(&work->menuTitleWindow, work->printQueue);
    PrintWindow_Flush(&work->msgWindow, work->printQueue);
    PrintWindow_Flush(&work->pocketNameWindow, work->printQueue);
}

// Whether a pocket's items show their count
static BOOL ItemMenuDisp_ShowsCount(u8 pocket) {
    BOOL showCount[] = { TRUE, TRUE, FALSE, TRUE, FALSE, TRUE };

    return showCount[pocket];
}
