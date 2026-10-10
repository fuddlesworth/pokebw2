#ifndef POKEBW2_GFL_WIH_H
#define POKEBW2_GFL_WIH_H

#include "types.h"
#include "gfl/heap.h"

// wih.c, in overlay 30: the wireless helper, which grew out of NitroSDK's demo wh.c; the DS Download Play parent uses it to
// pick a channel and to shut the wireless down. The comments give the wh.c functions these appear to be

// The filter that a scan passes each machine it finds to, with the filter's work (the GFL net work)
typedef BOOL (*NetScanFilter)(const void *info, void *work);
// How the scan filters the machines it finds, and how long a parent scans for children
void func_ov030_02175334(NetScanFilter filter);
void func_ov030_02175658(u16 time);

// The helper's states (WH_SYSSTATE_*)
enum {
    WH_SYSSTATE_STOP,
    WH_SYSSTATE_IDLE,
    WH_SYSSTATE_SCANNING,
    WH_SYSSTATE_BUSY,
    WH_SYSSTATE_CONNECTED,
    WH_SYSSTATE_DATASHARING,
    WH_SYSSTATE_KEYSHARING,
    WH_SYSSTATE_MEASURECHANNEL,
    WH_SYSSTATE_CONNECT_FAIL,
    WH_SYSSTATE_ERROR,
    WH_SYSSTATE_FATAL,
};

// Called when the wireless has ended, with whether that succeeded. The callers ignore its result
typedef BOOL (*WHCallback)(BOOL success);

int func_ov030_02174e58(void);  // WH_GetSystemState
BOOL func_ov030_02174e90(void); // WH_StartMeasureChannel
u16 func_ov030_02175030(void);  // WH_GetMeasureChannel
// Starts the helper with its work on the heap given
BOOL func_ov030_021750f0(HeapID heapId, WHCallback callback, u32 unused);
// Frees the helper's work
void func_ov030_02175164(void);
BOOL func_ov030_021754a0(void);                // WH_Finalize
BOOL func_ov030_02175578(WHCallback callback); // WH_End

#endif // POKEBW2_GFL_WIH_H
