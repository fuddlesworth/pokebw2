#ifndef POKEBW2_APP_MB_PARENT_MB_PARENT_H
#define POKEBW2_APP_MB_PARENT_MB_PARENT_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The parent of a DS Download Play session (ov181, mb_parent_sys.c), which the Poké Transfer Lab starts for Poké
// Transfer, and the start menu's item for what appears to be the Pokémon Dream Radar's transfer
typedef struct {
    // TRUE from the start menu, FALSE from the Poké Transfer Lab
    u8 startMenu;
    // Set by the Poké Transfer Lab
    GameData *gameData;
} MBParentParam;

extern GameProcFunctions MB_PARENT_PROC_FUNCTIONS;

// The DS Download Play parent of Unova Link's Memory Link (mb_parent_dataconv_sys.c). The names are ours

// What MBDataConv_Request starts or changes
enum {
    MB_DATACONV_REQUEST_DISTRIBUTE,
    MB_DATACONV_REQUEST_HOLD_REBOOT,
    MB_DATACONV_REQUEST_CANCEL,
    MB_DATACONV_REQUEST_CONNECT,
};

// MBDataConv_GetResult: 0 when the data arrived, else why not
enum {
    MB_DATACONV_RESULT_OK,
    MB_DATACONV_RESULT_TIMEOUT,
    MB_DATACONV_RESULT_CHILD_ERROR_1,
    MB_DATACONV_RESULT_CHILD_ERROR_2,
    MB_DATACONV_RESULT_CHILD_ERROR_3,
};

MBDataConv *MBDataConv_Create(HeapID heapId);
void MBDataConv_Delete(MBDataConv *conv);
void MBDataConv_Update(MBDataConv *conv);
BOOL MBDataConv_Request(MBDataConv *conv, u32 request);
// Whether the last request has finished
BOOL MBDataConv_IsIdle(MBDataConv *conv);
// The name and the introduction of the program that the children see
void MBDataConv_SetGameInfo(MBDataConv *conv, StrBuf *name, StrBuf *intro);
u32 MBDataConv_GetResult(MBDataConv *conv);
// The save data that the child sent back
void *MBDataConv_GetReceivedData(MBDataConv *conv);

#endif // POKEBW2_APP_MB_PARENT_H
