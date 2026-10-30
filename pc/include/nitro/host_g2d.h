#ifndef POKEBW2_HOST_G2D_H
#define POKEBW2_HOST_G2D_H
#include "nnsys/g2d.h"
NNSG2dBinaryBlockHeader *PcG2d_ImportBlock(void *owner, const NNSG2dBinaryBlockHeader *block);
void PcG2d_Release(void *owner);
#endif
