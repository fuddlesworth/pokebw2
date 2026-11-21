#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_CALL_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_CALL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Xtransceiver's calls: the list of the machines nearby, joining one's call or calling up to three, and waiting
// for an answer

CtvtCall *CtvtCall_Create(CommTvtWork *sys, HeapID heapId);
void CtvtCall_Delete(CommTvtWork *sys, CtvtCall *call);
void CtvtCall_Enter(CommTvtWork *sys, CtvtCall *call);
void CtvtCall_Leave(CommTvtWork *sys, CtvtCall *call);
// Returns the mode to go on to, COMM_TVT_MODE_CALL to stay
int CtvtCall_Main(CommTvtWork *sys, CtvtCall *call);
// Whether mac is a machine of the list that runs Black or White
BOOL CtvtCall_IsBlackWhite(CommTvtWork *sys, CtvtCall *call, const u8 *mac);

#endif // POKEBW2_APP_COMM_TVT_CTVT_CALL_H
