#include "g2di_load.h"
#include "nnsys/g2d.h"
#ifdef POKEBW2_PC
#include "nitro/host_g2d.h"
#endif

// What NitroSystem's G2D file loaders share: finding a block in a loaded file, and the user extended cell attributes of
// cell and multi-cell banks. The file name is NitroSystem's, as ntrtwl's reconstruction of NitroSystem has it

NNSG2dBinaryBlockHeader *NNS_G2dFindBinaryBlock(NNSG2dBinaryFileHeader *file, u32 kind) {
    NNSG2dBinaryBlockHeader *blk = (NNSG2dBinaryBlockHeader *)((u8 *)file + file->headerSize);
    u16 i;

    for (i = 0; i < file->dataBlocks; i++) {
        if (blk->kind == kind) {
#ifdef POKEBW2_PC
            return PcG2d_ImportBlock(file, blk);
#else
            return blk;
#endif
        }
        blk = (NNSG2dBinaryBlockHeader *)((u8 *)blk + blk->size);
    }
    return NULL;
}

void NNSi_G2dUnpackUserExCellAttrBank(NNSG2dUserExCellAttrBank *attrBank) {
    u16 i;

    NNS_G2D_UNPACK_OFFSET_PTR(attrBank->pCellAttrArray, attrBank);
    for (i = 0; i < attrBank->numCells; i++) {
#ifdef POKEBW2_PC
        NNS_G2D_UNPACK_OFFSET_PTR(attrBank->pCellAttrArray[i].pAttr, attrBank);
#else
        // The SDK assigns the macro's result again here.
        attrBank->pCellAttrArray[i].pAttr = NNS_G2D_UNPACK_OFFSET_PTR(attrBank->pCellAttrArray[i].pAttr, attrBank);
#endif
    }
}
