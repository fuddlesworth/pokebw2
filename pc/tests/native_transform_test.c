#include "../gx_backend.h"
#include "nitro/host_gx.h"
#include "nnsys/g3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "Transform check failed at line %d: %s\n", __LINE__, #condition);                          \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

int main(void) {
    // Exercise every specialized texture path with reflection, negative translations and scale deltas.
    for (unsigned mode = 0; mode < 3; mode++) {
        for (unsigned flags = 0; flags < 8; flags++) {
            for (int sign = -1; sign <= 1; sign += 2) {
                NNSG3dMatAnmResult material = { 0 };
                material.flag = flags | NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SET;
                material.origWidth = material.origHeight = 32;
                material.magW = material.magH = FX32_ONE;
                material.scaleS = material.scaleT = 2 * FX32_ONE;
                material.cosR = -FX32_ONE;
                material.transS = sign * FX32_ONE;
                material.transT = -sign * FX32_ONE;
                double scale = flags & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SCALEONE ? 1 : 2;
                double rotation = flags & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_ROTZERO ? 1 : -1;
                double s = flags & NNS_G3D_MATANM_RESULTFLAG_TEXMTX_TRANSZERO ? 0 : sign;
                double t = -s;
                double expected_s, expected_t;
                PcGx_ResetHardware();
                if (mode == 0) {
                    NNSi_G3dSendTexSRT3dsMax(&material);
                    expected_s = 32 * (0.5 + scale * rotation * (-0.5 - s));
                    expected_t = 32 * (0.5 + scale * rotation * (-0.5 + t));
                } else if (mode == 1) {
                    NNSi_G3dSendTexSRTXsi(&material);
                    expected_s = -32 * scale * rotation * s;
                    expected_t = -32 * (scale * rotation * (1 - t) - 1);
                } else {
                    NNSi_G3dSendTexSRTSi3d(&material);
                    expected_s = -32 * scale * s;
                    expected_t = -32 * scale * t;
                    rotation = 1;
                }
                G3_TexImageParam(GX_TEXFMT_DIRECT, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S32, GX_TEXSIZE_T32,
                                 GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_USE, 0);
                G3_Begin(GX_BEGIN_TRIANGLES);
                G3_TexCoord(0, 0);
                G3_Vtx(0, 0, 0);
                G3_TexCoord(FX32_ONE, FX32_ONE);
                G3_Vtx(FX32_ONE, 0, 0);
                G3_Vtx(0, FX32_ONE, 0);
                G3_End();
                CHECK(PcGx.vertex_count == 3 && !PcGx.failed);
                CHECK(PcGx.vertices[0].uv[0] == expected_s && PcGx.vertices[0].uv[1] == expected_t);
                CHECK(PcGx.vertices[1].uv[0] == expected_s + scale * rotation);
                CHECK(PcGx.vertices[1].uv[1] == expected_t + scale * rotation);
            }
        }
    }
    PcGx_ResetHardware();
    NNSG3dResMdl model = { 0 };
    NNSG3dRenderObj object = { 0 };
    NNSG3dRS state = { 0 };
    u8 command[] = { NNS_G3D_SBC_NODEMIX, 2, 1, 0, 0, 128 };
    object.resMdl = &model;
    state.pRenderObj = &object;
    state.c = command;
    state.isEvpCached[0] = 1;
    MTX_Identity44_(&NNS_G3dRSOnGlb.evpCache[0].clip);
    MTX_Identity33_(&NNS_G3dRSOnGlb.evpCache[0].normal);
    NNS_G3dRSOnGlb.evpCache[0].clip.m[3][0] = 4 * FX32_ONE;
    NNSi_G3dFuncSbc_NODEMIX(&state, 0);
    MtxFx44 clip;
    MtxFx33 normal;
    CHECK(G3X_GetClipMtx(&clip) == 0 && G3X_GetVectorMtx(&normal) == 0);
    CHECK(clip.m[0][0] == FX32_ONE / 2 && clip.m[3][0] == 2 * FX32_ONE);
    CHECK(normal.m[0][0] == FX32_ONE / 2 && normal.m[1][1] == FX32_ONE / 2);
    CHECK(normal.m[0][1] == 0 && state.c == command + sizeof(command));
    u32 registers[4] = { 0 };
    MtxFx22 affine = { ._00 = FX32_ONE, ._01 = -FX32_ONE, ._10 = 0, ._11 = FX32_ONE };
    G2x_SetBGyAffine_((uptr)registers, &affine, -1, -1, 0, 0);
    CHECK(registers[0] == 0xff000100u && registers[1] == 0x01000000u);
    CHECK(registers[2] == 0xffffff00u && registers[3] == 0);
    G2_SetBG0Offset(-1, -5);
    G2S_SetBG2Offset(-513, -517);
    CHECK(PC_GX_REG32(0x04000010) == 0x01fb01ffu);
    CHECK(PC_GX_REG32(0x04001018) == 0x01fb01ffu);
    puts("Native texture branches, NODEMIX matrices and affine packing passed");
    return EXIT_SUCCESS;
}
