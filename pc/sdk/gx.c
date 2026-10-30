#include "../gx_backend.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

PcGxFrame PcGx;
u16 PcGxPower;
u32 PcGxStatus;
u16 PcGxDisplay3D;
u16 PcGx1DotDepth;
PcGxRasterState PcGxRaster;
u32 PcGxMainDisplay, PcGxSubDisplay;
PcGx2DMemory PcGx2D[2];
PcGx2DRegisters PcGxRegisters[2];
u8 PcGxBackground[2][524288];
u8 PcGxLcdc[0xa4000];
u32 PcGxCapturedBanks;
u32 PcGxWideCapture[PC_CANVAS_WIDTH * PC_CANVAS_HEIGHT];
BOOL PcGxWideCaptureValid;
static PcGxCaptureCallback captureCallback;
static GXVRamClearImage clearImageBanks;
static void Fail(const char *reason, u32 value);

void PcGx_SetCaptureCallback(PcGxCaptureCallback callback) {
    captureCallback = callback;
}

void PcGx_Capture(void) {
    u32 control = reg_GX_DISPCAPCNT;
    if ((control & 0x80000000u) && captureCallback)
        captureCallback(control);
    reg_GX_DISPCAPCNT &= ~0x80000000u;
}

u32 PcGx_GetPolygonCount(void) {
    return PcGx.vertex_count / 3;
}

u32 PcGx_GetVertexCount(void) {
    return PcGx.vertex_count;
}

void *G2_GetOBJCharPtr(void) {
    return PcGx2D[0].object;
}

void *G2S_GetOBJCharPtr(void) {
    return PcGx2D[1].object;
}

void GX_LoadOBJ(const void *src, u32 offset, u32 size) {
    if (offset > sizeof(PcGx2D[0].object) || size > sizeof(PcGx2D[0].object) - offset) {
        Fail("2D VRAM upload out of bounds", offset);
        return;
    }
    memcpy(PcGx2D[0].object + offset, src, size);
}

void GX_LoadOBJPltt(const void *src, u32 offset, u32 size) {
    if (offset > sizeof(PcGx2D[0].object_palette) || size > sizeof(PcGx2D[0].object_palette) - offset) {
        Fail("2D VRAM upload out of bounds", offset);
        return;
    }
    memcpy(PcGx2D[0].object_palette + offset, src, size);
}

void GX_LoadOBJExtPltt(const void *src, u32 offset, u32 size) {
    if (!size)
        return;
    if (offset > sizeof(PcGx2D[0].object_extended_palette) ||
        size > sizeof(PcGx2D[0].object_extended_palette) - offset) {
        Fail("2D VRAM upload out of bounds", offset);
        return;
    }
    memcpy(PcGx2D[0].object_extended_palette + offset, src, size);
}

void GX_LoadOAM(const void *src, u32 offset, u32 size) {
    if (offset > sizeof(PcGx2D[0].oam) || size > sizeof(PcGx2D[0].oam) - offset) {
        Fail("2D VRAM upload out of bounds", offset);
        return;
    }
    memcpy(PcGx2D[0].oam + offset, src, size);
}

void GX_BeginLoadOBJExtPltt(void) {
}

void GX_EndLoadOBJExtPltt(void) {
}

void GXS_LoadOBJ(const void *src, u32 offset, u32 size) {
    if (offset > sizeof(PcGx2D[1].object) || size > sizeof(PcGx2D[1].object) - offset) {
        Fail("2D VRAM upload out of bounds", offset);
        return;
    }
    memcpy(PcGx2D[1].object + offset, src, size);
}

void GXS_LoadOBJPltt(const void *src, u32 offset, u32 size) {
    if (offset > sizeof(PcGx2D[1].object_palette) || size > sizeof(PcGx2D[1].object_palette) - offset) {
        Fail("2D VRAM upload out of bounds", offset);
        return;
    }
    memcpy(PcGx2D[1].object_palette + offset, src, size);
}

void GXS_LoadOBJExtPltt(const void *src, u32 offset, u32 size) {
    if (!size)
        return;
    if (offset > sizeof(PcGx2D[1].object_extended_palette) ||
        size > sizeof(PcGx2D[1].object_extended_palette) - offset) {
        Fail("2D VRAM upload out of bounds", offset);
        return;
    }
    memcpy(PcGx2D[1].object_extended_palette + offset, src, size);
}

void GXS_LoadOAM(const void *src, u32 offset, u32 size) {
    if (offset > sizeof(PcGx2D[1].oam) || size > sizeof(PcGx2D[1].oam) - offset) {
        Fail("2D VRAM upload out of bounds", offset);
        return;
    }
    memcpy(PcGx2D[1].oam + offset, src, size);
}

void GXS_BeginLoadOBJExtPltt(void) {
}

void GXS_EndLoadOBJExtPltt(void) {
}
typedef struct {
    float m[4][4];
} Matrix;
static struct {
    Matrix projection, position, vector, texture;
    Matrix position_stack[32], vector_stack[32], projection_stack, texture_stack;
    u32 mode, stack, primitive, polygon, texparam, palette, diffuse_ambient, specular_emission;
    u32 commands, pending, arguments[32], argument_count, direct_op;
    float position_xyz[3], uv[2], base_uv[2], color[3], lights[4][3], light_colors[4][3];
    float shininess[128];
    PcGxVertex primitive_vertices[4];
    unsigned primitive_count;
} gx;

static Matrix Identity(void) {
    Matrix m = { 0 };
    for (int i = 0; i < 4; i++)
        m.m[i][i] = 1;
    return m;
}
static Matrix Multiply(Matrix a, Matrix b) {
    Matrix out = { 0 };
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                out.m[i][j] += a.m[i][k] * b.m[k][j];
    return out;
}
static void Transform(const float in[4], Matrix m, float out[4]) {
    for (int j = 0; j < 4; j++) {
        out[j] = 0;
        for (int i = 0; i < 4; i++)
            out[j] += in[i] * m.m[i][j];
    }
}
static Matrix *Current(void) {
    return gx.mode == 0 ? &gx.projection : gx.mode == 3 ? &gx.texture : &gx.position;
}
static void Fail(const char *reason, u32 value) {
    if (!PcGx.failed)
        snprintf(PcGx.error, sizeof(PcGx.error), "%s: 0x%x", reason, value);
    PcGx.failed = true;
}
static void Color(u32 packed, float out[3]) {
    for (int i = 0; i < 3; i++)
        out[i] = ((packed >> (i * 5)) & 31) / 31.0f;
}
static float Signed10(u32 word) {
    return (float)((s32)(word << 22) >> 22);
}
static void Triangle(PcGxVertex a, PcGxVertex b, PcGxVertex c) {
    if (PcGx.vertex_count + 3 > PC_GX_MAX_VERTICES) {
        Fail("Geometry capacity exceeded", PcGx.vertex_count);
        return;
    }
    PcGx.vertices[PcGx.vertex_count++] = a;
    PcGx.vertices[PcGx.vertex_count++] = b;
    PcGx.vertices[PcGx.vertex_count++] = c;
}
static void Vertex(void) {
    PcGxVertex v = { .texture = gx.texparam, .palette = gx.palette, .polygon = gx.polygon };
    float local[4] = { gx.position_xyz[0], gx.position_xyz[1], gx.position_xyz[2], 1 }, eye[4];
    Transform(local, gx.position, eye);
    Transform(eye, gx.projection, v.clip);
    if (PcGx.wide_scene)
        v.clip[0] *= PcGx.projection_scale_x;
    v.uv[0] = gx.uv[0];
    v.uv[1] = gx.uv[1];
    if ((gx.texparam >> 30) == 3) {
        float tex[4];
        Transform(local, gx.texture, tex);
        v.uv[0] += tex[0];
        v.uv[1] += tex[1];
    }
    memcpy(v.color, gx.color, sizeof(gx.color));
    v.color[3] = ((gx.polygon >> 16) & 31) / 31.0f;
    unsigned n = gx.primitive_count++;
    gx.primitive_vertices[n % 4] = v;
    PcGxVertex *p = gx.primitive_vertices;
    if (gx.primitive == 0 && n % 3 == 2) {
        Triangle(p[(n - 2) % 4], p[(n - 1) % 4], v);
    } else if (gx.primitive == 1 && n % 4 == 3) {
        Triangle(p[0], p[1], p[2]);
        Triangle(p[0], p[2], p[3]);
    } else if (gx.primitive == 2 && n >= 2) {
        Triangle(p[(n - 2 + (n & 1)) % 4], p[(n - 1 - (n & 1)) % 4], v);
    } else if (gx.primitive == 3 && n >= 3 && (n & 1)) {
        Triangle(p[(n - 3) % 4], p[(n - 2) % 4], v);
        Triangle(p[(n - 3) % 4], v, p[(n - 1) % 4]);
    }
}
static unsigned ArgumentCount(u32 op) {
    switch (op) {
    case 0:
    case 0x11:
    case 0x15:
    case 0x41:
        return 0;
    case 0x16:
    case 0x18:
        return 16;
    case 0x17:
    case 0x19:
        return 12;
    case 0x1a:
        return 9;
    case 0x1b:
    case 0x1c:
    case 0x70:
        return 3;
    case 0x23:
    case 0x71:
        return 2;
    case 0x34:
        return 32;
    case 0x10:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x40:
    case 0x50:
    case 0x60:
    case 0x72:
        return 1;
    default:
        Fail("Unknown geometry command", op);
        return 0;
    }
}
static void Execute(u32 op, const u32 *a) {
    Matrix *current = Current();
    switch (op) {
    case 0:
        break;
    case 0x10:
        gx.mode = a[0] & 3;
        break;
    case 0x11:
        if (gx.mode == 0) {
            gx.projection_stack = gx.projection;
            PcGxStatus |= REG_G3X_GXSTAT_PJ_MASK;
        } else if (gx.mode == 3)
            gx.texture_stack = gx.texture;
        else {
            gx.position_stack[gx.stack & 31] = gx.position;
            gx.vector_stack[gx.stack & 31] = gx.vector;
            gx.stack = (gx.stack + 1) & 31;
            PcGxStatus = (PcGxStatus & ~REG_G3X_GXSTAT_PV_MASK) | (gx.stack << REG_G3X_GXSTAT_PV_SHIFT);
        }
        break;
    case 0x12:
        if (gx.mode == 0) {
            gx.projection = gx.projection_stack;
            PcGxStatus &= ~REG_G3X_GXSTAT_PJ_MASK;
        } else if (gx.mode == 3)
            gx.texture = gx.texture_stack;
        else {
            int amount = (s32)(a[0] << 26) >> 26;
            gx.stack = (gx.stack - amount) & 31;
            gx.position = gx.position_stack[gx.stack];
            gx.vector = gx.vector_stack[gx.stack];
            PcGxStatus = (PcGxStatus & ~REG_G3X_GXSTAT_PV_MASK) | (gx.stack << REG_G3X_GXSTAT_PV_SHIFT);
        }
        break;
    case 0x13:
        if (gx.mode == GX_MTXMODE_PROJECTION)
            gx.projection_stack = gx.projection;
        else if (gx.mode == GX_MTXMODE_TEXTURE)
            gx.texture_stack = gx.texture;
        else {
            gx.position_stack[a[0] & 31] = gx.position;
            gx.vector_stack[a[0] & 31] = gx.vector;
        }
        break;
    case 0x14:
        if (gx.mode == GX_MTXMODE_PROJECTION)
            gx.projection = gx.projection_stack;
        else if (gx.mode == GX_MTXMODE_TEXTURE)
            gx.texture = gx.texture_stack;
        else {
            gx.position = gx.position_stack[a[0] & 31];
            gx.vector = gx.vector_stack[a[0] & 31];
        }
        break;
    case 0x15:
        *current = Identity();
        if (gx.mode == 2)
            gx.vector = Identity();
        break;
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1a: {
        Matrix m = Identity();
        unsigned rows = op == 0x1a ? 3 : 4, cols = op == 0x16 || op == 0x18 ? 4 : 3;
        for (unsigned i = 0; i < rows; i++)
            for (unsigned j = 0; j < cols; j++)
                m.m[i][j] = (s32)a[i * cols + j] / 4096.0f;
        bool load = op == 0x16 || op == 0x17;
        *current = load ? m : Multiply(m, *current);
        if (gx.mode == 2) {
            m.m[3][0] = m.m[3][1] = m.m[3][2] = 0;
            gx.vector = load ? m : Multiply(m, gx.vector);
        }
        break;
    }
    case 0x1b:
    case 0x1c: {
        Matrix m = Identity();
        for (int i = 0; i < 3; i++)
            m.m[op == 0x1b ? i : 3][i] = (s32)a[i] / 4096.0f;
        *current = Multiply(m, *current);
        break;
    }
    case 0x20:
        Color(a[0], gx.color);
        break;
    case 0x21: {
        float local[4] = { Signed10(a[0]) / 512, Signed10(a[0] >> 10) / 512, Signed10(a[0] >> 20) / 512, 0 }, normal[4];
        Transform(local, gx.vector, normal);
        float diffuse[3], ambient[3], specular[3], emission[3];
        Color(gx.diffuse_ambient, diffuse);
        Color(gx.diffuse_ambient >> 16, ambient);
        Color(gx.specular_emission, specular);
        Color(gx.specular_emission >> 16, emission);
        memcpy(gx.color, emission, sizeof(emission));
        for (int light = 0; light < 4; light++)
            if (gx.polygon & (1u << light)) {
                float dot = 0, halfdot = 0;
                for (int i = 0; i < 3; i++) {
                    dot -= normal[i] * gx.lights[light][i];
                    halfdot -= normal[i] * (gx.lights[light][i] + (i == 2 ? -1 : 0)) * 0.5f;
                }
                float shine = fmaxf(0, halfdot);
                shine *= shine;
                if (gx.specular_emission & 0x8000)
                    shine = gx.shininess[(unsigned)fminf(127, shine * 128)];
                for (int i = 0; i < 3; i++)
                    gx.color[i] +=
                        gx.light_colors[light][i] * (ambient[i] + diffuse[i] * fmaxf(0, dot) + specular[i] * shine);
            }
        for (int i = 0; i < 3; i++)
            gx.color[i] = fminf(1, gx.color[i]);
        if ((gx.texparam >> 30) == 2) {
            float tex[4];
            Transform(local, gx.texture, tex);
            gx.uv[0] = gx.base_uv[0] + tex[0];
            gx.uv[1] = gx.base_uv[1] + tex[1];
        }
        break;
    }
    case 0x22: {
        float uv[4] = { (s16)a[0] / 16.0f, (s16)(a[0] >> 16) / 16.0f, 1.0f / 16, 1.0f / 16 }, transformed[4];
        gx.base_uv[0] = uv[0];
        gx.base_uv[1] = uv[1];
        if ((gx.texparam >> 30) == 1) {
            Transform(uv, gx.texture, transformed);
            gx.uv[0] = transformed[0];
            gx.uv[1] = transformed[1];
        } else {
            gx.uv[0] = uv[0];
            gx.uv[1] = uv[1];
        }
        break;
    }
    case 0x23:
        gx.position_xyz[0] = (s16)a[0] / 4096.0f;
        gx.position_xyz[1] = (s16)(a[0] >> 16) / 4096.0f;
        gx.position_xyz[2] = (s16)a[1] / 4096.0f;
        Vertex();
        break;
    case 0x24:
        for (int i = 0; i < 3; i++)
            gx.position_xyz[i] = Signed10(a[0] >> (i * 10)) / 64;
        Vertex();
        break;
    case 0x25:
    case 0x26:
    case 0x27:
        gx.position_xyz[op == 0x27 ? 1 : 0] = (s16)a[0] / 4096.0f;
        gx.position_xyz[op == 0x25 ? 1 : 2] = (s16)(a[0] >> 16) / 4096.0f;
        Vertex();
        break;
    case 0x28:
        for (int i = 0; i < 3; i++)
            gx.position_xyz[i] += Signed10(a[0] >> (i * 10)) / 4096;
        Vertex();
        break;
    case 0x29:
        gx.polygon = a[0];
        break;
    case 0x2a:
        gx.texparam = a[0];
        break;
    case 0x2b:
        gx.palette = a[0];
        break;
    case 0x30:
        gx.diffuse_ambient = a[0];
        if (a[0] & 0x8000)
            Color(a[0], gx.color);
        break;
    case 0x31:
        gx.specular_emission = a[0];
        break;
    case 0x32: {
        float v[4] = { Signed10(a[0]) / 512, Signed10(a[0] >> 10) / 512, Signed10(a[0] >> 20) / 512, 0 }, out[4];
        Transform(v, gx.vector, out);
        memcpy(gx.lights[a[0] >> 30], out, 3 * sizeof(float));
        break;
    }
    case 0x33:
        Color(a[0], gx.light_colors[a[0] >> 30]);
        break;
    case 0x34:
        for (unsigned i = 0; i < 128; i++)
            gx.shininess[i] = ((a[i / 4] >> ((i % 4) * 8)) & 255) / 256.0f;
        break;
    case 0x40:
        gx.primitive = a[0] & 3;
        gx.primitive_count = 0;
        break;
    case 0x41:
    case 0x50:
    case 0x60:
        break;
    case 0x70: {
        s16 box[6] = { (s16)a[0], (s16)(a[0] >> 16), (s16)a[1], (s16)(a[1] >> 16), (s16)a[2], (s16)(a[2] >> 16) };
        unsigned outside_all = 63;
        for (unsigned corner = 0; corner < 8; corner++) {
            float local[4] = { (box[0] + ((corner & 1) ? box[3] : 0)) / 4096.0f,
                               (box[1] + ((corner & 2) ? box[4] : 0)) / 4096.0f,
                               (box[2] + ((corner & 4) ? box[5] : 0)) / 4096.0f, 1 },
                  eye[4], clip[4];
            Transform(local, gx.position, eye);
            Transform(eye, gx.projection, clip);
            if (PcGx.wide_scene)
                clip[0] *= PcGx.projection_scale_x;
            unsigned outside = (clip[0] < -clip[3] ? 1u : 0u) | (clip[0] > clip[3] ? 2u : 0u) |
                               (clip[1] < -clip[3] ? 4u : 0u) | (clip[1] > clip[3] ? 8u : 0u) |
                               (clip[2] < -clip[3] ? 16u : 0u) | (clip[2] > clip[3] ? 32u : 0u);
            outside_all &= outside;
        }
        PcGxStatus = (PcGxStatus & ~REG_G3X_GXSTAT_TR_MASK) | (outside_all ? 0 : REG_G3X_GXSTAT_TR_MASK);
        break;
    }
    default:
        Fail("Unimplemented geometry operation", op);
        break;
    }
}
static void Advance(void) {
    while (gx.commands && !PcGx.failed) {
        gx.pending = gx.commands & 255;
        if (ArgumentCount(gx.pending))
            return;
        Execute(gx.pending, gx.arguments);
        gx.commands >>= 8;
    }
    gx.pending = 0;
}
void PcGx_Write(uptr address, u32 value) {
    if (PcGx.failed)
        return;
    if (address == 0x04000400) {
        if (!gx.commands) {
            gx.commands = value;
            gx.argument_count = 0;
            Advance();
        } else {
            gx.arguments[gx.argument_count++] = value;
            if (gx.argument_count == ArgumentCount(gx.pending)) {
                Execute(gx.pending, gx.arguments);
                gx.argument_count = 0;
                gx.commands >>= 8;
                Advance();
            }
        }
    } else {
        u32 op = (u32)(address - 0x04000400) / 4;
        if (gx.argument_count && gx.direct_op != op) {
            Fail("Interrupted geometry command", op);
            return;
        }
        gx.direct_op = op;
        gx.arguments[gx.argument_count++] = value;
        if (gx.argument_count >= ArgumentCount(op)) {
            Execute(op, gx.arguments);
            gx.argument_count = 0;
        }
    }
}
void PcGx_BeginFrame(void) {
    PcGx.vertex_count = 0;
    PcGx.composite_scene = false;
    PcGx.dex_ui = false;
    PcGx.party_ui = false;
    PcGx.menu_layer_count = 0;
    PcGx.battle_ui = false;
}
void PcGx_ClearFifo(void) {
    gx.commands = gx.pending = gx.argument_count = gx.direct_op = 0;
}
void PcGx_ResetHardware(void) {
    memset(&gx, 0, sizeof(gx));
    PcGx.vertex_count = 0;
    PcGx.failed = false;
    PcGx.error[0] = 0;
    PcGxStatus = 0;
    PcGx1DotDepth = 0;
    PcGx.wide_scene = false;
    PcGx.projection_scale_x = 0.75f;
    gx.projection = gx.position = gx.vector = gx.texture = Identity();
    for (int i = 0; i < 32; i++)
        gx.position_stack[i] = gx.vector_stack[i] = Identity();
    gx.projection_stack = gx.texture_stack = Identity();
    gx.polygon = 0x001f00c0;
    gx.color[0] = gx.color[1] = gx.color[2] = 1;
}
void PcGx_ReadClipMatrix(void *output) {
    MtxFx44 *dest = output;
    Matrix clip = Multiply(gx.position, gx.projection);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            dest->m[i][j] = (fx32)lrintf(clip.m[i][j] * 4096);
}
void PcGx_ReadVectorMatrix(void *output) {
    MtxFx33 *dest = output;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            dest->m[i][j] = (fx32)lrintf(gx.vector.m[i][j] * 4096);
}
void GX_BeginLoadTex(void) {
}
void GX_EndLoadTex(void) {
}
void GX_BeginLoadTexPltt(void) {
}
void GX_EndLoadTexPltt(void) {
}
void GX_LoadTex(const void *src, u32 offset, u32 size) {
    if (offset > sizeof(PcGx.textures) || size > sizeof(PcGx.textures) - offset) {
        Fail("Texture upload out of bounds", offset);
        return;
    }
    memcpy(PcGx.textures + offset, src, size);
}
void GX_LoadTexPltt(const void *src, u32 offset, u32 size) {
    if (offset > sizeof(PcGx.palettes) || size > sizeof(PcGx.palettes) - offset) {
        Fail("Palette upload out of bounds", offset);
        return;
    }
    memcpy(PcGx.palettes + offset, src, size);
}
void PcGx_ReadRearPlane(PcGxRearPixel pixels[256 * 192]) {
    bool image = (PcGxDisplay3D & REG_G3X_DISP3DCNT_PRI_MASK) != 0;
    u32 offset = reg_G3X_CLRIMAGE_OFFSET;
    for (unsigned y = 0; y < 192; y++) {
        for (unsigned x = 0; x < 256; x++) {
            unsigned source = (((y + (offset >> 8)) & 255) * 256) + ((x + offset) & 255);
            u32 color = reg_G3X_CLEAR_COLOR;
            u32 depth = reg_G3X_CLEAR_DEPTH & 0x7fff;
            if (image) {
                color = (PcGx.clear_color[source] & 0x7fff) | ((PcGx.clear_color[source] & 0x8000) ? 31u << 16 : 0);
                depth = PcGx.clear_depth[source] & 0x7fff;
            }
            pixels[y * 256 + x].color = color;
            // The hardware expands 15-bit Z to 24 bits; only the far endpoint fills the low nine bits.
            pixels[y * 256 + x].depth = (float)((depth << 9) + (depth == 0x7fff ? 511 : 0)) / 16777215.0f;
        }
    }
}

void GX_SetBankForClearImage(GXVRamClearImage banks) {
    clearImageBanks = banks;
    if (banks == GX_VRAM_CLEARIMAGE_NONE)
        PcGxDisplay3D &= ~REG_G3X_DISP3DCNT_PRI_MASK;
    else
        PcGxDisplay3D |= REG_G3X_DISP3DCNT_PRI_MASK;
}

GXVRamClearImage GX_ResetBankForClearImage(void) {
    GXVRamClearImage previous = clearImageBanks;
    clearImageBanks = GX_VRAM_CLEARIMAGE_NONE;
    return previous;
}

GXVRamClearImage GX_DisableBankForClearImage(void) {
    return GX_ResetBankForClearImage();
}

void GX_BeginLoadClearImage(void) {
    // Host clear images have dedicated storage; no VRAM bank remap is required.
}
void GX_LoadClearImageColor(const void *src, u32 size) {
    if (size > sizeof(PcGx.clear_color)) {
        Fail("Clear color image upload out of bounds", size);
        return;
    }
    memcpy(PcGx.clear_color, src, size);
}
void GX_LoadClearImageDepth(const void *src, u32 size) {
    if (size > sizeof(PcGx.clear_depth)) {
        Fail("Clear depth image upload out of bounds", size);
        return;
    }
    memcpy(PcGx.clear_depth, src, size);
}
void GX_EndLoadClearImage(void) {
    // Uploads are synchronous on the game thread.
}
