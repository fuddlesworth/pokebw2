#include "types.h"
#include "field/el_scoreboard.h"
#include "field/field.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A texture or palette offset for which no resource was created
#define EL_SCOREBOARD_OFFSET_NONE (-1)

struct ElScoreboard {
    u32 unk0;
    u16 widthIndex;
    u16 heightIndex;
    u32 texVramKey;
    u32 plttVramKey;
    u32 texOffset;
    u32 plttOffset;
    u32 frame;
};

struct ElScoreboardPaletteTarget {
    u32 plttVramKey;
    u32 vramOffset;
};

// Declared in reverse, as the compiler emits them in reverse order
static const u16 sScoreboardPalette3[4] = { 0x0000, 0x18c6, 0x0d73, 0x021f };
static const u16 sScoreboardPalette2[4] = { 0x0000, 0x2108, 0x0d73, 0x021f };
static const u16 sScoreboardPalette1[4] = { 0x0000, 0x18c6, 0x0df3, 0x031f };
static const u16 sScoreboardPalette0[4] = { 0x0000, 0x2108, 0x0df3, 0x031f };

static const u16 *sScoreboardPalettes[4] = { sScoreboardPalette3, sScoreboardPalette2, sScoreboardPalette1,
                                             sScoreboardPalette0 };

void ElScoreboard_UploadPalette(ElScoreboardPaletteTarget *target, s32 frame);

ElScoreboard *ElScoreboard_Create(G3DResource *texture, const char *texName, const char *plName, const StrBuf *text,
                                  u16 x, u16 y, HeapID heapId) {
    G3DTextDrawResource resource;
    ElScoreboard *board = GFL_HeapAllocate(heapId, sizeof(ElScoreboard), TRUE, "el_scoreboard.c", 412);

    board->unk0 = 0;
    board->widthIndex = G3DTextDraw_GetTexDimCount() - 1;
    board->heightIndex = G3DTextDraw_GetTexDimCount() - 1;
    board->texOffset = EL_SCOREBOARD_OFFSET_NONE;
    board->plttOffset = EL_SCOREBOARD_OFFSET_NONE;
    if (G3DTextDraw_CreateResource(texture, texName, 0, plName, text, x, y, 0xc40, heapId, &resource)) {
        board->texVramKey = resource.texVramKey;
        board->plttVramKey = resource.plttVramKey;
        board->texOffset = resource.texOffset;
        board->widthIndex = resource.widthIndex;
        board->heightIndex = resource.heightIndex;
        board->plttOffset = resource.plttOffset;
        board->frame = 0;
    }
    return board;
}

void ElScoreboard_Free(ElScoreboard *board) {
    GFL_HeapFree(board);
}

void ElScoreboard_Update(ElScoreboard *board) {
    ElScoreboardPaletteTarget target;

    board->frame++;
    if (board->plttOffset != EL_SCOREBOARD_OFFSET_NONE) {
        target.plttVramKey = board->plttVramKey;
        target.vramOffset = board->plttOffset;
        ElScoreboard_UploadPalette(&target, board->frame);
    }
}

void ElScoreboard_UploadPalette(ElScoreboardPaletteTarget *target, s32 frame) {
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_3D_TEX_PLTT, target->vramOffset + (u16)target->plttVramKey * 8,
                                       sScoreboardPalettes[(frame & 0x1f) / 8], 8);
}
