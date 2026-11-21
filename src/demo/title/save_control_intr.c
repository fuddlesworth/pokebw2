#include "types.h"
#include "gfl/backup_system.h"
#include "gfl/heap.h"
#include "gfl/ui.h"
#include "save/save_control.h"
#include "save/save_control_intr.h"

// Saving a new game in the background. A soft reset cancels it, and it does nothing if save data is already present

enum {
    STATE_IDLE,
    STATE_SAVING,
    STATE_PAUSED,
    STATE_DONE,
};

struct SaveControlIntr {
    SaveControl *save;
    u8 state;
    // The state to go back to after a pause
    u8 resumeState;
    u8 pauseRequested;
    u8 disabled;
    // The save control's flags, cleared while saving and put back afterwards
    u32 giftsLoaded;
    u32 giftsBroken;
    u32 keysLoaded;
    u32 keysBroken;
};

static void SaveControlIntr_OnSoftReset(void *work);

SaveControlIntr *SaveControlIntr_Create(HeapID heapId, SaveControl *save) {
    SaveControlIntr *intr = GFL_HeapAllocate(heapId, sizeof(SaveControlIntr), TRUE, "save_control_intr.c", 71);

    intr->save = save;
    if (SaveControl_IsDataAlreadyPresent(save) == TRUE) {
        intr->disabled = TRUE;
    } else {
        GCTX_HIDSetSoftResetCallback(SaveControlIntr_OnSoftReset, intr);
    }
    return intr;
}

void SaveControlIntr_Free(SaveControlIntr *intr) {
    GCTX_HIDSetSoftResetCallback(NULL, NULL);
    GFL_HeapFree(intr);
}

void SaveControlIntr_Start(SaveControlIntr *intr) {
    if (intr->disabled == TRUE) {
        return;
    }
    SaveControl_StartSave(intr->save);
    if (intr->disabled == FALSE) {
        TrainerDataSave_SetIntroSave(intr->save);
        intr->giftsLoaded = SaveControl_GetGiftsLoaded(intr->save);
        intr->giftsBroken = SaveControl_GetGiftsBroken(intr->save);
        intr->keysLoaded = SaveControl_GetKeysLoaded(intr->save);
        intr->keysBroken = SaveControl_GetKeysBroken(intr->save);
        SaveControl_SetGiftsLoaded(intr->save, FALSE);
        SaveControl_SetGiftsBroken(intr->save, FALSE);
        SaveControl_SetKeysLoaded(intr->save, FALSE);
        SaveControl_SetKeysBroken(intr->save, FALSE);
    }
    intr->state = STATE_SAVING;
}

u32 SaveControlIntr_Update(SaveControlIntr *intr) {
    u32 result;

    if (intr->disabled == TRUE) {
        return SAVE_RESULT_OK;
    }
    switch (intr->state) {
    case STATE_SAVING:
        if (intr->pauseRequested == TRUE && getLockIDStatus_inline_stub() == FALSE) {
            intr->resumeState = intr->state;
            intr->state = STATE_PAUSED;
            return SAVE_RESULT_CONTINUE;
        }
        result = SaveControl_ContinueSave(intr->save);
        if (result - SAVE_RESULT_OK <= 1) {
            intr->state = STATE_DONE;
            SaveControl_SetGiftsLoaded(intr->save, intr->giftsLoaded);
            SaveControl_SetGiftsBroken(intr->save, intr->giftsBroken);
            SaveControl_SetKeysLoaded(intr->save, intr->keysLoaded);
            SaveControl_SetKeysBroken(intr->save, intr->keysBroken);
        }
        return result;
    case STATE_PAUSED:
        if (intr->pauseRequested == FALSE) {
            intr->state = intr->resumeState;
        }
        return SAVE_RESULT_CONTINUE;
    case STATE_DONE:
        return SAVE_RESULT_OK;
    }
    return SAVE_RESULT_CONTINUE;
}

void SaveControlIntr_Pause(SaveControlIntr *intr) {
    if (intr->state != STATE_DONE) {
        intr->pauseRequested = TRUE;
    }
}

void SaveControlIntr_Resume(SaveControlIntr *intr) {
    if (intr->state != STATE_DONE) {
        intr->pauseRequested = FALSE;
    }
}

BOOL SaveControlIntr_IsPausedOrDone(SaveControlIntr *intr) {
    if (intr->disabled == TRUE || intr->state == STATE_DONE) {
        return TRUE;
    }
    if (intr->state == STATE_PAUSED) {
        return TRUE;
    }
    return FALSE;
}

void SaveControlIntr_Nop(SaveControlIntr *intr) {
}

BOOL SaveControlIntr_IsDone(SaveControlIntr *intr) {
    if (intr->disabled == TRUE) {
        return TRUE;
    }
    if (intr->state == STATE_DONE) {
        return TRUE;
    }
    return FALSE;
}

static void SaveControlIntr_OnSoftReset(void *work) {
    SaveControlIntr *intr = work;

    if (intr->disabled == FALSE && intr->state != STATE_DONE) {
        SaveControl_CancelSave(intr->save);
        intr->disabled = TRUE;
    }
}
