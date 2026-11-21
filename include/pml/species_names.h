#ifndef POKEBW2_PML_SPECIES_NAMES_H
#define POKEBW2_PML_SPECIES_NAMES_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The species names' message data (species_names.c), loaded once by PML_SpeciesNamesResidentInit
extern MsgData *g_PMLSpeciesNamesResident;

void PML_SpeciesNamesResidentInit(HeapID heapId);

#endif // POKEBW2_PML_SPECIES_NAMES_H
