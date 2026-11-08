#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_BTN_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_BTN_H

#include "types.h"
#include "app/battle_recorder/br_res.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "struct_decls.h"
#include "system/bmp_oam.h"
#include "system/printsys.h"

// The Battle Recorder's menu buttons (br_btn.c), and the button objects its screens use

#define BR_BTN_SYS_STACK_MAX 6

typedef struct {
    u16 menuID;
    u16 btnID;
} BrBtnStack;

// The menus that were open, kept while another screen runs
typedef struct {
    BrBtnStack stack[BR_BTN_SYS_STACK_MAX];
    u32 stack_num;
} BrBtnRecovery;

// What BrBtnSys_GetInput returns once a button has been pressed
enum {
    BR_BTN_INPUT_NONE,
    // A button that starts a screen
    BR_BTN_INPUT_SELECT,
    // The button that leaves the Battle Recorder
    BR_BTN_INPUT_EXIT,
};

// A button: a cell actor with its label in a bitmap OAM
struct BrBtn {
    ClActor *clwk;
    BmpOamActor *oam;
    GFLBitmap *bmp;
    u32 unkC;
    u32 display;
    u32 width;
};

BrBtnSys *BrBtnSys_Init(int menuID, ClActUnit *unit, BrRes *res, BrRecordInfo *recordInfo, BrBtnRecovery *recovery,
                        BrBallEffect *ballEff, HeapID heapId);
void BrBtnSys_Exit(BrBtnSys *p_wk);
void BrBtnSys_Main(BrBtnSys *p_wk);
// BR_BTN_INPUT_*, with the pressed button's BR_BTN_DATA_PARAM_DATA1 and DATA2
u32 BrBtnSys_GetInput(const BrBtnSys *cp_wk, u32 *p_data1, u32 *p_data2);
BOOL BrBtnSys_IsBusy(const BrBtnSys *cp_wk);
u32 BrBtnSys_GetExitResult(const BrBtnSys *cp_wk);

// A button labelled with the message msgID of msg
BrBtn *BrBtn_InitEx(const ClActorSetup *setup, u16 msgID, u16 width, u32 display, ClActUnit *unit, BmpOamSys *bmpoam,
                    Font *font, MsgData *msg, const BrResObjData *obj, HeapID heapId);
BrBtn *BrBtn_Init(const ClActorSetup *setup, const StrBuf *str, u16 width, u32 display, ClActUnit *unit,
                  BmpOamSys *bmpoam, Font *font, const BrResObjData *obj, HeapID heapId);
void BrBtn_Exit(BrBtn *p_wk);
// Whether the touch is on the button, playing the press sound if it is
BOOL BrBtn_GetTrg(const BrBtn *cp_wk, u32 x, u32 y);
BOOL BrBtn_GetHit(const BrBtn *cp_wk, u32 x, u32 y);

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_BTN_H
