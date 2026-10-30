#include "layout.h"

PcViewport PcViewport_Fit(uint32_t width, uint32_t height) {
    PcViewport viewport = { 0 };
    if (width == 0 || height == 0) {
        return viewport;
    }
    if ((uint64_t)width * PC_CANVAS_HEIGHT > (uint64_t)height * PC_CANVAS_WIDTH) {
        viewport.height = height;
        viewport.width = (uint32_t)((uint64_t)height * PC_CANVAS_WIDTH / PC_CANVAS_HEIGHT);
    } else {
        viewport.width = width;
        viewport.height = (uint32_t)((uint64_t)width * PC_CANVAS_HEIGHT / PC_CANVAS_WIDTH);
    }
    viewport.x = (width - viewport.width) / 2;
    viewport.y = (height - viewport.height) / 2;
    return viewport;
}

bool PcViewport_MapPointer(PcViewport viewport, float x, float y, float *canvas_x, float *canvas_y) {
    // Negated comparisons reject NaN as well as points outside the content.
    if (viewport.width == 0 || viewport.height == 0 || !(x >= viewport.x && y >= viewport.y) ||
        !(x < (double)viewport.x + viewport.width && y < (double)viewport.y + viewport.height)) {
        return false;
    }
    *canvas_x = (x - viewport.x) * PC_CANVAS_WIDTH / viewport.width;
    *canvas_y = (y - viewport.y) * PC_CANVAS_HEIGHT / viewport.height;
    return true;
}
