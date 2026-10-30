#ifndef POKEBW2_APP_PMS_SELECT_H
#define POKEBW2_APP_PMS_SELECT_H

#include "types.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/pms_data.h"

// pms_select.c: the phrase select of overlay 185, with which the trainer card picks its greeting from the saved
// phrases, and edits them with the phrase input. The names are ours

typedef struct {
    SaveControl *save;
    // Set when the player backs out, and result is NULL
    BOOL cancel;
    // The phrase chosen
    PMSData *result;
} PMSSelectParam;

extern const GameProcFunctions PMS_SELECT_PROC_FUNCTIONS;

#ifdef POKEBW2_PC
typedef struct {
    Font *font;
    AppTaskMenu *menu;
    u32 cursor, top, count;
    BOOL ready;
    u16 phrases[3][256];
    BOOL empty[3];
} PMSSelectNativeState;
BOOL PMSSelect_IsNativeActive(void);
BOOL PMSSelect_IsNativeRunning(void);
BOOL PMSSelect_GetNativeState(PMSSelectNativeState *state);
BOOL PMSSelect_NativeSelect(u32 index);
#endif

#endif // POKEBW2_APP_PMS_SELECT_H
