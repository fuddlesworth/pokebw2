#ifndef POKEBW2_GFL_DWC_RAPCOMMON_H
#define POKEBW2_GFL_DWC_RAPCOMMON_H

#include "types.h"
#include "gfl/heap.h"

// dwc_rapcommon.c, in overlay 11: what the Wi-Fi connection shares with the applications that use it

void func_ov011_021520a0(u32 a0, u32 size, HeapID heapId);
void func_ov011_02152158(void);
// A state of the connection from 0 to 4, which callers switch on
int func_ov011_02152404(u32 a0, u32 a1);

#endif // POKEBW2_GFL_DWC_RAPCOMMON_H
