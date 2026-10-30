#include "../gx_backend.h"
#include "gfl/bg_sys.h"
#include "gfl/g3d.h"
#include "gfl/overlay.h"
#include "gfl/ui.h"
#include "nitro/crypto/rc4.h"
#include "nitro/host_cp.h"
#include "nitro/math.h"
#include "nnsys/fnd.h"
#include "nnsys/gfd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sanitized builds retain network callback tables. These application-only entry
// points must never run in a renderer test; keep that boundary explicit.
void GCTX_HIDUnblockSoftReset(u8 flags) {
    (void)flags;
    abort();
}

BOOL GFL_OvlLoad(u32 id) {
    (void)id;
    abort();
}

void GFL_OvlUnload(u32 id) {
    (void)id;
    abort();
}

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);                                            \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

static int CheckRearPlane(void) {
    static PcGxRearPixel pixels[256 * 192];
    static u16 colors[256 * 256], depths[256 * 256];
    G3X_Init();
    PcGx_ReadRearPlane(pixels);
    CHECK(((pixels[0].color >> 16) & 31) == 0);
    CHECK(pixels[0].depth == 1.0f);
    G3X_SetClearColor(GX_RGB(5, 12, 23), 17, 0x4000, 9, TRUE);
    PcGx_ReadRearPlane(pixels);
    CHECK((pixels[0].color & 0x7fff) == GX_RGB(5, 12, 23));
    CHECK(((pixels[0].color >> 16) & 31) == 17);
    CHECK(fabsf(pixels[0].depth - 8388608.0f / 16777215.0f) < 0.000001f);
    CHECK(memcmp(&pixels[0], &pixels[256 * 192 - 1], sizeof(pixels[0])) == 0);
    G3X_SetClearColor(0, 0, 0, 0, FALSE);
    PcGx_ReadRearPlane(pixels);
    CHECK(pixels[0].depth == 0 && pixels[0].color == 0);

    colors[255 * 256 + 255] = 0x8000 | GX_RGB(31, 0, 0);
    depths[255 * 256 + 255] = 0xffff;
    colors[255 * 256] = GX_RGB(0, 31, 0);
    depths[255 * 256] = 0x4000;
    colors[255] = 0x8000 | GX_RGB(0, 0, 31);
    depths[255] = 0;
    GX_BeginLoadClearImage();
    GX_LoadClearImageColor(colors, sizeof(colors));
    GX_LoadClearImageDepth(depths, sizeof(depths));
    GX_EndLoadClearImage();
    reg_G3X_CLRIMAGE_OFFSET = 0xffff;
    // Uploading buffers alone must not enable the image rear plane.
    PcGx_ReadRearPlane(pixels);
    CHECK(pixels[0].color == 0 && pixels[0].depth == 0);
    GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_256_AB);
    PcGx_ReadRearPlane(pixels);
    CHECK(pixels[0].color == ((31u << 16) | GX_RGB(31, 0, 0)));
    CHECK(pixels[0].depth == 1.0f);
    CHECK(pixels[1].color == GX_RGB(0, 31, 0));
    CHECK(fabsf(pixels[1].depth - 8388608.0f / 16777215.0f) < 0.000001f);
    CHECK(pixels[256].color == ((31u << 16) | GX_RGB(0, 0, 31)));
    CHECK(pixels[256].depth == 0);
    GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_NONE);
    G3X_SetClearColor(GX_RGB(31, 31, 31), 31, 0x7fff, 0, FALSE);
    PcGx_ReadRearPlane(pixels);
    CHECK(pixels[0].color == ((31u << 16) | GX_RGB(31, 31, 31)));
    CHECK(pixels[0].depth == 1.0f);
    G3X_Init();
    return EXIT_SUCCESS;
}

int main(void) {
    CHECK(CheckRearPlane() == EXIT_SUCCESS);
    NNSG3dResAnmHeader animation_header = { .category0 = NNS_G3D_DATABLK_JNT_ANM };
    NNSG3dAnmObj *animation = calloc(1, NNS_G3D_ANMOBJ_SIZE(32));
    CHECK(animation);
    animation->resAnm = &animation_header;
    animation->numMapData = 32;
    animation->mapData[31] = NNS_G3D_ANMOBJ_MAPDATA_EXIST;
    NNSG3dRenderObj render_object = { 0 };
    NNS_G3dRenderObjAddAnmObj(&render_object, animation);
    CHECK(render_object.hintJntAnmExist[0] == 0x80000000u);
    NNS_G3dRenderObjRemoveAnmObj(&render_object, animation);
    free(animation);
    G3X_SetDisp1DotDepth(FX32_ONE);
    CHECK(PcGx1DotDepth == 8);
    PcGx_ResetHardware();
    CHECK(PcGx1DotDepth == 0);
    PC_GX_REG16(REG_MOSAIC_ADDR) = 0xab00;
    PC_GX_REG16(REG_DB_MOSAIC_ADDR) = 0xcd00;
    G2_SetBGMosaicSize(3, 5);
    G2S_SetBGMosaicSize(2, 7);
    CHECK(PC_GX_REG16(REG_MOSAIC_ADDR) == 0xab53);
    CHECK(PC_GX_REG16(REG_DB_MOSAIC_ADDR) == 0xcd72);
    CHECK(G2_GetOBJCharPtr() == PcGx2D[0].object);
    CHECK(G2S_GetOBJCharPtr() == PcGx2D[1].object);
    PcGxLcdc[0x60000] = 0x5a;
    PcGxCapturedBanks = GX_VRAM_D;
    GX_SetBankForBG(GX_VRAM_BG_128_D);
    CHECK(PcGxBackground[0][0] == 0x5a);
    PcGxBackground[0][1] = 0xa5;
    CHECK(GX_ResetBankForBG() == GX_VRAM_BG_128_D);
    CHECK(PcGxLcdc[0x60001] == 0xa5);
    GX_SetBankForTex(GX_VRAM_TEX_01_AD);
    CHECK(PcGx.textures[0x20000] == 0x5a && PcGx.textures[0x20001] == 0xa5);
    PcGxWideCaptureValid = TRUE;
    PcGxCapturedBanks = GX_VRAM_D;
    PcGxMainDisplay = (1u << 10) | GX_BGMODE_5;
    PcGxRegisters[0].halves[6] = 0x4084;
    CHECK(PcGx_WideCaptureActive());
    PcGxMainDisplay = (1u << 10) | GX_BGMODE_0;
    PcGxRegisters[0].halves[6] = 0x1f12;
    CHECK(!PcGx_WideCaptureActive());
    PcGxMainDisplay = (1u << 10) | GX_BGMODE_5;
    CHECK(!PcGx_WideCaptureActive());
    PcGxCapturedBanks = 0;
    PcGx_ResetHardware();
    GXBoxTestParam box = { 0, 0, 0, FX32_ONE / 2, FX32_ONE / 2, FX32_ONE / 2 };
    u32 box_words[3];
    s32 box_result;
    memcpy(box_words, &box, sizeof(box));
    for (unsigned i = 0; i < 3; i++)
        PcGx_Write(0x040005c0, box_words[i]);
    CHECK(G3X_GetBoxTestResult(&box_result) == 0 && box_result != 0);
    box.x = 4 * FX32_ONE;
    memcpy(box_words, &box, sizeof(box));
    for (unsigned i = 0; i < 3; i++)
        PcGx_Write(0x040005c0, box_words[i]);
    CHECK(G3X_GetBoxTestResult(&box_result) == 0 && box_result == 0);
    box.x = FX32_ONE + FX32_ONE / 8;
    memcpy(box_words, &box, sizeof(box));
    for (unsigned i = 0; i < 3; i++)
        PcGx_Write(0x040005c0, box_words[i]);
    CHECK(G3X_GetBoxTestResult(&box_result) == 0 && box_result == 0);
    PcGx.wide_scene = true;
    for (unsigned i = 0; i < 3; i++)
        PcGx_Write(0x040005c0, box_words[i]);
    CHECK(G3X_GetBoxTestResult(&box_result) == 0 && box_result != 0);
    PcGx.wide_scene = false;
    CHECK(!PcGx.failed);
    NNSGfdVramTransferTask transfers[2];
    const u16 palette[] = { 0x1234, 0x5678 };
    NNS_GfdInitVramTransferManager(transfers, 2);
    CHECK(NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 2, palette, sizeof(palette)));
    CHECK(NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_PLTT_SUB, 6, palette, sizeof(palette)));
    CHECK(!NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 2, palette, sizeof(palette)));
    CHECK(memcmp(PcGx2D[0].background_palette + 2, palette, sizeof(palette)) != 0);
    NNS_GfdDoVramTransfer();
    CHECK(memcmp(PcGx2D[0].background_palette + 2, palette, sizeof(palette)) == 0);
    CHECK(memcmp(PcGx2D[1].object_palette + 6, palette, sizeof(palette)) == 0);
    CHECK(NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_MAIN, 2, palette, sizeof(palette)));
    NNS_GfdClearVramTransferManagerTask();
    MATHSHA1Context sha1;
    u8 digest[MATH_SHA1_DIGEST_SIZE];
    static const u8 expected[20] = { 0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
                                     0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d };
    MATH_SHA1Init(&sha1);
    MATH_SHA1Update(&sha1, "a", 1);
    MATH_SHA1Update(&sha1, "bc", 2);
    MATH_SHA1GetHash(&sha1, digest);
    CHECK(memcmp(digest, expected, sizeof(digest)) == 0);
    CRYPTORC4Context cipher;
    u8 encrypted[9];
    static const u8 cipher_expected[9] = { 0xbb, 0xf3, 0x16, 0xe8, 0xd9, 0x40, 0xaf, 0x0a, 0xd3 };
    CRYPTO_RC4Init(&cipher, "Key", 3);
    CRYPTO_RC4Encrypt(&cipher, "Pla", 3, encrypted);
    CRYPTO_RC4Encrypt(&cipher, "intext", 6, encrypted + 3);
    CHECK(memcmp(encrypted, cipher_expected, sizeof(encrypted)) == 0);
    void *arena = malloc(4096);
    CHECK(arena);
    NNSFndHeapHandle heap = NNS_FndCreateExpHeapEx(arena, 4096, NNS_FND_HEAP_OPT_0_CLEAR);
    CHECK(heap);
    u32 free_size = NNS_FndGetTotalFreeSizeForExpHeap(heap);
    void *head = NNS_FndAllocFromExpHeapEx(heap, 3, 4);
    void *tail = NNS_FndAllocFromExpHeapEx(heap, 17, -64);
    CHECK(head && tail && head != tail);
    CHECK((uintptr_t)head % sizeof(void *) == 0 && (uintptr_t)tail % 64 == 0);
    CHECK(*(const u8 *)head == 0 && *(const u8 *)tail == 0);
    CHECK(NNS_FndGetSizeForMBlockExpHeap(head) >= 3);
    CHECK(NNS_FndResizeForMBlockExpHeap(heap, head, 65) >= 65);
    NNS_FndFreeToExpHeap(heap, tail);
    NNS_FndFreeToExpHeap(heap, head);
    CHECK(NNS_FndGetTotalFreeSizeForExpHeap(heap) == free_size);
    NNS_FndDestroyExpHeap(heap);
    free(arena);

    CHECK(FX_Div(3 * FX32_ONE, 2 * FX32_ONE) == FX32_ONE + FX32_ONE / 2);
    CHECK(FX_Div(-3 * FX32_ONE, 2 * FX32_ONE) == -FX32_ONE - FX32_ONE / 2);
    CHECK(FX_Sqrt(9 * FX32_ONE) == 3 * FX32_ONE);
    PcCpSqrtMode = 1;
    PcCpSqrtParameter.wide = UINT64_MAX;
    CHECK(*PcCp_GetSqrt() == UINT32_MAX);

    G3X_Init();
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_Translate(FX32_ONE, 2 * FX32_ONE, 3 * FX32_ONE);
    G3_PushMtx();
    G3_Scale(2 * FX32_ONE, 3 * FX32_ONE, 4 * FX32_ONE);
    MtxFx44 matrix;
    G3X_GetClipMtx(&matrix);
    CHECK(matrix.m[0][0] == 2 * FX32_ONE && matrix.m[1][1] == 3 * FX32_ONE && matrix.m[2][2] == 4 * FX32_ONE);
    CHECK(matrix.m[3][0] == FX32_ONE && matrix.m[3][1] == 2 * FX32_ONE && matrix.m[3][2] == 3 * FX32_ONE);
    G3_PopMtx(1);
    G3X_GetClipMtx(&matrix);
    CHECK(matrix.m[0][0] == FX32_ONE && matrix.m[3][0] == FX32_ONE);
    // MCSS alternates orthographic sprites with perspective shadows in the same draw.
    G3_StoreMtx(0);
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_Scale(2 * FX32_ONE, 3 * FX32_ONE, FX32_ONE);
    G3_StoreMtx(0);
    G3_Identity();
    G3_RestoreMtx(0);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_Identity();
    G3_RestoreMtx(0);
    G3X_GetClipMtx(&matrix);
    CHECK(matrix.m[0][0] == 2 * FX32_ONE && matrix.m[1][1] == 3 * FX32_ONE);
    CHECK(matrix.m[3][0] == 2 * FX32_ONE && matrix.m[3][1] == 6 * FX32_ONE);
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_Identity();
    G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_NORMAL, GX_TEXSIZE_S8, GX_TEXSIZE_T8, GX_TEXREPEAT_NONE,
                     GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, 0);
    G3_TexCoord(2 * FX32_ONE, 3 * FX32_ONE);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 63, 15, 0);
    G3_Begin(GX_BEGIN_TRIANGLES);
    PcGx.wide_scene = true;
    for (unsigned i = 0; i < 3; i++) {
        G3_Normal(FX32_ONE / 2, 0, 0);
        G3_Vtx((fx16)(i * FX32_ONE), 0, 0);
    }
    G3_End();
    CHECK(!PcGx.failed && PcGx.vertex_count == 3);
    CHECK(((PcGx.vertices[0].polygon >> 24) & 63) == 63);
    CHECK((PcGx.vertices[0].polygon & PC_GX_POLYGON_UI) == 0);
    CHECK(fabsf(PcGx.vertices[1].clip[0] - 0.75f) < 0.001f);
    PcGx.wide_scene = false;
    CHECK(PcGx.vertices[0].uv[0] == PcGx.vertices[1].uv[0]);
    CHECK(PcGx.vertices[1].uv[0] == PcGx.vertices[2].uv[0]);

    GFL_BGSysCreate(HEAPID_SYSTEM);
    const BGSetup setup = { .screenSize = 0x800,
                            .resolution = BGRES_256x256,
                            .colorMode = GX_BG_COLORMODE_16,
                            .screenBase = GX_BG_SCRBASE_0xf800,
                            .charBase = GX_BG_CHARBASE_0x04000,
                            .charSize = 0x4000,
                            .priority = 2 };
    GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
    CHECK(G2_GetBG3CharPtr() == PcGxBackground[0] + 0x4000);
    CHECK(G2_GetBG3ScrPtr() == PcGxBackground[0] + 0xf800);
    const u32 tile[8] = { 0x12345678, 0x9abcdef0 };
    GFL_BGSysLoadChar(3, tile, sizeof(tile), 1);
    CHECK(memcmp(PcGxBackground[0] + 0x4020, tile, sizeof(tile)) == 0);
    GFL_BGSysFillScr(3, 0x5011);
    GFL_BGSysLoadScr(3);
    CHECK(((const u16 *)(const void *)(PcGxBackground[0] + 0xf800))[0] == 0x5011);
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysUpdate();
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysUpdate();
    GFL_BGSysLoadScr(3);
    GFL_BGSysFree();
    CHECK(!PcGx.failed);
    GFL_G3DSysCreate(FALSE, 4, FALSE, 6, 0, HEAPID_SYSTEM, NULL);
    NNSGfdTexKey first_texture = NNS_GfdAllocTexVram(4096, FALSE, 0);
    NNSGfdTexKey second_texture = NNS_GfdAllocTexVram(4096, FALSE, 0);
    CHECK(first_texture && second_texture && first_texture != second_texture);
    CHECK(NNS_GfdFreeTexVram(first_texture) == 0);
    CHECK(NNS_GfdFreeTexVram(second_texture) == 0);
    GFL_G3DSysFree();
    CHECK(!PcGx.failed);
    PcGx.vertex_count = 0;
    PcGxMainDisplay = 1u << 11;
    PcGx_DrawBackgroundRegion(0, (PcViewport){ 336, 0, 48, 216 }, (SDL_FRect){ 255, 0, 0, 192 }, 8);
    CHECK(PcGx.vertex_count == 6);
    CHECK(PcGx.vertices[0].clip[0] == 0.75f && PcGx.vertices[2].clip[0] == 1.0f);
    CHECK(PcGx.vertices[0].uv[0] == 255 && PcGx.vertices[2].uv[0] == 255);
    CHECK(PcGx.vertices[2].uv[1] == 192);
    PcGx.vertex_count = 0;
    PcGxMainDisplay = 1u << 12;
    memset(PcGx2D[0].oam, 0, sizeof(PcGx2D[0].oam));
    GXOamAttr *oam = (GXOamAttr *)(void *)PcGx2D[0].oam;
    for (unsigned i = 0; i < 128; i++)
        oam[i].rsMode = 2;
    oam[0].rsMode = oam[1].rsMode = 0;
    oam[0].y = 8;
    oam[1].y = 160;
    oam[1].x = 200;
    PcGx_DrawBannerObjects(0, (PcViewport){ 48, 0, 288, 216 });
    CHECK(PcGx.vertex_count == 12);
    CHECK(PcGx.vertices[0].clip[0] == -1.0f);
    CHECK(fabsf(PcGx.vertices[6].clip[0] - 0.671875f) < 0.000001f);
    CHECK(fabsf(PcGx.vertices[1].clip[0] - PcGx.vertices[0].clip[0] - 0.046875f) < 0.000001f);
    puts("Native fixed-point math, geometry state, and original GFL backgrounds passed");
    return EXIT_SUCCESS;
}
