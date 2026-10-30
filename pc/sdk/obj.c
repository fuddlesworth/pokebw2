#include "../gx_backend.h"
#include "nnsys/g2d.h"
#include <string.h>

void PcGx_DrawObjects(unsigned engine, PcViewport viewport) {
    if (engine > 1 || !((engine ? PcGxSubDisplay : PcGxMainDisplay) & (1u << 12)))
        return;
    for (unsigned i = 0; i < 128; i++) {
        GXOamAttr oam;
        memcpy(&oam, PcGx2D[engine].oam + i * sizeof(oam), sizeof(oam));
        if (oam.rsMode == 2 || oam.shape == 3 || oam.objMode == GX_OAM_MODE_OBJWND)
            continue;
        if (PcGx.vertex_count + 6 > PC_GX_MAX_VERTICES) {
            PcGx.failed = true;
            SDL_strlcpy(PcGx.error, "Object geometry capacity exceeded", sizeof(PcGx.error));
            return;
        }
        GXOamShape shape = (GXOamShape)(oam.attr01 & GX_OAM_ATTR01_SHAPE_MASK);
        int width = NNS_G2dGetOamSizeX(&shape), height = NNS_G2dGetOamSizeY(&shape);
        int bounds_width = width * (oam.rsMode == 3 ? 2 : 1);
        int bounds_height = height * (oam.rsMode == 3 ? 2 : 1);
        int x = oam.x >= 256 ? (int)oam.x - 512 : (int)oam.x;
        int y = oam.y >= 192 ? (int)oam.y - 256 : (int)oam.y;
        const unsigned corners[] = { 0, 1, 2, 0, 2, 3 };
        for (unsigned j = 0; j < 6; j++) {
            float u = corners[j] == 1 || corners[j] == 2 ? bounds_width : 0;
            float v = corners[j] >= 2 ? bounds_height : 0;
            PcGx.vertices[PcGx.vertex_count++] =
                (PcGxVertex){ .clip = { (viewport.x + (x + u) * viewport.width / 256) * 2 / PC_CANVAS_WIDTH - 1,
                                        1 - (viewport.y + (y + v) * viewport.height / 192) * 2 / PC_CANVAS_HEIGHT,
                                        0.495f + oam.priority * 0.08f + i * 0.00001f, 1 },
                              .uv = { u, v },
                              .color = { 1, 1, 1, 1 },
                              .texture = i | ((u32)width << 8) | ((u32)height << 16),
                              .palette = engine,
                              .polygon = 0x40000000u | (31u << 16) | 192u };
        }
    }
}

void PcGx_DrawBannerObjects(unsigned engine, PcViewport viewport) {
    u32 first = PcGx.vertex_count;
    PcGx_DrawObjects(engine, viewport);
    // Each OAM quad belongs to the upper location label or lower season label. Translate the whole quad so
    // glyphs retain their scale and slide timing while their anchors move to the widescreen edges.
    for (u32 i = first; i < PcGx.vertex_count; i += 6) {
        bool upper = PcGx.vertices[i].clip[1] + PcGx.vertices[i + 2].clip[1] > 0;
        float offset = upper ? -(float)viewport.x : (float)(PC_CANVAS_WIDTH - viewport.x - viewport.width);
        for (unsigned j = 0; j < 6; j++)
            PcGx.vertices[i + j].clip[0] += offset * 2 / PC_CANVAS_WIDTH;
    }
}
