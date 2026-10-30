#include "gfl/std.h"
#include "nitro/host_g2d.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Pointer-bearing resource records keep their DS layout on disk. Only the loader boundary widens them;
// NitroSystem's original unpacking, animation and rendering functions operate on the resulting native records.
typedef struct {
    u32 fmt, extended, size, data;
} DiskPalette;
typedef struct {
    u16 height, width;
    u32 format, mapping, character_format, size, data;
} DiskCharacters;
typedef struct {
    u16 count, attributes;
    u32 cells, mapping, transfer, strings, extended;
} DiskCellBank;
typedef struct {
    u16 count, attributes;
    u32 oams;
} DiskCell;
typedef struct {
    u32 size, entries;
} DiskTransfer;
typedef struct {
    u16 count, attribute_count;
    u32 entries;
} DiskCellAttributes;
typedef struct {
    u16 count, frames;
    u32 sequences, frame_array, contents, strings, extended;
} DiskAnimBank;
typedef struct {
    u16 count, loop;
    u32 type, mode, frames;
} DiskSequence;
typedef struct {
    u32 content;
    u16 duration, padding;
} DiskFrame;
typedef struct {
    u16 count, padding;
    u32 attributes, frames;
} DiskSequenceAttributes;
typedef struct {
    u16 count, padding;
    u32 cells, hierarchy, strings, extended;
} DiskMultiCellBank;
typedef struct {
    u16 nodes, animations;
    u32 hierarchy;
} DiskMultiCell;
typedef struct NativeResource {
    struct NativeResource *next;
    void *owner;
    size_t capacity, used;
    NNSG2dBinaryBlockHeader header;
    u8 data[];
} NativeResource;
static NativeResource *resources;

static void *Allocate(NativeResource *resource, size_t size) {
    size_t offset = (resource->used + 7) & ~(size_t)7;
    if (offset > resource->capacity || size > resource->capacity - offset) {
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Native G2D resource capacity exceeded");
    }
    resource->used = offset + size;
    return resource->data + offset;
}
static const void *Raw(const NNSG2dBinaryBlockHeader *block, size_t offset, size_t size) {
    size_t length = block->size - sizeof(*block);
    if (offset > length || size > length - offset) {
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid G2D resource range %zu+%zu/%zu", offset, size, length);
    }
    return (const u8 *)(block + 1) + offset;
}
static void *Offset(const void *pointer, const void *base) {
    return (void *)((uptr)pointer - (uptr)base);
}

static void *CellAttributes(NativeResource *resource, const NNSG2dBinaryBlockHeader *block, u32 offset) {
    const NNSG2dUserExDataBlock *raw_header = Raw(block, offset, sizeof(*raw_header));
    const DiskCellAttributes *raw = Raw(block, offset + sizeof(*raw_header), sizeof(*raw));
    NNSG2dUserExDataBlock *header = Allocate(resource, sizeof(*header) + sizeof(NNSG2dUserExCellAttrBank));
    *header = *raw_header;
    NNSG2dUserExCellAttrBank *bank = (void *)(header + 1);
    NNSG2dUserExCellAttr *entries = Allocate(resource, raw->count * sizeof(*entries));
    const u32 *raw_entries = Raw(block, offset + sizeof(*raw_header) + raw->entries, raw->count * sizeof(u32));
    bank->numCells = raw->count;
    bank->numAttribute = raw->attribute_count;
    bank->pCellAttrArray = Offset(entries, bank);
    for (u32 i = 0; i < raw->count; i++) {
        entries[i].pAttr =
            Offset(Raw(block, offset + sizeof(*raw_header) + raw_entries[i], raw->attribute_count * sizeof(u32)), bank);
    }
    return header;
}

static void *AnimationAttributes(NativeResource *resource, const NNSG2dBinaryBlockHeader *block, u32 offset) {
    const NNSG2dUserExDataBlock *raw_header = Raw(block, offset, sizeof(*raw_header));
    size_t base = offset + sizeof(*raw_header);
    const DiskCellAttributes *raw = Raw(block, base, sizeof(*raw));
    NNSG2dUserExDataBlock *header = Allocate(resource, sizeof(*header) + sizeof(NNSG2dUserExAnimAttrBank));
    *header = *raw_header;
    NNSG2dUserExAnimAttrBank *bank = (void *)(header + 1);
    bank->numSequences = raw->count;
    bank->numAttribute = raw->attribute_count;
    NNSG2dUserExAnimSequenceAttr *sequences = Allocate(resource, raw->count * sizeof(*sequences));
    const DiskSequenceAttributes *raw_sequences = Raw(block, base + raw->entries, raw->count * sizeof(*raw_sequences));
    bank->pAnmSeqAttrArray = Offset(sequences, bank);
    for (u32 i = 0; i < raw->count; i++) {
        const DiskSequenceAttributes *input = &raw_sequences[i];
        NNSG2dUserExAnimFrameAttr *frames = Allocate(resource, input->count * sizeof(*frames));
        const u32 *raw_frames = Raw(block, base + input->frames, input->count * sizeof(u32));
        sequences[i].numFrames = input->count;
        sequences[i].pAttr = Offset(Raw(block, base + input->attributes, raw->attribute_count * sizeof(u32)), bank);
        sequences[i].pAnmFrmAttrArray = Offset(frames, bank);
        for (u32 j = 0; j < input->count; j++) {
            frames[j].pAttr = Offset(Raw(block, base + raw_frames[j], raw->attribute_count * sizeof(u32)), bank);
        }
    }
    return header;
}

NNSG2dBinaryBlockHeader *PcG2d_ImportBlock(void *owner, const NNSG2dBinaryBlockHeader *block) {
    if (block->kind == NNS_G2D_BINBLK_SIG_SCRDATA)
        return (NNSG2dBinaryBlockHeader *)block;
    if (block->size < sizeof(*block) || block->size > 64 * 1024 * 1024) {
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid G2D block size");
    }
    size_t capacity = (size_t)block->size * 4 + 512;
    NativeResource *resource = calloc(1, sizeof(*resource) + capacity);
    if (!resource)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Unable to allocate native G2D resource");
    resource->owner = owner;
    resource->capacity = capacity;
    resource->header = *block;
    resource->next = resources;
    resources = resource;
    switch (block->kind) {
    case NNS_G2D_BINBLK_SIG_PALETTEDATA: {
        const DiskPalette *raw = Raw(block, 0, sizeof(*raw));
        NNSG2dPaletteData *native = Allocate(resource, sizeof(*native));
        native->fmt = raw->fmt;
        native->bExtendedPlt = raw->extended;
        native->szByte = raw->size;
        native->pRawData = Offset(Raw(block, raw->data, raw->size), native);
        break;
    }
    case NNS_G2D_BINBLK_SIG_CHARACTERDATA: {
        const DiskCharacters *raw = Raw(block, 0, sizeof(*raw));
        NNSG2dCharacterData *native = Allocate(resource, sizeof(*native));
        native->H = raw->height;
        native->W = raw->width;
        native->pixelFmt = raw->format;
        native->mappingType = raw->mapping;
        native->characterFmt = raw->character_format;
        native->szByte = raw->size;
        native->pRawData = Offset(Raw(block, raw->data, raw->size), native);
        break;
    }
    case NNS_G2D_BINBLK_SIG_PALETTECOMPINFO: {
        const DiskCellAttributes *raw = Raw(block, 0, sizeof(*raw));
        NNSG2dPaletteCompressInfo *native = Allocate(resource, sizeof(*native));
        native->numPalette = raw->count;
        native->pPlttIdxTbl = Offset(Raw(block, raw->entries, raw->count * sizeof(u16)), native);
        break;
    }
    case NNS_G2D_BLKSIG_CELLBANK: {
        const DiskCellBank *raw = Raw(block, 0, sizeof(*raw));
        NNSG2dCellDataBank *bank = Allocate(resource, sizeof(*bank));
        bool bounds = (raw->attributes & NNS_G2D_CELLBK_ATTR_CELLWITHBR) != 0;
        size_t stride = bounds ? sizeof(NNSG2dCellDataWithBR) : sizeof(NNSG2dCellData);
        size_t raw_stride = sizeof(DiskCell) + (bounds ? sizeof(NNSG2dCellBoundingRectS16) : 0);
        u8 *cells = Allocate(resource, raw->count * stride);
        size_t oam_offset = raw->cells + raw->count * raw_stride;
        size_t oam_size = block->size - sizeof(*block) - oam_offset;
        void *oams = Allocate(resource, oam_size);
        memcpy(oams, Raw(block, oam_offset, oam_size), oam_size);
        bank->numCells = raw->count;
        bank->cellBankAttr = raw->attributes;
        bank->mappingMode = raw->mapping;
        bank->pCellDataArrayHead = Offset(cells, bank);
        for (u32 i = 0; i < raw->count; i++) {
            const DiskCell *input = Raw(block, raw->cells + i * raw_stride, raw_stride);
            NNSG2dCellData *cell = (void *)(cells + i * stride);
            cell->numOAMAttrs = input->count;
            cell->cellAttr = input->attributes;
            Raw(block, oam_offset + input->oams, input->count * sizeof(NNSG2dCellOAMAttrData));
            cell->pOamAttrArray = (void *)(uptr)input->oams;
            if (bounds)
                memcpy(&((NNSG2dCellDataWithBR *)cell)->boundingRect, input + 1, sizeof(NNSG2dCellBoundingRectS16));
        }
        if (raw->transfer) {
            const DiskTransfer *input = Raw(block, raw->transfer, sizeof(*input));
            NNSG2dVramTransferData *transfer = Allocate(resource, sizeof(*transfer));
            transfer->szByteMax = input->size;
            transfer->pCellTransferDataArray = Offset(
                Raw(block, raw->transfer + input->entries, raw->count * sizeof(NNSG2dCellVramTransferData)), transfer);
            bank->pVramTransferData = Offset(transfer, bank);
        }
        if (raw->extended)
            bank->pExtendedData = Offset(CellAttributes(resource, block, raw->extended), bank);
        break;
    }
    case NNS_G2D_BLKSIG_ANIMBANK: {
        const DiskAnimBank *raw = Raw(block, 0, sizeof(*raw));
        NNSG2dAnimBankData *bank = Allocate(resource, sizeof(*bank));
        NNSG2dAnimSequenceData *sequences = Allocate(resource, raw->count * sizeof(*sequences));
        NNSG2dAnimFrameData *frames = Allocate(resource, raw->frames * sizeof(*frames));
        // The original animation code reads every element through the SRT type, including two-byte indices.
        // Give each native element that type's alignment without changing the serialized contents.
        NNSG2dAnimDataSRT *contents = Allocate(resource, raw->frames * sizeof(*contents));
        const DiskSequence *raw_sequences = Raw(block, raw->sequences, raw->count * sizeof(*raw_sequences));
        const DiskFrame *raw_frames = Raw(block, raw->frame_array, raw->frames * sizeof(*raw_frames));
        bank->numSequences = raw->count;
        bank->numTotalFrames = raw->frames;
        bank->pSequenceArrayHead = Offset(sequences, bank);
        bank->pFrameArrayHead = Offset(frames, bank);
        bank->pAnimContents = Offset(contents, bank);
        for (u32 i = 0; i < raw->count; i++) {
            const DiskSequence *input = &raw_sequences[i];
            size_t first = input->frames / sizeof(DiskFrame);
            if (input->frames % sizeof(DiskFrame) || first > raw->frames || input->count > raw->frames - first) {
                GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid G2D animation frame range");
            }
            sequences[i].numFrames = input->count;
            sequences[i].loopStartFrameIdx = input->loop;
            sequences[i].animType = input->type;
            sequences[i].playMode = input->mode;
            sequences[i].pAnmFrameArray = Offset(frames + first, frames);
            size_t content_size;
            switch (input->type & 0xff) {
            case NNS_G2D_ANIMELEMENT_INDEX:
                content_size = sizeof(u16);
                break;
            case NNS_G2D_ANIMELEMENT_INDEX_T:
                content_size = sizeof(NNSG2dAnimDataT);
                break;
            case NNS_G2D_ANIMELEMENT_INDEX_SRT:
                content_size = sizeof(NNSG2dAnimDataSRT);
                break;
            default:
                GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid G2D animation element type");
                return NULL;
            }
            for (size_t j = first; j < first + input->count; j++) {
                memcpy(&contents[j], Raw(block, (size_t)raw->contents + raw_frames[j].content, content_size),
                       content_size);
            }
        }
        for (u32 i = 0; i < raw->frames; i++) {
            frames[i].frames = raw_frames[i].duration;
            frames[i].pContent = Offset(&contents[i], contents);
        }
        if (raw->extended)
            bank->pExtendedData = Offset(AnimationAttributes(resource, block, raw->extended), bank);
        break;
    }
    case NNS_G2D_BLKSIG_MULTICELLBANK: {
        const DiskMultiCellBank *raw = Raw(block, 0, sizeof(*raw));
        NNSG2dMultiCellDataBank *bank = Allocate(resource, sizeof(*bank));
        NNSG2dMultiCellData *cells = Allocate(resource, raw->count * sizeof(*cells));
        const DiskMultiCell *raw_cells = Raw(block, raw->cells, raw->count * sizeof(*raw_cells));
        const void *hierarchy = Raw(block, raw->hierarchy, 0);
        bank->numMultiCellData = raw->count;
        bank->pMultiCellDataArray = Offset(cells, bank);
        bank->pHierarchyDataArray = Offset(hierarchy, bank);
        for (u32 i = 0; i < raw->count; i++) {
            cells[i].numNodes = raw_cells[i].nodes;
            cells[i].numCellAnim = raw_cells[i].animations;
            Raw(block, raw->hierarchy + raw_cells[i].hierarchy,
                raw_cells[i].nodes * sizeof(NNSG2dMultiCellHierarchyData));
            cells[i].pHierDataArray = (void *)(uptr)raw_cells[i].hierarchy;
        }
        if (raw->extended)
            bank->pExtendedData = Offset(CellAttributes(resource, block, raw->extended), bank);
        break;
    }
    default:
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Native layout for G2D block 0x%08x is not implemented", block->kind);
    }
    return &resource->header;
}
void PcG2d_Release(void *owner) {
    NativeResource **link = &resources;
    while (*link) {
        NativeResource *resource = *link;
        if (resource->owner == owner) {
            *link = resource->next;
            free(resource);
        } else
            link = &resource->next;
    }
}
