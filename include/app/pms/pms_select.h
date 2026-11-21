#ifndef POKEBW2_APP_PMS_PMS_SELECT_H
#define POKEBW2_APP_PMS_PMS_SELECT_H

#include "types.h"
#include "struct_decls.h"
#include "system/pms_data.h"
#include "system/savedata/save_control.h"

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

#endif // POKEBW2_APP_PMS_SELECT_H
