#ifndef POKEBW2_APP_PMS_INPUT_H
#define POKEBW2_APP_PMS_INPUT_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The phrase input, overlay 185's pms_input.c: the work that the view and the screens' parts read. The names are
// ours, guessed

// The proc's functions, which overlay 215's own table also points at
BOOL PMSInput_Init(GameProc *proc, u32 *state, void *param, void *work);
BOOL PMSInput_Main(GameProc *proc, u32 *state, void *param, void *work);
BOOL PMSInput_Exit(GameProc *proc, u32 *state, void *param, void *work);

// Where the input is, keys or touch
int *PMSInput_GetKeyModePtr(const PMSInputWork *wk);
u32 PMSInput_GetInputMode(const PMSInputWork *wk);
u16 PMSInput_GetSentenceType(const PMSInputWork *wk);
// The word at a place of the sentence, or the word being written
u16 PMSInput_GetEditWord(const PMSInputWork *wk, u32 index);
// The sentence's text, with its words as commands
StrBuf *PMSInput_GetEditSourceString(const PMSInputWork *wk, HeapID heapId);
// Whether the categories are listed by initial instead of by group
u32 PMSInput_GetCategoryMode(const PMSInputWork *wk);
u32 PMSInput_GetEditAreaCursorPos(const PMSInputWork *wk);
u32 PMSInput_GetButtonCursorPos(const PMSInputWork *wk);
u32 PMSInput_GetCategoryCursorPos(const PMSInputWork *wk);
// Where the categories' cursor was before the erase button was pressed
u32 PMSInput_GetCategoryPosSaved(const PMSInputWork *wk);
// The words of the chosen group or of the search, and the two places of the word window's buttons
u32 PMSInput_GetCategoryWordMax(const PMSInputWork *wk);
void PMSInput_GetCategoryWord(const PMSInputWork *wk, u32 index, StrBuf *buf);
u32 PMSInput_GetWordWinCursorPos(const PMSInputWork *wk);
int PMSInput_GetWordWinScrollVector(const PMSInputWork *wk);
BOOL PMSInput_GetWordWinUpArrowVisible(const PMSInputWork *wk);
BOOL PMSInput_GetWordWinDownArrowVisible(const PMSInputWork *wk);
BOOL PMSInput_HasStartSentence(const PMSInputWork *wk);
TCBManager *PMSInput_GetTCBManager(const PMSInputWork *wk);
// The word window's first line and its last first line
void PMSInput_GetWordWinScroll(const PMSInputWork *wk, u16 *top, u16 *scrollMax);
// Whether every word of the sentence is filled in
BOOL PMSInput_IsEditComplete(const PMSInputWork *wk);
// The search's letters and results, through pmsi_search.c
void PMSInput_GetSearchInputStr(const PMSInputWork *wk, StrBuf *buf);
void PMSInput_ResetSearch(const PMSInputWork *wk);
u32 PMSInput_GetSearchResultCount(const PMSInputWork *wk);
u32 PMSInput_GetSearchInputLen(const PMSInputWork *wk);
void PMSInput_GetSearchResultStr(const PMSInputWork *wk, u32 index, StrBuf *buf);
// Puts the categories' cursor on the first category
void PMSInput_ResetCategoryPos(PMSInputWork *wk);

#ifdef POKEBW2_PC
enum { PMS_NATIVE_EDIT, PMS_NATIVE_CATEGORIES, PMS_NATIVE_WORDS };
typedef struct {
    Font *font;
    u32 phase, mode, categoryMode, category, cursor, count, editCursor, editCount, button;
    BOOL ready, buttonFocused, complete, fixedSentence;
    u16 preview[256];
    u16 words[2][64];
} PMSInputNativeState;
PMSInputWork *PMSInput_GetNativeWork(void);
BOOL PMSInput_GetNativeState(PMSInputNativeState *state);
BOOL PMSInput_SelectNative(u32 index);
void PMSInput_NativeConfirm(BOOL cancel);
BOOL PMSInput_GetNativeCategory(u32 index, StrBuf *label);
#endif

#endif // POKEBW2_APP_PMS_INPUT_H
