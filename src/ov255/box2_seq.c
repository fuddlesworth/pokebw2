#include "types.h"
#include "app/box2_bgwfrm.h"
#include "app/box2_bmp.h"
#include "app/box2_main.h"
#include "app/box2_obj.h"
#include "app/box2_seq.h"
#include "app/box2_ui.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/math.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/box.h"
#include "system/app_taskmenu.h"
#include "system/bgwinfrm.h"
#include "system/cursor_move.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "text/system/box.h"

// The PC box's sequences: one function per state of the box, run each frame by Box2Seq_Main, each returning the
// next state. Names are ours

typedef int (*Box2SeqFunc)(Box2SysWork *syswk);

// What the yes/no menu's answers run, by the question asked
typedef struct {
    Box2SeqFunc yes;
    Box2SeqFunc no;
} Box2YesNoFuncs;

// The sub procs, with the state after each
typedef struct {
    Box2SeqFunc call;
    Box2SeqFunc exit;
    int nextSeq;
} Box2SubProc;

// Phases of Box2Seq_OpenMenu.
enum {
    BOX2_OPEN_MENU_SHOW_MENU = 0,
    BOX2_OPEN_MENU_RESUME = 1,
};

// Phases of Box2Seq_ArrangeMoveStart, Box2Seq_RangeStart.
enum {
    BOX2_START_BOX_MOVE_CLOSE_MENU = 0,
    BOX2_START_BOX_MOVE_OPEN_BOX_LIST = 1,
    BOX2_START_BOX_MOVE_READY = 2,
};

// Phases of Box2Seq_ArrangeMovePartyStart, Box2Seq_RangePartyOut.
enum {
    BOX2_START_PARTY_MOVE_CLOSE_MENU = 0,
    BOX2_START_PARTY_MOVE_OPEN_PARTY = 1,
    BOX2_START_PARTY_MOVE_READY = 2,
};

// Phases of Box2Seq_ArrangeMovePartyOut, Box2Seq_RangePartyAway.
enum {
    BOX2_MOVE_TO_PARTY_CLOSE_BOX_LIST = 0,
    BOX2_MOVE_TO_PARTY_OPEN_PARTY = 1,
    BOX2_MOVE_TO_PARTY_READY = 2,
};

// Phases of Box2Seq_RangePartyIn, Box2Seq_MovePartyIn.
enum {
    BOX2_QUICK_PARTY_IN_PREPARE = 0,
    BOX2_QUICK_PARTY_IN_CHECK_AND_SLIDE = 1,
    BOX2_QUICK_PARTY_IN_READY = 2,
};

// Phases of Box2Seq_RangePartySwitch, Box2Seq_MovePartySwitch.
enum {
    BOX2_QUICK_PARTY_SWITCH_PREPARE = 0,
    BOX2_QUICK_PARTY_SWITCH_RESUME_PARTY = 1,
    BOX2_QUICK_PARTY_SWITCH_RESUME_TRAY = 2,
};

// Phases of Box2Seq_RangePartyClose, Box2Seq_MovePartyClose.
enum {
    BOX2_QUICK_PARTY_CLOSE_CLOSE_PARTY = 0,
    BOX2_QUICK_PARTY_CLOSE_OPEN_BOX_LIST = 1,
    BOX2_QUICK_PARTY_CLOSE_READY = 2,
};

// Phases of Box2Seq_ArrangePartyOut, Box2Seq_ArrangeBoxPartyOut.
enum {
    BOX2_MENU_PARTY_OUT_OPEN_PARTY = 0,
    BOX2_MENU_PARTY_OUT_READY = 1,
};

// Phases of Box2Seq_ArrangePartyAway, Box2Seq_ArrangeBoxPartyAway.
enum {
    BOX2_MENU_PARTY_AWAY_CLOSE_PARTY = 0,
    BOX2_MENU_PARTY_AWAY_READY = 1,
};

// Phases of Box2Seq_WithdrawCheckParty.
enum {
    BOX2_WITHDRAW_CHECK_CLOSE_MENU = 0,
    BOX2_WITHDRAW_CHECK_CHECK_CAPACITY = 1,
};

// Phases of Box2Seq_WithdrawTake.
enum {
    BOX2_WITHDRAW_TAKE_OPEN_PARTY = 0,
    BOX2_WITHDRAW_TAKE_MOVE_POKEMON = 1,
    BOX2_WITHDRAW_TAKE_COMMIT_AND_CLOSE_PARTY = 2,
    BOX2_WITHDRAW_TAKE_READY = 3,
};

// Phases of Box2Seq_DepositPickParty.
enum {
    BOX2_DEPOSIT_PICK_CLOSE_MENU = 0,
    BOX2_DEPOSIT_PICK_CHECK_DEPOSIT = 1,
    BOX2_DEPOSIT_PICK_HIDE_PARTY = 2,
    BOX2_DEPOSIT_PICK_POSITION_CURSOR = 3,
    BOX2_DEPOSIT_PICK_CHOOSE_BOX = 4,
};

// Phases of Box2Seq_DepositPut.
enum {
    BOX2_DEPOSIT_PUT_POSITION_CURSOR = 0,
    BOX2_DEPOSIT_PUT_REOPEN_PARTY = 1,
    BOX2_DEPOSIT_PUT_RETURN_POKEMON = 2,
    BOX2_DEPOSIT_PUT_SHOW_PARTY = 3,
    BOX2_DEPOSIT_PUT_PLACE_POKEMON = 4,
    BOX2_DEPOSIT_PUT_COMMIT = 5,
    BOX2_DEPOSIT_PUT_CANCEL = 6,
    BOX2_DEPOSIT_PUT_READY = 7,
};

// Phases of Box2Seq_ItemPartyIn.
enum {
    BOX2_ITEM_PARTY_IN_PREPARE = 0,
    BOX2_ITEM_PARTY_IN_OPEN_PARTY = 1,
    BOX2_ITEM_PARTY_IN_READY = 2,
};

// Phases of Box2Seq_ItemPartyAway.
enum {
    BOX2_ITEM_PARTY_AWAY_CLOSE_PARTY = 0,
    BOX2_ITEM_PARTY_AWAY_READY = 1,
};

// Phases of Box2Seq_ItemPartyReturn.
enum {
    BOX2_ITEM_PARTY_RETURN_HIDE_ICON = 0,
    BOX2_ITEM_PARTY_RETURN_WAIT_ICON_HIDE = 1,
};

// Phases of Box2Seq_DreamWorldPick.
enum {
    BOX2_DREAM_WORLD_PICK_CLOSE_MENU = 0,
    BOX2_DREAM_WORLD_PICK_CHECK_ELIGIBILITY = 1,
};

// Phases of Box2Seq_TakeItem.
enum {
    BOX2_TAKE_ITEM_CHECK_ITEM = 0,
    BOX2_TAKE_ITEM_MAIL_ERROR = 1,
    BOX2_TAKE_ITEM_EGG_ERROR = 2,
    BOX2_TAKE_ITEM_BATTLE_BOX_LOCKED_ERROR = 3,
};

// Phases of Box2Seq_MarkingOpen.
enum {
    BOX2_OPEN_MARKINGS_CLOSE_MENU = 0,
    BOX2_OPEN_MARKINGS_OPEN_MARKINGS = 1,
    BOX2_OPEN_MARKINGS_READY = 2,
};

// Phases of Box2Seq_ReleaseCheck.
enum {
    BOX2_RELEASE_CHECK_CLOSE_MENU = 0,
    BOX2_RELEASE_CHECK_CHECK_ELIGIBILITY = 1,
};

// Phases of Box2Seq_ReleaseDo.
enum {
    BOX2_RELEASE_POKEMON_PRINT_RELEASED = 0,
    BOX2_RELEASE_POKEMON_PRINT_BYE = 1,
    BOX2_RELEASE_POKEMON_CLEAR_POKEMON = 2,
    BOX2_RELEASE_POKEMON_FINISH_PARTY_SORT = 3,
};

// Phases of Box2Seq_ReleaseMoveLocked.
enum {
    BOX2_RELEASE_BLOCKED_RESTORE_SIZE = 0,
    BOX2_RELEASE_BLOCKED_EXPLAIN_BLOCK = 1,
    BOX2_RELEASE_BLOCKED_RETURN_TO_MENU = 2,
};

// Phases of Box2Seq_BoxMenuOpen.
enum {
    BOX2_OPEN_BOX_MENU_CLOSE_OLD_MENU = 0,
    BOX2_OPEN_BOX_MENU_OPEN_MENU = 1,
    BOX2_OPEN_BOX_MENU_READY = 2,
};

// Phases of Box2Seq_BoxMenuClose.
enum {
    BOX2_CLOSE_BOX_MENU_CLOSE_MENU = 0,
    BOX2_CLOSE_BOX_MENU_RESTORE_CURSOR = 1,
};

// Phases of Box2Seq_BoxJumpOpen.
enum {
    BOX2_OPEN_BOX_JUMP_CLOSE_MENU = 0,
    BOX2_OPEN_BOX_JUMP_OPEN_BOX_LIST = 1,
    BOX2_OPEN_BOX_JUMP_READY = 2,
};

// Phases of Box2Seq_BoxJumpClose.
enum {
    BOX2_CLOSE_BOX_JUMP_CLOSE_BOX_LIST = 0,
    BOX2_CLOSE_BOX_JUMP_RESTORE_CURSOR = 1,
};

// Phases of Box2Seq_WallpaperMenuOpen.
enum {
    BOX2_OPEN_WALLPAPER_MENU_CLOSE_OLD_MENU = 0,
    BOX2_OPEN_WALLPAPER_MENU_OPEN_MENU = 1,
    BOX2_OPEN_WALLPAPER_MENU_READY = 2,
};

// Phases of Box2Seq_WallpaperChange.
enum {
    BOX2_CHANGE_WALLPAPER_FADE_OUT = 0,
    BOX2_CHANGE_WALLPAPER_APPLY_WALLPAPER = 1,
    BOX2_CHANGE_WALLPAPER_FADE_IN = 2,
    BOX2_CHANGE_WALLPAPER_READY = 3,
};

// Why a party Pokemon cannot be deposited.
enum {
    BOX2_DEPOSIT_ALLOWED = 0,
    BOX2_DEPOSIT_LAST_BATTLER = 1,
    BOX2_DEPOSIT_MAIL = 2,
};

static int Box2Seq_Init(Box2SysWork *syswk);
static int Box2Seq_Release(Box2SysWork *syswk);
static int Box2Seq_Wipe(Box2SysWork *syswk);
static int Box2Seq_PaletteFade(Box2SysWork *syswk);
static int Box2Seq_Wait(Box2SysWork *syswk);
static int Box2Seq_VFunc(Box2SysWork *syswk);
static int Box2Seq_TrgWait(Box2SysWork *syswk);
static int Box2Seq_YesNo(Box2SysWork *syswk);
static int Box2Seq_ButtonAnm(Box2SysWork *syswk);
static int Box2Seq_SubProcCall(Box2SysWork *syswk);
static int Box2Seq_SubProcMain(Box2SysWork *syswk);
static int Box2Seq_Start(Box2SysWork *syswk);
static int Box2Seq_StartWait(Box2SysWork *syswk);
static int Box2Seq_PokeMoveEnd(Box2SysWork *syswk);
static int Box2Seq_OpenMenu(Box2SysWork *syswk);
static int Box2Seq_BoxListScrollTouch(Box2SysWork *syswk);
static int Box2Seq_LeaveState(Box2SysWork *syswk);
static int Box2Seq_ArrangeMain(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveToBoxList(Box2SysWork *syswk);
static int Box2Seq_ArrangeMenuClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveStart(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveMain(Box2SysWork *syswk);
static int Box2Seq_ArrangeMovePartyStart(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveEnd(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveErrClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeMovePartyOut(Box2SysWork *syswk);
static int Box2Seq_RangeStart(Box2SysWork *syswk);
static int Box2Seq_RangeMain(Box2SysWork *syswk);
static int Box2Seq_RangePartyOut(Box2SysWork *syswk);
static int Box2Seq_RangeMoveEnd(Box2SysWork *syswk);
static int Box2Seq_RangeMoveErrClose(Box2SysWork *syswk);
static int Box2Seq_RangePartyAway(Box2SysWork *syswk);
static int Box2Seq_RangePartyIn(Box2SysWork *syswk);
static int Box2Seq_RangePartyMain(Box2SysWork *syswk);
static int Box2Seq_RangePartySwitch(Box2SysWork *syswk);
static int Box2Seq_RangePartyClose(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyOut(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyMain(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyAway(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyMenuClose(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyMoveEnd(Box2SysWork *syswk);
static int Box2Seq_MovePartyIn(Box2SysWork *syswk);
static int Box2Seq_MovePartyMain(Box2SysWork *syswk);
static int Box2Seq_MovePartySwitch(Box2SysWork *syswk);
static int Box2Seq_MovePartyClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxMain(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxMoveEnd(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxMenuClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyOut(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyMain(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyAway(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyMenuClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyMoveEnd(Box2SysWork *syswk);
static int Box2Seq_SubProcReturn(Box2SysWork *syswk);
static int Box2Seq_WithdrawMain(Box2SysWork *syswk);
static int Box2Seq_WithdrawMenuClose(Box2SysWork *syswk);
static int Box2Seq_WithdrawCheckParty(Box2SysWork *syswk);
static int Box2Seq_WithdrawTake(Box2SysWork *syswk);
static int Box2Seq_WithdrawEnd(Box2SysWork *syswk);
static int Box2Seq_DepositPartyMain(Box2SysWork *syswk);
static int Box2Seq_DepositPartyMenuClose(Box2SysWork *syswk);
static int Box2Seq_DepositPartyReturn(Box2SysWork *syswk);
static int Box2Seq_DepositPickParty(Box2SysWork *syswk);
static int Box2Seq_DepositPickBox(Box2SysWork *syswk);
static int Box2Seq_DepositPut(Box2SysWork *syswk);
static int Box2Seq_DepositFullClose(Box2SysWork *syswk);
static int Box2Seq_DepositEnd(Box2SysWork *syswk);
static int Box2Seq_ItemMain(Box2SysWork *syswk);
static int Box2Seq_ItemMenuClose(Box2SysWork *syswk);
static int Box2Seq_ItemToBag(Box2SysWork *syswk);
static int Box2Seq_ItemHideIcon(Box2SysWork *syswk);
static int Box2Seq_ItemBack(Box2SysWork *syswk);
static int Box2Seq_ItemBackToMenu(Box2SysWork *syswk);
static int Box2Seq_ItemPartyIn(Box2SysWork *syswk);
static int Box2Seq_ItemPartyOutMain(Box2SysWork *syswk);
static int Box2Seq_ItemPartyAway(Box2SysWork *syswk);
static int Box2Seq_ItemDropKey(Box2SysWork *syswk);
static int Box2Seq_ItemDropTouch(Box2SysWork *syswk);
static int Box2Seq_ItemMailClose(Box2SysWork *syswk);
static int Box2Seq_ItemDropKeyPartyOut(Box2SysWork *syswk);
static int Box2Seq_ItemPartyMain(Box2SysWork *syswk);
static int Box2Seq_ItemPartyMenuClose(Box2SysWork *syswk);
static int Box2Seq_ItemPartyDrop(Box2SysWork *syswk);
static int Box2Seq_ItemPartySwap(Box2SysWork *syswk);
static int Box2Seq_ItemPartyReturn(Box2SysWork *syswk);
static int Box2Seq_DreamWorldMain(Box2SysWork *syswk);
static int Box2Seq_DreamWorldMenuOpened(Box2SysWork *syswk);
static int Box2Seq_DreamWorldPick(Box2SysWork *syswk);
static int Box2Seq_DreamWorldMenuClose(Box2SysWork *syswk);
static int Box2Seq_CallPokeStatus(Box2SysWork *syswk);
static int Box2Seq_TakeItem(Box2SysWork *syswk);
static int Box2Seq_CallBoxSearch(Box2SysWork *syswk);
static int Box2Seq_SummaryReturn(Box2SysWork *syswk);
static int Box2Seq_NameInputReturn(Box2SysWork *syswk);
static int Box2Seq_BagReturn(Box2SysWork *syswk);
static int Box2Seq_BoxSearchReturn(Box2SysWork *syswk);
static int Box2Seq_GivenItemMsg(Box2SysWork *syswk);
static int Box2Seq_MarkingOpen(Box2SysWork *syswk);
static int Box2Seq_MarkingMain(Box2SysWork *syswk);
static int Box2Seq_MarkingClose(Box2SysWork *syswk);
static int Box2Seq_MarkingCursorMoveEnd(Box2SysWork *syswk);
static int Box2Seq_ReleaseCheck(Box2SysWork *syswk);
static int Box2Seq_ReleaseCheckMoves(Box2SysWork *syswk);
static int Box2Seq_ReleaseDo(Box2SysWork *syswk);
static int Box2Seq_ReleaseMoveLocked(Box2SysWork *syswk);
static int Box2Seq_BoxMenuOpen(Box2SysWork *syswk);
static int Box2Seq_BoxMenuMain(Box2SysWork *syswk);
static int Box2Seq_BoxMenuClose(Box2SysWork *syswk);
static int Box2Seq_BoxJumpOpen(Box2SysWork *syswk);
static int Box2Seq_BoxJumpMain(Box2SysWork *syswk);
static int Box2Seq_BoxJumpClose(Box2SysWork *syswk);
static int Box2Seq_BoxJumpReturn(Box2SysWork *syswk);
static int Box2Seq_WallpaperMenuOpen(Box2SysWork *syswk);
static int Box2Seq_WallpaperMenuMain(Box2SysWork *syswk);
static int Box2Seq_WallpaperChange(Box2SysWork *syswk);
static int Box2Seq_CallNameInput(Box2SysWork *syswk);
static int Box2Seq_AskContinue(Box2SysWork *syswk);
static int Box2Seq_YesTakeItem(Box2SysWork *syswk);
static int Box2Seq_YesRelease(Box2SysWork *syswk);
static int Box2Seq_NoRelease(Box2SysWork *syswk);
static int Box2Seq_YesItemToBag(Box2SysWork *syswk);
static int Box2Seq_Leave(Box2SysWork *syswk);
static int Box2Seq_ReturnToMain(Box2SysWork *syswk);
static int Box2Seq_YesDreamWorldPick(Box2SysWork *syswk);
static int Box2Seq_NoDreamWorldPick(Box2SysWork *syswk);
static int Box2Seq_NoBackToMenu(Box2SysWork *syswk);

static int Box2Seq_Goto(Box2SysWork *syswk, int seq);
static int Box2Seq_WipeIn(Box2SysWork *syswk, int nextSeq);
static int Box2Seq_RunRequestedVFunc(Box2SysWork *syswk, int seq);
static int Box2Seq_PokeMoveEndTo(Box2SysWork *syswk, int seq);
static int Box2Seq_OpenYesNo(Box2SysWork *syswk, u32 type);
static int Box2Seq_AnimFrameButton(Box2SysWork *syswk, u32 frame, int seq);
static int Box2Seq_AnimActorButton(Box2SysWork *syswk, u32 id, u32 anim, int seq);
static int Box2Seq_AnimMarkButton(Box2SysWork *syswk, u32 button, int seq);
static int Box2Seq_WipeOutCallSubProc(Box2SysWork *syswk, u32 type);
static int Box2Seq_ArrangeGiveItemReturn(Box2SysWork *syswk);
static int Box2Seq_ItemGiveItemReturn(Box2SysWork *syswk);
static int Box2Seq_BattleBoxGiveItemReturn(Box2SysWork *syswk);
static int Box2Seq_WithdrawSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_DepositSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_ArrangeSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_BattleBoxSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_BattleBoxMoveSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_ScrollTrayLeft(Box2SysWork *syswk, u32 frameOut, int seq);
static int Box2Seq_ScrollTrayRight(Box2SysWork *syswk, u32 frameOut, int seq);
static int Box2Seq_ArrangePickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ArrangePartyPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ArrangeBoxPartyPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_DepositPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_OpenPartyPokeMenu(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_MovePartyPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_WithdrawPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_OpenPokeMenu(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_ArrangePickUpKey(Box2SysWork *syswk, u32 pos);
static void AllocPokeMoveWork(Box2SysWork *syswk);
static void FreePokeMoveWork(Box2SysWork *syswk);
static int Box2Seq_ItemTakeTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemOpenMenu(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemPartyOutTakeTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemPartyOutTakeKey(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemDropHeldKey(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemPartyTakeTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemPartyOpenMenu(Box2SysWork *syswk, u32 pos);
static int Box2Seq_MovePartyFrame(Box2SysWork *syswk, int seq);
static int Box2Seq_NextMoveMode(Box2SysWork *syswk, int seq);
static int Box2Seq_SetMoveMode(Box2SysWork *syswk, int seq);

static int Box2Seq_MovePickUpTouched(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_MoveDropKey(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_ScrollToPickedBox(Box2SysWork *syswk, int seq);
static void AllocBoxListDragWork(Box2SysWork *syswk);
static int Box2Seq_MovePartyFrameTo(Box2SysWork *syswk, int seq);
static int Box2Seq_RangePickUpTouched(Box2SysWork *syswk, int seq);
static int Box2Seq_MovePickUpKey(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_RangePickUpKey(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_RangeDropKey(Box2SysWork *syswk, u32 pos, int seq);
static u32 GetDepositBlock(Box2SysWork *syswk);
static void AllocItemMoveWork(Box2SysWork *syswk);
static void FreeItemMoveWork(Box2SysWork *syswk);
static void PutPartyLeft(Box2SysWork *syswk);
static void PutPartyRight(Box2SysWork *syswk);
static void ToggleMark(Box2SysWork *syswk, u32 mark);
static int Box2Seq_PickWallpaper(Box2SysWork *syswk, u32 pos);
static void DimActorsForContinuePrompt(Box2SysWork *syswk);
static void CancelRange(Box2SysWork *syswk);
static void SortRangeEnds(Box2SysWork *syswk);

// The menus' items: a message and whether the item closes the menu. MWCC sorts static data by size, unstably, so
// tables of one size are declared in the order that lays them out as the ROM has them
static const Box2MenuItem sGiveItemMenu[] = { { Box_Text_Give, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Cancel_2, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sDreamWorldMenu[] = { { Box_Text_Tuck, BOX2_MENU_ITEM_PLAIN },
                                                { Box_Text_Cancel_2, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sHeldItemMenu[] = { { Box_Text_Move_3, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Bag, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Cancel_2, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sBoxMenu[] = { { Box_Text_SwitchBox, BOX2_MENU_ITEM_PLAIN },
                                         { Box_Text_Wallpaper, BOX2_MENU_ITEM_PLAIN },
                                         { Box_Text_Name, BOX2_MENU_ITEM_PLAIN },
                                         { Box_Text_Cancel, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sBattleBoxMenu[] = { { Box_Text_Move_2, BOX2_MENU_ITEM_PLAIN },
                                               { Box_Text_Summary, BOX2_MENU_ITEM_PLAIN },
                                               { Box_Text_HeldItems, BOX2_MENU_ITEM_PLAIN },
                                               { Box_Text_Cancel_2, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sDepositMenu[] = { { Box_Text_Deposit, BOX2_MENU_ITEM_PLAIN },
                                             { Box_Text_Summary, BOX2_MENU_ITEM_PLAIN },
                                             { Box_Text_Marking, BOX2_MENU_ITEM_PLAIN },
                                             { Box_Text_Release, BOX2_MENU_ITEM_PLAIN },
                                             { Box_Text_Cancel_2, BOX2_MENU_ITEM_ARROW } };
// The wallpaper themes and their wallpapers, the special ones on two pages
static const Box2MenuItem sSpecialMenu[] = { { Box_Text_Monochrome, BOX2_MENU_ITEM_PLAIN },
                                             { Box_Text_TeamPlasma, BOX2_MENU_ITEM_PLAIN },
                                             { Box_Text_Movie, BOX2_MENU_ITEM_PLAIN },
                                             { Box_Text_Pwt, BOX2_MENU_ITEM_PLAIN },
                                             { Box_Text_Back, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sMiscMenu[] = { { Box_Text_PokeCenter, BOX2_MENU_ITEM_PLAIN },
                                          { Box_Text_Machine, BOX2_MENU_ITEM_PLAIN },
                                          { Box_Text_Checks, BOX2_MENU_ITEM_PLAIN },
                                          { Box_Text_Simple, BOX2_MENU_ITEM_PLAIN },
                                          { Box_Text_Back, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sThemeMenu[] = { { Box_Text_Scenery1, BOX2_MENU_ITEM_PLAIN },
                                           { Box_Text_Scenery2, BOX2_MENU_ITEM_PLAIN },
                                           { Box_Text_Scenery3, BOX2_MENU_ITEM_PLAIN },
                                           { Box_Text_Misc, BOX2_MENU_ITEM_PLAIN },
                                           { Box_Text_Back, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sScenery1Menu[] = { { Box_Text_Forest, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_City, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Desert, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Savanna, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Back, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sScenery2Menu[] = { { Box_Text_Crag, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Volcano, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Snow, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Cave, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Back, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sScenery3Menu[] = { { Box_Text_Beach, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Seafloor, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_River, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Sky, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Back, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sWithdrawMenu[] = { { Box_Text_Withdraw, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Summary, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Marking, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Release, BOX2_MENU_ITEM_PLAIN },
                                              { Box_Text_Cancel_2, BOX2_MENU_ITEM_ARROW } };
static const Box2MenuItem sThemeMenuSpecial[] = {
    { Box_Text_Special, BOX2_MENU_ITEM_PLAIN },  { Box_Text_Scenery1, BOX2_MENU_ITEM_PLAIN },
    { Box_Text_Scenery2, BOX2_MENU_ITEM_PLAIN }, { Box_Text_Scenery3, BOX2_MENU_ITEM_PLAIN },
    { Box_Text_Misc, BOX2_MENU_ITEM_PLAIN },     { Box_Text_Back, BOX2_MENU_ITEM_ARROW }
};
static const Box2MenuItem sSpecialMenuNext[] = {
    { Box_Text_Monochrome, BOX2_MENU_ITEM_PLAIN }, { Box_Text_TeamPlasma, BOX2_MENU_ITEM_PLAIN },
    { Box_Text_Movie, BOX2_MENU_ITEM_PLAIN },      { Box_Text_Pwt, BOX2_MENU_ITEM_PLAIN },
    { Box_Text_Next, BOX2_MENU_ITEM_PLAIN },       { Box_Text_Back, BOX2_MENU_ITEM_ARROW }
};
static const Box2MenuItem sSpecialMenu2[] = {
    { Box_Text_Kyurem1, BOX2_MENU_ITEM_PLAIN },  { Box_Text_Kyurem2, BOX2_MENU_ITEM_PLAIN },
    { Box_Text_Reshiram, BOX2_MENU_ITEM_PLAIN }, { Box_Text_Zekrom, BOX2_MENU_ITEM_PLAIN },
    { Box_Text_Previous, BOX2_MENU_ITEM_PLAIN }, { Box_Text_Back, BOX2_MENU_ITEM_ARROW }
};
static const Box2MenuItem sArrangeMenu[] = {
    { Box_Text_Move, BOX2_MENU_ITEM_PLAIN },      { Box_Text_Summary, BOX2_MENU_ITEM_PLAIN },
    { Box_Text_HeldItems, BOX2_MENU_ITEM_PLAIN }, { Box_Text_Marking, BOX2_MENU_ITEM_PLAIN },
    { Box_Text_Release, BOX2_MENU_ITEM_PLAIN },   { Box_Text_Cancel_2, BOX2_MENU_ITEM_ARROW }
};

static const Box2SubProc sSubProcs[] = {
    { Box2Main_PokeStatusCall, Box2Main_PokeStatusExit, BOX2SEQ_SUMMARY_RETURN },
    { Box2Main_BagCall, Box2Main_BagExit, BOX2SEQ_BAG_RETURN },
    { Box2Main_NameInputCall, Box2Main_NameInputExit, BOX2SEQ_NAME_INPUT_RETURN },
    { Box2Main_BoxSearchCall, Box2Main_BoxSearchExit, BOX2SEQ_BOX_SEARCH_RETURN },
};

// clang-format off
static const Box2SeqFunc sMainSeq[] = {
    Box2Seq_Init, Box2Seq_Release, Box2Seq_Wipe,
    Box2Seq_PaletteFade, Box2Seq_Wait, Box2Seq_VFunc,
    Box2Seq_TrgWait, Box2Seq_YesNo, Box2Seq_ButtonAnm,
    Box2Seq_SubProcCall, Box2Seq_SubProcMain, Box2Seq_Start,
    Box2Seq_StartWait, Box2Seq_PokeMoveEnd, Box2Seq_OpenMenu,
    Box2Seq_BoxListScrollTouch, Box2Seq_LeaveState, Box2Seq_ArrangeMain,
    Box2Seq_ArrangeMoveToBoxList, Box2Seq_ArrangeMenuClose, Box2Seq_ArrangeMoveStart,
    Box2Seq_ArrangeMoveMain, Box2Seq_ArrangeMovePartyStart, Box2Seq_ArrangeMoveEnd,
    Box2Seq_ArrangeMoveErrClose, Box2Seq_ArrangeMovePartyOut, Box2Seq_ArrangePartyOut,
    Box2Seq_ArrangePartyMain, Box2Seq_ArrangePartyAway, Box2Seq_ArrangePartyMenuClose,
    Box2Seq_ArrangePartyMoveEnd, Box2Seq_MovePartyIn, Box2Seq_MovePartyMain,
    Box2Seq_MovePartySwitch, Box2Seq_MovePartyClose, Box2Seq_RangeStart,
    Box2Seq_RangeMain, Box2Seq_RangePartyOut, Box2Seq_RangeMoveEnd,
    Box2Seq_RangeMoveErrClose, Box2Seq_RangePartyAway, Box2Seq_RangePartyIn,
    Box2Seq_RangePartyMain, Box2Seq_RangePartySwitch, Box2Seq_RangePartyClose,
    Box2Seq_ArrangeBoxMain, Box2Seq_ArrangeBoxMoveEnd, Box2Seq_ArrangeBoxMenuClose,
    Box2Seq_ArrangeBoxPartyOut, Box2Seq_ArrangeBoxPartyMain, Box2Seq_ArrangeBoxPartyAway,
    Box2Seq_ArrangeBoxPartyMenuClose, Box2Seq_ArrangeBoxPartyMoveEnd, Box2Seq_SubProcReturn,
    Box2Seq_WithdrawMain, Box2Seq_WithdrawMenuClose, Box2Seq_WithdrawCheckParty,
    Box2Seq_WithdrawTake, Box2Seq_WithdrawEnd, Box2Seq_DepositPartyMain,
    Box2Seq_DepositPartyMenuClose, Box2Seq_DepositPartyReturn, Box2Seq_DepositPickParty,
    Box2Seq_DepositPickBox, Box2Seq_DepositPut, Box2Seq_DepositFullClose,
    Box2Seq_DepositEnd, Box2Seq_ItemMain, Box2Seq_ItemMenuClose,
    Box2Seq_ItemToBag, Box2Seq_ItemHideIcon, Box2Seq_ItemBack,
    Box2Seq_ItemBackToMenu, Box2Seq_ItemPartyIn, Box2Seq_ItemPartyOutMain,
    Box2Seq_ItemPartyAway, Box2Seq_ItemDropKey, Box2Seq_ItemDropTouch,
    Box2Seq_ItemMailClose, Box2Seq_ItemDropKeyPartyOut, Box2Seq_ItemPartyMain,
    Box2Seq_ItemPartyMenuClose, Box2Seq_ItemPartyDrop, Box2Seq_ItemPartySwap,
    Box2Seq_ItemPartyReturn, Box2Seq_DreamWorldMain, Box2Seq_DreamWorldMenuOpened,
    Box2Seq_DreamWorldPick, Box2Seq_DreamWorldMenuClose, Box2Seq_CallPokeStatus,
    Box2Seq_TakeItem, Box2Seq_CallBoxSearch, Box2Seq_SummaryReturn,
    Box2Seq_NameInputReturn, Box2Seq_BagReturn, Box2Seq_BoxSearchReturn,
    Box2Seq_GivenItemMsg, Box2Seq_MarkingOpen, Box2Seq_MarkingMain,
    Box2Seq_MarkingClose, Box2Seq_MarkingCursorMoveEnd, Box2Seq_ReleaseCheck,
    Box2Seq_ReleaseCheckMoves, Box2Seq_ReleaseDo, Box2Seq_ReleaseMoveLocked,
    Box2Seq_BoxMenuOpen, Box2Seq_BoxMenuMain, Box2Seq_BoxMenuClose,
    Box2Seq_BoxJumpOpen, Box2Seq_BoxJumpMain, Box2Seq_BoxJumpClose,
    Box2Seq_BoxJumpReturn, Box2Seq_WallpaperMenuOpen, Box2Seq_WallpaperMenuMain,
    Box2Seq_WallpaperChange, Box2Seq_CallNameInput, Box2Seq_AskContinue,
};

// clang-format on

// clang-format off
static const Box2YesNoFuncs sYesNoFuncs[] = {
    { Box2Seq_YesTakeItem, Box2Seq_NoBackToMenu },
    { Box2Seq_YesRelease, Box2Seq_NoRelease },
    { Box2Seq_YesItemToBag, Box2Seq_NoBackToMenu },
    { Box2Seq_Leave, Box2Seq_ReturnToMain },
    { Box2Seq_ReturnToMain, Box2Seq_Leave },
    { Box2Seq_YesDreamWorldPick, Box2Seq_NoDreamWorldPick },
};

// clang-format on

BOOL Box2Seq_Main(Box2SysWork *syswk, u32 *seq) {
    if (syswk->app == NULL || syswk->app->printQueue == NULL || PrintQueue_IsEmpty(syswk->app->printQueue) != FALSE) {
        if (syswk->app != NULL) {
            Box2Bmp_FlushTrayName(syswk);
        }
        *seq = sMainSeq[*seq](syswk);
    }
    if (*seq == BOX2SEQ_END) {
        return FALSE;
    }
    if (syswk->app != NULL) {
        Box2Bmp_UpdatePrint(syswk->app);
        Box2Obj_UpdateAnim(syswk->app);
        Box2Obj_UpdateRangeDots(syswk);
    }
    return TRUE;
}

static int Box2Seq_Init(Box2SysWork *syswk) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(0);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BOX2_APP, 0x80000);
    syswk->app = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2AppWork), TRUE, "box2_seq.c", 737);
    syswk->app->pokeIconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_BOX2_APP);
    GCTX_HIDGetRepeat(&syswk->app->keyRepeatWait, &syswk->app->keyRepeatStart);
    setKeypressFramecounts(6, 6);
    Box2Main_InitVramBanks();
    Box2Main_InitBg(syswk);
    Box2Main_InitPaletteFade(syswk);
    Box2Main_LoadBgGraphics(syswk);
    Box2Main_InitMsg(syswk);
    Box2Main_InitDexData(syswk);
    Box2Bmp_CreateWindows(syswk);
    Box2Obj_Init(syswk);
    Box2Obj_SetTrayPokeIcons(syswk);
    Box2Obj_SetPartyPokeIcons(syswk);
    Box2Main_WallPaperSet(syswk, Box2Main_GetWallPaperNumber(syswk, syswk->tray), BOX2_TRAY_SCROLL_NONE);
    Box2Frame_Init(syswk);
    Box2Bmp_PrintTitle(syswk);
    Box2Bmp_PrintInfoLabels(syswk);
    Box2Obj_MakeAllTrayIconChars(syswk);
    Box2Ui_CreateCursor(syswk);
    Box2Main_InitYesNo(syswk);
    Box2Main_SetBlendAlpha(TRUE);
    GFL_NetSetIconScreen(TRUE, HEAPID_BOX2_APP);
    Box2Main_InitVBlank(syswk);
    return syswk->nextSeq;
}

static int Box2Seq_Release(Box2SysWork *syswk) {
    Box2Main_ExitVBlank(syswk);
    Box2Main_ExitYesNo(syswk);
    Box2Ui_DeleteCursor(syswk);
    Box2Frame_Delete(syswk->app);
    Box2Obj_Exit(syswk->app);
    Box2Bmp_FreeWindows(syswk);
    Box2Main_ExitDexData(syswk);
    Box2Main_ExitMsg(syswk);
    Box2Main_ExitPaletteFade(syswk);
    Box2Main_ExitBg(syswk);
    setKeypressFramecounts(syswk->app->keyRepeatWait, syswk->app->keyRepeatStart);
    GFL_ArcToolFree(syswk->app->pokeIconArc);
    GFL_HeapFree(syswk->app);
    GFL_HeapDelete(HEAPID_BOX2_APP);
    syswk->app = NULL;
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return syswk->nextSeq;
}

static int Box2Seq_Wipe(Box2SysWork *syswk) {
    if (GFL_WipeIsFinished() == TRUE) {
        return syswk->app->wipeSeq;
    }
    return BOX2SEQ_WIPE;
}

static int Box2Seq_PaletteFade(Box2SysWork *syswk) {
    if (PaletteFade_GetActiveMask(syswk->app->palFade) == FALSE) {
        return syswk->nextSeq;
    }
    return BOX2SEQ_PALETTE_FADE;
}

static int Box2Seq_Wait(Box2SysWork *syswk) {
    if (syswk->app->wait == 0) {
        return syswk->nextSeq;
    }
    syswk->app->wait--;
    return BOX2SEQ_WAIT;
}

static int Box2Seq_VFunc(Box2SysWork *syswk) {
    if (syswk->app->vfunk.func != NULL && syswk->app->vfunk.func(syswk) == FALSE) {
        syswk->app->vfunk.func = NULL;
        return syswk->app->vfuncNextSeq;
    }
    return BOX2SEQ_VFUNC;
}

static int Box2Seq_TrgWait(Box2SysWork *syswk) {
    if (GFL_HIDIsTouchPressed() == TRUE) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        GCTX_HIDSetUsingTouch(TRUE);
        return syswk->nextSeq;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        GCTX_HIDSetUsingTouch(FALSE);
        return syswk->nextSeq;
    }
    return BOX2SEQ_TRGWAIT;
}

static int Box2Seq_YesNo(Box2SysWork *syswk) {
    u32 sel;

    AppTaskMenu_Update(syswk->app->yesNoMenu);
    if (AppTaskMenu_IsFlashFinished(syswk->app->yesNoMenu) == TRUE) {
        sel = AppTaskMenu_GetCursorPos(syswk->app->yesNoMenu);
        AppTaskMenu_Free(syswk->app->yesNoMenu);
        setKeypressFramecounts(6, 6);
        if (sel == 0) {
            return sYesNoFuncs[syswk->app->ynID].yes(syswk);
        }
        return sYesNoFuncs[syswk->app->ynID].no(syswk);
    }
    return BOX2SEQ_YESNO;
}

static int Box2Seq_ButtonAnm(Box2SysWork *syswk) {
    if (Box2Main_ButtonAnmMain(syswk) == FALSE) {
        return syswk->nextSeq;
    }
    return BOX2SEQ_BUTTON_ANM;
}

static int Box2Seq_SubProcCall(Box2SysWork *syswk) {
    sSubProcs[syswk->subProcType].call(syswk);
    return BOX2SEQ_SUBPROC_MAIN;
}

static int Box2Seq_SubProcMain(Box2SysWork *syswk) {
    if (syswk->procMgrResult != TRUE) {
        sSubProcs[syswk->subProcType].exit(syswk);
        syswk->nextSeq = sSubProcs[syswk->subProcType].nextSeq;
        return BOX2SEQ_INIT;
    }
    return BOX2SEQ_SUBPROC_MAIN;
}

static int Box2Seq_Start(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_PC_LOGIN);
    switch (syswk->param->mode) {
    case BOX2_MODE_DEPOSIT:
        Box2Seq_PutPartyLeft(syswk);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        syswk->app->msgNextSeq = BOX2SEQ_DEPOSIT_PARTY_MAIN;
        break;
    case BOX2_MODE_WITHDRAW:
        Box2Main_PokeInfoPut(syswk, 0);
        syswk->app->msgNextSeq = BOX2SEQ_WITHDRAW_MAIN;
        break;
    case BOX2_MODE_MOVE_POKEMON:
        Box2Main_PokeInfoPut(syswk, 0);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->app->msgNextSeq = BOX2SEQ_ARRANGE_MAIN;
        break;
    case BOX2_MODE_MOVE_ITEMS:
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        Box2Main_PokeInfoPut(syswk, 0);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->app->msgNextSeq = BOX2SEQ_ITEM_MAIN;
        break;
    case BOX2_MODE_BATTLE_BOX:
        if (PokeParty_GetPkmCount(syswk->param->party) == 0) {
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Main_PokeInfoPut(syswk, 0);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, 0);
            syswk->app->msgNextSeq = BOX2SEQ_ARRANGE_BOX_MAIN;
        } else {
            Box2Seq_PutPartyLeft(syswk);
            Box2Obj_ShowScrollArrows(syswk->app, FALSE);
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
            Box2Frame_ShowBoxListButton(syswk->app);
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_PARTY, 0);
            syswk->app->msgNextSeq = BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
        }
        break;
    case BOX2_MODE_DREAM_WORLD:
        Box2Main_PokeInfoPut(syswk, 0);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        syswk->app->msgNextSeq = BOX2SEQ_DREAM_WORLD_MAIN;
        break;
    }
    return Box2Seq_StartWait(syswk);
}

static int Box2Seq_StartWait(Box2SysWork *syswk) {
    if (PrintQueue_IsEmpty(syswk->app->printQueue) == FALSE) {
        return BOX2SEQ_START_WAIT;
    }
    return Box2Seq_WipeOut(syswk, syswk->app->msgNextSeq);
}

static int Box2Seq_PokeMoveEnd(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
    if (syswk->app->menuMsgPending == 1) {
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            if (syswk->app->getItem == 0) {
                Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_CAN_HOLD, BOX2_WIN_MESSAGE);
            } else {
                Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_SELECTED, BOX2_WIN_MESSAGE);
            }
        } else {
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        }
        syswk->app->menuMsgPending = 0;
    }
    return syswk->nextSeq;
}

static int Box2Seq_OpenMenu(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_OPEN_MENU_SHOW_MENU:
        switch (syswk->param->mode) {
        case BOX2_MODE_WITHDRAW:
            Box2Bmp_OpenMenu(syswk, sWithdrawMenu, 5);
            break;
        case BOX2_MODE_DEPOSIT:
            Box2Bmp_OpenMenu(syswk, sDepositMenu, 5);
            break;
        case BOX2_MODE_MOVE_POKEMON:
            Box2Bmp_OpenMenu(syswk, sArrangeMenu, 6);
            break;
        case BOX2_MODE_MOVE_ITEMS:
            Box2Seq_OpenItemMenu(syswk, syswk->app->getItem);
            break;
        case BOX2_MODE_BATTLE_BOX:
            Box2Bmp_OpenMenu(syswk, sBattleBoxMenu, 4);
            break;
        case BOX2_MODE_DREAM_WORLD:
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE_TALL);
            Box2Bmp_OpenMenu(syswk, sDreamWorldMenu, 2);
            break;
        }
        if (syswk->app->iconsDimmed == 1) {
            syswk->app->iconsDimmed = 0;
            if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
                if (syswk->pos >= BOX2_PARTY_POS) {
                    Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD | BOX2_ICON_CHECK_ELIGIBILITY, FALSE);
                } else {
                    Box2Obj_ShowScrollArrows(syswk->app, TRUE);
                    Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, FALSE);
                }
            } else if (syswk->pos >= BOX2_PARTY_POS) {
                Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, FALSE);
            } else {
                Box2Obj_ShowScrollArrows(syswk->app, TRUE);
                Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
            }
        }
        Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_OPEN_MENU);
    case BOX2_OPEN_MENU_RESUME:
        syswk->app->subSeq = BOX2_OPEN_MENU_SHOW_MENU;
        Box2Obj_ShowCursor(syswk, TRUE);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        switch (syswk->param->mode) {
        case BOX2_MODE_WITHDRAW:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
            return BOX2SEQ_WITHDRAW_MAIN;
        case BOX2_MODE_DEPOSIT:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
            return BOX2SEQ_DEPOSIT_PARTY_MAIN;
        case BOX2_MODE_MOVE_POKEMON:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
            if (syswk->pos < BOX2_PARTY_POS) {
                return BOX2SEQ_ARRANGE_MAIN;
            }
            return BOX2SEQ_ARRANGE_PARTY_MAIN;
        case BOX2_MODE_MOVE_ITEMS:
            if (syswk->app->getItem == 0) {
                Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_CAN_HOLD, BOX2_WIN_MESSAGE);
            } else {
                Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_SELECTED, BOX2_WIN_MESSAGE);
            }
            if (syswk->pos < BOX2_PARTY_POS) {
                return BOX2SEQ_ITEM_MAIN;
            }
            return BOX2SEQ_ITEM_PARTY_MAIN;
        case BOX2_MODE_BATTLE_BOX:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
            if (syswk->pos < BOX2_PARTY_POS) {
                return BOX2SEQ_ARRANGE_BOX_MAIN;
            }
            return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
        case BOX2_MODE_DREAM_WORLD:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
            return BOX2SEQ_DREAM_WORLD_MAIN;
        }
    }
    return BOX2SEQ_OPEN_MENU;
}

// Scrolls the box list by touch
static int Box2Seq_BoxListScrollTouch(Box2SysWork *syswk) {
    u32 x, y;
    Box2BoxListDrag *work;
    u32 oldY;
    int diff;

    if (Box2Ui_GetBoxListTouch(&x, &y) == FALSE) {
        int pos;

        GFL_HeapFree(syswk->app->subWork);
        pos = Box2Ui_GetBoxListRowAt(syswk->app->tpx, syswk->app->tpy);
        if (pos >= 0) {
            Box2Obj_ShowBoxNameFrame(syswk, pos, TRUE);
        }
        return syswk->nextSeq;
    }
    work = syswk->app->subWork;
    oldY = syswk->app->tpy;
    syswk->app->tpy = y;
    diff = oldY - y;
    if (MATH_ABS(diff) >= 3) {
        work->cnt = MATH_ABS(diff) / 8;
        if (y < oldY) {
            work->dir = 1;
            Box2Main_StartBoxListScroll(syswk, 1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == BOX2SEQ_ARRANGE_MOVE_MAIN) {
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollNext, BOX2SEQ_BOX_LIST_SCROLL_TOUCH);
            }
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxJumpScrollNext, BOX2SEQ_BOX_LIST_SCROLL_TOUCH);
        }
        if (y > oldY) {
            work->dir = -1;
            Box2Main_StartBoxListScroll(syswk, -1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == BOX2SEQ_ARRANGE_MOVE_MAIN) {
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollPrevious, BOX2SEQ_BOX_LIST_SCROLL_TOUCH);
            }
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxJumpScrollPrevious, BOX2SEQ_BOX_LIST_SCROLL_TOUCH);
        }
    }
    if (work->cnt != 0) {
        work->cnt--;
        if (work->dir == 1) {
            Box2Main_StartBoxListScroll(syswk, 1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == BOX2SEQ_ARRANGE_MOVE_MAIN) {
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollNext, BOX2SEQ_BOX_LIST_SCROLL_TOUCH);
            }
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxJumpScrollNext, BOX2SEQ_BOX_LIST_SCROLL_TOUCH);
        }
        if (work->dir == -1) {
            Box2Main_StartBoxListScroll(syswk, -1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == BOX2SEQ_ARRANGE_MOVE_MAIN) {
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollPrevious, BOX2SEQ_BOX_LIST_SCROLL_TOUCH);
            }
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxJumpScrollPrevious, BOX2SEQ_BOX_LIST_SCROLL_TOUCH);
        }
    }
    return BOX2SEQ_BOX_LIST_SCROLL_TOUCH;
}

static int Box2Seq_LeaveState(Box2SysWork *syswk) {
    return Box2Seq_Leave(syswk);
}

// The arrangement's main state: waits for a touch or for the cursor to pick something
static int Box2Seq_ArrangeMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->fromParty = 0;
        return Box2Seq_NextMoveMode(syswk, BOX2SEQ_ARRANGE_MOVE_START);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    }

    res = Box2Ui_GetTrayPokeTrigger();
    if (res != CURSORMOVE_NONE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sArrangeMenu, 6);
            }
            Box2Main_PokeInfoPut(syswk, res);
            return Box2Seq_ArrangePickUpTouched(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowPartyButton(syswk->app);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_ARRANGE_MAIN);
        }
        return BOX2SEQ_ARRANGE_MAIN;
    }

    res = Box2Ui_UpdateCursor3(syswk);
    switch (res) {
    case BOX2_ARRANGE_TRAY_INPUT_TITLE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
    case BOX2_ARRANGE_TRAY_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ARRANGE_TRAY_INPUT_TITLE);
        syswk->app->oldCurPos = BOX2_ARRANGE_TRAY_INPUT_TITLE;
        Box2Ui_PlaceCursorActor(syswk, BOX2_ARRANGE_TRAY_INPUT_TITLE);
        return Box2Seq_ScrollTrayLeft(syswk, 1, BOX2SEQ_ARRANGE_MAIN);
    case BOX2_ARRANGE_TRAY_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ARRANGE_TRAY_INPUT_TITLE);
        syswk->app->oldCurPos = BOX2_ARRANGE_TRAY_INPUT_TITLE;
        Box2Ui_PlaceCursorActor(syswk, BOX2_ARRANGE_TRAY_INPUT_TITLE);
        return Box2Seq_ScrollTrayRight(syswk, 1, BOX2SEQ_ARRANGE_MAIN);
    case BOX2_ARRANGE_TRAY_INPUT_PARTY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_PARTY_BUTTON, Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_PARTY_OUT));
    case BOX2_ARRANGE_TRAY_INPUT_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_CLOSE_BUTTON, 8, BOX2SEQ_LEAVE_STATE);
    case BOX2_ARRANGE_TRAY_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case BOX2_ARRANGE_TRAY_INPUT_MOVE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->fromParty = 0;
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIRST, Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_MOVE_START));
    case BOX2_ARRANGE_TRAY_INPUT_SUMMARY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->subProcReturn = BOX2_SUBRETURN_MENU_TRAY;
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 37;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_SECOND, BOX2SEQ_CALL_POKE_STATUS);
    case BOX2_ARRANGE_TRAY_INPUT_HELD_ITEMS:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->subProcReturn = BOX2_SUBRETURN_MENU_TRAY;
        syswk->curRcvPos = 38;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_THIRD, Box2Seq_Goto(syswk, BOX2SEQ_TAKE_ITEM));
    case BOX2_ARRANGE_TRAY_INPUT_MARKINGS:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, Box2Seq_Goto(syswk, BOX2SEQ_MARKING_OPEN));
    case BOX2_ARRANGE_TRAY_INPUT_RELEASE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_RELEASE_CHECK));
    case BOX2_ARRANGE_TRAY_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ARRANGE_MENU_CLOSE);
    }
    case BOX2_ARRANGE_TRAY_INPUT_MENU_MODE:
        break;
    case BOX2_ARRANGE_TRAY_INPUT_SINGLE_MODE:
        syswk->fromParty = 0;
        syswk->moveMode = BOX2_MOVE_MODE_SINGLE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_SetMoveMode(syswk, BOX2SEQ_ARRANGE_MOVE_START);
    case BOX2_ARRANGE_TRAY_INPUT_RANGE_MODE:
        syswk->fromParty = 0;
        syswk->moveMode = BOX2_MOVE_MODE_RANGE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_SetMoveMode(syswk, BOX2SEQ_RANGE_START);
    case BOX2_ARRANGE_TRAY_INPUT_SEARCH:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_ARRANGE_TRAY_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 1, BOX2SEQ_ARRANGE_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_ARRANGE_TRAY_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 1, BOX2SEQ_ARRANGE_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != BOX2_ARRANGE_TRAY_INPUT_MOVE && pos != BOX2_ARRANGE_TRAY_INPUT_SUMMARY &&
                   pos != BOX2_ARRANGE_TRAY_INPUT_HELD_ITEMS && pos != BOX2_ARRANGE_TRAY_INPUT_MARKINGS &&
                   pos != BOX2_ARRANGE_TRAY_INPUT_RELEASE && pos != BOX2_ARRANGE_TRAY_INPUT_CANCEL) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_MAIN));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ARRANGE_MENU_CLOSE);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sArrangeMenu, 6);
            Box2Main_PokeInfoPut(syswk, res);
            Box2Ui_StartCursorMove(syswk->app, BOX2_BOXLIST_POS, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
            syswk->app->oldCurPos = BOX2_BOXLIST_POS;
            return Box2Seq_OpenPokeMenu(syswk, res, BOX2SEQ_ARRANGE_MAIN);
        }
        break;
    }
    return BOX2SEQ_ARRANGE_MAIN;
}

static int Box2Seq_ArrangeMoveToBoxList(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
    syswk->app->oldCurPos = BOX2_BOXLIST_POS;
    Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
    Box2Obj_ShowCursor(syswk, TRUE);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
    return BOX2SEQ_ARRANGE_MAIN;
}

static int Box2Seq_ArrangeMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowPartyButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_MAIN));
}

static int Box2Seq_ArrangeMoveStart(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_START_BOX_MOVE_CLOSE_MENU:
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE && syswk->fromMenu == 0) {
            syswk->getTray = BOX2_GET_NONE;
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (syswk->pos >= BOX2_PARTY_POS) {
                syswk->pos = 0;
                Box2Main_PokeInfoPut(syswk, syswk->pos);
            }
        } else {
            syswk->getTray = syswk->tray;
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_ARRANGE_MOVE_START);
    case BOX2_START_BOX_MOVE_OPEN_BOX_LIST:
        syswk->app->subSeq++;
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return Box2Seq_MovePartyFrame(syswk, BOX2SEQ_ARRANGE_MOVE_START);
        }
    case BOX2_START_BOX_MOVE_READY:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = BOX2_START_BOX_MOVE_CLOSE_MENU;
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE) {
            if (syswk->fromMenu == 0) {
                if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_DISABLED,
                                       BOX2_BUTTON_ENABLED);
                } else {
                    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED,
                                       BOX2_BUTTON_ENABLED);
                }
                Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_SINGLE_MODE, TRUE);
                return BOX2SEQ_ARRANGE_MOVE_MAIN;
            }
            Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_SINGLE_MODE, TRUE);
        } else {
            Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        }
        return Box2Seq_ArrangePickUpKey(syswk, syswk->pos);
    }
    return BOX2SEQ_ARRANGE_MOVE_START;
}

// Sets the held Pokémon's position and tray
static inline void Box2Seq_SetGetPos(Box2SysWork *syswk, u8 pos, u8 tray) {
    syswk->pos = pos;
    syswk->getTray = tray;
}

// Moving Pokémon: waits for a touch or for the cursor to pick one
static int Box2Seq_ArrangeMoveMain(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if (syswk->moveDone == 1) {
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        syswk->moveDone = 0;
        return Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_START);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != BOX2_MOVE_MODE_MENU &&
        syswk->pickState == BOX2_PICK_NONE) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = BOX2_MOVE_MODE_RANGE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_START);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        } else {
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY_HELD;
            syswk->subProcCursorPos = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    }
    if (syswk->pickState == BOX2_PICK_NONE) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res);
                CursorMove_SetPos(syswk->app->cursorMove, res);
                return Box2Seq_MovePickUpTouched(syswk, res, BOX2SEQ_ARRANGE_MOVE_MAIN);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res);
            Box2Ui_SetButtonStates(syswk, res);
            syswk->app->oldCurPos = res;
            Box2Ui_PlaceCursorActor(syswk, res);
            Box2Main_PokeInfoOff(syswk);
            return BOX2SEQ_ARRANGE_MOVE_MAIN;
        }
    }
    if (Box2Ui_GetBoxListScrollTouch(&x, &y) == TRUE) {
        syswk->app->tpx = x;
        syswk->app->tpy = y;
        syswk->nextSeq = BOX2SEQ_ARRANGE_MOVE_MAIN;
        AllocBoxListDragWork(syswk);
        return BOX2SEQ_BOX_LIST_SCROLL_TOUCH;
    }

    res = Box2Ui_UpdateCursor4(syswk);
    switch (res) {
    case BOX2_MOVE_BOXLIST_INPUT_TITLE:
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
    case BOX2_MOVE_BOXLIST_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_MOVE_BOXLIST_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_MOVE_BOXLIST_INPUT_TITLE);
        Box2Ui_SetButtonStates(syswk, 30);
        syswk->app->oldCurPos = BOX2_MOVE_BOXLIST_INPUT_TITLE;
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_ARRANGE_MOVE_MAIN);
    case BOX2_MOVE_BOXLIST_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_MOVE_BOXLIST_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_MOVE_BOXLIST_INPUT_TITLE);
        Box2Ui_SetButtonStates(syswk, 30);
        syswk->app->oldCurPos = BOX2_MOVE_BOXLIST_INPUT_TITLE;
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_ARRANGE_MOVE_MAIN);
    case BOX2_MOVE_BOXLIST_INPUT_PARTY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeInfoOff(syswk);
        }
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_PARTY_BUTTON,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_OUT));
    case BOX2_MOVE_BOXLIST_INPUT_SUMMARY:
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos >= BOX2_PARTY_POS ||
                Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                break;
            }
            syswk->pos = pos;
            syswk->getTray = syswk->tray;
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        } else {
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY_HELD;
            syswk->subProcCursorPos = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SUMMARY_BUTTON, 1, BOX2SEQ_CALL_POKE_STATUS);
    case BOX2_MOVE_BOXLIST_INPUT_BACK:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_START));
    case BOX2_MOVE_BOXLIST_INPUT_MENU_MODE:
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU || syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_START);
    case BOX2_MOVE_BOXLIST_INPUT_SINGLE_MODE:
        break;
    case BOX2_MOVE_BOXLIST_INPUT_RANGE_MODE:
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU || syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_RANGE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_START);
    case BOX2_MOVE_BOXLIST_INPUT_SEARCH:
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        } else {
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY_HELD;
            syswk->subProcCursorPos = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    case CURSORMOVE_CANCEL:
        if (syswk->pickState == BOX2_PICK_KEY) {
            return Box2Seq_MoveDropKey(syswk, BOX2_GET_NONE, BOX2SEQ_ARRANGE_MOVE_MAIN);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_START));
    case CURSORMOVE_SCROLL_U:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, -1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollPrevious, BOX2SEQ_ARRANGE_MOVE_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_D:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_LAST) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, 1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollNext, BOX2SEQ_ARRANGE_MOVE_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_MOVE_BOXLIST_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_ARRANGE_MOVE_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_MOVE_BOXLIST_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_ARRANGE_MOVE_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_SELECT1);
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->pickState == BOX2_PICK_NONE) {
            if (pos < BOX2_PARTY_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        }
        if (pos >= BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST && pos <= BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_LAST) {
            Box2Obj_ShowBoxNameFrame(syswk, pos - 34, TRUE);
        } else {
            Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_MOVE_MAIN));
    }
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case BOX2_MOVE_BOXLIST_INPUT_LIST_SCROLL_UP:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, -1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollPrevious, BOX2SEQ_ARRANGE_MOVE_MAIN);
    case BOX2_MOVE_BOXLIST_INPUT_LIST_SCROLL_DOWN:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, 1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollNext, BOX2SEQ_ARRANGE_MOVE_MAIN);
    case BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST:
    case BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_SECOND:
    case BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_THIRD:
    case BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_LAST:
        Box2Obj_ShowBoxNameFrame(syswk, res - 34, TRUE);
        if (syswk->pickState == BOX2_PICK_KEY) {
            return Box2Seq_MoveDropKey(syswk, res + 2, BOX2SEQ_ARRANGE_MOVE_MAIN);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoOff(syswk);
        syswk->app->pickedTray = syswk->trayScroll + res - 34;
        if (syswk->app->pickedTray >= syswk->trayMax) {
            syswk->app->pickedTray -= syswk->trayMax;
        }
        if (syswk->app->pickedTray != syswk->tray) {
            return Box2Seq_ScrollToPickedBox(syswk, BOX2SEQ_ARRANGE_MOVE_MAIN);
        }
        break;
    case CURSORMOVE_NONE:
        break;
    default:
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        if (syswk->pickState == BOX2_PICK_KEY) {
            return Box2Seq_MoveDropKey(syswk, res, BOX2SEQ_ARRANGE_MOVE_MAIN);
        }
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Seq_SetGetPos(syswk, res, syswk->tray);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
            return Box2Seq_ArrangePickUpKey(syswk, syswk->pos);
        }
        break;
    }
    return BOX2SEQ_ARRANGE_MOVE_MAIN;
}

static int Box2Seq_ArrangeMovePartyStart(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case BOX2_START_PARTY_MOVE_CLOSE_MENU:
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        if (CursorMove_GetPos(syswk->app->cursorMove) >= BOX2_PARTY_POS) {
            Box2Main_PokeInfoOff(syswk);
        }
        if (syswk->moveMode != BOX2_MOVE_MODE_RANGE) {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN,
                               BOX2_BUTTON_ENABLED);
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            syswk->app->subSeq++;
            return Box2Seq_MovePartyFrameTo(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_START);
        }
        return BOX2SEQ_RANGE_START;
    case BOX2_START_PARTY_MOVE_OPEN_PARTY:
        if (syswk->fromParty == 1) {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            syswk->app->subSeq++;
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_ARRANGE_MOVE_PARTY_START);
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, pos);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        Box2Main_PokeInfoPut(syswk, pos);
        syswk->app->subSeq = BOX2_START_PARTY_MOVE_CLOSE_MENU;
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        return BOX2SEQ_ARRANGE_MAIN;
    case BOX2_START_PARTY_MOVE_READY:
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->app->subSeq = BOX2_START_PARTY_MOVE_CLOSE_MENU;
        return BOX2SEQ_MOVE_PARTY_SWITCH;
    }
    return BOX2SEQ_ARRANGE_MOVE_PARTY_START;
}

// After a move: puts the cursor back, and says why a move failed
static int Box2Seq_ArrangeMoveEnd(Box2SysWork *syswk) {
    u8 pos;
    u8 n;

    Box2Main_PokeDataMove(syswk);
    syswk->pickState = BOX2_PICK_NONE;
    syswk->getTray = BOX2_GET_NONE;
    if (syswk->app->moveErr == BOX2_MOVE_ERR_NONE) {
        pos = CursorMove_GetPos(syswk->app->cursorMove);
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos);
        Box2Obj_ShowCursor(syswk, TRUE);
        pos = syswk->pos;
    }
    syswk->app->oldCurPos = pos;
    FreePokeMoveWork(syswk);
    if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == BOX2SEQ_MOVE_PARTY_MAIN) {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = BOX2SEQ_ARRANGE_MOVE_MAIN;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_PARTY_POS &&
                   Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == BOX2SEQ_ARRANGE_MOVE_MAIN) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos > 32) {
                pos = syswk->pos;
            }
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = BOX2SEQ_MOVE_PARTY_MAIN;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_BOXLIST_POS &&
                   Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (pos < BOX2_BOXLIST_POS &&
               Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
        Box2Main_PokeInfoPut(syswk, pos);
        n = 0;
    } else {
        Box2Main_PokeInfoOff(syswk);
        n = 2;
    }
    if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
        syswk->moveDone = 1;
        n = syswk->summaryButtonState;
    }
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, n, BOX2_BUTTON_ENABLED);
    switch (syswk->app->moveErr) {
    case BOX2_MOVE_ERR_MAIL:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_MAIL, BOX2_WIN_MESSAGE);
        return BOX2SEQ_ARRANGE_MOVE_ERR_CLOSE;
    case BOX2_MOVE_ERR_LAST_BATTLER:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_LAST_BATTLER, BOX2_WIN_MESSAGE);
        return BOX2SEQ_ARRANGE_MOVE_ERR_CLOSE;
    case BOX2_MOVE_ERR_EGG:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_EGG, BOX2_WIN_MESSAGE);
        return BOX2SEQ_ARRANGE_MOVE_ERR_CLOSE;
    case BOX2_MOVE_ERR_BATTLE_BOX_LOCKED:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintBattleBoxLocked(syswk, FALSE);
        return BOX2SEQ_ARRANGE_MOVE_ERR_CLOSE;
    }
    if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON) {
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_ShowPartyButton(syswk->app);
        } else {
            Box2Frame_ShowBoxListButton(syswk->app);
        }
    }
    return syswk->nextSeq;
}

// Closes the message about a move that failed
static int Box2Seq_ArrangeMoveErrClose(Box2SysWork *syswk) {
    if (Box2Seq_TrgWait(syswk) == BOX2SEQ_TRGWAIT) {
        return BOX2SEQ_ARRANGE_MOVE_ERR_CLOSE;
    }
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    Box2Bmp_ClearMsgWindow(syswk->app, syswk->app->moveErr == BOX2_MOVE_ERR_BATTLE_BOX_LOCKED ? 27 : 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON) {
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_ShowPartyButton(syswk->app);
        } else {
            Box2Frame_ShowBoxListButton(syswk->app);
        }
    }
    syswk->app->moveErr = BOX2_MOVE_ERR_NONE;
    return syswk->nextSeq;
}

static int Box2Seq_ArrangeMovePartyOut(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_MOVE_TO_PARTY_CLOSE_BOX_LIST:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrameTo(syswk, BOX2SEQ_ARRANGE_MOVE_PARTY_OUT);
    case BOX2_MOVE_TO_PARTY_OPEN_PARTY:
        Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        if (syswk->pickState == BOX2_PICK_KEY) {
            Box2Obj_PutPartyIconsLeftBelow(syswk, TRUE);
        } else {
            Box2Obj_PutPartyIconsLeftBelow(syswk, FALSE);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_ARRANGE_MOVE_PARTY_OUT);
    case BOX2_MOVE_TO_PARTY_READY:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, BOX2_PARTY_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_PARTY_POS);
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        }
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->subSeq = BOX2_MOVE_TO_PARTY_CLOSE_BOX_LIST;
        return BOX2SEQ_MOVE_PARTY_MAIN;
    }
    return BOX2SEQ_ARRANGE_MOVE_PARTY_OUT;
}

// Starts picking a range
static int Box2Seq_RangeStart(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_START_BOX_MOVE_CLOSE_MENU:
        syswk->fromMenu = 0;
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->getTray = BOX2_GET_NONE;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->pos >= BOX2_PARTY_POS) {
            syswk->pos = 0;
            Box2Main_PokeInfoPut(syswk, syswk->pos);
        }
        Box2Obj_ShowCursor(syswk, FALSE);
        syswk->app->subSeq++;
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_RANGE_START);
        }
    case BOX2_START_BOX_MOVE_OPEN_BOX_LIST:
        syswk->app->subSeq++;
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return Box2Seq_MovePartyFrame(syswk, BOX2SEQ_RANGE_START);
        }
    case BOX2_START_BOX_MOVE_READY:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = BOX2_START_BOX_MOVE_CLOSE_MENU;
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_DISABLED,
                               BOX2_BUTTON_ENABLED);
        } else {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED,
                               BOX2_BUTTON_ENABLED);
        }
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_RANGE_MODE, TRUE);
        return BOX2SEQ_RANGE_MAIN;
    }
    return BOX2SEQ_RANGE_START;
}

// Picking a range: waits for a touch or for the cursor to pick or drop one
static int Box2Seq_RangeMain(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != BOX2_MOVE_MODE_MENU &&
        syswk->pickState == BOX2_PICK_NONE) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_OUT);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && syswk->pickState == BOX2_PICK_NONE) {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        Box2Seq_SetGetPos(syswk, pos, syswk->tray);
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    }

    if (syswk->pickState == BOX2_PICK_NONE) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            Box2Main_PokeInfoPut(syswk, res);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
            Box2Obj_SetRangeAnm(syswk, 1);
            Box2Obj_SetRangeRect(syswk, res, res, 0);
            syswk->app->rangeSelect.startPos = res;
            syswk->app->rangeSelect.endPos = res;
            syswk->app->rangeByKey = FALSE;
            syswk->pickState = BOX2_PICK_TOUCH_TRAY;
            syswk->getTray = syswk->tray;
            Box2Ui_SetButtonStates(syswk, res);
            return BOX2SEQ_RANGE_MAIN;
        }
    } else if (syswk->pickState == BOX2_PICK_TOUCH_TRAY) {
        if (GFL_HIDIsTouchHeld() == TRUE) {
            res = Box2Ui_GetTrayPokeHeld();
            if (res != CURSORMOVE_NONE) {
                Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, res, 0);
                syswk->app->rangeSelect.endPos = res;
            }
        } else {
            u32 width, height;

            SortRangeEnds(syswk);
            syswk->pos = Box2Obj_GetTrayRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, FALSE);
                Box2Bmp_PutRangeCount(syswk);
                syswk->pickState = BOX2_PICK_RANGE_HELD;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                CancelRange(syswk);
            }
        }
        return BOX2SEQ_RANGE_MAIN;
    } else if (syswk->pickState == BOX2_PICK_RANGE_HELD && syswk->app->rangeByKey == FALSE) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            // The touch position's y doubles as the row counter
            for (y = 0; y < syswk->app->rangeHeight; y++) {
                if (res >= syswk->pos + y * 6 && res < syswk->app->rangeWidth + (syswk->pos + y * 6)) {
                    GFL_SndSEPlay(SEQ_SE_SYS_39);
                    GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                    Box2Main_PokeInfoPut(syswk, syswk->pos);
                    syswk->app->rangeByKey = TRUE;
                    return Box2Seq_RangePickUpTouched(syswk, BOX2SEQ_RANGE_MAIN);
                }
            }
        }
    }

    if (Box2Ui_GetBoxListScrollTouch(&x, &y) == TRUE) {
        syswk->app->tpx = x;
        syswk->app->tpy = y;
        syswk->nextSeq = BOX2SEQ_RANGE_MAIN;
        AllocBoxListDragWork(syswk);
        return BOX2SEQ_BOX_LIST_SCROLL_TOUCH;
    }

    res = Box2Ui_UpdateCursor4(syswk);
    if (res != CURSORMOVE_NONE) {
        syswk->app->rangeByKey = TRUE;
    }
    switch (res) {
    case BOX2_MOVE_BOXLIST_INPUT_TITLE:
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        if (syswk->pickState == BOX2_PICK_NONE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_PokeInfoOff(syswk);
            return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
        }
        if (syswk->pickState == BOX2_PICK_KEY) {
            CancelRange(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        break;
    case BOX2_MOVE_BOXLIST_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_MOVE_BOXLIST_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_MOVE_BOXLIST_INPUT_TITLE);
        Box2Ui_SetButtonStates(syswk, 30);
        syswk->app->oldCurPos = BOX2_MOVE_BOXLIST_INPUT_TITLE;
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->pickState == BOX2_PICK_KEY) {
            Box2Main_PokeSelectOff(syswk);
            CancelRange(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_RANGE_MAIN);
    case BOX2_MOVE_BOXLIST_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_MOVE_BOXLIST_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_MOVE_BOXLIST_INPUT_TITLE);
        Box2Ui_SetButtonStates(syswk, 30);
        syswk->app->oldCurPos = BOX2_MOVE_BOXLIST_INPUT_TITLE;
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->pickState == BOX2_PICK_KEY) {
            Box2Main_PokeSelectOff(syswk);
            CancelRange(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_RANGE_MAIN);
    case BOX2_MOVE_BOXLIST_INPUT_PARTY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeInfoOff(syswk);
        } else if (syswk->pickState == BOX2_PICK_KEY) {
            CancelRange(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_PARTY_BUTTON, Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_AWAY));
    case BOX2_MOVE_BOXLIST_INPUT_SUMMARY: {
        u8 pos;

        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS ||
            Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            break;
        }
        syswk->pos = pos;
        syswk->getTray = syswk->tray;
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SUMMARY_BUTTON, 1, BOX2SEQ_CALL_POKE_STATUS);
    }
    case BOX2_MOVE_BOXLIST_INPUT_BACK:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_OUT));
    case BOX2_MOVE_BOXLIST_INPUT_MENU_MODE:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_OUT);
    case BOX2_MOVE_BOXLIST_INPUT_SINGLE_MODE:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_SINGLE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_OUT);
    case BOX2_MOVE_BOXLIST_INPUT_RANGE_MODE:
        break;
    case BOX2_MOVE_BOXLIST_INPUT_SEARCH:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        Box2Seq_SetGetPos(syswk, pos, syswk->tray);
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    case CURSORMOVE_CANCEL:
        if (syswk->pickState == BOX2_PICK_KEY) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_PARTY_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
            CancelRange(syswk);
            break;
        }
        if (syswk->pickState != BOX2_PICK_NONE) {
            Box2Obj_SetRangeAnm(syswk, 0);
            return Box2Seq_RangeDropKey(syswk, BOX2_GET_NONE, BOX2SEQ_RANGE_MAIN);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_OUT));
    case CURSORMOVE_SCROLL_U:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST) {
            if (syswk->pickState == BOX2_PICK_KEY) {
                Box2Main_PokeSelectOff(syswk);
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, -1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollPrevious, BOX2SEQ_RANGE_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_D:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_LAST) {
            if (syswk->pickState == BOX2_PICK_KEY) {
                Box2Main_PokeSelectOff(syswk);
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, 1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollNext, BOX2SEQ_RANGE_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_MOVE_BOXLIST_INPUT_TITLE) {
            if (syswk->pickState == BOX2_PICK_KEY) {
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_RANGE_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_MOVE_BOXLIST_INPUT_TITLE) {
            if (syswk->pickState == BOX2_PICK_KEY) {
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_RANGE_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_SELECT1);
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->pickState == BOX2_PICK_NONE) {
            if (pos < BOX2_PARTY_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        } else if (syswk->pickState == BOX2_PICK_KEY && pos < BOX2_PARTY_POS) {
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, pos, 0);
        }
        if (pos >= BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST && pos <= BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_LAST) {
            Box2Obj_ShowBoxNameFrame(syswk, pos - 34, TRUE);
        } else {
            Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_RANGE_MAIN));
    }
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case BOX2_MOVE_BOXLIST_INPUT_LIST_SCROLL_UP:
        if (syswk->pickState == BOX2_PICK_KEY) {
            CancelRange(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, -1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollPrevious, BOX2SEQ_RANGE_MAIN);
    case BOX2_MOVE_BOXLIST_INPUT_LIST_SCROLL_DOWN:
        if (syswk->pickState == BOX2_PICK_KEY) {
            CancelRange(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, 1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollNext, BOX2SEQ_RANGE_MAIN);
    case BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST:
    case BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_SECOND:
    case BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_THIRD:
    case BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_LAST: {
        // The button's slot in the box list
        u32 slot = res - 34;

        Box2Obj_ShowBoxNameFrame(syswk, slot, TRUE);
        if (syswk->pickState == BOX2_PICK_RANGE_HELD) {
            return Box2Seq_RangeDropKey(syswk, slot + 36, BOX2SEQ_RANGE_MAIN);
        }
        if (syswk->pickState == BOX2_PICK_KEY) {
            CancelRange(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoOff(syswk);
        syswk->app->pickedTray = syswk->trayScroll + slot;
        if (syswk->app->pickedTray >= syswk->trayMax) {
            syswk->app->pickedTray -= syswk->trayMax;
        }
        if (syswk->app->pickedTray != syswk->tray) {
            return Box2Seq_ScrollToPickedBox(syswk, BOX2SEQ_RANGE_MAIN);
        }
        break;
    }
    case CURSORMOVE_NONE:
        break;
    default:
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        if (syswk->pickState == BOX2_PICK_KEY) {
            u32 width, height;

            syswk->app->rangeSelect.endPos = res;
            SortRangeEnds(syswk);
            syswk->pos = Box2Obj_GetTrayRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, FALSE);
                Box2Bmp_PutRangeCount(syswk);
                syswk->pickState = BOX2_PICK_RANGE_HELD;
                syswk->app->oldCurPos = syswk->pos;
                return Box2Seq_RangePickUpKey(syswk, syswk->pos, BOX2SEQ_RANGE_MAIN);
            }
            CancelRange(syswk);
        } else if (syswk->pickState == BOX2_PICK_RANGE_HELD) {
            if (Box2Main_RangePutCheck(syswk, syswk->tray, res) != FALSE) {
                Box2Obj_SetRangeAnm(syswk, 0);
                return Box2Seq_RangeDropKey(syswk, res, BOX2SEQ_RANGE_MAIN);
            }
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            syswk->getTray = syswk->tray;
            syswk->pos = res;
            syswk->app->rangeSelect.startPos = res;
            syswk->app->rangeSelect.endPos = res;
            syswk->app->rangeByKey = TRUE;
            Box2Obj_SetRangeRect(syswk, res, res, 0);
            Box2Obj_SetRangeAnm(syswk, 1);
            syswk->pickState = BOX2_PICK_KEY;
            syswk->app->rangeWidth = 1;
            syswk->app->rangeHeight = 1;
            Box2Ui_SetButtonStates(syswk, res);
        }
        break;
    }
    return BOX2SEQ_RANGE_MAIN;
}

// Range mode: brings the party's frame out
static int Box2Seq_RangePartyOut(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case BOX2_START_PARTY_MOVE_CLOSE_MENU:
        Box2Main_ClearRangeFlags(syswk);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        if (CursorMove_GetPos(syswk->app->cursorMove) >= BOX2_PARTY_POS) {
            Box2Main_PokeInfoOff(syswk);
        }
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN,
                               BOX2_BUTTON_ENABLED);
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            syswk->app->subSeq++;
            return Box2Seq_MovePartyFrameTo(syswk, BOX2SEQ_RANGE_PARTY_OUT);
        }
        return BOX2SEQ_ARRANGE_MOVE_START;
    case BOX2_START_PARTY_MOVE_OPEN_PARTY:
        if (syswk->fromParty == 1) {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            syswk->app->subSeq++;
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_RANGE_PARTY_OUT);
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, pos);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        Box2Main_PokeInfoPut(syswk, pos);
        syswk->app->subSeq = BOX2_START_PARTY_MOVE_CLOSE_MENU;
        return BOX2SEQ_ARRANGE_MAIN;
    case BOX2_START_PARTY_MOVE_READY:
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->app->subSeq = BOX2_START_PARTY_MOVE_CLOSE_MENU;
        return BOX2SEQ_MOVE_PARTY_SWITCH;
    }
    return BOX2SEQ_RANGE_PARTY_OUT;
}

// After a move in range mode: puts the cursor back, and says why a move failed
static int Box2Seq_RangeMoveEnd(Box2SysWork *syswk) {
    u8 pos;
    u8 n;

    Box2Main_PokeDataMove(syswk);
    syswk->pickState = BOX2_PICK_NONE;
    syswk->getTray = BOX2_GET_NONE;
    if (syswk->app->moveErr == BOX2_MOVE_ERR_NONE) {
        pos = CursorMove_GetPos(syswk->app->cursorMove);
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos);
        Box2Obj_ShowCursor(syswk, TRUE);
        pos = syswk->pos;
    }
    syswk->app->oldCurPos = pos;
    FreePokeMoveWork(syswk);
    if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == BOX2SEQ_RANGE_PARTY_MAIN) {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = BOX2SEQ_RANGE_MAIN;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_PARTY_POS &&
                   Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == BOX2SEQ_RANGE_MAIN) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos > 32) {
                pos = syswk->pos;
            }
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = BOX2SEQ_RANGE_PARTY_MAIN;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_BOXLIST_POS &&
                   Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (pos < BOX2_BOXLIST_POS &&
               Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
        Box2Main_PokeInfoPut(syswk, pos);
        n = 0;
    } else {
        Box2Main_PokeInfoOff(syswk);
        n = 2;
    }
    if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
        syswk->moveDone = 1;
        n = syswk->summaryButtonState;
    }
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, n, BOX2_BUTTON_ENABLED);
    Box2Obj_SetRangeAnm(syswk, 0);
    switch (syswk->app->moveErr) {
    case BOX2_MOVE_ERR_MAIL:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_MAIL, BOX2_WIN_MESSAGE);
        return BOX2SEQ_RANGE_MOVE_ERR_CLOSE;
    case BOX2_MOVE_ERR_LAST_BATTLER:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_LAST_BATTLER, BOX2_WIN_MESSAGE);
        return BOX2SEQ_RANGE_MOVE_ERR_CLOSE;
    case BOX2_MOVE_ERR_EGG:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_EGG, BOX2_WIN_MESSAGE);
        return BOX2SEQ_RANGE_MOVE_ERR_CLOSE;
    case BOX2_MOVE_ERR_BATTLE_BOX_LOCKED:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintBattleBoxLocked(syswk, FALSE);
        return BOX2SEQ_RANGE_MOVE_ERR_CLOSE;
    }
    if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON) {
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_ShowPartyButton(syswk->app);
        } else {
            Box2Frame_ShowBoxListButton(syswk->app);
        }
    }
    return syswk->nextSeq;
}

// Closes the message about a move in range mode that failed
static int Box2Seq_RangeMoveErrClose(Box2SysWork *syswk) {
    if (Box2Seq_TrgWait(syswk) == BOX2SEQ_TRGWAIT) {
        return BOX2SEQ_RANGE_MOVE_ERR_CLOSE;
    }
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    Box2Bmp_ClearMsgWindow(syswk->app, syswk->app->moveErr == BOX2_MOVE_ERR_BATTLE_BOX_LOCKED ? 27 : 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON) {
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_ShowPartyButton(syswk->app);
        } else {
            Box2Frame_ShowBoxListButton(syswk->app);
        }
    }
    syswk->app->moveErr = BOX2_MOVE_ERR_NONE;
    return syswk->nextSeq;
}

// Range mode: puts the party's frame away
static int Box2Seq_RangePartyAway(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_MOVE_TO_PARTY_CLOSE_BOX_LIST:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrameTo(syswk, BOX2SEQ_RANGE_PARTY_AWAY);
    case BOX2_MOVE_TO_PARTY_OPEN_PARTY:
        Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        if (syswk->pickState == BOX2_PICK_RANGE_HELD) {
            Box2Obj_PutPartyIconsLeftBelow(syswk, TRUE);
            Box2Obj_HidePartyRangeIcons(syswk);
        } else if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Obj_PutPartyIconsLeftBelow(syswk, FALSE);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_RANGE_PARTY_AWAY);
    case BOX2_MOVE_TO_PARTY_READY:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, BOX2_PARTY_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_PARTY_POS);
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        }
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        if (syswk->pickState == BOX2_PICK_RANGE_HELD) {
            Box2Obj_ShowRangeIcons(syswk, FALSE);
        }
        syswk->app->subSeq = BOX2_MOVE_TO_PARTY_CLOSE_BOX_LIST;
        return BOX2SEQ_RANGE_PARTY_MAIN;
    }
    return BOX2SEQ_RANGE_PARTY_AWAY;
}

// Range mode: brings the party's frame in and puts the menu away
static int Box2Seq_RangePartyIn(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_QUICK_PARTY_IN_PREPARE:
        syswk->fromMenu = 0;
        if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            syswk->app->subSeq = BOX2_QUICK_PARTY_IN_READY;
            break;
        }
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            if (syswk->pos >= 6) {
                syswk->pos = BOX2_PARTY_POS;
            } else {
                syswk->pos += BOX2_PARTY_POS;
            }
        } else if (syswk->pos >= BOX2_PARTY_POS) {
            syswk->pos = 0;
        }
        syswk->getTray = BOX2_GET_NONE;
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Obj_ShowPokeOutline(syswk, FALSE);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_RANGE_PARTY_IN);
    case BOX2_QUICK_PARTY_IN_CHECK_AND_SLIDE:
        if (syswk->param->mode == BOX2_MODE_BATTLE_BOX && syswk->param->battleBoxLocked == 1) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintBattleBoxLocked(syswk, FALSE);
            syswk->moveMode = BOX2_MOVE_MODE_MENU;
            syswk->app->subSeq = BOX2_QUICK_PARTY_IN_PREPARE;
            syswk->nextSeq = BOX2SEQ_SUB_PROC_RETURN;
            return BOX2SEQ_TRGWAIT;
        }
        syswk->app->subSeq++;
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlidePartyRight(syswk->app->bgWinFrame);
        } else {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            Box2Obj_PutPartyIconsLeftBelow(syswk, FALSE);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_RANGE_PARTY_IN);
    case BOX2_QUICK_PARTY_IN_READY:
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = BOX2_QUICK_PARTY_IN_PREPARE;
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_DISABLED,
                               BOX2_BUTTON_ENABLED);
        } else {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED,
                               BOX2_BUTTON_ENABLED);
        }
        if (syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
            Box2Frame_HidePartyButton(syswk->app);
            Box2Frame_HideBoxListButton(syswk->app);
            CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_BOXES);
        }
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_RANGE_MODE, TRUE);
        return BOX2SEQ_RANGE_PARTY_MAIN;
    }
    return BOX2SEQ_RANGE_PARTY_IN;
}

// Picking a range with the party out: waits for a touch or for the cursor to pick or drop one
static int Box2Seq_RangePartyMain(Box2SysWork *syswk) {
    u32 i;
    u32 res;

    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->pickState == BOX2_PICK_NONE) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_SWITCH);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_ShowCursor(syswk);
            return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
        }
        return BOX2SEQ_RANGE_PARTY_MAIN;
    }

    if (syswk->pickState == BOX2_PICK_NONE) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            Box2Main_PokeInfoPut(syswk, res);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            Box2Obj_SetRangeAnm(syswk, 1);
            Box2Obj_SetRangeRect(syswk, res, res, 0);
            syswk->app->rangeSelect.startPos = (u8)res;
            syswk->app->rangeSelect.endPos = (u8)res;
            syswk->app->rangeByKey = FALSE;
            syswk->pickState = BOX2_PICK_TOUCH_TRAY;
            syswk->getTray = syswk->tray;
            Box2Ui_SetButtonStates(syswk, res);
            return BOX2SEQ_RANGE_PARTY_MAIN;
        }
        res = Box2Ui_GetPartyPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            Box2Obj_SetRangeAnm(syswk, 1);
            Box2Obj_SetRangeRect(syswk, res + BOX2_PARTY_POS, res + BOX2_PARTY_POS, 0);
            res += BOX2_PARTY_POS;
            syswk->app->rangeSelect.startPos = (u8)res;
            syswk->app->rangeSelect.endPos = (u8)res;
            syswk->app->rangeByKey = FALSE;
            syswk->pickState = BOX2_PICK_TOUCH_PARTY;
            syswk->getTray = BOX2_GET_NONE;
            Box2Ui_SetButtonStates(syswk, res);
            return BOX2SEQ_RANGE_PARTY_MAIN;
        }
    } else if (syswk->pickState == BOX2_PICK_TOUCH_TRAY) {
        if (GFL_HIDIsTouchHeld() == TRUE) {
            res = Box2Ui_GetTrayPokeHeld();
            if (res != CURSORMOVE_NONE) {
                Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, res, 0);
                syswk->app->rangeSelect.endPos = res;
            }
        } else {
            u32 width, height;

            SortRangeEnds(syswk);
            syswk->pos = Box2Obj_GetTrayRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, FALSE);
                Box2Bmp_PutRangeCount(syswk);
                syswk->pickState = BOX2_PICK_RANGE_HELD;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                CancelRange(syswk);
            }
        }
        return BOX2SEQ_RANGE_PARTY_MAIN;
    } else if (syswk->pickState == BOX2_PICK_TOUCH_PARTY) {
        if (GFL_HIDIsTouchHeld() == TRUE) {
            if (Box2Ui_IsPartyRightHeld() == TRUE) {
                res = Box2Ui_GetPartyPokeHeld();
                if (res != CURSORMOVE_NONE) {
                    Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, res + BOX2_PARTY_POS, 0);
                    syswk->app->rangeSelect.endPos = res + BOX2_PARTY_POS;
                }
            }
        } else {
            u32 width, height;

            SortRangeEnds(syswk);
            syswk->pos = Box2Obj_GetPartyRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, FALSE);
                Box2Bmp_PutRangeCount(syswk);
                syswk->pickState = BOX2_PICK_RANGE_HELD;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                CancelRange(syswk);
            }
        }
        return BOX2SEQ_RANGE_PARTY_MAIN;
    } else if (syswk->pickState == BOX2_PICK_RANGE_HELD && syswk->app->rangeByKey == FALSE) {
        if (syswk->pos < BOX2_PARTY_POS) {
            res = Box2Ui_GetTrayPokeTrigger();
            if (res != CURSORMOVE_NONE) {
                for (i = 0; i < syswk->app->rangeHeight; i++) {
                    if (res >= syswk->pos + i * 6 && res < syswk->app->rangeWidth + (syswk->pos + i * 6)) {
                        GFL_SndSEPlay(SEQ_SE_SYS_39);
                        GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                        Box2Main_PokeInfoPut(syswk, syswk->pos);
                        syswk->app->rangeByKey = TRUE;
                        return Box2Seq_RangePickUpTouched(syswk, BOX2SEQ_RANGE_PARTY_MAIN);
                    }
                }
            }
        } else {
            res = Box2Ui_GetPartyPokeTrigger();
            if (res != CURSORMOVE_NONE) {
                // The range's first slot in the party
                u32 top = syswk->pos - BOX2_PARTY_POS;

                for (i = 0; i < syswk->app->rangeHeight; i++) {
                    if (res >= top + i * 2 && res < syswk->app->rangeWidth + (top + i * 2)) {
                        GFL_SndSEPlay(SEQ_SE_SYS_39);
                        Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos,
                                             1);
                        GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                        Box2Main_PokeInfoPut(syswk, syswk->pos);
                        syswk->app->rangeByKey = TRUE;
                        return Box2Seq_RangePickUpTouched(syswk, BOX2SEQ_RANGE_PARTY_MAIN);
                    }
                }
            }
        }
    }

    res = Box2Ui_UpdateCursor6(syswk);
    if (res != CURSORMOVE_NONE) {
        syswk->app->rangeByKey = TRUE;
    }
    switch (res) {
    case BOX2_MOVE_PARTY_INPUT_TITLE:
        if (syswk->pickState == BOX2_PICK_NONE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_PokeInfoOff(syswk);
            return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
        }
        if (syswk->pickState == BOX2_PICK_KEY) {
            CancelRange(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        break;
    case BOX2_MOVE_PARTY_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->pickState == BOX2_PICK_KEY) {
            Box2Main_PokeSelectOff(syswk);
            CancelRange(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_RANGE_PARTY_MAIN);
    case BOX2_MOVE_PARTY_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->pickState == BOX2_PICK_KEY) {
            Box2Main_PokeSelectOff(syswk);
            CancelRange(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_RANGE_PARTY_MAIN);
    case BOX2_MOVE_PARTY_INPUT_BOXES:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeInfoOff(syswk);
        } else if (syswk->pickState == BOX2_PICK_KEY) {
            CancelRange(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_BOX_LIST_BUTTON,
                                       Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_CLOSE));
    case BOX2_MOVE_PARTY_INPUT_SUMMARY: {
        u8 pos;

        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_BOXLIST_POS ||
            Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            break;
        }
        syswk->pos = pos;
        syswk->getTray = syswk->tray;
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SUMMARY_BUTTON, 1, BOX2SEQ_CALL_POKE_STATUS);
    }
    case BOX2_MOVE_PARTY_INPUT_BACK:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_SWITCH));
    case BOX2_MOVE_PARTY_INPUT_MENU_MODE:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_SWITCH);
    case BOX2_MOVE_PARTY_INPUT_SINGLE_MODE:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_SINGLE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_SWITCH);
    case BOX2_MOVE_PARTY_INPUT_RANGE_MODE:
        break;
    case BOX2_MOVE_PARTY_INPUT_SEARCH: {
        u8 pos;

        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        Box2Seq_SetGetPos(syswk, pos, syswk->tray);
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    }
    case CURSORMOVE_CANCEL:
        if (syswk->pickState == BOX2_PICK_KEY) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
            CancelRange(syswk);
            break;
        }
        if (syswk->pickState != BOX2_PICK_NONE) {
            Box2Obj_SetRangeAnm(syswk, 0);
            return Box2Seq_RangeDropKey(syswk, BOX2_GET_NONE, BOX2SEQ_RANGE_PARTY_MAIN);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_RANGE_PARTY_SWITCH));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            if (syswk->pickState == BOX2_PICK_KEY) {
                Box2Main_PokeSelectOff(syswk);
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_RANGE_PARTY_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            if (syswk->pickState == BOX2_PICK_KEY) {
                Box2Main_PokeSelectOff(syswk);
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_RANGE_PARTY_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_SELECT1);
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->pickState == BOX2_PICK_NONE) {
            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        } else if (syswk->pickState == BOX2_PICK_KEY) {
            // The range stays in the tray or in the party it started in
            if (syswk->app->rangeSelect.startPos < BOX2_PARTY_POS) {
                if (pos < BOX2_PARTY_POS) {
                    Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, pos, 0);
                }
            } else if (pos >= BOX2_PARTY_POS && pos < BOX2_BOXLIST_POS) {
                Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, pos, 0);
            }
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_RANGE_PARTY_MAIN));
    }
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
    case CURSORMOVE_NONE:
        break;
    default:
        if (syswk->pickState == BOX2_PICK_KEY) {
            u32 width, height;

            if ((syswk->app->rangeSelect.endPos < BOX2_PARTY_POS && res >= BOX2_PARTY_POS) ||
                (syswk->app->rangeSelect.endPos >= BOX2_PARTY_POS && res < BOX2_PARTY_POS)) {
                CancelRange(syswk);
                Box2Main_PokeInfoPut(syswk, CursorMove_GetPos(syswk->app->cursorMove));
                break;
            }
            syswk->app->rangeSelect.endPos = res;
            SortRangeEnds(syswk);
            if (syswk->app->rangeSelect.startPos < BOX2_PARTY_POS) {
                syswk->pos =
                    Box2Obj_GetTrayRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            } else {
                syswk->pos =
                    Box2Obj_GetPartyRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            }
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, FALSE);
                Box2Bmp_PutRangeCount(syswk);
                syswk->pickState = BOX2_PICK_RANGE_HELD;
                syswk->app->oldCurPos = syswk->pos;
                return Box2Seq_RangePickUpKey(syswk, syswk->pos, BOX2SEQ_RANGE_PARTY_MAIN);
            }
            CancelRange(syswk);
        } else if (syswk->pickState == BOX2_PICK_RANGE_HELD) {
            if (Box2Main_RangePutCheck(syswk, syswk->tray, res) != FALSE) {
                Box2Obj_SetRangeAnm(syswk, 0);
                return Box2Seq_RangeDropKey(syswk, res, BOX2SEQ_RANGE_PARTY_MAIN);
            }
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            if (res < BOX2_PARTY_POS) {
                syswk->getTray = syswk->tray;
            }
            syswk->pos = (u8)res;
            syswk->app->rangeSelect.startPos = (u8)res;
            syswk->app->rangeSelect.endPos = (u8)res;
            syswk->app->rangeByKey = TRUE;
            Box2Obj_SetRangeRect(syswk, res, res, 0);
            Box2Obj_SetRangeAnm(syswk, 1);
            syswk->pickState = BOX2_PICK_KEY;
            syswk->app->rangeWidth = 1;
            syswk->app->rangeHeight = 1;
            Box2Ui_SetButtonStates(syswk, res);
        }
        break;
    }
    return BOX2SEQ_RANGE_PARTY_MAIN;
}

// Range mode: puts the party's frame away or out
static int Box2Seq_RangePartySwitch(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case BOX2_QUICK_PARTY_SWITCH_PREPARE:
        Box2Main_ClearRangeFlags(syswk);
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN,
                               BOX2_BUTTON_ENABLED);
            if (syswk->fromParty == 0) {
                Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
                syswk->app->subSeq = BOX2_QUICK_PARTY_SWITCH_RESUME_TRAY;
            } else {
                Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
                Box2Obj_ShowScrollArrows(syswk->app, FALSE);
                Box2Frame_SlidePartyLeft(syswk->app->bgWinFrame);
                syswk->app->subSeq = BOX2_QUICK_PARTY_SWITCH_RESUME_PARTY;
            }
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_RANGE_PARTY_SWITCH);
        }
        return BOX2SEQ_MOVE_PARTY_IN;
    case BOX2_QUICK_PARTY_SWITCH_RESUME_PARTY:
        syswk->app->subSeq = BOX2_QUICK_PARTY_SWITCH_PREPARE;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos < BOX2_PARTY_POS || pos >= BOX2_BOXLIST_POS) {
            pos = 0;
        } else {
            pos -= BOX2_PARTY_POS;
        }
        Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON) {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_PARTY, pos);
            return BOX2SEQ_ARRANGE_PARTY_MAIN;
        }
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_PARTY, pos);
        return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
    case BOX2_QUICK_PARTY_SWITCH_RESUME_TRAY:
        syswk->app->subSeq = BOX2_QUICK_PARTY_SWITCH_PREPARE;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Main_PokeInfoPut(syswk, pos);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        if (syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, pos);
            return BOX2SEQ_ARRANGE_BOX_MAIN;
        }
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, pos);
        return BOX2SEQ_ARRANGE_MAIN;
    }
    return BOX2SEQ_RANGE_PARTY_SWITCH;
}

// Range mode: closes the party's frame
static int Box2Seq_RangePartyClose(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_QUICK_PARTY_CLOSE_CLOSE_PARTY:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_RANGE_PARTY_CLOSE);
    case BOX2_QUICK_PARTY_CLOSE_OPEN_BOX_LIST:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrame(syswk, BOX2SEQ_RANGE_PARTY_CLOSE);
    case BOX2_QUICK_PARTY_CLOSE_READY:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, BOX2_MOVE_BOXLIST_INPUT_PARTY);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->app->oldCurPos = 39;
        syswk->app->subSeq = BOX2_QUICK_PARTY_CLOSE_CLOSE_PARTY;
        return BOX2SEQ_RANGE_MAIN;
    }
    return BOX2SEQ_RANGE_PARTY_CLOSE;
}

// Brings the party's frame out
static int Box2Seq_ArrangePartyOut(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_MENU_PARTY_OUT_OPEN_PARTY:
        Box2Main_PokeSelectOff(syswk);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Frame_PutPartyLeftBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        Box2Obj_PutPartyIconsLeftBelow(syswk, FALSE);
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_ARRANGE_PARTY_OUT);
    case BOX2_MENU_PARTY_OUT_READY:
        syswk->app->subSeq = BOX2_MENU_PARTY_OUT_OPEN_PARTY;
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        Box2Frame_ShowBoxListButton(syswk->app);
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ITEM_PARTY, 0);
            return BOX2SEQ_ITEM_PARTY_MAIN;
        }
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_PARTY, 0);
        return BOX2SEQ_ARRANGE_PARTY_MAIN;
    }
    return BOX2SEQ_ARRANGE_PARTY_OUT;
}

// The party's main state: waits for a touch or for the cursor to pick something
static int Box2Seq_ArrangePartyMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->fromParty = 1;
        return Box2Seq_NextMoveMode(syswk, BOX2SEQ_MOVE_PARTY_IN);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    }

    res = Box2Ui_GetLeftPartyPokeTrigger();
    if (res != CURSORMOVE_NONE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sArrangeMenu, 6);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return Box2Seq_ArrangePartyPickUpTouched(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_ARRANGE_PARTY_MAIN);
        }
        return BOX2SEQ_ARRANGE_PARTY_MAIN;
    }

    res = Box2Ui_UpdateCursor5(syswk);
    switch (res) {
    case BOX2_ARRANGE_PARTY_INPUT_BOXES:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_PARTY_BUTTON, Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_PARTY_AWAY));
    case BOX2_ARRANGE_PARTY_INPUT_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_CLOSE_BUTTON, 8, BOX2SEQ_LEAVE_STATE);
    case BOX2_ARRANGE_PARTY_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_PARTY_AWAY));
    case BOX2_ARRANGE_PARTY_INPUT_MOVE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->fromParty = 1;
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIRST, Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_IN));
    case BOX2_ARRANGE_PARTY_INPUT_SUMMARY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->getTray = syswk->tray;
        syswk->subProcReturn = BOX2_SUBRETURN_MENU_PARTY;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_SECOND, BOX2SEQ_CALL_POKE_STATUS);
    case BOX2_ARRANGE_PARTY_INPUT_HELD_ITEMS:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->subProcReturn = BOX2_SUBRETURN_MENU_PARTY;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_THIRD, Box2Seq_Goto(syswk, BOX2SEQ_TAKE_ITEM));
    case BOX2_ARRANGE_PARTY_INPUT_MARKINGS:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, Box2Seq_Goto(syswk, BOX2SEQ_MARKING_OPEN));
    case BOX2_ARRANGE_PARTY_INPUT_RELEASE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_RELEASE_CHECK));
    case BOX2_ARRANGE_PARTY_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ARRANGE_PARTY_MENU_CLOSE);
    }
    case BOX2_ARRANGE_PARTY_INPUT_MENU_MODE:
        break;
    case BOX2_ARRANGE_PARTY_INPUT_SINGLE_MODE:
        syswk->fromParty = 1;
        syswk->moveMode = BOX2_MOVE_MODE_SINGLE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_SetMoveMode(syswk, BOX2SEQ_MOVE_PARTY_IN);
    case BOX2_ARRANGE_PARTY_INPUT_RANGE_MODE:
        syswk->fromParty = 1;
        syswk->moveMode = BOX2_MOVE_MODE_RANGE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_SetMoveMode(syswk, BOX2SEQ_RANGE_PARTY_IN);
    case BOX2_ARRANGE_PARTY_INPUT_SEARCH:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < PARTY_SIZE) {
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        } else if (pos == BOX2_ARRANGE_PARTY_INPUT_BOXES) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_PARTY_MAIN));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ARRANGE_PARTY_MENU_CLOSE);
        }
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_PARTY_AWAY));
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sArrangeMenu, 6);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            Box2Ui_StartCursorMove(syswk->app, BOX2_ARRANGE_PARTY_INPUT_MOVE,
                                   CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_ARRANGE_PARTY_INPUT_MOVE);
            syswk->app->oldCurPos = BOX2_ARRANGE_PARTY_INPUT_MOVE;
            return Box2Seq_OpenPartyPokeMenu(syswk, res + BOX2_PARTY_POS, BOX2SEQ_ARRANGE_PARTY_MAIN);
        }
        break;
    }
    return BOX2SEQ_ARRANGE_PARTY_MAIN;
}

// Puts the party's frame away
static int Box2Seq_ArrangePartyAway(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_MENU_PARTY_AWAY_CLOSE_PARTY:
        Box2Main_PokeSelectOff(syswk);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_ARRANGE_PARTY_AWAY);
    case BOX2_MENU_PARTY_AWAY_READY:
        syswk->app->subSeq = BOX2_MENU_PARTY_AWAY_CLOSE_PARTY;
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Frame_ShowPartyButton(syswk->app);
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ITEM_TRAY, BOX2_ITEM_TRAY_INPUT_PARTY);
            return BOX2SEQ_ITEM_MAIN;
        }
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, BOX2_ARRANGE_TRAY_INPUT_PARTY);
        return BOX2SEQ_ARRANGE_MAIN;
    }
    return BOX2SEQ_ARRANGE_PARTY_AWAY;
}

// Closes the party's menu
static int Box2Seq_ArrangePartyMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowBoxListButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame,
                            Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_PARTY_MAIN));
}

// After a move in the party
static int Box2Seq_ArrangePartyMoveEnd(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_SYS_40);
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, BOX2_ARRANGE_PARTY_INPUT_MOVE);
    syswk->app->oldCurPos = BOX2_ARRANGE_PARTY_INPUT_MOVE;
    Box2Ui_PlaceCursorActor(syswk, BOX2_ARRANGE_PARTY_INPUT_MOVE);
    Box2Obj_ShowCursor(syswk, TRUE);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
    return BOX2SEQ_ARRANGE_PARTY_MAIN;
}

// Moving Pokémon with the party out: brings the party's frame in
static int Box2Seq_MovePartyIn(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_QUICK_PARTY_IN_PREPARE:
        if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            syswk->app->subSeq = BOX2_QUICK_PARTY_IN_READY;
            break;
        }
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE && syswk->fromMenu == 0) {
            syswk->getTray = BOX2_GET_NONE;
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
                if (syswk->pos >= 6) {
                    syswk->pos = BOX2_PARTY_POS;
                } else {
                    syswk->pos += BOX2_PARTY_POS;
                }
            } else if (syswk->pos >= BOX2_PARTY_POS) {
                syswk->pos = 0;
            }
            Box2Main_PokeInfoPut(syswk, syswk->pos);
        } else {
            if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
                syswk->getTray = BOX2_GET_NONE;
            } else {
                syswk->getTray = syswk->tray;
            }
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
        }
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_MOVE_PARTY_IN);
    case BOX2_QUICK_PARTY_IN_CHECK_AND_SLIDE:
        if (syswk->param->mode == BOX2_MODE_BATTLE_BOX && syswk->param->battleBoxLocked == 1) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintBattleBoxLocked(syswk, FALSE);
            syswk->moveMode = BOX2_MOVE_MODE_MENU;
            syswk->app->subSeq = BOX2_QUICK_PARTY_IN_PREPARE;
            syswk->nextSeq = BOX2SEQ_SUB_PROC_RETURN;
            return BOX2SEQ_TRGWAIT;
        }
        syswk->app->subSeq++;
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlidePartyRight(syswk->app->bgWinFrame);
        } else {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            Box2Obj_PutPartyIconsLeftBelow(syswk, FALSE);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_MOVE_PARTY_IN);
    case BOX2_QUICK_PARTY_IN_READY:
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = BOX2_QUICK_PARTY_IN_PREPARE;
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE) {
            if (syswk->fromMenu == 0) {
                if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_DISABLED,
                                       BOX2_BUTTON_ENABLED);
                } else {
                    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED,
                                       BOX2_BUTTON_ENABLED);
                }
                if (syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
                    Box2Frame_HidePartyButton(syswk->app);
                    Box2Frame_HideBoxListButton(syswk->app);
                    CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_BOXES);
                }
                Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_SINGLE_MODE, TRUE);
                return BOX2SEQ_MOVE_PARTY_MAIN;
            }
            Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_SINGLE_MODE, TRUE);
        } else {
            Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        }
        Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        return Box2Seq_MovePickUpKey(syswk, syswk->pos, BOX2SEQ_MOVE_PARTY_MAIN);
    }
    return BOX2SEQ_MOVE_PARTY_IN;
}

// Moving Pokémon with the party out: waits for a touch or for the cursor to pick one
static int Box2Seq_MovePartyMain(Box2SysWork *syswk) {
    u32 res;

    if (syswk->moveDone == 1) {
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        syswk->moveDone = 0;
        return Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_SWITCH);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != BOX2_MOVE_MODE_MENU &&
        syswk->pickState == BOX2_PICK_NONE) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = BOX2_MOVE_MODE_RANGE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_SWITCH);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        } else {
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY_HELD;
            syswk->subProcCursorPos = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    }
    if (syswk->pickState == BOX2_PICK_NONE) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res);
                return Box2Seq_MovePickUpTouched(syswk, res, BOX2SEQ_MOVE_PARTY_MAIN);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res);
            Box2Ui_SetButtonStates(syswk, res);
            syswk->app->oldCurPos = res;
            Box2Ui_PlaceCursorActor(syswk, res);
            Box2Main_PokeInfoOff(syswk);
            return BOX2SEQ_MOVE_PARTY_MAIN;
        }
        res = Box2Ui_GetPartyPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
                return Box2Seq_MovePickUpTouched(syswk, res + BOX2_PARTY_POS, BOX2SEQ_MOVE_PARTY_MAIN);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            Box2Ui_SetButtonStates(syswk, res + BOX2_PARTY_POS);
            syswk->app->oldCurPos = res + BOX2_PARTY_POS;
            Box2Ui_PlaceCursorActor(syswk, res + BOX2_PARTY_POS);
            Box2Main_PokeInfoOff(syswk);
            return BOX2SEQ_MOVE_PARTY_MAIN;
        }
    }

    res = Box2Ui_UpdateCursor6(syswk);
    switch (res) {
    case BOX2_MOVE_PARTY_INPUT_TITLE:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
    case BOX2_MOVE_PARTY_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_MOVE_PARTY_MAIN);
    case BOX2_MOVE_PARTY_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_MOVE_PARTY_MAIN);
    case BOX2_MOVE_PARTY_INPUT_BOXES:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeInfoOff(syswk);
        }
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_BOX_LIST_BUTTON,
                                       Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_CLOSE));
    case BOX2_MOVE_PARTY_INPUT_SUMMARY:
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos >= BOX2_BOXLIST_POS ||
                Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                break;
            }
            syswk->pos = pos;
            syswk->getTray = syswk->tray;
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        } else {
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY_HELD;
            syswk->subProcCursorPos = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SUMMARY_BUTTON, 1, BOX2SEQ_CALL_POKE_STATUS);
    case BOX2_MOVE_PARTY_INPUT_BACK:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_SWITCH));
    case BOX2_MOVE_PARTY_INPUT_MENU_MODE:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_SWITCH);
    case BOX2_MOVE_PARTY_INPUT_SINGLE_MODE:
        break;
    case BOX2_MOVE_PARTY_INPUT_RANGE_MODE:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_RANGE;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_SWITCH);
    case BOX2_MOVE_PARTY_INPUT_SEARCH:
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        } else {
            syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY_HELD;
            syswk->subProcCursorPos = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    case CURSORMOVE_CANCEL:
        if (syswk->pickState == BOX2_PICK_KEY) {
            return Box2Seq_MoveDropKey(syswk, BOX2_GET_NONE, BOX2SEQ_MOVE_PARTY_MAIN);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_SWITCH));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_MOVE_PARTY_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_MOVE_PARTY_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_MOVE_PARTY_MAIN));
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (syswk->pickState == BOX2_PICK_KEY) {
            return Box2Seq_MoveDropKey(syswk, res, BOX2SEQ_MOVE_PARTY_MAIN);
        }
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Seq_SetGetPos(syswk, res, syswk->tray);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
            return Box2Seq_MovePickUpKey(syswk, syswk->pos, BOX2SEQ_MOVE_PARTY_MAIN);
        }
        break;
    }
    return BOX2SEQ_MOVE_PARTY_MAIN;
}

// Moving Pokémon with the party out: puts the party's frame away or out
static int Box2Seq_MovePartySwitch(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case BOX2_QUICK_PARTY_SWITCH_PREPARE:
        if (syswk->moveMode != BOX2_MOVE_MODE_RANGE) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN,
                               BOX2_BUTTON_ENABLED);
            if (syswk->fromParty == 0) {
                Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
                syswk->app->subSeq = BOX2_QUICK_PARTY_SWITCH_RESUME_TRAY;
            } else {
                Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
                Box2Obj_ShowScrollArrows(syswk->app, FALSE);
                Box2Frame_SlidePartyLeft(syswk->app->bgWinFrame);
                syswk->app->subSeq = BOX2_QUICK_PARTY_SWITCH_RESUME_PARTY;
            }
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_MOVE_PARTY_SWITCH);
        }
        return BOX2SEQ_RANGE_PARTY_IN;
    case BOX2_QUICK_PARTY_SWITCH_RESUME_PARTY:
        syswk->app->subSeq = BOX2_QUICK_PARTY_SWITCH_PREPARE;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos < BOX2_PARTY_POS || pos >= BOX2_BOXLIST_POS) {
            pos = 0;
        } else {
            pos -= BOX2_PARTY_POS;
        }
        Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON) {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_PARTY, pos);
            return BOX2SEQ_ARRANGE_PARTY_MAIN;
        }
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_PARTY, pos);
        return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
    case BOX2_QUICK_PARTY_SWITCH_RESUME_TRAY:
        syswk->app->subSeq = BOX2_QUICK_PARTY_SWITCH_PREPARE;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Main_PokeInfoPut(syswk, pos);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        if (syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, pos);
            return BOX2SEQ_ARRANGE_BOX_MAIN;
        }
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, pos);
        return BOX2SEQ_ARRANGE_MAIN;
    }
    return BOX2SEQ_MOVE_PARTY_SWITCH;
}

// Moving Pokémon: closes the party's frame
static int Box2Seq_MovePartyClose(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_QUICK_PARTY_CLOSE_CLOSE_PARTY:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_MOVE_PARTY_CLOSE);
    case BOX2_QUICK_PARTY_CLOSE_OPEN_BOX_LIST:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrame(syswk, BOX2SEQ_MOVE_PARTY_CLOSE);
    case BOX2_QUICK_PARTY_CLOSE_READY:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, BOX2_MOVE_BOXLIST_INPUT_PARTY);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->app->oldCurPos = 39;
        syswk->app->subSeq = BOX2_QUICK_PARTY_CLOSE_CLOSE_PARTY;
        return BOX2SEQ_ARRANGE_MOVE_MAIN;
    }
    return BOX2SEQ_MOVE_PARTY_CLOSE;
}

// The box's main state while the party is out of the way: waits for a touch or for the cursor to pick something
static int Box2Seq_ArrangeBoxMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->fromParty = 0;
        return Box2Seq_NextMoveMode(syswk, BOX2SEQ_MOVE_PARTY_IN);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    }

    res = Box2Ui_GetTrayPokeTrigger();
    if (res != CURSORMOVE_NONE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sBattleBoxMenu, 4);
            }
            Box2Main_PokeInfoPut(syswk, res);
            syswk->fromParty = 0;
            return Box2Seq_MovePartyPickUpTouched(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowPartyButton(syswk->app);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_ARRANGE_BOX_MAIN);
        }
        return BOX2SEQ_ARRANGE_BOX_MAIN;
    }

    res = Box2Ui_UpdateCursor13(syswk);
    switch (res) {
    case BOX2_BATTLE_BOX_TRAY_INPUT_TITLE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
    case BOX2_BATTLE_BOX_TRAY_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BATTLE_BOX_TRAY_INPUT_TITLE);
        syswk->app->oldCurPos = BOX2_BATTLE_BOX_TRAY_INPUT_TITLE;
        Box2Ui_PlaceCursorActor(syswk, BOX2_BATTLE_BOX_TRAY_INPUT_TITLE);
        return Box2Seq_ScrollTrayLeft(syswk, 1, BOX2SEQ_ARRANGE_BOX_MAIN);
    case BOX2_BATTLE_BOX_TRAY_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BATTLE_BOX_TRAY_INPUT_TITLE);
        syswk->app->oldCurPos = BOX2_BATTLE_BOX_TRAY_INPUT_TITLE;
        Box2Ui_PlaceCursorActor(syswk, BOX2_BATTLE_BOX_TRAY_INPUT_TITLE);
        return Box2Seq_ScrollTrayRight(syswk, 1, BOX2SEQ_ARRANGE_BOX_MAIN);
    case BOX2_BATTLE_BOX_TRAY_INPUT_PARTY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_PARTY_BUTTON,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_BOX_PARTY_OUT));
    case BOX2_BATTLE_BOX_TRAY_INPUT_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_CLOSE_BUTTON, 8, BOX2SEQ_LEAVE_STATE);
    case BOX2_BATTLE_BOX_TRAY_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case BOX2_BATTLE_BOX_TRAY_INPUT_MOVE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->fromParty = 0;
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_THIRD, Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_IN));
    case BOX2_BATTLE_BOX_TRAY_INPUT_SUMMARY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->subProcReturn = BOX2_SUBRETURN_MENU_TRAY;
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 37;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, BOX2SEQ_CALL_POKE_STATUS);
    case BOX2_BATTLE_BOX_TRAY_INPUT_HELD_ITEMS:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->subProcReturn = BOX2_SUBRETURN_MENU_TRAY;
        syswk->curRcvPos = 38;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_TAKE_ITEM));
    case BOX2_BATTLE_BOX_TRAY_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ARRANGE_BOX_MENU_CLOSE);
    }
    case BOX2_BATTLE_BOX_TRAY_INPUT_MENU_MODE:
        break;
    case BOX2_BATTLE_BOX_TRAY_INPUT_SINGLE_MODE:
        syswk->fromParty = 0;
        syswk->moveMode = BOX2_MOVE_MODE_SINGLE;
        return Box2Seq_SetMoveMode(syswk, BOX2SEQ_MOVE_PARTY_IN);
    case BOX2_BATTLE_BOX_TRAY_INPUT_RANGE_MODE:
        syswk->fromParty = 0;
        syswk->moveMode = BOX2_MOVE_MODE_RANGE;
        return Box2Seq_SetMoveMode(syswk, BOX2SEQ_RANGE_PARTY_IN);
    case BOX2_BATTLE_BOX_TRAY_INPUT_SEARCH:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_TRAY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BATTLE_BOX_TRAY_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 1, BOX2SEQ_ARRANGE_BOX_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BATTLE_BOX_TRAY_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 1, BOX2SEQ_ARRANGE_BOX_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != BOX2_BATTLE_BOX_TRAY_INPUT_MOVE && pos != BOX2_BATTLE_BOX_TRAY_INPUT_SUMMARY &&
                   pos != BOX2_BATTLE_BOX_TRAY_INPUT_HELD_ITEMS && pos != BOX2_BATTLE_BOX_TRAY_INPUT_CANCEL) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_BOX_MAIN));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ARRANGE_BOX_MENU_CLOSE);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sBattleBoxMenu, 4);
            Box2Main_PokeInfoPut(syswk, res);
            Box2Ui_StartCursorMove(syswk->app, BOX2_BOXLIST_POS, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
            syswk->app->oldCurPos = BOX2_BOXLIST_POS;
            return Box2Seq_OpenPokeMenu(syswk, res, BOX2SEQ_ARRANGE_BOX_MAIN);
        }
        break;
    }
    return BOX2SEQ_ARRANGE_BOX_MAIN;
}

// After a move while the party is out of the way
static int Box2Seq_ArrangeBoxMoveEnd(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_SYS_40);
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
    syswk->app->oldCurPos = BOX2_BOXLIST_POS;
    Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
    return BOX2SEQ_ARRANGE_BOX_MAIN;
}

// Closes the menu while the party is out of the way
static int Box2Seq_ArrangeBoxMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowPartyButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame,
                            Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_BOX_MAIN));
}

// Brings the party's frame out from the box's main state
static int Box2Seq_ArrangeBoxPartyOut(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_MENU_PARTY_OUT_OPEN_PARTY:
        Box2Main_PokeSelectOff(syswk);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Frame_PutPartyLeftBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        Box2Obj_PutPartyIconsLeftBelow(syswk, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_ARRANGE_BOX_PARTY_OUT);
    case BOX2_MENU_PARTY_OUT_READY:
        syswk->app->subSeq = BOX2_MENU_PARTY_OUT_OPEN_PARTY;
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_PARTY, 0);
        return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
    }
    return BOX2SEQ_ARRANGE_BOX_PARTY_OUT;
}

// The party's main state from the box's main state: waits for a touch or for the cursor to pick something
static int Box2Seq_ArrangeBoxPartyMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->fromParty = 1;
        return Box2Seq_NextMoveMode(syswk, BOX2SEQ_MOVE_PARTY_IN);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    }

    res = Box2Ui_GetLeftPartyPokeTrigger();
    if (res != CURSORMOVE_NONE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sBattleBoxMenu, 4);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return Box2Seq_ArrangeBoxPartyPickUpTouched(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_ARRANGE_BOX_PARTY_MAIN);
        }
        return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
    }

    res = Box2Ui_UpdateCursor14(syswk);
    switch (res) {
    case BOX2_BATTLE_BOX_PARTY_INPUT_BOXES:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_PARTY_BUTTON,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_BOX_PARTY_AWAY));
    case BOX2_BATTLE_BOX_PARTY_INPUT_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_CLOSE_BUTTON, 8, BOX2SEQ_LEAVE_STATE);
    case BOX2_BATTLE_BOX_PARTY_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_BOX_PARTY_AWAY));
    case BOX2_BATTLE_BOX_PARTY_INPUT_MOVE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->fromParty = 1;
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_THIRD, Box2Seq_Goto(syswk, BOX2SEQ_MOVE_PARTY_IN));
    case BOX2_BATTLE_BOX_PARTY_INPUT_SUMMARY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->subProcReturn = BOX2_SUBRETURN_MENU_PARTY;
        syswk->getTray = syswk->tray;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, BOX2SEQ_CALL_POKE_STATUS);
    case BOX2_BATTLE_BOX_PARTY_INPUT_HELD_ITEMS:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->subProcReturn = BOX2_SUBRETURN_MENU_PARTY;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_TAKE_ITEM));
    case BOX2_BATTLE_BOX_PARTY_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ARRANGE_BOX_PARTY_MENU_CLOSE);
    }
    case BOX2_BATTLE_BOX_PARTY_INPUT_MENU_MODE:
        break;
    case BOX2_BATTLE_BOX_PARTY_INPUT_SINGLE_MODE:
        syswk->fromParty = 1;
        syswk->moveMode = BOX2_MOVE_MODE_SINGLE;
        return Box2Seq_SetMoveMode(syswk, BOX2SEQ_MOVE_PARTY_IN);
    case BOX2_BATTLE_BOX_PARTY_INPUT_RANGE_MODE:
        syswk->fromParty = 1;
        syswk->moveMode = BOX2_MOVE_MODE_RANGE;
        return Box2Seq_SetMoveMode(syswk, BOX2SEQ_RANGE_PARTY_IN);
    case BOX2_BATTLE_BOX_PARTY_INPUT_SEARCH:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->subProcReturn = BOX2_SUBRETURN_MOVE_PARTY;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_SEARCH_BUTTON, 1, BOX2SEQ_CALL_BOX_SEARCH);
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ARRANGE_BOX_PARTY_MENU_CLOSE);
        }
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_BOX_PARTY_AWAY));
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < PARTY_SIZE) {
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        } else if (pos == BOX2_BATTLE_BOX_PARTY_INPUT_BOXES) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_BOX_PARTY_MAIN));
    }
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sBattleBoxMenu, 4);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            Box2Ui_StartCursorMove(syswk->app, BOX2_BATTLE_BOX_PARTY_INPUT_MOVE,
                                   CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_BATTLE_BOX_PARTY_INPUT_MOVE);
            syswk->app->oldCurPos = BOX2_BATTLE_BOX_PARTY_INPUT_MOVE;
            return Box2Seq_OpenPartyPokeMenu(syswk, res + BOX2_PARTY_POS, BOX2SEQ_ARRANGE_BOX_PARTY_MAIN);
        }
        break;
    }
    return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
}

// Puts the party's frame away, back to the box's main state
static int Box2Seq_ArrangeBoxPartyAway(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_MENU_PARTY_AWAY_CLOSE_PARTY:
        Box2Main_PokeSelectOff(syswk);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_ARRANGE_BOX_PARTY_AWAY);
    case BOX2_MENU_PARTY_AWAY_READY:
        syswk->app->subSeq = BOX2_MENU_PARTY_AWAY_CLOSE_PARTY;
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, BOX2_BATTLE_BOX_TRAY_INPUT_PARTY);
        return BOX2SEQ_ARRANGE_BOX_MAIN;
    }
    return BOX2SEQ_ARRANGE_BOX_PARTY_AWAY;
}

// Closes the party's menu, from the box's main state
static int Box2Seq_ArrangeBoxPartyMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowBoxListButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame,
                            Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ARRANGE_BOX_PARTY_MAIN));
}

// After a move in the party, from the box's main state
static int Box2Seq_ArrangeBoxPartyMoveEnd(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, BOX2_BATTLE_BOX_PARTY_INPUT_MOVE);
    syswk->app->oldCurPos = BOX2_BATTLE_BOX_PARTY_INPUT_MOVE;
    Box2Ui_PlaceCursorActor(syswk, BOX2_BATTLE_BOX_PARTY_INPUT_MOVE);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
    return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
}

// Back from a sub process to the box's main state or the party
static int Box2Seq_SubProcReturn(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE_TALL);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Main_PutMessageFrame(syswk);
    if (GCTX_HIDIsUsingTouch() == FALSE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
        Box2Frame_ShowBoxListButton(syswk->app);
        return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
    }
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
    Box2Ui_PlaceCursorActor(syswk, syswk->pos);
    Box2Frame_ShowPartyButton(syswk->app);
    return BOX2SEQ_ARRANGE_BOX_MAIN;
}

// The box's main state in the withdraw mode: waits for a touch or for the cursor to pick something
static int Box2Seq_WithdrawMain(Box2SysWork *syswk) {
    u32 res;

    res = Box2Ui_GetTrayPokeTrigger();
    if (res != CURSORMOVE_NONE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sWithdrawMenu, 5);
            }
            Box2Main_PokeInfoPut(syswk, res);
            return Box2Seq_WithdrawPickUpTouched(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_WITHDRAW_MAIN);
        }
        return BOX2SEQ_WITHDRAW_MAIN;
    }

    res = Box2Ui_UpdateCursor2(syswk);
    switch (res) {
    case BOX2_WITHDRAW_INPUT_TITLE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
    case BOX2_WITHDRAW_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_WITHDRAW_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_WITHDRAW_INPUT_TITLE);
        return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_WITHDRAW_MAIN);
    case BOX2_WITHDRAW_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_WITHDRAW_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_WITHDRAW_INPUT_TITLE);
        return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_WITHDRAW_MAIN);
    case BOX2_WITHDRAW_INPUT_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_CLOSE_BUTTON, 8, BOX2SEQ_LEAVE_STATE);
    case BOX2_WITHDRAW_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case BOX2_WITHDRAW_INPUT_WITHDRAW:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_SECOND,
                                       Box2Seq_Goto(syswk, BOX2SEQ_WITHDRAW_CHECK_PARTY));
    case BOX2_WITHDRAW_INPUT_SUMMARY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 36;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_THIRD, BOX2SEQ_CALL_POKE_STATUS);
    case BOX2_WITHDRAW_INPUT_MARKINGS:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, Box2Seq_Goto(syswk, BOX2SEQ_MARKING_OPEN));
    case BOX2_WITHDRAW_INPUT_RELEASE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_RELEASE_CHECK));
    case BOX2_WITHDRAW_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_WITHDRAW_MENU_CLOSE);
    }
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_WITHDRAW_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_WITHDRAW_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_WITHDRAW_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_WITHDRAW_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != BOX2_WITHDRAW_INPUT_WITHDRAW && pos != BOX2_WITHDRAW_INPUT_SUMMARY &&
                   pos != BOX2_WITHDRAW_INPUT_MARKINGS && pos != BOX2_WITHDRAW_INPUT_RELEASE &&
                   pos != BOX2_WITHDRAW_INPUT_CANCEL) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_WITHDRAW_MAIN));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_WITHDRAW_MENU_CLOSE);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sWithdrawMenu, 5);
            Box2Main_PokeInfoPut(syswk, res);
            Box2Ui_StartCursorMove(syswk->app, BOX2_WITHDRAW_INPUT_WITHDRAW, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_WITHDRAW_INPUT_WITHDRAW);
            return Box2Seq_OpenPokeMenu(syswk, res, BOX2SEQ_WITHDRAW_MAIN);
        }
        break;
    }
    return BOX2SEQ_WITHDRAW_MAIN;
}

// Closes the menu in the withdraw mode
static int Box2Seq_WithdrawMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_WITHDRAW_MAIN));
}

// Withdraw mode: closes the menu before taking a Pokémon into the party, unless it is full
static int Box2Seq_WithdrawCheckParty(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_WITHDRAW_CHECK_CLOSE_MENU:
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_WITHDRAW_CHECK_PARTY);
    case BOX2_WITHDRAW_CHECK_CHECK_CAPACITY:
        if (PokeParty_GetPkmCount(syswk->param->party) != PARTY_SIZE) {
            return Box2Seq_Goto(syswk, BOX2SEQ_WITHDRAW_TAKE);
        }
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintPokeMoveMsg(syswk, 0, BOX2_POKEMOVE_MSG_PARTY_FULL, BOX2_WIN_MESSAGE);
        syswk->nextSeq = BOX2SEQ_OPEN_MENU;
        return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
    }
    return BOX2SEQ_WITHDRAW_CHECK_PARTY;
}

// Withdraw mode: takes a Pokémon into the party
static int Box2Seq_WithdrawTake(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_WITHDRAW_TAKE_OPEN_PARTY:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Obj_PutPartyIconsRightBelow(syswk);
        Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        syswk->iconPosMode = BOX2_ICON_POS_PARTY_RIGHT;
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_WITHDRAW_TAKE);
    case BOX2_WITHDRAW_TAKE_MOVE_POKEMON:
        syswk->getTray = syswk->tray;
        Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        AllocPokeMoveWork(syswk);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyInPokeMove, BOX2SEQ_WITHDRAW_TAKE);
    case BOX2_WITHDRAW_TAKE_COMMIT_AND_CLOSE_PARTY:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Main_PokeDataMove(syswk);
        FreePokeMoveWork(syswk);
        Box2Main_PokeInfoOff(syswk);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_WITHDRAW_TAKE);
    case BOX2_WITHDRAW_TAKE_READY:
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos);
        Box2Obj_ShowCursor(syswk, TRUE);
        syswk->pos = BOX2_GET_NONE;
        syswk->app->subSeq = BOX2_WITHDRAW_TAKE_OPEN_PARTY;
        return BOX2SEQ_WITHDRAW_MAIN;
    }
    return BOX2SEQ_WITHDRAW_TAKE;
}

// Withdraw mode: after a Pokémon was taken into the party
static int Box2Seq_WithdrawEnd(Box2SysWork *syswk) {
    Box2PokeMoveWork *work = syswk->app->vfunk.work;
    u32 pos = work->getPos;

    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    Box2Obj_ShowScrollArrows(syswk->app, TRUE);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    if (syswk->pos != BOX2_GET_NONE) {
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        pos = 35;
    }
    CursorMove_SetPos(syswk->app->cursorMove, pos);
    Box2Ui_PlaceCursorActor(syswk, pos);
    return BOX2SEQ_WITHDRAW_MAIN;
}

// The party's main state in the deposit mode: waits for a touch or for the cursor to pick something
static int Box2Seq_DepositPartyMain(Box2SysWork *syswk) {
    u32 res;

    res = Box2Ui_GetLeftPartyPokeTrigger();
    if (res != CURSORMOVE_NONE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sDepositMenu, 5);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return Box2Seq_DepositPickUpTouched(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res + 60);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_DEPOSIT_PARTY_MAIN);
        }
        return BOX2SEQ_DEPOSIT_PARTY_MAIN;
    }

    res = Box2Ui_UpdateCursor0(syswk);
    switch (res) {
    case BOX2_DEPOSIT_PARTY_INPUT_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_CLOSE_BUTTON, 8, BOX2SEQ_LEAVE_STATE);
    case BOX2_DEPOSIT_PARTY_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case BOX2_DEPOSIT_PARTY_INPUT_DEPOSIT:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_SECOND, Box2Seq_Goto(syswk, BOX2SEQ_DEPOSIT_PICK_PARTY));
    case BOX2_DEPOSIT_PARTY_INPUT_SUMMARY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->curRcvPos = 9;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_THIRD, BOX2SEQ_CALL_POKE_STATUS);
    case BOX2_DEPOSIT_PARTY_INPUT_MARKINGS:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, Box2Seq_Goto(syswk, BOX2SEQ_MARKING_OPEN));
    case BOX2_DEPOSIT_PARTY_INPUT_RELEASE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_RELEASE_CHECK));
    case BOX2_DEPOSIT_PARTY_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_DEPOSIT_PARTY_MENU_CLOSE);
    }
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < PARTY_SIZE) {
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        } else if (pos != BOX2_DEPOSIT_PARTY_INPUT_DEPOSIT && pos != BOX2_DEPOSIT_PARTY_INPUT_SUMMARY &&
                   pos != BOX2_DEPOSIT_PARTY_INPUT_MARKINGS && pos != BOX2_DEPOSIT_PARTY_INPUT_RELEASE &&
                   pos != BOX2_DEPOSIT_PARTY_INPUT_CANCEL) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_DEPOSIT_PARTY_MAIN));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_DEPOSIT_PARTY_MENU_CLOSE);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sDepositMenu, 5);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            Box2Ui_StartCursorMove(syswk->app, BOX2_DEPOSIT_PARTY_INPUT_DEPOSIT,
                                   CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_DEPOSIT_PARTY_INPUT_DEPOSIT);
            return Box2Seq_OpenPartyPokeMenu(syswk, res + BOX2_PARTY_POS, BOX2SEQ_DEPOSIT_PARTY_MAIN);
        }
        break;
    }
    return BOX2SEQ_DEPOSIT_PARTY_MAIN;
}

// Closes the party's menu in the deposit mode
static int Box2Seq_DepositPartyMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame,
                            Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_DEPOSIT_PARTY_MAIN));
}

// Back to the party's main state in the deposit mode
static int Box2Seq_DepositPartyReturn(Box2SysWork *syswk) {
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
    Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
    Box2Obj_ShowCursor(syswk, TRUE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    return BOX2SEQ_DEPOSIT_PARTY_MAIN;
}

// Deposit mode: picks a Pokémon of the party up to deposit it, unless it can't leave the party
static int Box2Seq_DepositPickParty(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_DEPOSIT_PICK_CLOSE_MENU:
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_DEPOSIT_PICK_PARTY);
    case BOX2_DEPOSIT_PICK_CHECK_DEPOSIT:
        switch (GetDepositBlock(syswk)) {
        case BOX2_DEPOSIT_ALLOWED:
            syswk->pickState = BOX2_PICK_KEY;
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
            Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
            Box2Obj_ShowCursor(syswk, TRUE);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
            syswk->app->subSeq++;
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveGetKey, BOX2SEQ_DEPOSIT_PICK_PARTY);
        case BOX2_DEPOSIT_LAST_BATTLER:
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_PrintReleaseMsg(syswk, BOX2_RELEASE_MSG_LAST_POKEMON, BOX2_WIN_MESSAGE);
            syswk->nextSeq = BOX2SEQ_DEPOSIT_PARTY_RETURN;
            syswk->app->subSeq = BOX2_DEPOSIT_PICK_CLOSE_MENU;
            return BOX2SEQ_TRGWAIT;
        case BOX2_DEPOSIT_MAIL:
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_PrintPokeMoveMsg(syswk, 0, BOX2_POKEMOVE_MSG_REMOVE_MAIL, BOX2_WIN_MESSAGE);
            syswk->nextSeq = BOX2SEQ_DEPOSIT_PARTY_RETURN;
            syswk->app->subSeq = BOX2_DEPOSIT_PICK_CLOSE_MENU;
            return BOX2SEQ_TRGWAIT;
        }
        break;
    case BOX2_DEPOSIT_PICK_HIDE_PARTY:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_DEPOSIT_PICK_PARTY);
    case BOX2_DEPOSIT_PICK_POSITION_CURSOR:
        Box2Ui_StartMoveToPartyPos(syswk, syswk->pos - BOX2_PARTY_POS);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMove, BOX2SEQ_DEPOSIT_PICK_PARTY);
    case BOX2_DEPOSIT_PICK_CHOOSE_BOX:
        GFL_HeapFree(syswk->app->vfunk.work);
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Bmp_PrintPokeMoveMsg(syswk, 0, BOX2_POKEMOVE_MSG_WHICH_BOX, BOX2_WIN_DEPOSIT_PROMPT);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_DEPOSIT_BOX, BOX2_DEPOSIT_BOX_INPUT_TITLE);
        syswk->app->subSeq = BOX2_DEPOSIT_PICK_CLOSE_MENU;
        return BOX2SEQ_DEPOSIT_PICK_BOX;
    }
    return BOX2SEQ_DEPOSIT_PICK_PARTY;
}

// Deposit mode: picks the box to deposit the Pokémon in
static int Box2Seq_DepositPickBox(Box2SysWork *syswk) {
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            res = BOX2_DEPOSIT_BOX_INPUT_SCROLL_LEFT;
        } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            res = BOX2_DEPOSIT_BOX_INPUT_SCROLL_RIGHT;
        }
    }

    switch (res) {
    case BOX2_DEPOSIT_BOX_INPUT_TITLE:
    case BOX2_DEPOSIT_BOX_INPUT_BOX:
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_DEPOSIT_BOX_INPUT_TITLE);
        if (countEmptySlotsInBox(syswk->param->boxes, syswk->tray) == 0) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintPokeMoveMsg(syswk, 0, BOX2_POKEMOVE_MSG_BOX_FULL, BOX2_WIN_MESSAGE);
            syswk->nextSeq = BOX2SEQ_DEPOSIT_FULL_CLOSE;
            return BOX2SEQ_TRGWAIT;
        } else {
            int pos;
            int tray;

            tray = syswk->tray;
            pos = BOX2_DEPOSIT_BOX_INPUT_TITLE;

            BoxSaveAccessor_GetNextFreeBoxSlot(syswk->param->boxes, &tray, &pos);
            syswk->app->pokePutKey = pos;
            syswk->iconPosMode = BOX2_ICON_POS_TRAY;
            syswk->getTray = syswk->tray;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return Box2Seq_Goto(syswk, BOX2SEQ_DEPOSIT_PUT);
        }
    case BOX2_DEPOSIT_BOX_INPUT_SCROLL_LEFT:
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_DEPOSIT_BOX_INPUT_TITLE);
    case CURSORMOVE_SCROLL_L:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_DEPOSIT_PICK_BOX);
    case BOX2_DEPOSIT_BOX_INPUT_SCROLL_RIGHT:
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_DEPOSIT_BOX_INPUT_TITLE);
    case CURSORMOVE_SCROLL_R:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_DEPOSIT_PICK_BOX);
    case BOX2_DEPOSIT_BOX_INPUT_BACK:
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->app->pokePutKey = BOX2_GET_NONE;
        syswk->getTray = syswk->tray;
        syswk->iconPosMode = BOX2_ICON_POS_PARTY_LEFT;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, Box2Seq_Goto(syswk, BOX2SEQ_DEPOSIT_PUT));
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_NONE:
    case CURSORMOVE_CURSOR_MOVE:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    }
    return BOX2SEQ_DEPOSIT_PICK_BOX;
}

// Deposit mode: deposits the Pokémon in the picked box, or puts it back in the party
static int Box2Seq_DepositPut(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_DEPOSIT_PUT_POSITION_CURSOR:
        if (syswk->app->pokePutKey == BOX2_GET_NONE) {
            Box2Ui_StartMoveToCursor1(syswk, syswk->pos);
            syswk->app->subSeq = BOX2_DEPOSIT_PUT_REOPEN_PARTY;
        } else {
            Box2Ui_StartMoveToCursor1(syswk, syswk->app->pokePutKey);
            syswk->app->subSeq = BOX2_DEPOSIT_PUT_PLACE_POKEMON;
        }
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMove, BOX2SEQ_DEPOSIT_PUT);
    case BOX2_DEPOSIT_PUT_REOPEN_PARTY:
        GFL_HeapFree(syswk->app->vfunk.work);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        syswk->app->subSeq = BOX2_DEPOSIT_PUT_RETURN_POKEMON;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_DEPOSIT_PUT);
    case BOX2_DEPOSIT_PUT_RETURN_POKEMON:
        AllocPokeMoveWork(syswk);
        syswk->app->subSeq = BOX2_DEPOSIT_PUT_CANCEL;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMovePutKey, BOX2SEQ_DEPOSIT_PUT);
    case BOX2_DEPOSIT_PUT_SHOW_PARTY:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        syswk->app->subSeq = BOX2_DEPOSIT_PUT_READY;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_DEPOSIT_PUT);
    case BOX2_DEPOSIT_PUT_PLACE_POKEMON:
        GFL_HeapFree(syswk->app->vfunk.work);
        AllocPokeMoveWork(syswk);
        Box2Obj_ShowPokeOutline(syswk, FALSE);
        syswk->app->subSeq = BOX2_DEPOSIT_PUT_COMMIT;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyOutPutKey, BOX2SEQ_DEPOSIT_PUT);
    case BOX2_DEPOSIT_PUT_COMMIT:
        Box2Main_PokeDataMove(syswk);
        FreePokeMoveWork(syswk);
        Box2Main_PokeInfoOff(syswk);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, FALSE);
        syswk->app->subSeq = BOX2_DEPOSIT_PUT_SHOW_PARTY;
        break;
    case BOX2_DEPOSIT_PUT_CANCEL:
        FreePokeMoveWork(syswk);
    case BOX2_DEPOSIT_PUT_READY:
        syswk->pickState = BOX2_PICK_NONE;
        Box2Obj_ShowPokeOutline(syswk, FALSE);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_DEPOSIT_PARTY, syswk->pos - BOX2_PARTY_POS);
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        syswk->app->subSeq = BOX2_DEPOSIT_PUT_POSITION_CURSOR;
        return BOX2SEQ_DEPOSIT_PARTY_MAIN;
    }
    return BOX2SEQ_DEPOSIT_PUT;
}

// Deposit mode: closes the message about a full box
static int Box2Seq_DepositFullClose(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Bmp_PrintPokeMoveMsg(syswk, 0, BOX2_POKEMOVE_MSG_WHICH_BOX, BOX2_WIN_DEPOSIT_PROMPT);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    return BOX2SEQ_DEPOSIT_PICK_BOX;
}

// Deposit mode: after a deposit
static int Box2Seq_DepositEnd(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    if (syswk->pos == BOX2_GET_NONE) {
        CursorMove_SetPos(syswk->app->cursorMove, 0);
        Box2Ui_PlaceCursorActor(syswk, 0);
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
    } else {
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_DEPOSIT_PARTY_INPUT_DEPOSIT);
        Box2Ui_PlaceCursorActor(syswk, BOX2_DEPOSIT_PARTY_INPUT_DEPOSIT);
    }
    return BOX2SEQ_DEPOSIT_PARTY_MAIN;
}

// The item arrangement's main state: waits for a touch or for the cursor to pick something
static int Box2Seq_ItemMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        res = Box2Seq_NextMoveMode(syswk, BOX2SEQ_ITEM_PARTY_IN);
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE && syswk->fromMenu == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_PARTY_POS) {
                syswk->pos = pos;
            } else {
                syswk->pos = 0;
            }
        }
        return res;
    }

    res = Box2Ui_GetTrayPokeTrigger();
    if (res != CURSORMOVE_NONE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            Box2Main_PokeInfoPut(syswk, res);
            return Box2Seq_ItemTakeTouched(syswk, res);
        }
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Ui_StartCursorMove(syswk->app, res, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, res);
            return Box2Seq_ItemMenuClose(syswk);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        return BOX2SEQ_ITEM_MAIN;
    }

    res = Box2Ui_UpdateCursor7(syswk);
    switch (res) {
    case BOX2_ITEM_TRAY_INPUT_TITLE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
    case BOX2_ITEM_TRAY_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_TRAY_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_ITEM_TRAY_INPUT_TITLE);
        return Box2Seq_ScrollTrayLeft(syswk, 1, BOX2SEQ_ITEM_MAIN);
    case BOX2_ITEM_TRAY_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_TRAY_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_ITEM_TRAY_INPUT_TITLE);
        return Box2Seq_ScrollTrayRight(syswk, 1, BOX2SEQ_ITEM_MAIN);
    case BOX2_ITEM_TRAY_INPUT_PARTY:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_PARTY_BUTTON, Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_PARTY_OUT));
    case BOX2_ITEM_TRAY_INPUT_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_CLOSE_BUTTON, 8, BOX2SEQ_LEAVE_STATE);
    case BOX2_ITEM_TRAY_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case BOX2_ITEM_TRAY_INPUT_MOVE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, Box2Seq_Goto(syswk, BOX2SEQ_ITEM_PARTY_IN));
    case BOX2_ITEM_TRAY_INPUT_BAG:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->getItem == 0) {
            syswk->subProcReturn = BOX2_SUBRETURN_MENU_TRAY;
            syswk->curRcvPos = 37;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_TAKE_ITEM));
        }
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_TAKE_ITEM));
    case BOX2_ITEM_TRAY_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ITEM_MENU_CLOSE);
    }
    case BOX2_ITEM_TRAY_INPUT_MENU_MODE:
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE) {
            syswk->moveMode = BOX2_MOVE_MODE_MENU;
            return Box2Seq_SetMoveMode(syswk, BOX2SEQ_ITEM_PARTY_IN);
        }
        break;
    case BOX2_ITEM_TRAY_INPUT_SINGLE_MODE:
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
            syswk->moveMode = BOX2_MOVE_MODE_SINGLE;
            res = Box2Seq_SetMoveMode(syswk, BOX2SEQ_ITEM_PARTY_IN);
            if (syswk->fromMenu == 0) {
                u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

                if (pos < BOX2_PARTY_POS) {
                    syswk->pos = pos;
                } else {
                    syswk->pos = 0;
                }
            }
            return res;
        }
        break;
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_ITEM_TRAY_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 1, BOX2SEQ_ITEM_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_ITEM_TRAY_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 1, BOX2SEQ_ITEM_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != BOX2_ITEM_TRAY_INPUT_MOVE && pos != BOX2_ITEM_TRAY_INPUT_BAG &&
                   pos != BOX2_ITEM_TRAY_INPUT_CANCEL) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ITEM_MAIN));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ITEM_MENU_CLOSE);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return Box2Seq_ItemOpenMenu(syswk, res);
        }
        break;
    }
    return BOX2SEQ_ITEM_MAIN;
}

// The item arrangement: closes the menu
static int Box2Seq_ItemMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowPartyButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuOpen, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ITEM_MAIN));
}

// The item arrangement: puts the held item in the bag
static int Box2Seq_ItemToBag(Box2SysWork *syswk) {
    if (Box2Obj_IsAnimActive(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        return BOX2SEQ_ITEM_TO_BAG;
    }
    Box2Seq_OpenItemMenu(syswk, 0);
    Box2Obj_SetItemIconAffine(syswk->app, FALSE);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_PLACED_IN_BAG, BOX2_WIN_MESSAGE);
    if (syswk->pos < BOX2_PARTY_POS) {
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
    } else {
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
    }
    syswk->app->getItem = 0;
    syswk->nextSeq = BOX2SEQ_ITEM_BACK;
    return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
}

// The item arrangement: hides the item icon
static int Box2Seq_ItemHideIcon(Box2SysWork *syswk) {
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        Box2Frame_ShowPartyButton(syswk->app);
    } else {
        Box2Frame_ShowBoxListButton(syswk->app);
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemIconHide, BOX2SEQ_ITEM_BACK);
}

// The item arrangement: back to the box's or the party's main state
static int Box2Seq_ItemBack(Box2SysWork *syswk) {
    int seq;

    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos);
        Box2Frame_ShowPartyButton(syswk->app);
        seq = BOX2SEQ_ITEM_MAIN;
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
        Box2Frame_ShowBoxListButton(syswk->app);
        seq = BOX2SEQ_ITEM_PARTY_MAIN;
    }
    Box2Obj_ShowCursor(syswk, TRUE);
    Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
    return seq;
}

// The item arrangement: back to the main state with the cursor on the menu's button
static int Box2Seq_ItemBackToMenu(Box2SysWork *syswk) {
    u32 pos = 37;

    if (syswk->app->getItem != 0) {
        pos = 36;
    }
    CursorMove_SetPos(syswk->app->cursorMove, pos);
    syswk->app->oldCurPos = pos;
    Box2Ui_PlaceCursorActor(syswk, pos);
    Box2Obj_ShowCursor(syswk, TRUE);
    return BOX2SEQ_ITEM_MAIN;
}

// The item arrangement: brings the party's frame in
static int Box2Seq_ItemPartyIn(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_ITEM_PARTY_IN_PREPARE:
        syswk->app->subSeq++;
        Box2Obj_ShowCursor(syswk, FALSE);
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            syswk->partyWasLeft = 1;
        } else {
            syswk->partyWasLeft = 0;
        }
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE) {
            if (syswk->fromMenu == 0) {
                syswk->getTray = BOX2_GET_NONE;
                syswk->app->getItem = 0;
                Box2Main_PutMessageFrame(syswk);
                Box2Main_PokeInfoPut(syswk, syswk->pos);
            } else {
                if (syswk->app->getItem == 0) {
                    syswk->getTray = BOX2_GET_NONE;
                    syswk->fromMenu = 0;
                }
                Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
                Box2Main_PutMessageFrame(syswk);
                Box2Obj_ShowPokeOutline(syswk, FALSE);
                Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeFrameMove, BOX2SEQ_ITEM_PARTY_IN);
            }
        } else {
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeFrameMove, BOX2SEQ_ITEM_PARTY_IN);
        }
    case BOX2_ITEM_PARTY_IN_OPEN_PARTY:
        syswk->app->subSeq++;
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlidePartyRight(syswk->app->bgWinFrame);
        } else {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            Box2Obj_PutPartyIconsLeftBelow(syswk, FALSE);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_ITEM_PARTY_IN);
    case BOX2_ITEM_PARTY_IN_READY:
        syswk->app->subSeq = BOX2_ITEM_PARTY_IN_PREPARE;
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_BOXES);
        CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_SUMMARY);
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE) {
            if (syswk->fromMenu == 0) {
                Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN,
                                   BOX2_BUTTON_HIDDEN);
                Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_SINGLE_MODE, TRUE);
                return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
            }
            Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_SINGLE_MODE, TRUE);
        } else {
            Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        }
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        return Box2Seq_ItemPartyOutTakeKey(syswk, syswk->pos);
    }
    return BOX2SEQ_ITEM_PARTY_IN;
}

// The item arrangement with the party out: waits for a touch or for the cursor to pick something
static int Box2Seq_ItemPartyOutMain(Box2SysWork *syswk) {
    u32 res;

    if (syswk->moveDone == 1) {
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        syswk->moveDone = 0;
        return Box2Seq_Goto(syswk, BOX2SEQ_ITEM_PARTY_AWAY);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != BOX2_MOVE_MODE_MENU &&
        syswk->pickState == BOX2_PICK_NONE) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_Goto(syswk, BOX2SEQ_ITEM_PARTY_AWAY);
    }
    if (syswk->pickState == BOX2_PICK_NONE) {
        res = Box2Ui_GetPartyPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
                Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
                return Box2Seq_ItemPartyOutTakeTouched(syswk, res + BOX2_PARTY_POS);
            }
            Box2Main_PokeInfoOff(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
        }
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != CURSORMOVE_NONE) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                Box2Main_PokeInfoPut(syswk, res);
                Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
                return Box2Seq_ItemPartyOutTakeTouched(syswk, res);
            }
            Box2Main_PokeInfoOff(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, res);
            return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
        }
    }

    res = Box2Ui_UpdateCursor6(syswk);
    switch (res) {
    case BOX2_MOVE_PARTY_INPUT_TITLE:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_OPEN);
    case BOX2_MOVE_PARTY_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_ITEM_PARTY_OUT_MAIN);
    case BOX2_MOVE_PARTY_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        if (syswk->pickState == BOX2_PICK_NONE) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_ITEM_PARTY_OUT_MAIN);
    case BOX2_MOVE_PARTY_INPUT_BOXES:
    case BOX2_MOVE_PARTY_INPUT_SUMMARY:
        break;
    case BOX2_MOVE_PARTY_INPUT_BACK:
        if (syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, Box2Seq_Goto(syswk, BOX2SEQ_ITEM_PARTY_AWAY));
    case BOX2_MOVE_PARTY_INPUT_MENU_MODE:
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU || syswk->pickState != BOX2_PICK_NONE) {
            break;
        }
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_Goto(syswk, BOX2SEQ_ITEM_PARTY_AWAY);
    case BOX2_MOVE_PARTY_INPUT_SINGLE_MODE:
    case BOX2_MOVE_PARTY_INPUT_RANGE_MODE:
    case BOX2_MOVE_PARTY_INPUT_SEARCH:
        break;
    case CURSORMOVE_CANCEL:
        if (syswk->pickState == BOX2_PICK_KEY) {
            return Box2Seq_ItemDropHeldKey(syswk, BOX2_GET_NONE);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, Box2Seq_Goto(syswk, BOX2SEQ_ITEM_PARTY_AWAY));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_ITEM_PARTY_OUT_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_ITEM_PARTY_OUT_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        if (syswk->pickState == BOX2_PICK_NONE) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ITEM_PARTY_OUT_MAIN));
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (syswk->pickState == BOX2_PICK_KEY) {
            return Box2Seq_ItemDropHeldKey(syswk, res);
        }
        return Box2Seq_ItemPartyOutTakeKey(syswk, res);
    }
    return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
}

// The item arrangement: puts the party's frame away
static int Box2Seq_ItemPartyAway(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case BOX2_ITEM_PARTY_AWAY_CLOSE_PARTY:
        Box2Main_PokeInfoOff(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        if (syswk->partyWasLeft == 0) {
            Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        } else {
            Box2Obj_ShowScrollArrows(syswk->app, FALSE);
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
            Box2Frame_SlidePartyLeft(syswk->app->bgWinFrame);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_ITEM_PARTY_AWAY);
    case BOX2_ITEM_PARTY_AWAY_READY:
        syswk->app->subSeq = BOX2_ITEM_PARTY_AWAY_CLOSE_PARTY;
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        if (syswk->partyWasLeft == 0) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos >= BOX2_BOXLIST_POS) {
                pos = 0;
            } else if (pos >= BOX2_PARTY_POS) {
                pos = 0;
            }
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ITEM_TRAY, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            Box2Frame_ShowPartyButton(syswk->app);
            return BOX2SEQ_ITEM_MAIN;
        } else {
            u8 cur;

            pos = 0;
            cur = CursorMove_GetPos(syswk->app->cursorMove);
            if (cur < BOX2_BOXLIST_POS && cur >= BOX2_PARTY_POS) {
                pos = cur - BOX2_PARTY_POS;
            }
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ITEM_PARTY, pos);
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
            Box2Frame_ShowBoxListButton(syswk->app);
            return BOX2SEQ_ITEM_PARTY_MAIN;
        }
    }
    return BOX2SEQ_ITEM_PARTY_AWAY;
}

// The item arrangement: an item was dropped on a Pokémon by the cursor
static int Box2Seq_ItemDropKey(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u16 item;

    if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
        syswk->moveDone = 1;
    }
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    work = syswk->app->vfunk.work;
    if (work->putPos == syswk->pos && (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray)) {
        u16 setPos = work->setPos;

        FreeItemMoveWork(syswk);
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        if (PML_ItemIsMail(syswk->app->getItem) == TRUE && setPos != syswk->pos) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintMailCantMove(syswk, BOX2_WIN_MESSAGE);
            syswk->nextSeq = BOX2SEQ_ITEM_MAIL_CLOSE;
            return BOX2SEQ_TRGWAIT;
        }
        return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
    }

    if (syswk->getTray == syswk->tray || syswk->pos >= BOX2_PARTY_POS) {
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, TRUE);
    }
    Box2Obj_SetIconBlend(syswk->app, work->putPos, FALSE);
    item = Box2Main_GetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, work->putPos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, work->putPos);
        Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, work->putPos, syswk->app->pokeIconId[work->putPos]);
    }
    Box2Main_PokeInfoPut(syswk, work->putPos);
    CursorMove_SetPos(syswk->app->cursorMove, work->putPos);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->pos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->getTray, syswk->pos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->pos);
        if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
    }

    if (syswk->app->getItem == 0) {
        syswk->pos = work->putPos;
        FreeItemMoveWork(syswk);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
    }
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_SetItemIconAffine(syswk->app, TRUE);
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    Box2Obj_InitItemOutline(syswk->app);
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemIconPutBack, BOX2SEQ_ITEM_DROP_TOUCH);
}

// The item arrangement: an item was dropped on a Pokémon by touch
static int Box2Seq_ItemDropTouch(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u8 oldPos;
    u16 setPos;
    u16 item;
    u8 pos;

    if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
        syswk->moveDone = 1;
    }
    work = syswk->app->vfunk.work;
    Box2Obj_SetIconBlend(syswk->app, work->putPos, FALSE);
    oldPos = syswk->pos;
    setPos = work->setPos;
    syswk->pos = work->putPos;
    FreeItemMoveWork(syswk);
    item = syswk->app->getItem;
    syswk->app->getItem = 0;
    if (PML_ItemIsMail(item) == TRUE && setPos != oldPos) {
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_PrintMailCantMove(syswk, BOX2_WIN_MESSAGE);
        syswk->nextSeq = BOX2SEQ_ITEM_MAIL_CLOSE;
        return BOX2SEQ_TRGWAIT;
    }
    Box2Obj_ShowCursor(syswk, TRUE);
    pos = CursorMove_GetPos(syswk->app->cursorMove);
    if (pos < BOX2_BOXLIST_POS) {
        Box2Main_PokeInfoPut(syswk, pos);
    }
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
}

// The item arrangement: closes the message about mail
static int Box2Seq_ItemMailClose(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindowNow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
}

// The item arrangement: an item was dropped on a Pokémon by the cursor, with the party out
static int Box2Seq_ItemDropKeyPartyOut(Box2SysWork *syswk) {
    u16 item;

    if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
        syswk->moveDone = 1;
    }
    if (syswk->app->pokePutKey == syswk->app->getItemInitPos &&
        (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray)) {
        syswk->app->getItem = 0;
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
    }

    if (syswk->getTray == syswk->tray || syswk->app->getItemInitPos >= BOX2_PARTY_POS) {
        Box2Obj_SetIconBlend(syswk->app, syswk->app->getItemInitPos, TRUE);
    }
    Box2Obj_SetIconBlend(syswk->app, syswk->app->pokePutKey, FALSE);
    item = Box2Main_GetPokeParam(syswk, syswk->app->pokePutKey, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, syswk->app->pokePutKey, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->app->pokePutKey)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->app->pokePutKey);
        Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->app->pokePutKey,
                                syswk->app->pokeIconId[syswk->app->pokePutKey]);
    }
    Box2Main_PokeInfoPut(syswk, syswk->app->pokePutKey);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->app->getItemInitPos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->getTray, syswk->app->getItemInitPos)) ==
        TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->app->getItemInitPos);
        if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->app->getItemInitPos,
                                    syswk->app->pokeIconId[syswk->app->getItemInitPos]);
        }
    }

    if (syswk->app->getItem == 0) {
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
    }
    AllocItemMoveWork(syswk);
    ((Box2ItemMoveWork *)syswk->app->vfunk.work)->putPos = syswk->app->pokePutKey;
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_SetItemIconAffine(syswk->app, TRUE);
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    Box2Obj_InitItemOutline(syswk->app);
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemIconPutBack, BOX2SEQ_ITEM_DROP_TOUCH);
}

// The item arrangement in the party: waits for a touch or for the cursor to pick something
static int Box2Seq_ItemPartyMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        res = Box2Seq_NextMoveMode(syswk, BOX2SEQ_ITEM_PARTY_IN);
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE && syswk->fromMenu == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < PARTY_SIZE) {
                syswk->pos = pos + BOX2_PARTY_POS;
            } else {
                syswk->pos = BOX2_PARTY_POS;
            }
        }
        return res;
    }

    res = Box2Ui_GetLeftPartyPokeTrigger();
    if (res != CURSORMOVE_NONE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return Box2Seq_ItemPartyTakeTouched(syswk, res + BOX2_PARTY_POS);
        }
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Ui_StartCursorMove(syswk->app, (u8)res, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            return Box2Seq_ItemPartyMenuClose(syswk);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        return BOX2SEQ_ITEM_PARTY_MAIN;
    }

    res = Box2Ui_UpdateCursor8(syswk);
    switch (res) {
    case BOX2_ITEM_PARTY_INPUT_BOXES:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_PARTY_BUTTON, Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_PARTY_AWAY));
    case BOX2_ITEM_PARTY_INPUT_CLOSE:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_CLOSE_BUTTON, 8, BOX2SEQ_LEAVE_STATE);
    case BOX2_ITEM_PARTY_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_PARTY_AWAY));
    case BOX2_ITEM_PARTY_INPUT_MOVE:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, Box2Seq_Goto(syswk, BOX2SEQ_ITEM_PARTY_IN));
    case BOX2_ITEM_PARTY_INPUT_BAG:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->getItem == 0) {
            syswk->subProcReturn = BOX2_SUBRETURN_MENU_PARTY;
            syswk->curRcvPos = 10;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_TAKE_ITEM));
        }
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_TAKE_ITEM));
    case BOX2_ITEM_PARTY_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ITEM_PARTY_MENU_CLOSE);
    }
    case BOX2_ITEM_PARTY_INPUT_MENU_MODE:
        if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE) {
            syswk->moveMode = BOX2_MOVE_MODE_MENU;
            return Box2Seq_SetMoveMode(syswk, BOX2SEQ_ITEM_PARTY_IN);
        }
        break;
    case BOX2_ITEM_PARTY_INPUT_SINGLE_MODE:
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
            syswk->moveMode = BOX2_MOVE_MODE_SINGLE;
            res = Box2Seq_SetMoveMode(syswk, BOX2SEQ_ITEM_PARTY_IN);
            if (syswk->fromMenu == 0) {
                u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

                if (pos < PARTY_SIZE) {
                    syswk->pos = pos + BOX2_PARTY_POS;
                } else {
                    syswk->pos = BOX2_PARTY_POS;
                }
            }
            return res;
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < PARTY_SIZE) {
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        } else if (pos == BOX2_ITEM_PARTY_INPUT_BOXES) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ITEM_PARTY_MAIN));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_ITEM_PARTY_MENU_CLOSE);
        }
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9,
                                       Box2Seq_Goto(syswk, BOX2SEQ_ARRANGE_PARTY_AWAY));
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return Box2Seq_ItemPartyOpenMenu(syswk, res + BOX2_PARTY_POS);
        }
        break;
    }
    return BOX2SEQ_ITEM_PARTY_MAIN;
}

// The item arrangement in the party: closes the menu
static int Box2Seq_ItemPartyMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowBoxListButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuOpen,
                            Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ITEM_PARTY_MAIN));
}

// The item arrangement in the party: an item was dropped
static int Box2Seq_ItemPartyDrop(Box2SysWork *syswk) {
    Box2ItemMoveWork *work = syswk->app->vfunk.work;

    if (syswk->app->getItem == 0) {
        FreeItemMoveWork(syswk);
        Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_CAN_HOLD, BOX2_WIN_MESSAGE);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_PARTY_INPUT_BAG);
        syswk->app->oldCurPos = BOX2_ITEM_PARTY_INPUT_BAG;
        Box2Ui_PlaceCursorActor(syswk, BOX2_ITEM_PARTY_INPUT_BAG);
        Box2Obj_ShowCursor(syswk, TRUE);
        return BOX2SEQ_ITEM_PARTY_MAIN;
    }
    if (work->putPos == syswk->pos) {
        u16 setPos = work->setPos;

        FreeItemMoveWork(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_PARTY_INPUT_MOVE);
        syswk->app->oldCurPos = BOX2_ITEM_PARTY_INPUT_MOVE;
        Box2Ui_PlaceCursorActor(syswk, BOX2_ITEM_PARTY_INPUT_MOVE);
        if (PML_ItemIsMail(syswk->app->getItem) == TRUE && setPos != syswk->pos) {
            Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
            Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_MAIL_CANT_MOVE, BOX2_WIN_MESSAGE);
            GFL_SndSEPlay(SEQ_SE_BEEP);
            syswk->nextSeq = BOX2SEQ_OPEN_MENU;
            return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
        }
        Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_SELECTED, BOX2_WIN_MESSAGE);
        Box2Obj_ShowCursor(syswk, TRUE);
        return BOX2SEQ_ITEM_PARTY_MAIN;
    }
    if (PML_ItemIsMail(syswk->app->getItem) == TRUE) {
        FreeItemMoveWork(syswk);
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_MAIL_CANT_MOVE, BOX2_WIN_MESSAGE);
        GFL_SndSEPlay(SEQ_SE_BEEP);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_PARTY_INPUT_MOVE);
        syswk->app->oldCurPos = BOX2_ITEM_PARTY_INPUT_MOVE;
        Box2Ui_PlaceCursorActor(syswk, BOX2_ITEM_PARTY_INPUT_MOVE);
        Box2Obj_ShowCursor(syswk, TRUE);
        syswk->nextSeq = BOX2SEQ_OPEN_MENU;
        return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
    }
    Box2Obj_SetIconBlend(syswk->app, work->putPos, FALSE);
    Box2Obj_SetIconBlend(syswk->app, syswk->pos, TRUE);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Obj_SetItemIconAffine(syswk->app, TRUE);
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
    return BOX2SEQ_ITEM_PARTY_SWAP;
}

// The item arrangement in the party: swaps the items once the icon has landed
static int Box2Seq_ItemPartySwap(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u16 item;

    if (Box2Obj_IsAnimActive(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        return BOX2SEQ_ITEM_PARTY_SWAP;
    }
    Box2Obj_SetItemIconAffine(syswk->app, FALSE);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    work = syswk->app->vfunk.work;
    item = Box2Main_GetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, work->putPos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, work->putPos);
        Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, work->putPos, syswk->app->pokeIconId[work->putPos]);
    }
    Box2Main_PokeInfoPut(syswk, work->putPos);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->pos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
        if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos)) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
    }

    if (syswk->app->getItem == 0) {
        syswk->pos = work->putPos;
        FreeItemMoveWork(syswk);
        Box2Frame_ShowBoxListButton(syswk->app);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        syswk->app->oldCurPos = syswk->pos - BOX2_PARTY_POS;
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        return BOX2SEQ_ITEM_PARTY_MAIN;
    }
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_SetItemIconAffine(syswk->app, TRUE);
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    Box2Obj_InitItemOutline(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuCancel, Box2Seq_Goto(syswk, BOX2SEQ_ITEM_PARTY_RETURN));
}

// The item arrangement in the party: puts the item back where it was taken
static int Box2Seq_ItemPartyReturn(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_ITEM_PARTY_RETURN_HIDE_ICON:
        GFL_SndSEPlay(SEQ_SE_SYS_40);
        Box2Obj_SetIconBlend(syswk->app, ((Box2ItemMoveWork *)syswk->app->vfunk.work)->putPos, FALSE);
        Box2Obj_ShowPokeOutline(syswk, FALSE);
        Box2Obj_SetItemIconAffine(syswk->app, TRUE);
        Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        syswk->app->oldCurPos = syswk->pos - BOX2_PARTY_POS;
        FreeItemMoveWork(syswk);
        syswk->app->subSeq++;
        break;
    case BOX2_ITEM_PARTY_RETURN_WAIT_ICON_HIDE:
        if (Box2Obj_IsAnimActive(syswk->app, BOX2_ACTOR_ITEM_ICON) != TRUE) {
            Box2Main_PokeInfoPut(syswk, syswk->pos);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
            Box2Frame_ShowBoxListButton(syswk->app);
            return BOX2SEQ_ITEM_PARTY_MAIN;
        }
        break;
    }
    return BOX2SEQ_ITEM_PARTY_RETURN;
}

// The Dream World mode's main state: waits for a touch or for the cursor to pick a Pokémon
static int Box2Seq_DreamWorldMain(Box2SysWork *syswk) {
    u32 res = Box2Ui_UpdateCursor15(syswk);

    switch (res) {
    case BOX2_DREAM_WORLD_INPUT_TITLE:
        Box2Main_PokeSelectOff(syswk);
        break;
    case BOX2_DREAM_WORLD_INPUT_SCROLL_LEFT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_DREAM_WORLD_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_DREAM_WORLD_INPUT_TITLE);
        return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_DREAM_WORLD_MAIN);
    case BOX2_DREAM_WORLD_INPUT_SCROLL_RIGHT:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_DREAM_WORLD_INPUT_TITLE);
        Box2Ui_PlaceCursorActor(syswk, BOX2_DREAM_WORLD_INPUT_TITLE);
        return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_DREAM_WORLD_MAIN);
    case BOX2_DREAM_WORLD_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        syswk->param->tray = BOX2_GET_NONE;
        syswk->param->position = BOX2_GET_NONE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case BOX2_DREAM_WORLD_INPUT_TUCK_IN:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, Box2Seq_Goto(syswk, BOX2SEQ_DREAM_WORLD_PICK));
    case BOX2_DREAM_WORLD_INPUT_CANCEL: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_DREAM_WORLD_MENU_CLOSE);
    }
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_DREAM_WORLD_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 0, BOX2SEQ_DREAM_WORLD_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_DREAM_WORLD_INPUT_TITLE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 0, BOX2SEQ_DREAM_WORLD_MAIN);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != BOX2_DREAM_WORLD_INPUT_TUCK_IN && pos != BOX2_DREAM_WORLD_INPUT_CANCEL) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_DREAM_WORLD_MAIN));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, BOX2SEQ_DREAM_WORLD_MENU_CLOSE);
        }
        syswk->param->closed = FALSE;
        syswk->param->tray = BOX2_GET_NONE;
        syswk->param->position = BOX2_GET_NONE;
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, BOX2SEQ_ASK_CONTINUE);
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_D:
    case CURSORMOVE_SCROLL_U:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            syswk->pos = res;
            Box2Bmp_OpenMenu(syswk, sDreamWorldMenu, 2);
            Box2Main_PokeInfoPut(syswk, res);
            Box2Obj_PutPokeOutline(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_DREAM_WORLD_INPUT_TUCK_IN);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
                return Box2Seq_DreamWorldMenuOpened(syswk);
            }
            Box2Ui_StartCursorMove(syswk->app, BOX2_DREAM_WORLD_INPUT_TUCK_IN, res);
            Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame,
                                    Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_DREAM_WORLD_MENU_OPENED));
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoPut(syswk, res);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_DREAM_WORLD_MAIN);
        }
        break;
    }
    return BOX2SEQ_DREAM_WORLD_MAIN;
}

// Dream World mode: the picked Pokémon's menu was already open
static int Box2Seq_DreamWorldMenuOpened(Box2SysWork *syswk) {
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
    return BOX2SEQ_DREAM_WORLD_MAIN;
}

// Dream World mode: picks the Pokémon, unless it is an egg or can't be picked
static int Box2Seq_DreamWorldPick(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_DREAM_WORLD_PICK_CLOSE_MENU:
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_DREAM_WORLD_PICK);
    case BOX2_DREAM_WORLD_PICK_CHECK_ELIGIBILITY:
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_IS_EGG, NULL) != 0) {
            Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_EGG, BOX2_WIN_MESSAGE);
            syswk->nextSeq = BOX2SEQ_OPEN_MENU;
            return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
        }
        if (Box2Main_IsSpeciesSendable(syswk, syswk->pos) == FALSE) {
            Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_NOT_SLEEPY, BOX2_WIN_MESSAGE_TALL);
            syswk->nextSeq = BOX2SEQ_OPEN_MENU;
            return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
        }
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        Box2Bmp_PrintTuckIn(syswk);
        syswk->app->subSeq = BOX2_DREAM_WORLD_PICK_CLOSE_MENU;
        return Box2Seq_OpenYesNo(syswk, BOX2_YESNO_TUCK_IN);
    }
    return BOX2SEQ_DREAM_WORLD_PICK;
}

// Dream World mode: closes the menu
static int Box2Seq_DreamWorldMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame,
                            Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_DREAM_WORLD_MAIN));
}

static int Box2Seq_CallPokeStatus(Box2SysWork *syswk) {
    return Box2Seq_WipeOutCallSubProc(syswk, BOX2_SUBPROC_SUMMARY);
}

// Takes the held item into the bag, unless the Pokémon is an egg or holds mail
static int Box2Seq_TakeItem(Box2SysWork *syswk) {
    u32 item;

    switch (syswk->app->subSeq) {
    case BOX2_TAKE_ITEM_CHECK_ITEM:
        if (syswk->param->mode == BOX2_MODE_BATTLE_BOX && syswk->param->battleBoxLocked == 1) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            syswk->app->subSeq = BOX2_TAKE_ITEM_BATTLE_BOX_LOCKED_ERROR;
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_TAKE_ITEM);
        }
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_IS_EGG, NULL) == 0) {
            item = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
            if (item == 0) {
                return Box2Seq_WipeOutCallSubProc(syswk, BOX2_SUBPROC_BAG);
            }
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, FALSE);
            Box2Obj_ShowScrollArrows(syswk->app, FALSE);
            if (PML_ItemIsMail(item) == TRUE) {
                Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
                syswk->app->subSeq = BOX2_TAKE_ITEM_MAIL_ERROR;
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_TAKE_ITEM);
            }
            if (syswk->pos < BOX2_PARTY_POS) {
                Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
            } else {
                Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, TRUE);
            }
            Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
            syswk->app->iconsDimmed = 1;
            Box2Frame_HideMenu(syswk->app->bgWinFrame);
            Box2Bmp_PrintTakeItem(syswk, item, BOX2_WIN_MESSAGE);
            if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
                return Box2Seq_OpenYesNo(syswk, BOX2_YESNO_PUT_ITEM_IN_BAG);
            }
            return Box2Seq_OpenYesNo(syswk, BOX2_YESNO_TAKE_ITEM);
        }
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq = BOX2_TAKE_ITEM_EGG_ERROR;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_TAKE_ITEM);
    case BOX2_TAKE_ITEM_MAIL_ERROR:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintMailCantTake(syswk, BOX2_WIN_MESSAGE);
        syswk->nextSeq = BOX2SEQ_OPEN_MENU;
        return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
    case BOX2_TAKE_ITEM_EGG_ERROR:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_EGG_CANT_HOLD, BOX2_WIN_MESSAGE);
        syswk->nextSeq = BOX2SEQ_OPEN_MENU;
        return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
    case BOX2_TAKE_ITEM_BATTLE_BOX_LOCKED_ERROR:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintBattleBoxLocked(syswk, TRUE);
        syswk->nextSeq = BOX2SEQ_SUB_PROC_RETURN;
        return BOX2SEQ_TRGWAIT;
    }
    return BOX2SEQ_TAKE_ITEM;
}

static int Box2Seq_CallBoxSearch(Box2SysWork *syswk) {
    return Box2Seq_WipeOutCallSubProc(syswk, BOX2_SUBPROC_SEARCH);
}

// Back from the summary: shows the Pokémon and fades in
static int Box2Seq_SummaryReturn(Box2SysWork *syswk) {
    if (syswk->getTray != BOX2_GET_NONE) {
        Box2Main_PokeInfoPutCore(syswk, syswk->getTray, syswk->pos);
    } else {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
    }
    Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
    Box2Obj_SetToolButtons(syswk, syswk->moveMode + 9, TRUE);
    switch (syswk->param->mode) {
    case BOX2_MODE_DEPOSIT:
        syswk->app->msgNextSeq = Box2Seq_DepositSummaryReturn(syswk);
        break;
    case BOX2_MODE_WITHDRAW:
        syswk->app->msgNextSeq = Box2Seq_WithdrawSummaryReturn(syswk);
        break;
    case BOX2_MODE_MOVE_POKEMON:
        syswk->app->msgNextSeq = Box2Seq_ArrangeSummaryReturn(syswk);
        break;
    case BOX2_MODE_BATTLE_BOX:
        syswk->app->msgNextSeq = Box2Seq_BattleBoxSummaryReturn(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from box name entry: puts the cursor on the box list and fades in
static int Box2Seq_NameInputReturn(Box2SysWork *syswk) {
    Box2Main_PokeInfoOff(syswk);
    switch (syswk->param->mode) {
    case BOX2_MODE_WITHDRAW:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_WITHDRAW, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = BOX2SEQ_WITHDRAW_MAIN;
        break;
    case BOX2_MODE_MOVE_POKEMON:
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = BOX2SEQ_ARRANGE_MAIN;
        break;
    case BOX2_MODE_MOVE_ITEMS:
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ITEM_TRAY, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = BOX2SEQ_ITEM_MAIN;
        break;
    case BOX2_MODE_BATTLE_BOX:
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = BOX2SEQ_ARRANGE_BOX_MAIN;
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from the Bag: fades in
static int Box2Seq_BagReturn(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case BOX2_MODE_MOVE_POKEMON:
        syswk->app->msgNextSeq = Box2Seq_ArrangeGiveItemReturn(syswk);
        break;
    case BOX2_MODE_MOVE_ITEMS:
        syswk->app->msgNextSeq = Box2Seq_ItemGiveItemReturn(syswk);
        break;
    case BOX2_MODE_BATTLE_BOX:
        syswk->app->msgNextSeq = Box2Seq_BattleBoxGiveItemReturn(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from Pokemon search: restores the moving layout and fades in
static int Box2Seq_BoxSearchReturn(Box2SysWork *syswk) {
    u32 max;

    if (syswk->subProcReturn == BOX2_SUBRETURN_MOVE_TRAY || syswk->subProcReturn == BOX2_SUBRETURN_MOVE_TRAY_HELD) {
        max = BOX2_PARTY_POS;
    } else {
        max = BOX2_BOXLIST_POS;
    }
    if (syswk->pos < max) {
        if (syswk->getTray != BOX2_GET_NONE) {
            Box2Main_PokeInfoPutCore(syswk, syswk->getTray, syswk->pos);
        } else {
            Box2Main_PokeInfoPut(syswk, syswk->pos);
        }
    } else {
        Box2Main_PokeInfoOff(syswk);
    }
    Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
    Box2Obj_SetToolButtons(syswk, syswk->moveMode + 9, TRUE);
    switch (syswk->param->mode) {
    case BOX2_MODE_MOVE_POKEMON:
        syswk->app->msgNextSeq = Box2Seq_ArrangeMoveSummaryReturn(syswk);
        break;
    case BOX2_MODE_BATTLE_BOX:
        syswk->app->msgNextSeq = Box2Seq_BattleBoxMoveSummaryReturn(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Says which item was given
static int Box2Seq_GivenItemMsg(Box2SysWork *syswk) {
    Box2Bmp_PrintGiveItem(syswk, syswk->subRet, BOX2_WIN_MESSAGE);
    if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
        syswk->nextSeq = BOX2SEQ_ITEM_HIDE_ICON;
    } else {
        syswk->nextSeq = BOX2SEQ_OPEN_MENU;
    }
    return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
}

// Opens the markings
static int Box2Seq_MarkingOpen(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_OPEN_MARKINGS_CLOSE_MENU:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_MARKING_OPEN);
    case BOX2_OPEN_MARKINGS_OPEN_MARKINGS:
        syswk->app->markings = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_MARKINGS, NULL);
        Box2Main_MarkingPutMain(syswk, syswk->app->markings);
        Box2Frame_SlideMarkingIn(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncMarkingFrameMove, BOX2SEQ_MARKING_OPEN);
    case BOX2_OPEN_MARKINGS_READY:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MARKINGS, 0);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, TRUE);
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        Box2Bmp_PrintTouchMark(syswk, BOX2_WIN_MESSAGE);
        return Box2Seq_Goto(syswk, BOX2SEQ_MARKING_MAIN);
    }
    return BOX2SEQ_MARKING_OPEN;
}

// The markings: toggles a mark, or sets or cancels them
static int Box2Seq_MarkingMain(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 0);
        break;
    case 1:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 1);
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 2);
        break;
    case 3:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 3);
        break;
    case 4:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 4);
        break;
    case 5:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 5);
        break;
    case BOX2_MARKINGS_INPUT_CONFIRM: {
        u8 mark = syswk->app->markings;

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_MARKINGS, mark);
        Box2Main_MarkingPutSub(syswk, mark);
        Box2Obj_UpdatePokeIconPalette(syswk, syswk->tray, syswk->pos);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimMarkButton(syswk, 0, BOX2SEQ_MARKING_CLOSE);
    }
    case BOX2_MARKINGS_INPUT_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimMarkButton(syswk, 1, BOX2SEQ_MARKING_CLOSE);
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimMarkButton(syswk, 1, BOX2SEQ_MARKING_CLOSE);
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, BOX2SEQ_MARKING_CURSOR_MOVE_END);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    }
    return BOX2SEQ_MARKING_MAIN;
}

// Closes the markings
static int Box2Seq_MarkingClose(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case BOX2_MODE_DEPOSIT:
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_DEPOSIT_PARTY, BOX2_DEPOSIT_PARTY_INPUT_MARKINGS);
        break;
    case BOX2_MODE_WITHDRAW:
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_WITHDRAW, BOX2_WITHDRAW_INPUT_MARKINGS);
        break;
    case BOX2_MODE_MOVE_POKEMON:
    default:
        if (syswk->pos < BOX2_PARTY_POS) {
            Box2Obj_ShowScrollArrows(syswk->app, TRUE);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, BOX2_ARRANGE_TRAY_INPUT_MARKINGS);
        } else {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_PARTY, BOX2_ARRANGE_PARTY_INPUT_MARKINGS);
        }
        break;
    }
    Box2Obj_ShowCursor(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
    }
    Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, FALSE);
    Box2Frame_SlideMarkingOut(syswk->app->bgWinFrame);
    GFL_SndSEPlay(SEQ_SE_SYS_42);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncMarkingFrameMove, Box2Seq_Goto(syswk, BOX2SEQ_OPEN_MENU));
}

static int Box2Seq_MarkingCursorMoveEnd(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
    return BOX2SEQ_MARKING_MAIN;
}

// Releasing: checks the Pokémon can be released
static int Box2Seq_ReleaseCheck(Box2SysWork *syswk) {
    BoxPkm *pkm;
    u16 species;
    u8 form;
    u32 msg;

    switch (syswk->app->subSeq) {
    case BOX2_RELEASE_CHECK_CLOSE_MENU:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, FALSE);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_RELEASE_CHECK);
    case BOX2_RELEASE_CHECK_CHECK_ELIGIBILITY:
        if (syswk->pos >= BOX2_PARTY_POS) {
            u32 partyPos;

            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, TRUE);
            Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
            partyPos = syswk->pos - BOX2_PARTY_POS;
            if (Box2Main_BattlePokeCheck(syswk, partyPos) == FALSE) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                Box2Bmp_PrintReleaseMsg(syswk, BOX2_RELEASE_MSG_LAST_POKEMON, BOX2_WIN_MESSAGE);
                syswk->app->iconsDimmed = 1;
                syswk->nextSeq = BOX2SEQ_OPEN_MENU;
                return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
            }
            if (PML_ItemIsMail(PokeParty_GetParam(PokeParty_GetPkm(syswk->param->party, partyPos), PKM_PARAM_ITEM,
                                                  NULL)) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                Box2Bmp_PrintPokeMoveMsg(syswk, 0, BOX2_POKEMOVE_MSG_REMOVE_MAIL, BOX2_WIN_MESSAGE);
                syswk->app->iconsDimmed = 1;
                syswk->nextSeq = BOX2SEQ_OPEN_MENU;
                return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
            }
        } else {
            Box2Obj_ShowScrollArrows(syswk->app, FALSE);
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
            Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        }
        pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);
        species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
        msg = BOX2_RELEASE_MSG_NONE;
        if (PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL) != 0) {
            msg = BOX2_RELEASE_MSG_EGG;
        } else if (isKyuremTransformed(species, form)) {
            msg = BOX2_RELEASE_MSG_FUSED_KYUREM;
        }
        if (msg != BOX2_RELEASE_MSG_NONE) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintReleaseMsg(syswk, msg, BOX2_WIN_MESSAGE);
            syswk->app->iconsDimmed = 1;
            syswk->nextSeq = BOX2SEQ_OPEN_MENU;
            return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
        }
        Box2Bmp_PrintReleaseMsg(syswk, BOX2_RELEASE_MSG_ASK, BOX2_WIN_MESSAGE);
        return Box2Seq_Goto(syswk, Box2Seq_OpenYesNo(syswk, BOX2_YESNO_RELEASE));
    }
    return BOX2SEQ_RELEASE_CHECK;
}

// Releasing: checks the moves the Pokémon takes away
static int Box2Seq_ReleaseCheckMoves(Box2SysWork *syswk) {
    Box2Main_PokeFreeMoveCheck(syswk);
    if (Box2PokeFree_Shrink(syswk->app->subWork) == FALSE) {
        if (((Box2PokeFreeWork *)syswk->app->subWork)->checkFlag != 0) {
            return Box2Seq_Goto(syswk, BOX2SEQ_RELEASE_MOVE_LOCKED);
        }
        Box2PokeFree_End(syswk->app->subWork);
        Box2Main_PokeFreeExit(syswk);
        return Box2Seq_Goto(syswk, BOX2SEQ_RELEASE_DO);
    }
    return BOX2SEQ_RELEASE_CHECK_MOVES;
}

// Releasing: says goodbye and removes the Pokémon
static int Box2Seq_ReleaseDo(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_RELEASE_POKEMON_PRINT_RELEASED:
        Box2Bmp_PrintReleaseMsg(syswk, BOX2_RELEASE_MSG_RELEASED, BOX2_WIN_MESSAGE);
        syswk->nextSeq = BOX2SEQ_RELEASE_DO;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case BOX2_RELEASE_POKEMON_PRINT_BYE:
        Box2Bmp_PrintReleaseMsg(syswk, BOX2_RELEASE_MSG_BYE_BYE, BOX2_WIN_MESSAGE);
        syswk->nextSeq = BOX2SEQ_RELEASE_DO;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case BOX2_RELEASE_POKEMON_CLEAR_POKEMON:
        Box2Main_ClearPokeData(syswk, syswk->tray, syswk->pos);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        Box2Main_PokeInfoOff(syswk);
        if (syswk->pos < BOX2_PARTY_POS) {
            Box2Obj_MakeTrayIconChar(syswk, syswk->tray);
            Box2Obj_RefreshTrayIcon(syswk, syswk->tray);
            if (syswk->param->mode == BOX2_MODE_WITHDRAW) {
                syswk->nextSeq = BOX2SEQ_WITHDRAW_MAIN;
            } else {
                syswk->nextSeq = BOX2SEQ_ARRANGE_MAIN;
                Box2Frame_ShowPartyButton(syswk->app);
            }
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_ShowCursor(syswk, TRUE);
            Box2Obj_ShowScrollArrows(syswk->app, TRUE);
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
            Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
            syswk->pos = BOX2_GET_NONE;
            syswk->app->subSeq = BOX2_RELEASE_POKEMON_PRINT_RELEASED;
            return syswk->nextSeq;
        }
        AllocPokeMoveWork(syswk);
        syswk->iconPosMode = BOX2_ICON_POS_PARTY_LEFT;
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, FALSE);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyPokeFreeSort, BOX2SEQ_RELEASE_DO);
    case BOX2_RELEASE_POKEMON_FINISH_PARTY_SORT:
        syswk->app->subSeq = BOX2_RELEASE_POKEMON_PRINT_RELEASED;
        FreePokeMoveWork(syswk);
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
        Box2Obj_ShowCursor(syswk, TRUE);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->param->mode == BOX2_MODE_DEPOSIT) {
            return BOX2SEQ_DEPOSIT_PARTY_MAIN;
        }
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        Box2Frame_ShowBoxListButton(syswk->app);
        return BOX2SEQ_ARRANGE_PARTY_MAIN;
    }
    return BOX2SEQ_RELEASE_DO;
}

// Releasing: says the Pokémon's move can't be lost, and puts it back
static int Box2Seq_ReleaseMoveLocked(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_RELEASE_BLOCKED_RESTORE_SIZE:
        if (Box2PokeFree_Grow(syswk->app->subWork) == FALSE) {
            Box2PokeFree_ResetActor(syswk->app->subWork);
            Box2Main_PokeFreeExit(syswk);
            Box2Obj_PutPokeOutline(syswk);
            Box2Bmp_PrintReleaseMsg(syswk, BOX2_RELEASE_MSG_CAME_BACK, BOX2_WIN_MESSAGE);
            syswk->nextSeq = BOX2SEQ_RELEASE_MOVE_LOCKED;
            syswk->app->subSeq++;
            return BOX2SEQ_TRGWAIT;
        }
        break;
    case BOX2_RELEASE_BLOCKED_EXPLAIN_BLOCK:
        Box2Bmp_PrintReleaseMsg(syswk, BOX2_RELEASE_MSG_WORRIED, BOX2_WIN_MESSAGE);
        syswk->nextSeq = BOX2SEQ_RELEASE_MOVE_LOCKED;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case BOX2_RELEASE_BLOCKED_RETURN_TO_MENU:
        syswk->app->subSeq = BOX2_RELEASE_BLOCKED_RESTORE_SIZE;
        return Box2Seq_NoRelease(syswk);
    }
    return BOX2SEQ_RELEASE_MOVE_LOCKED;
}

// Opens the box menu
static int Box2Seq_BoxMenuOpen(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_OPEN_BOX_MENU_CLOSE_OLD_MENU:
        syswk->app->subSeq++;
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, FALSE);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            if (Box2Obj_IsVisible(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
                Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
            }
            Box2Obj_ShowPokeOutline(syswk, FALSE);
            Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_BOX_MENU_OPEN);
        }
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
            Box2Frame_HidePartyButton(syswk->app);
            return Box2Seq_MovePartyFrameTo(syswk, BOX2SEQ_BOX_MENU_OPEN);
        }
        if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_HideBoxListButton(syswk->app);
            Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, BOX2SEQ_BOX_MENU_OPEN);
        }
        Box2Frame_HidePartyButton(syswk->app);
    case BOX2_OPEN_BOX_MENU_OPEN_MENU:
        Box2Bmp_OpenMenu(syswk, sBoxMenu, 4);
        Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_BOX_MENU_OPEN);
    case BOX2_OPEN_BOX_MENU_READY:
        syswk->app->subSeq = BOX2_OPEN_BOX_MENU_CLOSE_OLD_MENU;
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Bmp_PrintPrompt(syswk, BOX2_PROMPT_WHAT_TO_DO, BOX2_WIN_MESSAGE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BOX_MENU, BOX2_BOX_MENU_INPUT_SWITCH_BOX);
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
        Box2Obj_SetCursorAnim(syswk, BOX2_CURSOR_ANIM_READY);
        return BOX2SEQ_BOX_MENU_MAIN;
    }
    return BOX2SEQ_BOX_MENU_OPEN;
}

// The box menu: picks a choice
static int Box2Seq_BoxMenuMain(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case BOX2_BOX_MENU_INPUT_SWITCH_BOX:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_THIRD, Box2Seq_Goto(syswk, BOX2SEQ_BOX_JUMP_OPEN));
    case BOX2_BOX_MENU_INPUT_WALLPAPER:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->app->wallpaperTheme = BOX2_WALLPAPER_THEME_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH, Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
    case BOX2_BOX_MENU_INPUT_NAME:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH, BOX2SEQ_CALL_NAME_INPUT);
    case BOX2_BOX_MENU_INPUT_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_CLOSE));
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_CLOSE));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_BOX_MENU_MAIN));
    }
    return BOX2SEQ_BOX_MENU_MAIN;
}

// Closes the box menu
static int Box2Seq_BoxMenuClose(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_CLOSE_BOX_MENU_CLOSE_MENU:
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON || syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN,
                               BOX2_BUTTON_ENABLED);
        } else {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        }
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_BOX_MENU_CLOSE);
    case BOX2_CLOSE_BOX_MENU_RESTORE_CURSOR:
        syswk->app->subSeq = BOX2_CLOSE_BOX_MENU_CLOSE_MENU;
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, BOX2_PARTY_POS);
            return BOX2SEQ_ARRANGE_MAIN;
        }
        if (syswk->param->mode == BOX2_MODE_WITHDRAW) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_WITHDRAW, BOX2_PARTY_POS);
            return BOX2SEQ_WITHDRAW_MAIN;
        }
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ITEM_TRAY, BOX2_PARTY_POS);
            return BOX2SEQ_ITEM_MAIN;
        }
        if (syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, BOX2_PARTY_POS);
            return BOX2SEQ_ARRANGE_BOX_MAIN;
        }
    }
    return BOX2SEQ_BOX_MENU_CLOSE;
}

// Opens the box jump
static int Box2Seq_BoxJumpOpen(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_OPEN_BOX_JUMP_CLOSE_MENU:
        syswk->app->boxJumpOpen = 1;
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_BOX_JUMP_OPEN);
    case BOX2_OPEN_BOX_JUMP_OPEN_BOX_LIST:
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return Box2Seq_MovePartyFrame(syswk, BOX2SEQ_BOX_JUMP_OPEN);
    case BOX2_OPEN_BOX_JUMP_READY:
        syswk->app->subSeq = BOX2_OPEN_BOX_JUMP_CLOSE_MENU;
        Box2Obj_ShowBoxNameFrame(syswk, 0, TRUE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BOX_JUMP, BOX2_BOX_JUMP_INPUT_BOX_ROW_FIRST);
        syswk->app->oldCurPos = 1;
        Box2Bmp_PrintPrompt(syswk, BOX2_PROMPT_JUMP_TO_BOX, BOX2_WIN_MESSAGE_TALL_NARROW);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        return BOX2SEQ_BOX_JUMP_MAIN;
    }
    return BOX2SEQ_BOX_JUMP_OPEN;
}

// The box jump: picks a box in the list
static int Box2Seq_BoxJumpMain(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if (Box2Ui_GetBoxListScrollTouch(&x, &y) == TRUE) {
        syswk->app->tpy = y;
        syswk->nextSeq = BOX2SEQ_BOX_JUMP_MAIN;
        AllocBoxListDragWork(syswk);
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        return BOX2SEQ_BOX_LIST_SCROLL_TOUCH;
    }

    res = CursorMove_Update(syswk->app->cursorMove);
    switch (res) {
    case BOX2_BOX_JUMP_INPUT_SCROLL_UP:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, -1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxJumpScrollPrevious, BOX2SEQ_BOX_JUMP_MAIN);
    case BOX2_BOX_JUMP_INPUT_SCROLL_DOWN:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, 1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxJumpScrollNext, BOX2SEQ_BOX_JUMP_MAIN);
    case BOX2_BOX_JUMP_INPUT_BOX_ROW_FIRST:
    case BOX2_BOX_JUMP_INPUT_BOX_ROW_SECOND:
    case BOX2_BOX_JUMP_INPUT_BOX_ROW_THIRD:
    case BOX2_BOX_JUMP_INPUT_BOX_ROW_LAST:
        syswk->app->pickedTray = syswk->trayScroll + res - 1;
        if (syswk->app->pickedTray >= syswk->trayMax) {
            syswk->app->pickedTray -= syswk->trayMax;
        }
        if (syswk->app->pickedTray != syswk->tray) {
            Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
            return Box2Seq_ScrollToPickedBox(syswk, BOX2SEQ_BOX_JUMP_RETURN);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, res - 1, TRUE);
        break;
    case BOX2_BOX_JUMP_INPUT_BACK:
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimActorButton(syswk, BOX2_ACTOR_BACK_BUTTON, 9, Box2Seq_Goto(syswk, BOX2SEQ_BOX_JUMP_CLOSE));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_BOX_JUMP_MAIN));
    case CURSORMOVE_SCROLL_U:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOX_JUMP_INPUT_BOX_ROW_FIRST) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, -1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxJumpScrollPrevious, BOX2SEQ_BOX_JUMP_MAIN);
        }
        break;
    case CURSORMOVE_SCROLL_D:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOX_JUMP_INPUT_BOX_ROW_LAST) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, 1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxJumpScrollNext, BOX2SEQ_BOX_JUMP_MAIN);
        }
        break;
    }
    return BOX2SEQ_BOX_JUMP_MAIN;
}

// Closes the box jump
static int Box2Seq_BoxJumpClose(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_CLOSE_BOX_JUMP_CLOSE_BOX_LIST:
        syswk->app->boxJumpOpen = 0;
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE_TALL_NARROW);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_ShowBoxNameFrame(syswk, 0, FALSE);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrameTo(syswk, BOX2SEQ_BOX_JUMP_CLOSE);
    case BOX2_CLOSE_BOX_JUMP_RESTORE_CURSOR:
        syswk->app->subSeq = BOX2_CLOSE_BOX_JUMP_CLOSE_BOX_LIST;
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON || syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN,
                               BOX2_BUTTON_ENABLED);
        } else {
            Box2Obj_SetButtons(syswk, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
        }
        Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
        if (syswk->param->mode == BOX2_MODE_MOVE_POKEMON) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, BOX2_PARTY_POS);
            return BOX2SEQ_ARRANGE_MAIN;
        }
        if (syswk->param->mode == BOX2_MODE_WITHDRAW) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_WITHDRAW, BOX2_PARTY_POS);
            return BOX2SEQ_WITHDRAW_MAIN;
        }
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ITEM_TRAY, BOX2_PARTY_POS);
            return BOX2SEQ_ITEM_MAIN;
        }
        if (syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, BOX2_PARTY_POS);
            return BOX2SEQ_ARRANGE_BOX_MAIN;
        }
    }
    return BOX2SEQ_BOX_JUMP_CLOSE;
}

// The box jump: back from showing a box
static int Box2Seq_BoxJumpReturn(Box2SysWork *syswk) {
    Box2Obj_ShowBoxNameFrame(syswk, CursorMove_GetPos(syswk->app->cursorMove) - 1, TRUE);
    return BOX2SEQ_BOX_JUMP_MAIN;
}

// Opens a wallpaper menu: the themes, or a theme's wallpapers
static int Box2Seq_WallpaperMenuOpen(Box2SysWork *syswk) {
    u32 all;

    switch (syswk->app->subSeq) {
    case BOX2_OPEN_WALLPAPER_MENU_CLOSE_OLD_MENU:
        Box2Obj_ShowCursor(syswk, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_WALLPAPER_MENU_OPEN);
    case BOX2_OPEN_WALLPAPER_MENU_OPEN_MENU:
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            if (BoxSaveAccessor_CheckFlags(syswk->param->boxes, BOX2_WALLPAPER_UNLOCK_SPECIAL_1) == TRUE) {
                Box2Bmp_OpenMenu(syswk, sThemeMenuSpecial, 6);
            } else {
                Box2Bmp_OpenMenu(syswk, sThemeMenu, 5);
            }
        } else if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SCENERY_1) {
            Box2Bmp_OpenMenu(syswk, sScenery1Menu, 5);
        } else if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SCENERY_2) {
            Box2Bmp_OpenMenu(syswk, sScenery2Menu, 5);
        } else if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SCENERY_3) {
            Box2Bmp_OpenMenu(syswk, sScenery3Menu, 5);
        } else if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_MISC) {
            Box2Bmp_OpenMenu(syswk, sMiscMenu, 5);
        } else if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SPECIAL_1) {
            if (BoxSaveAccessor_CheckFlags(syswk->param->boxes, BOX2_WALLPAPER_UNLOCK_SPECIAL_2) == TRUE) {
                Box2Bmp_OpenMenu(syswk, sSpecialMenuNext, 6);
            } else {
                Box2Bmp_OpenMenu(syswk, sSpecialMenu, 5);
            }
        } else if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SPECIAL_2) {
            Box2Bmp_OpenMenu(syswk, sSpecialMenu2, 6);
        }
        Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, BOX2SEQ_WALLPAPER_MENU_OPEN);
    case BOX2_OPEN_WALLPAPER_MENU_READY:
        syswk->app->subSeq = BOX2_OPEN_WALLPAPER_MENU_CLOSE_OLD_MENU;
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            Box2Bmp_PrintPrompt(syswk, BOX2_PROMPT_THEME, BOX2_WIN_MESSAGE);
        } else {
            Box2Bmp_PrintPrompt(syswk, BOX2_PROMPT_WALLPAPER, BOX2_WIN_MESSAGE);
        }
        all = FALSE;
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            if (BoxSaveAccessor_CheckFlags(syswk->param->boxes, BOX2_WALLPAPER_UNLOCK_SPECIAL_1) == FALSE) {
                all = TRUE;
            }
        } else if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SPECIAL_1) {
            if (BoxSaveAccessor_CheckFlags(syswk->param->boxes, BOX2_WALLPAPER_UNLOCK_SPECIAL_2) == FALSE) {
                all = TRUE;
            }
        } else if (syswk->app->wallpaperTheme != BOX2_WALLPAPER_SPECIAL_2) {
            all = TRUE;
        }
        if (all == TRUE) {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_WALLPAPER_MENU, 1);
            CursorMove_DisablePos(syswk->app->cursorMove, 0);
        } else {
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_WALLPAPER_MENU, 0);
        }
        return BOX2SEQ_WALLPAPER_MENU_MAIN;
    }
    return BOX2SEQ_WALLPAPER_MENU_OPEN;
}

// The wallpaper menu: picks a theme or a wallpaper
static int Box2Seq_WallpaperMenuMain(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            syswk->app->wallpaperTheme = BOX2_WALLPAPER_SPECIAL_1;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIRST,
                                           Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
        }
        return Box2Seq_PickWallpaper(syswk, 0);
    case 1:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            syswk->app->wallpaperTheme = BOX2_WALLPAPER_SCENERY_1;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_SECOND,
                                           Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
        }
        return Box2Seq_PickWallpaper(syswk, 1);
    case 2:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            syswk->app->wallpaperTheme = BOX2_WALLPAPER_SCENERY_2;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_THIRD,
                                           Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
        }
        return Box2Seq_PickWallpaper(syswk, 2);
    case 3:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            syswk->app->wallpaperTheme = BOX2_WALLPAPER_SCENERY_3;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FOURTH,
                                           Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
        }
        return Box2Seq_PickWallpaper(syswk, 3);
    case 4:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            syswk->app->wallpaperTheme = BOX2_WALLPAPER_MISC;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH,
                                           Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
        }
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SPECIAL_1 &&
            BoxSaveAccessor_CheckFlags(syswk->param->boxes, BOX2_WALLPAPER_UNLOCK_SPECIAL_2) == TRUE) {
            syswk->app->wallpaperTheme = BOX2_WALLPAPER_SPECIAL_2;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH,
                                           Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
        }
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SPECIAL_2) {
            syswk->app->wallpaperTheme = BOX2_WALLPAPER_SPECIAL_1;
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_FIFTH,
                                           Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
        }
        return Box2Seq_PickWallpaper(syswk, 4);
    case BOX2_WALLPAPER_MENU_INPUT_BACK:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_CLOSE));
        }
        syswk->app->wallpaperTheme = BOX2_WALLPAPER_THEME_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (syswk->app->wallpaperTheme == BOX2_WALLPAPER_THEME_MENU) {
            return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, Box2Seq_Goto(syswk, BOX2SEQ_BOX_MENU_CLOSE));
        }
        syswk->app->wallpaperTheme = BOX2_WALLPAPER_THEME_MENU;
        return Box2Seq_AnimFrameButton(syswk, BOX2_FRAME_MENU_LAST, Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_MENU_OPEN));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_WALLPAPER_MENU_MAIN));
    }
    return BOX2SEQ_WALLPAPER_MENU_MAIN;
}

// Changes the box's wallpaper behind a fade to white
static int Box2Seq_WallpaperChange(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case BOX2_CHANGE_WALLPAPER_FADE_OUT:
        Box2Obj_ShowCursor(syswk, FALSE);
        PaletteFade_LoadFromVRAM(syswk->app->palFade, 0, 0, 0x200);
        PaletteFade_StartFade(syswk->app->palFade, 1, 0xc000, 0, 0, 16, 0x7fff, GFL_VBlankGetTCBMgr());
        syswk->nextSeq = BOX2SEQ_WALLPAPER_CHANGE;
        syswk->app->subSeq++;
        return BOX2SEQ_PALETTE_FADE;
    case BOX2_CHANGE_WALLPAPER_APPLY_WALLPAPER:
        Box2Main_WallPaperChange(syswk, syswk->app->wallpaperPos);
        BoxSaveAccessor_SetWallpaper(syswk->param->boxes, syswk->tray, syswk->app->wallpaperPos);
        Box2Obj_MakeTrayIconChar(syswk, syswk->tray);
        syswk->app->subSeq++;
        break;
    case BOX2_CHANGE_WALLPAPER_FADE_IN:
        PaletteFade_StartFade(syswk->app->palFade, 1, 0xc000, 0, 16, 0, 0x7fff, GFL_VBlankGetTCBMgr());
        syswk->nextSeq = BOX2SEQ_WALLPAPER_CHANGE;
        syswk->app->subSeq++;
        return BOX2SEQ_PALETTE_FADE;
    case BOX2_CHANGE_WALLPAPER_READY:
        Box2Obj_ShowCursor(syswk, TRUE);
        syswk->app->subSeq = BOX2_CHANGE_WALLPAPER_FADE_OUT;
        return BOX2SEQ_WALLPAPER_MENU_MAIN;
    }
    return BOX2SEQ_WALLPAPER_CHANGE;
}

static int Box2Seq_CallNameInput(Box2SysWork *syswk) {
    return Box2Seq_WipeOutCallSubProc(syswk, BOX2_SUBPROC_BOX_NAME);
}

// Opens the box's name entry
static int Box2Seq_AskContinue(Box2SysWork *syswk) {
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
    Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, FALSE);
    Box2Bmp_PrintContinue(syswk, BOX2_WIN_MESSAGE);
    DimActorsForContinuePrompt(syswk);
    return Box2Seq_OpenYesNo(syswk, BOX2_YESNO_CONTINUE);
}

// Goes to another sequence from its first step
static int Box2Seq_Goto(Box2SysWork *syswk, int seq) {
    syswk->app->subSeq = 0;
    return seq;
}

// Wipes out, then goes to nextSeq
int Box2Seq_WipeOut(Box2SysWork *syswk, int nextSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, HEAPID_BOX2_APP);
    syswk->app->wipeSeq = nextSeq;
    return BOX2SEQ_WIPE;
}

// Wipes in, then goes to nextSeq
static int Box2Seq_WipeIn(Box2SysWork *syswk, int nextSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, HEAPID_BOX2_APP);
    syswk->app->wipeSeq = nextSeq;
    return BOX2SEQ_WIPE;
}

// Runs a function each frame until it ends, then goes to nextSeq
int Box2Seq_RunVFunc(Box2SysWork *syswk, Box2VFunc func, int nextSeq) {
    syswk->app->vfuncNextSeq = nextSeq;
    Box2Main_VFuncSet(syswk->app, func);
    return BOX2SEQ_VFUNC;
}

// Runs the requested function each frame until it ends, then goes to nextSeq
static int Box2Seq_RunRequestedVFunc(Box2SysWork *syswk, int nextSeq) {
    syswk->app->vfuncNextSeq = nextSeq;
    Box2Main_VFuncReqSet(syswk->app);
    return BOX2SEQ_VFUNC;
}

static int Box2Seq_PokeMoveEndTo(Box2SysWork *syswk, int seq) {
    syswk->nextSeq = seq;
    return BOX2SEQ_POKE_MOVE_END;
}

// Opens a yes/no question
static int Box2Seq_OpenYesNo(Box2SysWork *syswk, u32 type) {
    syswk->app->ynID = type;
    if (type == BOX2_YESNO_RELEASE) {
        Box2Main_OpenYesNo(syswk, 1);
    } else {
        Box2Main_OpenYesNo(syswk, 0);
    }
    setKeypressFramecounts(syswk->app->keyRepeatWait, syswk->app->keyRepeatStart);
    return BOX2SEQ_YESNO;
}

// Takes the Pokémon's held item into the bag
static int Box2Seq_YesTakeItem(Box2SysWork *syswk) {
    u32 item = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);

    if (BagSave_AddItem(syswk->param->bag, item, 1, HEAPID_BOX2_APP) == TRUE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Bmp_PrintTookItem(syswk, item, BOX2_WIN_MESSAGE);
        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, 0);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        Box2Main_PokeInfoRewrite(syswk, syswk->pos);
        Box2Obj_UpdatePokeIconPalette(syswk, syswk->tray, syswk->pos);
    } else {
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintBagFull(syswk, BOX2_WIN_MESSAGE);
    }
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, FALSE);
    }
    syswk->nextSeq = BOX2SEQ_OPEN_MENU;
    return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
}

// Releasing: starts the release
static int Box2Seq_YesRelease(Box2SysWork *syswk) {
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Main_PokeFreeCreate(syswk);
    Box2PokeFree_Start(syswk->app->subWork);
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, FALSE);
    }
    return BOX2SEQ_RELEASE_CHECK_MOVES;
}

// Releasing: the release was cancelled
static int Box2Seq_NoRelease(Box2SysWork *syswk) {
    Box2Obj_ShowPokeOutline(syswk, TRUE);
    Box2Bmp_ClearMsgWindowNow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
    } else {
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, FALSE);
    }
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, FALSE);
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, Box2Seq_Goto(syswk, BOX2SEQ_OPEN_MENU));
}

// The item arrangement: puts the held item in the bag
static int Box2Seq_YesItemToBag(Box2SysWork *syswk) {
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, FALSE);
    }
    if (BagSave_AddItem(syswk->param->bag, syswk->app->getItem, 1, HEAPID_BOX2_APP) == TRUE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, 0);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Obj_ShowPokeOutline(syswk, FALSE);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Obj_SetItemIconAffine(syswk->app, TRUE);
        Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
        return BOX2SEQ_ITEM_TO_BAG;
    }
    GFL_SndSEPlay(SEQ_SE_BEEP);
    Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_BAG_FULL, BOX2_WIN_MESSAGE);
    syswk->nextSeq = BOX2SEQ_OPEN_MENU;
    return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
}

// Leaves the box
static int Box2Seq_Leave(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_PC_LOGOFF);
    Box2Main_UpdateChatter(syswk);
    syswk->nextSeq = BOX2SEQ_END;
    return Box2Seq_WipeIn(syswk, BOX2SEQ_RELEASE);
}

// Closes a message and goes back to the mode's main state
static int Box2Seq_ReturnToMain(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
    switch (syswk->param->mode) {
    case BOX2_MODE_DEPOSIT:
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, FALSE);
        return BOX2SEQ_DEPOSIT_PARTY_MAIN;
    case BOX2_MODE_WITHDRAW:
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
        return BOX2SEQ_WITHDRAW_MAIN;
    case BOX2_MODE_MOVE_POKEMON:
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, FALSE);
            Box2Frame_ShowBoxListButton(syswk->app);
            return BOX2SEQ_ARRANGE_PARTY_MAIN;
        }
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
        Box2Frame_ShowPartyButton(syswk->app);
        return BOX2SEQ_ARRANGE_MAIN;
    case BOX2_MODE_MOVE_ITEMS:
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD | BOX2_ICON_CHECK_ELIGIBILITY, FALSE);
            Box2Frame_ShowBoxListButton(syswk->app);
            return BOX2SEQ_ITEM_PARTY_MAIN;
        }
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, FALSE);
        Box2Frame_ShowPartyButton(syswk->app);
        return BOX2SEQ_ITEM_MAIN;
    case BOX2_MODE_BATTLE_BOX:
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, FALSE);
            Box2Frame_ShowBoxListButton(syswk->app);
            return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
        }
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, FALSE);
        Box2Frame_ShowPartyButton(syswk->app);
        return BOX2SEQ_ARRANGE_BOX_MAIN;
    case BOX2_MODE_DREAM_WORLD:
        Box2Obj_ShowScrollArrows(syswk->app, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        return BOX2SEQ_DREAM_WORLD_MAIN;
    }
    return BOX2SEQ_ARRANGE_MAIN;
}

// Dream World mode: picks the Pokémon and leaves the box
static int Box2Seq_YesDreamWorldPick(Box2SysWork *syswk) {
    Box2Main_UpdateChatter(syswk);
    syswk->param->tray = syswk->tray;
    syswk->param->position = syswk->pos;
    syswk->nextSeq = BOX2SEQ_END;
    return Box2Seq_WipeIn(syswk, BOX2SEQ_RELEASE);
}

// Dream World mode: the Pokémon wasn't picked
static int Box2Seq_NoDreamWorldPick(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE_TALL);
    Box2Main_PutMessageFrame(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
    Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    return BOX2SEQ_DREAM_WORLD_MAIN;
}

static int Box2Seq_NoBackToMenu(Box2SysWork *syswk) {
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, FALSE);
    }
    return Box2Seq_OpenMenu(syswk);
}

// Animates a frame's button, then goes to seq
static int Box2Seq_AnimFrameButton(Box2SysWork *syswk, u32 frame, int seq) {
    Box2Main_SetFrameButtonAnm(syswk, frame);
    syswk->nextSeq = seq;
    return BOX2SEQ_BUTTON_ANM;
}

// Animates one of the markings' buttons, then goes to seq
static int Box2Seq_AnimMarkButton(Box2SysWork *syswk, u32 button, int seq) {
    syswk->app->bawk.mode = BOX2_BTN_ANM_MODE_BG;
    syswk->app->bawk.id = BGWinFrame_GetBG(syswk->app->bgWinFrame, BOX2_FRAME_MARKINGS);
    syswk->app->bawk.pal1 = 13;
    syswk->app->bawk.pal2 = 12;
    syswk->app->bawk.seq = 0;
    syswk->app->bawk.cnt = 0;
    if (button == 0) {
        syswk->app->bawk.py = 14;
    } else {
        syswk->app->bawk.py = 17;
    }
    syswk->app->bawk.px = 21;
    syswk->app->bawk.sx = 11;
    syswk->app->bawk.sy = 3;
    syswk->nextSeq = seq;
    return BOX2SEQ_BUTTON_ANM;
}

// Animates an actor's button, then goes to seq
static int Box2Seq_AnimActorButton(Box2SysWork *syswk, u32 id, u32 anim, int seq) {
    syswk->app->bawk.mode = BOX2_BTN_ANM_MODE_OBJ;
    syswk->app->bawk.id = id;
    syswk->app->bawk.pal1 = anim;
    syswk->app->bawk.pal2 = 0;
    syswk->app->bawk.seq = 0;
    syswk->app->bawk.cnt = 0;
    syswk->app->bawk.px = 0;
    syswk->app->bawk.py = 0;
    syswk->app->bawk.sx = 0;
    syswk->app->bawk.sy = 0;
    syswk->nextSeq = seq;
    return BOX2SEQ_BUTTON_ANM;
}

// Wipes out to call a sub process
static int Box2Seq_WipeOutCallSubProc(Box2SysWork *syswk, u32 type) {
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        GCTX_HIDSetUsingTouch(FALSE);
    } else {
        GCTX_HIDSetUsingTouch(TRUE);
    }
    syswk->nextSeq = BOX2SEQ_SUBPROC_CALL;
    syswk->subProcType = type;
    return Box2Seq_WipeIn(syswk, BOX2SEQ_RELEASE);
}

// Back from the bag after giving an item, in the arrangement
static int Box2Seq_ArrangeGiveItemReturn(Box2SysWork *syswk) {
    int seq;

    switch (syswk->subProcReturn) {
    case BOX2_SUBRETURN_MENU_TRAY:
        Box2Frame_ShowPartyButton(syswk->app);
        seq = BOX2SEQ_ARRANGE_MAIN;
        break;
    case BOX2_SUBRETURN_MENU_PARTY:
        PutPartyLeft(syswk);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_PARTY, BOX2_ARRANGE_PARTY_INPUT_HELD_ITEMS);
        Box2Frame_ShowBoxListButton(syswk->app);
        seq = BOX2SEQ_ARRANGE_PARTY_MAIN;
        break;
    }
    Box2Obj_PutPokeOutline(syswk);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Bmp_OpenMenu(syswk, sArrangeMenu, 6);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        Box2Obj_UpdatePokeIconPalette(syswk, syswk->tray, syswk->pos);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    Box2Obj_ShowCursor(syswk, FALSE);
    return BOX2SEQ_GIVEN_ITEM_MSG;
}

// Back from the bag after giving an item, in the item arrangement
static int Box2Seq_ItemGiveItemReturn(Box2SysWork *syswk) {
    int seq;

    switch (syswk->subProcReturn) {
    case BOX2_SUBRETURN_MENU_TRAY:
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, BOX2_ICON_POS_TRAY);
        seq = BOX2SEQ_ITEM_MAIN;
        break;
    case BOX2_SUBRETURN_MENU_PARTY:
        PutPartyLeft(syswk);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, BOX2_ICON_POS_PARTY_LEFT);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ITEM_PARTY, BOX2_ITEM_PARTY_INPUT_BAG);
        seq = BOX2SEQ_ITEM_PARTY_MAIN;
        break;
    }
    Box2Seq_OpenItemMenu(syswk, (s16)syswk->subRet);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintItemMsg(syswk, BOX2_ITEM_MSG_CAN_HOLD, BOX2_WIN_MESSAGE);
        if (syswk->subProcReturn == BOX2_SUBRETURN_MENU_TRAY) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        } else {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        }
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
        syswk->app->getItem = syswk->subRet;
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Obj_ShowItemOutline(syswk->app);
    }
    if (syswk->subProcReturn == BOX2_SUBRETURN_MENU_TRAY) {
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        Box2Frame_ShowPartyButton(syswk->app);
    } else {
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD | BOX2_ICON_CHECK_ELIGIBILITY, TRUE);
        Box2Frame_ShowBoxListButton(syswk->app);
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    Box2Obj_ShowCursor(syswk, FALSE);
    return BOX2SEQ_GIVEN_ITEM_MSG;
}

// Back from the bag after giving an item, in the Battle Box mode
static int Box2Seq_BattleBoxGiveItemReturn(Box2SysWork *syswk) {
    int seq;

    switch (syswk->subProcReturn) {
    case BOX2_SUBRETURN_MENU_TRAY:
        Box2Frame_ShowPartyButton(syswk->app);
        seq = BOX2SEQ_ARRANGE_BOX_MAIN;
        break;
    case BOX2_SUBRETURN_MENU_PARTY:
        PutPartyLeft(syswk);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_PARTY, BOX2_BATTLE_BOX_PARTY_INPUT_HELD_ITEMS);
        Box2Frame_ShowBoxListButton(syswk->app);
        seq = BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
        break;
    }
    Box2Obj_PutPokeOutline(syswk);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Bmp_OpenMenu(syswk, sBattleBoxMenu, 4);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        Box2Obj_UpdatePokeIconPalette(syswk, syswk->tray, syswk->pos);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    Box2Obj_ShowCursor(syswk, FALSE);
    return BOX2SEQ_GIVEN_ITEM_MSG;
}

// Back from the summary in the withdraw mode
static int Box2Seq_WithdrawSummaryReturn(Box2SysWork *syswk) {
    Box2Bmp_OpenMenu(syswk, sWithdrawMenu, 5);
    Box2Frame_ShowMenu(syswk->app->bgWinFrame);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
    Box2Obj_PutPokeOutline(syswk);
    return BOX2SEQ_WITHDRAW_MAIN;
}

// Back from the summary in the deposit mode
static int Box2Seq_DepositSummaryReturn(Box2SysWork *syswk) {
    Box2Bmp_OpenMenu(syswk, sDepositMenu, 5);
    Box2Frame_ShowMenu(syswk->app->bgWinFrame);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
    Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
    Box2Obj_ShowScrollArrows(syswk->app, FALSE);
    PutPartyLeft(syswk);
    Box2Obj_PutPokeOutline(syswk);
    return BOX2SEQ_DEPOSIT_PARTY_MAIN;
}

// Back from the summary in the arrangement
static int Box2Seq_ArrangeSummaryReturn(Box2SysWork *syswk) {
    switch (syswk->subProcReturn) {
    case BOX2_SUBRETURN_MENU_TRAY:
        Box2Bmp_OpenMenu(syswk, sArrangeMenu, 6);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        Box2Obj_PutPokeOutline(syswk);
        return BOX2SEQ_ARRANGE_MAIN;
    case BOX2_SUBRETURN_MENU_PARTY:
        Box2Bmp_OpenMenu(syswk, sArrangeMenu, 6);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_PARTY, BOX2_ARRANGE_PARTY_INPUT_SUMMARY);
        PutPartyLeft(syswk);
        Box2Obj_PutPokeOutline(syswk);
        return BOX2SEQ_ARRANGE_PARTY_MAIN;
    case BOX2_SUBRETURN_MOVE_TRAY:
        if (syswk->fromParty == 1) {
            Box2Obj_PutPartyIconsRightBelow(syswk);
        }
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, syswk->pos);
        Box2Main_OpenBoxList(syswk);
        while (Box2Main_VFuncBoxListSlideOut(syswk) != FALSE) {
        }
        Box2Obj_PlaceTrayHighlight(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
            return BOX2SEQ_RANGE_MAIN;
        }
        return BOX2SEQ_ARRANGE_MOVE_MAIN;
    case BOX2_SUBRETURN_MOVE_TRAY_HELD:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        if (syswk->pos >= BOX2_PARTY_POS) {
            Box2Obj_PutPartyIconsRightBelow(syswk);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        } else if (syswk->tray == syswk->getTray) {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        } else {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, syswk->subProcCursorPos);
        Box2Main_OpenBoxList(syswk);
        while (Box2Main_VFuncBoxListSlideOut(syswk) != FALSE) {
        }
        Box2Obj_PlaceTrayHighlight(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        if (syswk->subProcCursorPos >= BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST &&
            syswk->subProcCursorPos <= BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_LAST) {
            Box2Obj_ShowBoxNameFrame(syswk, syswk->subProcCursorPos - BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST, TRUE);
        }
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return BOX2SEQ_ARRANGE_MOVE_MAIN;
    case BOX2_SUBRETURN_MOVE_PARTY:
        PutPartyRight(syswk);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->pos);
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
            return BOX2SEQ_RANGE_PARTY_MAIN;
        }
        return BOX2SEQ_MOVE_PARTY_MAIN;
    case BOX2_SUBRETURN_MOVE_PARTY_HELD:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        PutPartyRight(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->subProcCursorPos);
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return BOX2SEQ_MOVE_PARTY_MAIN;
    }
    return BOX2SEQ_INIT;
}

// Back from the summary in the Battle Box mode
static int Box2Seq_BattleBoxSummaryReturn(Box2SysWork *syswk) {
    switch (syswk->subProcReturn) {
    case BOX2_SUBRETURN_MENU_TRAY:
        Box2Bmp_OpenMenu(syswk, sBattleBoxMenu, 4);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, BOX2_BATTLE_BOX_TRAY_INPUT_SUMMARY);
        Box2Obj_PutPokeOutline(syswk);
        return BOX2SEQ_ARRANGE_BOX_MAIN;
    case BOX2_SUBRETURN_MENU_PARTY:
        Box2Bmp_OpenMenu(syswk, sBattleBoxMenu, 4);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, BOX2_POKEMOVE_MSG_SELECTED, BOX2_WIN_MESSAGE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_PARTY, BOX2_BATTLE_BOX_PARTY_INPUT_SUMMARY);
        PutPartyLeft(syswk);
        Box2Obj_PutPokeOutline(syswk);
        return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
    case BOX2_SUBRETURN_MOVE_PARTY:
        PutPartyRight(syswk);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->pos);
        CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_BOXES);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
            return BOX2SEQ_RANGE_PARTY_MAIN;
        }
        return BOX2SEQ_MOVE_PARTY_MAIN;
    case BOX2_SUBRETURN_MOVE_PARTY_HELD:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        PutPartyRight(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->subProcCursorPos);
        CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_BOXES);
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return BOX2SEQ_MOVE_PARTY_MAIN;
    }
    return BOX2SEQ_INIT;
}

// Back from the summary while moving Pokémon in the arrangement
static int Box2Seq_ArrangeMoveSummaryReturn(Box2SysWork *syswk) {
    switch (syswk->subProcReturn) {
    case BOX2_SUBRETURN_MOVE_TRAY:
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_TRAY, syswk->pos);
            syswk->pos = BOX2_GET_NONE;
            return BOX2SEQ_ARRANGE_MAIN;
        }
        if (syswk->fromParty == 1) {
            Box2Obj_PutPartyIconsRightBelow(syswk);
        }
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, syswk->pos);
        Box2Main_OpenBoxList(syswk);
        while (Box2Main_VFuncBoxListSlideOut(syswk) != FALSE) {
        }
        Box2Obj_PlaceTrayHighlight(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
            return BOX2SEQ_RANGE_MAIN;
        }
        return BOX2SEQ_ARRANGE_MOVE_MAIN;
    case BOX2_SUBRETURN_MOVE_TRAY_HELD:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        if (syswk->pos >= BOX2_PARTY_POS) {
            Box2Obj_PutPartyIconsRightBelow(syswk);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        } else if (syswk->tray == syswk->getTray) {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        } else {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_BOXLIST, syswk->subProcCursorPos);
        Box2Main_OpenBoxList(syswk);
        while (Box2Main_VFuncBoxListSlideOut(syswk) != FALSE) {
        }
        Box2Obj_PlaceTrayHighlight(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        if (syswk->subProcCursorPos >= BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST &&
            syswk->subProcCursorPos <= BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_LAST) {
            Box2Obj_ShowBoxNameFrame(syswk, syswk->subProcCursorPos - BOX2_MOVE_BOXLIST_INPUT_BOX_ROW_FIRST, TRUE);
        }
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return BOX2SEQ_ARRANGE_MOVE_MAIN;
    case BOX2_SUBRETURN_MOVE_PARTY:
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
            PutPartyLeft(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
            Box2Obj_ShowScrollArrows(syswk->app, FALSE);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_ARRANGE_PARTY, syswk->pos - BOX2_PARTY_POS);
            syswk->pos = BOX2_GET_NONE;
            return BOX2SEQ_ARRANGE_PARTY_MAIN;
        }
        PutPartyRight(syswk);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->pos);
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
            return BOX2SEQ_RANGE_PARTY_MAIN;
        }
        return BOX2SEQ_MOVE_PARTY_MAIN;
    case BOX2_SUBRETURN_MOVE_PARTY_HELD:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        PutPartyRight(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->subProcCursorPos);
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return BOX2SEQ_MOVE_PARTY_MAIN;
    }
    return BOX2SEQ_INIT;
}

// Back from the summary while moving Pokémon in the Battle Box mode
static int Box2Seq_BattleBoxMoveSummaryReturn(Box2SysWork *syswk) {
    switch (syswk->subProcReturn) {
    case BOX2_SUBRETURN_MOVE_TRAY:
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_TRAY, syswk->pos);
        syswk->pos = BOX2_GET_NONE;
        return BOX2SEQ_ARRANGE_BOX_MAIN;
    case BOX2_SUBRETURN_MOVE_PARTY:
        if (syswk->moveMode == BOX2_MOVE_MODE_MENU) {
            PutPartyLeft(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
            Box2Obj_ShowScrollArrows(syswk->app, FALSE);
            Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_BATTLE_BOX_PARTY, syswk->pos - BOX2_PARTY_POS);
            syswk->pos = BOX2_GET_NONE;
            return BOX2SEQ_ARRANGE_BOX_PARTY_MAIN;
        }
        PutPartyRight(syswk);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->pos);
        CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_BOXES);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
            return BOX2SEQ_RANGE_PARTY_MAIN;
        }
        return BOX2SEQ_MOVE_PARTY_MAIN;
    case BOX2_SUBRETURN_MOVE_PARTY_HELD:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        PutPartyRight(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, BOX2_CURSOR_MOVE_PARTY, syswk->subProcCursorPos);
        CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_BOXES);
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return BOX2SEQ_MOVE_PARTY_MAIN;
    }
    return BOX2SEQ_INIT;
}

// Scrolls the tray left
static int Box2Seq_ScrollTrayLeft(Box2SysWork *syswk, u32 frameOut, int seq) {
    Box2Main_ScrollTray(syswk, FALSE);
    if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        if (frameOut == 1) {
            Box2Frame_ShowPartyButton(syswk->app);
        }
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncTrayScrollLeft, seq);
}

// Scrolls the tray right
static int Box2Seq_ScrollTrayRight(Box2SysWork *syswk, u32 frameOut, int seq) {
    Box2Main_ScrollTray(syswk, TRUE);
    if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
        Box2Main_PutMessageFrame(syswk);
        if (frameOut == 1) {
            Box2Frame_ShowPartyButton(syswk->app);
        }
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncTrayScrollRight, seq);
}

// Moving Pokémon: picks up a touched Pokémon
static int Box2Seq_ArrangePickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->fromParty = 0;
    syswk->moveMode = BOX2_MOVE_MODE_MENU;
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowPartyButton(syswk->app);
    syswk->nextSeq = BOX2SEQ_ARRANGE_MOVE_MAIN;
    syswk->iconPosMode = BOX2_ICON_POS_TRAY;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouch, BOX2SEQ_ARRANGE_MOVE_TO_BOX_LIST);
}

// Moving Pokémon with the party out: picks up a touched Pokémon
static int Box2Seq_MovePartyPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->fromParty = 0;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    syswk->nextSeq = BOX2SEQ_MOVE_PARTY_MAIN;
    syswk->iconPosMode = BOX2_ICON_POS_TRAY;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncRangeMoveTouch, BOX2SEQ_ARRANGE_BOX_MOVE_END);
}

// Withdraw mode: picks up a touched Pokémon to take it into the party
static int Box2Seq_WithdrawPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_ShowScrollArrows(syswk->app, FALSE);
    Box2Obj_PutPartyIconsRightBelow(syswk);
    Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
    Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, TRUE);
    Box2Obj_SetIconBlend(syswk->app, BOX2_BOXLIST_POS, FALSE);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    syswk->iconPosMode = BOX2_ICON_POS_PARTY_RIGHT;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyInTouch, BOX2SEQ_WITHDRAW_END);
}

// Opens a Pokémon's menu
static int Box2Seq_OpenPokeMenu(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->app->menuMsgPending = 1;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, TRUE);
    Box2Obj_PutPokeOutline(syswk);
    Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, seq));
}

// The party's main state: picks up a touched Pokémon
static int Box2Seq_ArrangePartyPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->moveMode = BOX2_MOVE_MODE_MENU;
    syswk->getTray = syswk->tray;
    syswk->fromParty = 1;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowBoxListButton(syswk->app);
    syswk->nextSeq = BOX2SEQ_MOVE_PARTY_MAIN;
    syswk->iconPosMode = BOX2_ICON_POS_PARTY_LEFT;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouchParty, BOX2SEQ_ARRANGE_PARTY_MOVE_END);
}

// The party's main state from the box's: picks up a touched Pokémon
static int Box2Seq_ArrangeBoxPartyPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->fromParty = 1;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    syswk->nextSeq = BOX2SEQ_MOVE_PARTY_MAIN;
    syswk->iconPosMode = BOX2_ICON_POS_PARTY_LEFT;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouchParty, BOX2SEQ_ARRANGE_BOX_PARTY_MOVE_END);
}

// Deposit mode: picks up a touched Pokémon of the party to deposit it
static int Box2Seq_DepositPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, TRUE);
    Box2Obj_SetIconBlend(syswk->app, BOX2_BOXLIST_POS, FALSE);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    syswk->iconPosMode = BOX2_ICON_POS_PARTY_LEFT;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyOutTouch, BOX2SEQ_DEPOSIT_END);
}

// Opens a party Pokémon's menu
static int Box2Seq_OpenPartyPokeMenu(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->app->menuMsgPending = 1;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, TRUE);
    Box2Obj_PutPokeOutline(syswk);
    Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, seq));
}

// Moving Pokémon: picks up a touched Pokémon
static int Box2Seq_MovePickUpTouched(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->oldCurPos = pos;
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN);
    AllocPokeMoveWork(syswk);
    syswk->nextSeq = seq;
    syswk->iconPosMode = BOX2_ICON_POS_PARTY_RIGHT;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouch, BOX2SEQ_ARRANGE_MOVE_END);
}

// Picking a range: picks up the touched range
static int Box2Seq_RangePickUpTouched(Box2SysWork *syswk, int seq) {
    syswk->app->oldCurPos = syswk->pos;
    Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, syswk->app->rangeWidth, syswk->app->rangeHeight, FALSE);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED);
    AllocPokeMoveWork(syswk);
    syswk->nextSeq = seq;
    syswk->iconPosMode = BOX2_ICON_POS_PARTY_RIGHT;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouch, BOX2SEQ_RANGE_MOVE_END);
}

// Moving Pokémon by the keys: picks up the Pokémon under the cursor
static int Box2Seq_MovePickUpKey(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pickState = BOX2_PICK_KEY;
    syswk->pos = pos;
    Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED);
    if (syswk->param->mode == BOX2_MODE_BATTLE_BOX) {
        Box2Frame_HidePartyButton(syswk->app);
        Box2Frame_HideBoxListButton(syswk->app);
        CursorMove_DisablePos(syswk->app->cursorMove, BOX2_MOVE_PARTY_INPUT_BOXES);
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveGetKey, seq);
}

// Moving Pokémon by the keys in the arrangement: picks up the Pokémon under the cursor
static int Box2Seq_ArrangePickUpKey(Box2SysWork *syswk, u32 pos) {
    syswk->pickState = BOX2_PICK_KEY;
    syswk->pos = pos;
    Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_ENABLED, BOX2_BUTTON_ENABLED);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveGetKey, BOX2SEQ_ARRANGE_MOVE_MAIN);
}

// Picking a range: picks up the range by the keys
static int Box2Seq_RangePickUpKey(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveGetKey, seq);
}

// Moving Pokémon by the keys: drops the Pokémon, unless the box picked in the list is this one or full
static int Box2Seq_MoveDropKey(Box2SysWork *syswk, u32 pos, int seq) {
    if (pos >= BOX2_PARTY_POS) {
        // A box of the list
        if (pos >= BOX2_BOXLIST_POS && pos != BOX2_GET_NONE) {
            u32 tray = syswk->trayScroll + pos - BOX2_BOXLIST_POS;

            if (tray >= syswk->trayMax) {
                tray -= syswk->trayMax;
            }
            if (tray == syswk->tray) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                return BOX2SEQ_ARRANGE_MOVE_MAIN;
            }
            if (countEmptySlotsInBox(syswk->param->boxes, tray) == 0) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                return BOX2SEQ_ARRANGE_MOVE_MAIN;
            }
        }
    }
    syswk->app->pokePutKey = pos;
    syswk->iconPosMode = BOX2_ICON_POS_PARTY_RIGHT;
    syswk->nextSeq = seq;
    AllocPokeMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMovePutKey, BOX2SEQ_ARRANGE_MOVE_END);
}

// Picking a range: drops the range, unless the box picked in the list is this one or has too little room
static int Box2Seq_RangeDropKey(Box2SysWork *syswk, u32 pos, int seq) {
    if (pos >= BOX2_PARTY_POS) {
        // A box of the list
        if (pos >= BOX2_BOXLIST_POS && pos != BOX2_GET_NONE) {
            u32 tray = syswk->trayScroll + pos - BOX2_BOXLIST_POS;

            if (tray >= syswk->trayMax) {
                tray -= syswk->trayMax;
            }
            if (tray == syswk->tray) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                return BOX2SEQ_RANGE_MAIN;
            }
            if (countEmptySlotsInBox(syswk->param->boxes, tray) < Box2Main_GetRangeCount(syswk->app)) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                return BOX2SEQ_RANGE_MAIN;
            }
        }
    }
    Box2Obj_SetRangeAnm(syswk, 0);
    syswk->app->pokePutKey = pos;
    syswk->iconPosMode = BOX2_ICON_POS_PARTY_RIGHT;
    syswk->nextSeq = seq;
    AllocPokeMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMovePutKey, BOX2SEQ_RANGE_MOVE_END);
}

// Allocates the work of a Pokémon move
static void AllocPokeMoveWork(Box2SysWork *syswk) {
    syswk->app->vfunk.work = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2PokeMoveWork), TRUE, "box2_seq.c", 10826);
}

static void FreePokeMoveWork(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
}

// The item arrangement: takes the item of a touched Pokémon
static int Box2Seq_ItemTakeTouched(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->menuMsgPending = 1;
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Frame_HideMenu(syswk->app->bgWinFrame);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->app->getItem != 0) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, BOX2_ICON_POS_TRAY);
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    } else if (Box2Obj_IsVisible(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_ShowPokeOutline(syswk, FALSE);
    }
    Box2Obj_GetIconPos(syswk->pos, &x, &y, BOX2_ICON_POS_TRAY);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    AllocItemMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeGetTouch,
                            Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ITEM_BACK_TO_MENU));
}

// The item arrangement: opens the menu of a picked Pokémon
static int Box2Seq_ItemOpenMenu(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->menuMsgPending = 1;
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Seq_OpenItemMenu(syswk, (s16)syswk->app->getItem);
    Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
    if (syswk->app->getItem != 0) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, BOX2_ICON_POS_TRAY);
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Ui_StartCursorMove(syswk->app, BOX2_ITEM_TRAY_INPUT_MOVE, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_TRAY_INPUT_MOVE);
        syswk->app->oldCurPos = BOX2_ITEM_TRAY_INPUT_MOVE;
    } else {
        Box2Ui_StartCursorMove(syswk->app, BOX2_ITEM_TRAY_INPUT_BAG, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_TRAY_INPUT_BAG);
        syswk->app->oldCurPos = BOX2_ITEM_TRAY_INPUT_BAG;
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuClose, Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ITEM_MAIN));
}

// The item arrangement with the party out: takes the item of a touched Pokémon
static int Box2Seq_ItemPartyOutTakeTouched(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->pos = pos;
    if (pos < BOX2_PARTY_POS) {
        syswk->getTray = syswk->tray;
    } else {
        syswk->getTray = BOX2_GET_NONE;
    }
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    if (syswk->app->getItem == 0) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
    }
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, BOX2_ICON_POS_PARTY_RIGHT);
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_InitItemOutline(syswk->app);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    Box2Obj_GetIconPos(syswk->pos, &x, &y, BOX2_ICON_POS_PARTY_RIGHT);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    AllocItemMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangePartyGetTouch, BOX2SEQ_ITEM_DROP_KEY);
}

// The item arrangement with the party out: takes the item of the Pokémon under the cursor
static int Box2Seq_ItemPartyOutTakeKey(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->app->getItemInitPos = pos;
    if (pos < BOX2_PARTY_POS) {
        syswk->getTray = syswk->tray;
    } else {
        syswk->getTray = BOX2_GET_NONE;
    }
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    if (syswk->app->getItem == 0) {
        return BOX2SEQ_ITEM_PARTY_OUT_MAIN;
    }
    syswk->pickState = BOX2_PICK_KEY;
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    Box2Obj_PutItemIconAtCursor(syswk->app, syswk->pos, BOX2_ICON_POS_PARTY_RIGHT);
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeGetKey, BOX2SEQ_ITEM_PARTY_OUT_MAIN);
}

// The item arrangement: puts a held item back by the keys
static int Box2Seq_ItemPutBackKey(Box2SysWork *syswk) {
    u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

    if (pos < BOX2_BOXLIST_POS) {
        Box2Main_PokeInfoPut(syswk, pos);
    } else {
        Box2Main_PokeInfoOff(syswk);
    }
    AllocItemMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeKeyCancel, BOX2SEQ_ITEM_DROP_TOUCH);
}

// The item arrangement: drops a held item by the keys
static int Box2Seq_ItemDropHeldKey(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->app->pokePutKey = pos;
    syswk->pickState = BOX2_PICK_NONE;
    Box2Obj_GetPos(syswk->app, BOX2_ACTOR_CURSOR, &x, &y, CLACT_SURFACE_MAIN);
    Box2Obj_SetPos(syswk->app, BOX2_ACTOR_ITEM_ICON, x, y + 8, CLACT_SURFACE_MAIN);
    Box2Obj_ShowPokeOutline(syswk, FALSE);
    if (syswk->app->pokePutKey == BOX2_GET_NONE) {
        return Box2Seq_ItemPutBackKey(syswk);
    }
    if (Box2Main_PokeItemMoveCheck(syswk, syswk->pos, pos) == FALSE) {
        return Box2Seq_ItemPutBackKey(syswk);
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangePutKey, BOX2SEQ_ITEM_DROP_KEY_PARTY_OUT);
}

// The item arrangement in the party: takes the item of a touched Pokémon
static int Box2Seq_ItemPartyTakeTouched(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->pos = pos;
    syswk->getTray = BOX2_GET_NONE;
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Frame_HideMenu(syswk->app->bgWinFrame);
    Box2Bmp_ClearMsgWindow(syswk->app, BOX2_WIN_MESSAGE);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->app->getItem != 0) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, BOX2_ICON_POS_PARTY_LEFT);
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Obj_SetButtons(syswk, BOX2_BUTTON_DISABLED, BOX2_BUTTON_DISABLED, BOX2_BUTTON_HIDDEN, BOX2_BUTTON_HIDDEN);
    } else if (Box2Obj_IsVisible(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_ShowPokeOutline(syswk, FALSE);
    }
    Box2Obj_GetIconPos(syswk->pos, &x, &y, BOX2_ICON_POS_PARTY_LEFT);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    AllocItemMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeBoxPartyGetTouch, BOX2SEQ_ITEM_PARTY_DROP);
}

// The item arrangement in the party: opens the menu of a picked Pokémon
static int Box2Seq_ItemPartyOpenMenu(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->menuMsgPending = 1;
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Seq_OpenItemMenu(syswk, (s16)syswk->app->getItem);
    Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
    if (syswk->app->getItem != 0) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, BOX2_ICON_POS_PARTY_LEFT);
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Ui_StartCursorMove(syswk->app, BOX2_ITEM_PARTY_INPUT_MOVE, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_PARTY_INPUT_MOVE);
        syswk->app->oldCurPos = BOX2_ITEM_PARTY_INPUT_MOVE;
    } else {
        Box2Ui_StartCursorMove(syswk->app, BOX2_ITEM_PARTY_INPUT_BAG, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_ITEM_PARTY_INPUT_BAG);
        syswk->app->oldCurPos = BOX2_ITEM_PARTY_INPUT_BAG;
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuClose,
                            Box2Seq_PokeMoveEndTo(syswk, BOX2SEQ_ITEM_PARTY_MAIN));
}

// Allocates the work of an item icon's move
static void AllocItemMoveWork(Box2SysWork *syswk) {
    syswk->app->vfunk.work = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2ItemMoveWork), FALSE, "box2_seq.c", 11181);
}

static void FreeItemMoveWork(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
}

static void PutPartyLeft(Box2SysWork *syswk) {
    Box2Frame_PutPartyLeft(syswk->app->bgWinFrame);
    Box2Obj_PutPartyIconsLeft(syswk);
}

static void PutPartyRight(Box2SysWork *syswk) {
    Box2Frame_PutPartyRight(syswk->app->bgWinFrame);
    Box2Obj_PutPartyIconsRight(syswk);
}

void Box2Seq_PutPartyLeft(Box2SysWork *syswk) {
    Box2Obj_PutPartyIconsLeft(syswk);
    Box2Frame_PutPartyLeft(syswk->app->bgWinFrame);
}

// Scrolls to the box picked in the list
static int Box2Seq_ScrollToPickedBox(Box2SysWork *syswk, int seq) {
    BOOL right = Box2Main_IsTrayScrollRight(syswk, syswk->tray, syswk->app->pickedTray);
    Box2VFunc func;

    syswk->tray = syswk->app->pickedTray;
    Box2Obj_LoadTrayPokeIcons(syswk, syswk->tray);
    Box2Main_WallPaperSet(syswk, Box2Main_GetWallPaperNumber(syswk, syswk->tray), right);
    if (right == TRUE) {
        func = Box2Main_VFuncTrayScrollRight;
    } else {
        func = Box2Main_VFuncTrayScrollLeft;
    }
    syswk->nextSeq = seq;
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    return Box2Seq_RunVFunc(syswk, func, syswk->nextSeq);
}

// Allocates the work of a drag of the box list
static void AllocBoxListDragWork(Box2SysWork *syswk) {
    Box2BoxListDrag *work =
        GFL_HeapAllocate(HEAPID_TAIL(HEAPID_BOX2_APP), sizeof(Box2BoxListDrag), FALSE, "box2_seq.c", 11290);

    work->cnt = 0;
    work->dir = 0;
    syswk->app->subWork = work;
}

// Moves the party's frame in or out, then goes to seq
static int Box2Seq_MovePartyFrame(Box2SysWork *syswk, int seq) {
    Box2Main_OpenBoxList(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListSlideOut, seq);
}

// Moves the party's frame, then goes to seq
static int Box2Seq_MovePartyFrameTo(Box2SysWork *syswk, int seq) {
    Box2Frame_SlideBoxListOut(syswk->app->bgWinFrame);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListSlideIn, seq);
}

// Opens the item arrangement's menu, with or without a held item
void Box2Seq_OpenItemMenu(Box2SysWork *syswk, s16 item) {
    if (item != 0) {
        if (syswk->pos < BOX2_PARTY_POS) {
            CursorMove_EnablePos(syswk->app->cursorMove, BOX2_ITEM_TRAY_INPUT_MOVE);
        } else {
            CursorMove_EnablePos(syswk->app->cursorMove, BOX2_ITEM_PARTY_INPUT_MOVE);
        }
        Box2Bmp_OpenMenu(syswk, sHeldItemMenu, 3);
    } else {
        if (syswk->pos < BOX2_PARTY_POS) {
            CursorMove_DisablePos(syswk->app->cursorMove, BOX2_ITEM_TRAY_INPUT_MOVE);
        } else {
            CursorMove_DisablePos(syswk->app->cursorMove, BOX2_ITEM_PARTY_INPUT_MOVE);
        }
        Box2Bmp_OpenMenu(syswk, sGiveItemMenu, 2);
    }
}

// Deposit mode: whether the party Pokémon can be deposited: 0 if so, 1 for the last battler, 2 for mail
static u32 GetDepositBlock(Box2SysWork *syswk) {
    // The party position, then the result
    u32 val = syswk->pos - BOX2_PARTY_POS;

    if (Box2Main_BattlePokeCheck(syswk, val) == FALSE) {
        return BOX2_DEPOSIT_LAST_BATTLER;
    }
    if (PML_ItemIsMail(PokeParty_GetParam(PokeParty_GetPkm(syswk->param->party, val), PKM_PARAM_ITEM, NULL)) == TRUE) {
        val = BOX2_DEPOSIT_MAIL;
    } else {
        val = BOX2_DEPOSIT_ALLOWED;
    }
    return val;
}

// Toggles a mark
static void ToggleMark(Box2SysWork *syswk, u32 mark) {
    syswk->app->markings ^= 1 << mark;
    Box2Main_MarkingPutMain(syswk, syswk->app->markings);
}

// Picks a wallpaper of the theme's menu
static int Box2Seq_PickWallpaper(Box2SysWork *syswk, u32 pos) {
    if (syswk->app->wallpaperTheme <= BOX2_WALLPAPER_MISC ||
        (syswk->app->wallpaperTheme == BOX2_WALLPAPER_SPECIAL_1 &&
         BoxSaveAccessor_CheckFlags(syswk->param->boxes, BOX2_WALLPAPER_UNLOCK_SPECIAL_2) == FALSE)) {
        syswk->app->wallpaperPos = pos + (syswk->app->wallpaperTheme - 1) * 4 - 1;
    } else {
        syswk->app->wallpaperPos = pos + (syswk->app->wallpaperTheme - 1) * 4;
    }
    return Box2Seq_AnimFrameButton(syswk, pos, Box2Seq_Goto(syswk, BOX2SEQ_WALLPAPER_CHANGE));
}

// Dims the actors behind the continue-operations prompt
static void DimActorsForContinuePrompt(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case BOX2_MODE_DEPOSIT:
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, TRUE);
        break;
    case BOX2_MODE_WITHDRAW:
    case BOX2_MODE_MOVE_ITEMS:
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        break;
    case BOX2_MODE_MOVE_POKEMON:
    case BOX2_MODE_BATTLE_BOX:
    case BOX2_MODE_DREAM_WORLD:
        Box2Obj_ShowScrollArrows(syswk->app, FALSE);
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_PARTY_AND_HELD, TRUE);
        } else {
            Box2Obj_SetIconsBlend(syswk, BOX2_ICON_GROUP_TRAY, TRUE);
        }
        break;
    }
}

// Switches to the next way of moving Pokémon, by Select
static int Box2Seq_NextMoveMode(Box2SysWork *syswk, int seq) {
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    syswk->fromMenu = 0;
    if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
        syswk->moveMode = BOX2_MOVE_MODE_MENU;
    } else {
        syswk->moveMode++;
        if (syswk->moveMode == BOX2_MOVE_MODE_RANGE && syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            syswk->moveMode = BOX2_MOVE_MODE_MENU;
        }
    }
    if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
        syswk->fromMenu = 1;
    }
    syswk->moveDone = 0;
    Box2Main_ShowCursor(syswk);
    return Box2Seq_Goto(syswk, seq);
}

// Sets the way of moving Pokémon picked in the menu
static int Box2Seq_SetMoveMode(Box2SysWork *syswk, int seq) {
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    syswk->fromMenu = 0;
    if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
        syswk->fromMenu = 1;
    }
    syswk->moveDone = 0;
    return Box2Seq_Goto(syswk, seq);
}

// Cancels a range pick
static void CancelRange(Box2SysWork *syswk) {
    Box2Obj_SetRangeAnm(syswk, 0);
    syswk->pos = BOX2_GET_NONE;
    syswk->pickState = BOX2_PICK_NONE;
    Box2Ui_SetButtonStates(syswk, CursorMove_GetPos(syswk->app->cursorMove));
}

// Puts a range's ends in order
static void SortRangeEnds(Box2SysWork *syswk) {
    if (syswk->app->rangeSelect.startPos > syswk->app->rangeSelect.endPos) {
        u8 end = syswk->app->rangeSelect.endPos;

        syswk->app->rangeSelect.endPos = syswk->app->rangeSelect.startPos;
        syswk->app->rangeSelect.startPos = end;
    }
}
