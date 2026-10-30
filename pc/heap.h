#ifndef POKEBW2_PC_HEAP_H
#define POKEBW2_PC_HEAP_H

// Release all native game allocations, including independent root heaps, after host services stop.
void PcHeap_Shutdown(void);

#endif
