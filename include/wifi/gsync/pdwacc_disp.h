#ifndef POKEBW2_WIFI_GSYNC_PDWACC_DISP_H
#define POKEBW2_WIFI_GSYNC_PDWACC_DISP_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Dream World account screens' display (pdwacc_disp.c): their BGs, with a palette cycle and a scrolling top screen

PdwAccDisp *PdwAccDisp_Create(HeapID heapId);
void PdwAccDisp_Main(PdwAccDisp *disp);
void PdwAccDisp_Free(PdwAccDisp *disp);

#endif // POKEBW2_APP_GSYNC_PDWACC_DISP_H
