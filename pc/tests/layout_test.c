#include "layout.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Explicit checks also run in Release builds, where assert would be disabled.
#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "line %d: %s\n", __LINE__, #condition);                                                    \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

int main(void) {
    PcViewport viewport = PcViewport_Fit(1920, 1080);
    CHECK(viewport.x == 0 && viewport.y == 0 && viewport.width == 1920 && viewport.height == 1080);
    float x = -1, y = -1;
    CHECK(PcViewport_MapPointer(viewport, 960, 540, &x, &y));
    CHECK(x == 192 && y == 108);

    viewport = PcViewport_Fit(800, 600);
    CHECK(viewport.x == 0 && viewport.y == 75 && viewport.width == 800 && viewport.height == 450);
    CHECK(!PcViewport_MapPointer(viewport, 400, 74, &x, &y));
    CHECK(PcViewport_MapPointer(viewport, 0, 75, &x, &y));
    CHECK(x == 0 && y == 0);
    CHECK(!PcViewport_MapPointer(viewport, 800, 200, &x, &y));
    CHECK(!PcViewport_MapPointer(viewport, 400, 525, &x, &y));
    CHECK(!PcViewport_MapPointer(viewport, NAN, 100, &x, &y));
    CHECK(!PcViewport_MapPointer(viewport, INFINITY, 100, &x, &y));

    viewport = PcViewport_Fit(2560, 1080);
    CHECK(viewport.x == 320 && viewport.y == 0 && viewport.width == 1920 && viewport.height == 1080);
    CHECK(!PcViewport_MapPointer(viewport, 319, 0, &x, &y));
    CHECK(PcViewport_MapPointer(viewport, 1280, 540, &x, &y));
    CHECK(x == 192 && y == 108);

    // A high-DPI window passes physical pixels to the same mapping used by presentation.
    viewport = PcViewport_Fit(1600, 1200);
    CHECK(PcViewport_MapPointer(viewport, 800, 600, &x, &y));
    CHECK(x == 192 && y == 108);
    CHECK(!PcViewport_MapPointer(PcViewport_Fit(0, 1080), 0, 0, &x, &y));
    CHECK(!PcViewport_MapPointer(PcViewport_Fit(1, 1), 0, 0, &x, &y));
    viewport = PcViewport_Fit(UINT32_MAX, UINT32_MAX);
    CHECK(viewport.width == UINT32_MAX && viewport.height > 0 && viewport.height < UINT32_MAX);
    puts("Viewport and pointer checks passed");
    return EXIT_SUCCESS;
}
