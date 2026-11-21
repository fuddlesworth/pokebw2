#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_DRAW_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_DRAW_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Xtransceiver's drawing mode, over the video of the top screen

CtvtDraw *CtvtDraw_Create(CommTvtWork *sys, HeapID heapId);
void CtvtDraw_Delete(CommTvtWork *sys, CtvtDraw *draw);
void CtvtDraw_Enter(CommTvtWork *sys, CtvtDraw *draw);
void CtvtDraw_Leave(CommTvtWork *sys, CtvtDraw *draw);
// Returns the mode to go on to, COMM_TVT_MODE_DRAW to stay
int CtvtDraw_Main(CommTvtWork *sys, CtvtDraw *draw);

#endif // POKEBW2_APP_COMM_TVT_CTVT_DRAW_H
