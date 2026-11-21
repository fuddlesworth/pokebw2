#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_DRAW_SYSTEM_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_DRAW_SYSTEM_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The pens, by their shapes
enum {
    DRAW_PEN_DOT,
    DRAW_PEN_SMALL,
    DRAW_PEN_ROUND,
    DRAW_PEN_HEART,
    DRAW_PEN_POKE_BALL,
    DRAW_PEN_FACE,
    DRAW_PEN_STAR,
    DRAW_PEN_DROP,
    DRAW_PEN_MAX,
};

// One stroke, from (x0, y0) to (x1, y1)
struct DrawCommand {
    u8 x0;
    u8 y0;
    u8 x1;
    u8 y1;
    u16 color;
    u8 pen;
};

struct DrawSystemParam {
    HeapID heapId;
    u8 kind;
    u16 numCommands;
    u16 clipLeft;
    u16 clipRight;
    u16 clipTop;
    u16 clipBottom;
};

DrawSystem *DrawSystem_Create(const DrawSystemParam *param);
void DrawSystem_Delete(DrawSystem *sys);
void DrawSystem_Update(DrawSystem *sys);
// Draws the strokes added since the last call
void DrawSystem_Draw(DrawSystem *sys);
void DrawSystem_AddCommand(DrawSystem *sys, const DrawCommand *cmd);
void DrawSystem_SetWriteIndex(DrawSystem *sys, u16 index);

#endif // POKEBW2_APP_COMM_TVT_DRAW_SYSTEM_H
