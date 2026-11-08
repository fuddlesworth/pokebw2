#ifndef POKEBW2_SAVE_SAVE_CONTROL_INTR_H
#define POKEBW2_SAVE_SAVE_CONTROL_INTR_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Creates a new game's save data a step at a time while the intro runs, pausing around the name entries (ov162)

typedef struct SaveControlIntr SaveControlIntr;

SaveControlIntr *SaveControlIntr_Create(HeapID heapId, SaveControl *save);
void SaveControlIntr_Free(SaveControlIntr *intr);
void SaveControlIntr_Start(SaveControlIntr *intr);
// While saving, returns the save control's SAVE_RESULT_*. Idle/paused return CONTINUE;
// an already-finished or disabled task returns OK, even after an earlier NG.
u32 SaveControlIntr_Update(SaveControlIntr *intr);
void SaveControlIntr_Pause(SaveControlIntr *intr);
void SaveControlIntr_Resume(SaveControlIntr *intr);
BOOL SaveControlIntr_IsPausedOrDone(SaveControlIntr *intr);
void SaveControlIntr_Nop(SaveControlIntr *intr);
BOOL SaveControlIntr_IsDone(SaveControlIntr *intr);

#endif // POKEBW2_SAVE_SAVE_CONTROL_INTR_H
