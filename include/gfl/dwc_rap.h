#ifndef POKEBW2_GFL_DWC_RAP_H
#define POKEBW2_GFL_DWC_RAP_H

#include "types.h"
#include "gfl/heap.h"

// dwc_rap.c, in overlay 11: the GFL net's Wi-Fi connection, over Nintendo's DWC library

// Asked about a connection event, with its work and three of the event's values
typedef u32 (*DWCRapEventFunc)(void *work, int a1, int event, int a3);

void func_ov011_021516a0(BOOL a0);
// Sets the function called when the connection is lost
void func_ov011_02152040(void (*func)(void *work, int a1, int code), void *work);
// Sets a function asked about connection events, with its work
void func_ov011_0215205c(DWCRapEventFunc func, void *work);

#endif // POKEBW2_GFL_DWC_RAP_H
