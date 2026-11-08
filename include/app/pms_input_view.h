#ifndef POKEBW2_APP_PMS_INPUT_VIEW_H
#define POKEBW2_APP_PMS_INPUT_VIEW_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The phrase input's screens, overlay 185's pms_input_view.c: the commands that the input sends them, run as tasks,
// and the graphics its parts share. The names are ours, guessed

// The commands
enum {
    PMSIV_CMD_INIT,
    PMSIV_CMD_QUIT,
    PMSIV_CMD_FADEIN,
    PMSIV_CMD_UPDATE_EDITAREA,
    PMSIV_CMD_KTCHANGE_EDITAREA,
    PMSIV_CMD_KTCHANGE_CATEGORY,
    PMSIV_CMD_KTCHANGE_WORDWIN,
    PMSIV_CMD_EDITAREA_TO_BUTTON,
    PMSIV_CMD_BUTTON_TO_EDITAREA,
    PMSIV_CMD_BUTTON_TO_EDITAREA_SELECT,
    PMSIV_CMD_EDITAREA_TO_CATEGORY,
    PMSIV_CMD_CATEGORY_TO_EDITAREA,
    PMSIV_CMD_CATEGORY_TO_WORDWIN,
    PMSIV_CMD_WORDWIN_TO_CATEGORY,
    PMSIV_CMD_WORDWIN_TO_EDITAREA,
    PMSIV_CMD_WORDWIN_TO_BUTTON,
    PMSIV_CMD_MOVE_EDITAREA_CURSOR,
    PMSIV_CMD_MOVE_BUTTON_CURSOR,
    PMSIV_CMD_MOVE_CATEGORY_CURSOR,
    PMSIV_CMD_MOVE_WORDWIN_CURSOR,
    PMSIV_CMD_SCROLL_WORDWIN,
    PMSIV_CMD_PUSH_BUTTON,
    PMSIV_CMD_NOP_22,
    PMSIV_CMD_UPDATE_EDITAREA_CURSOR,
    PMSIV_CMD_CHANGE_CATEGORY_MODE_DISABLE,
    PMSIV_CMD_CHANGE_CATEGORY_MODE_ENABLE,
    PMSIV_CMD_NOP_26,
    PMSIV_CMD_NOP_27,
    PMSIV_CMD_NOP_28,
    PMSIV_CMD_NOP_29,
    PMSIV_CMD_CHANGE_CATEGORY_MODE,
    PMSIV_CMD_REDRAW_WORDWIN,
    PMSIV_CMD_FLASH_SEARCH_ERASE,
    PMSIV_CMD_MOVE_CATEGORY,
    PMSIV_CMD_MOVE_WORDWIN,
    PMSIV_CMD_COUNT,
};

// The resources of a set of OBJ graphics, loaded for each screen
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
} PMSIVObjRes;

PMSInputView *PMSIView_Create(const PMSInputWork *mwk, const PMSInputData *dwk);
void PMSIView_Delete(PMSInputView *vwk);
TCB *PMSIView_AddVTask(TCBFunc func, void *wk, u32 priority);
void PMSIView_SetCommand(PMSInputView *vwk, int cmd);
BOOL PMSIView_WaitCommandAll(PMSInputView *vwk);
BOOL PMSIView_WaitCommand(PMSInputView *vwk, int cmd);
u32 PMSIView_GetSentenceEditPosMax(PMSInputView *vwk);
u16 PMSIView_GetSentenceWord(PMSInputView *vwk, u32 index);
void PMSIView_GetSentenceWordArea(PMSInputView *vwk, TouchRect *rect, u8 index);
// Returns 1 when the edit button flash ends, otherwise -1.
int PMSIView_GetButtonFlashResult(PMSInputView *vwk);
void PMSIView_SetSentenceArrow(PMSInputView *vwk, u32 which);
PMSIVMenu *PMSIView_GetMenu(PMSInputView *vwk);
ClActUnit *PMSIView_GetActUnit(PMSInputView *vwk);
Font *PMSIView_GetFont(PMSInputView *vwk);
PrintQueue *PMSIView_GetPrintQueue(PMSInputView *vwk);
void PMSIView_GetObjRes2(PMSInputView *vwk, PMSIVObjRes *res, u32 lcd);
// The resources of a screen's OBJ graphics. bgPriority is not used
void PMSIView_GetObjRes(PMSInputView *vwk, PMSIVObjRes *res, u32 lcd, u32 bgPriority);
ClActor *PMSIView_AddActor(PMSInputView *vwk, const PMSIVObjRes *res, u32 x, u32 y, u32 priority, int drawArea);
int PMSIView_GetWordWinScrollDir(PMSInputView *vwk, u32 unused, u32 pos);
void PMSIView_SetWordWinScrollBarY(PMSInputView *vwk, u32 y);
u32 PMSIView_GetWordWinScrollBarPos(PMSInputView *vwk, u32 count);
// Loads the lower screen's background, the one with the category buttons or the plain one
void PMSIView_SetLowerScreen(PMSInputView *vwk, BOOL categories);
void PMSIView_ChangeKTEditArea(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk);
void PMSIView_ChangeKTCategory(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk);
void PMSIView_ChangeKTWordWin(PMSInputView *vwk, const PMSInputWork *mwk, const PMSInputData *dwk);

// The flash-result provider returns only these two values; 0 is not produced.
enum {
    PMSIV_BUTTON_FLASH_PENDING = -1,
    PMSIV_BUTTON_FLASH_DONE = 1,
};

// Pointer Y compared with the scroll bar Y; EQUAL is exact equality, not a thumb-extent hit.
enum {
    PMSIV_SCROLL_Y_EQUAL = 0,
    PMSIV_SCROLL_Y_ABOVE = 1,
    PMSIV_SCROLL_Y_BELOW = 2,
    PMSIV_SCROLL_Y_UNAVAILABLE = 3,
};

#endif // POKEBW2_APP_PMS_INPUT_VIEW_H
