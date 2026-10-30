#ifndef POKEBW2_PC_LAYOUT_H
#define POKEBW2_PC_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

// A provisional widescreen canvas for the host diagnostic, not a DS framebuffer size.
enum { PC_CANVAS_WIDTH = 384, PC_CANVAS_HEIGHT = 216 };

typedef struct PcViewport {
    uint32_t x, y, width, height;
} PcViewport;

PcViewport PcViewport_Fit(uint32_t width, uint32_t height);
bool PcViewport_MapPointer(PcViewport viewport, float x, float y, float *canvas_x, float *canvas_y);

#endif
