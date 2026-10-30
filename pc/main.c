#include "battle_ui.h"
#include "boot.h"
#include "dex_ui.h"
#include "filesystem.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/g3d.h"
#include "gfl/msg.h"
#include "gx_backend.h"
#include "heap.h"
#include "menu_ui.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "nnsys/g3d.h"
#include "party_ui.h"
#include "settings.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "timing.h"
#include "ui_draw.h"
#include "video.h"
#include <SDL3/SDL_main.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *model_file;
static char **launch_arguments;

void OS_ResetSystem(u32 parameter) {
    (void)parameter;
    SDL_Process *process = SDL_CreateProcess((const char *const *)launch_arguments, false);
    if (!process) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Restarting game: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_DestroyProcess(process);
    exit(EXIT_SUCCESS);
}
static void *texture_file;
static G3DResource *model_resource;
static G3DResource *texture_resource;
static G3DModel *model_object;
static bool model_system_loaded;
static bool model_texture_uploaded;
static void *animation_file;
static NNSG3dAnmObj *animation_object;
static u16 animation_frames;
static void *sprite_files[6];
static bool background_loaded;
static bool text_loaded;
static void *sprite_work;
static NNSG2dMultiCellAnimation sprite_animation;
static NNSG2dCellDataBank *sprite_cells;
static NNSG2dRendererInstance sprite_renderer;
static NNSG2dRenderSurface sprite_surface;
static NNSG2dImageProxy sprite_image;
static NNSG2dImagePaletteProxy sprite_palette;
static NNSGfdTexKey sprite_texture_key;
static NNSGfdPlttKey sprite_palette_key;
static bool hardware_sprites;
static NNSG2dOamManagerInstance sprite_oam;
static NNSG2dCellAnimation title_cell_animation;

static BOOL RegisterSpriteOam(const GXOamAttr *oam, u16 affine_index, BOOL double_affine) {
    (void)double_affine;
    return NNS_G2dEntryOamManagerOamWithAffineIdx(&sprite_oam, oam, affine_index);
}
static u16 RegisterSpriteAffine(const MtxFx22 *matrix) {
    return NNS_G2dEntryOamManagerAffine(&sprite_oam, matrix);
}

static void *LoadResource(const char *path, int entry, size_t *size) {
    void *data;
    if (entry < 0)
        data = SDL_LoadFile(path, size);
    else {
        const char *paths[] = { path };
        GFL_ArcSysInit(paths, 1);
        *size = GFL_ArcSysGetDataLength(0, (u32)entry);
        data = GFL_ArcSysReadHeapNew(0, (u32)entry, HEAPID_SYSTEM);
        GFL_ArcSysInit(NULL, 0);
    }
    if (data && *size >= 4 && (*(u8 *)data == 0x10 || *(u8 *)data == 0x11)) {
        *size = MI_GetUncompressedSize(data);
        void *decoded = SDL_malloc(*size);
        if (!decoded) {
            GFL_HeapFree(data);
            return NULL;
        }
        MI_UncompressLZ8(data, decoded);
        GFL_HeapFree(data);
        data = decoded;
    }
    return data;
}

static bool LoadSprite(const char *archive) {
    size_t size;
    sprite_files[0] = LoadResource(archive, hardware_sprites ? 5 : 25, &size);
    sprite_files[1] = LoadResource(archive, hardware_sprites ? 4 : 28, &size);
    sprite_files[2] = LoadResource(archive, hardware_sprites ? 6 : 27, &size);
    sprite_files[3] = LoadResource(archive, hardware_sprites ? 7 : 24, &size);
    if (!hardware_sprites) {
        sprite_files[4] = LoadResource(archive, 29, &size);
        sprite_files[5] = LoadResource(archive, 30, &size);
    }
    NNSG2dCharacterData *characters;
    NNSG2dPaletteData *palette;
    NNSG2dAnimBankData *animations, *multi_animations;
    NNSG2dMultiCellDataBank *multi_cells;
    if (!hardware_sprites) {
        GFL_BGSysCreate(HEAPID_SYSTEM);
        background_loaded = true;
        const BGSetup setup = { .screenSize = 0x800,
                                .resolution = BGRES_256x256,
                                .colorMode = GX_BG_COLORMODE_16,
                                .screenBase = GX_BG_SCRBASE_0xf800,
                                .charBase = GX_BG_CHARBASE_0x00000,
                                .charSize = 0x8000,
                                .priority = 3 };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        const char *paths[] = { archive };
        GFL_ArcSysInit(paths, 1);
        ArcTool *arc = GFL_ArcSysCreateFileHandle(0, HEAPID_SYSTEM);
        GFL_BGSysLoadArcNCGRStatic(arc, 0, 3, 0, 0, FALSE, HEAPID_SYSTEM);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 3, 0, 0, FALSE, HEAPID_SYSTEM);
        GFL_G2DIOLoadArcNCLRDefault(arc, 2, PALTYPE_MAIN_BG, 0, 0, HEAPID_SYSTEM);
        GFL_ArcToolFree(arc);
        GFL_ArcSysInit(NULL, 0);
        PcGxMainDisplay |= 1u << 11;
    }
    if (!sprite_files[0] || !sprite_files[1] || !sprite_files[2] ||
        !NNS_G2dGetUnpackedCharacterData(sprite_files[0], &characters) ||
        !NNS_G2dGetUnpackedPaletteData(sprite_files[1], &palette) ||
        !NNS_G2dGetUnpackedCellBank(sprite_files[2], &sprite_cells))
        return SDL_SetError("Unable to load intro sprite");
    if (!sprite_files[3] || !NNS_G2dGetUnpackedAnimBank(sprite_files[3], &animations))
        return SDL_SetError("Unable to load cell animation");
    if (hardware_sprites) {
        NNS_G2dInitCellAnimation(&title_cell_animation, NNS_G2dGetAnimSequenceByIdx(animations, 0), sprite_cells);
    } else {
        if (!sprite_files[4] || !sprite_files[5] || !NNS_G2dGetUnpackedMCAnimBank(sprite_files[4], &multi_animations) ||
            !NNS_G2dGetUnpackedMultiCellBank(sprite_files[5], &multi_cells))
            return SDL_SetError("Unable to load intro animation");
        sprite_work = SDL_calloc(1, NNS_G2dGetMCWorkAreaSize(multi_cells, NNS_G2D_MCTYPE_SHARE_CELLANIM));
        if (!sprite_work)
            return false;
        NNS_G2dInitMCAnimationInstance(&sprite_animation, sprite_work, animations, sprite_cells, multi_cells,
                                       NNS_G2D_MCTYPE_SHARE_CELLANIM);
        NNS_G2dSetAnimSequenceToMCAnimation(&sprite_animation, NNS_G2dGetAnimSequenceByIdx(multi_animations, 0));
    }
    NNS_G2dInitImageProxy(&sprite_image);
    NNS_G2dInitImagePaletteProxy(&sprite_palette);
    sprite_texture_key = NNS_GfdAllocTexVram(characters->szByte, FALSE, 0);
    sprite_palette_key = NNS_GfdAllocPlttVram(palette->szByte, FALSE, 0);
    if (!sprite_texture_key || !sprite_palette_key)
        return SDL_SetError("Sprite texture allocation failed");
    NNS_G2dLoadImage1DMapping(characters, NNS_GfdGetTexKeyAddr(sprite_texture_key), NNS_G2D_VRAM_TYPE_3DMAIN,
                              &sprite_image);
    NNS_G2dLoadPalette(palette, NNS_GfdGetPlttKeyAddr(sprite_palette_key), NNS_G2D_VRAM_TYPE_3DMAIN, &sprite_palette);
    if (hardware_sprites) {
        NNS_G2dLoadImage1DMapping(characters, 0, NNS_G2D_VRAM_TYPE_2DMAIN, &sprite_image);
        NNS_G2dLoadPalette(palette, 0, NNS_G2D_VRAM_TYPE_2DMAIN, &sprite_palette);
        NNS_G2dInitOamManagerModule();
        if (!NNS_G2dGetNewOamManagerInstanceAsFastTransferMode(&sprite_oam, 0, 128, NNS_G2D_OAMTYPE_MAIN))
            return SDL_SetError("Unable to create original OAM manager");
        PcGxMainDisplay |= 1u << 12;
    }
    NNS_G2dInitRenderer(&sprite_renderer);
    NNS_G2dInitRenderSurface(&sprite_surface);
    sprite_surface.type = NNS_G2D_SURFACETYPE_MAIN3D;
    sprite_surface.viewRect.sizeView.x = PC_CANVAS_WIDTH * FX32_ONE;
    sprite_surface.viewRect.sizeView.y = PC_CANVAS_HEIGHT * FX32_ONE;
    if (hardware_sprites) {
        sprite_surface.type = NNS_G2D_SURFACETYPE_MAIN2D;
        sprite_surface.viewRect.sizeView.x = 256 * FX32_ONE;
        sprite_surface.viewRect.sizeView.y = 192 * FX32_ONE;
        sprite_surface.pFuncOamRegister = RegisterSpriteOam;
        sprite_surface.pFuncOamAffineRegister = RegisterSpriteAffine;
    }
    NNS_G2dAddRendererTargetSurface(&sprite_renderer, &sprite_surface);
    NNS_G2dSetRendererImageProxy(&sprite_renderer, &sprite_image, &sprite_palette);
    SDL_Log("Original G2D sprite: %u cells, %u character bytes, format=%u", sprite_cells->numCells, characters->szByte,
            characters->characterFmt);
    return !PcGx.failed;
}

static bool LoadIntroText(const char *root) {
    char font_path[4096], message_path[4096];
    if (SDL_snprintf(font_path, sizeof(font_path), "%s/a/0/2/3", root) >= (int)sizeof(font_path) ||
        SDL_snprintf(message_path, sizeof(message_path), "%s/a/0/0/2", root) >= (int)sizeof(message_path))
        return SDL_SetError("Resource path is too long");
    const char *paths[] = { font_path, message_path };
    GFL_ArcSysInit(paths, 2);
    Font *font = GFL_FontCreate(0, 0, 1, FALSE, HEAPID_SYSTEM);
    MsgData *messages = GFL_MsgSysLoadData(TRUE, 1, 54, HEAPID_SYSTEM);
    GFL_TextRndInit(HEAPID_SYSTEM);
    text_loaded = true;
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    StrBuf *message = GFL_StrBufCreate(1600, HEAPID_SYSTEM);
    GFL_MsgDataLoadStrbuf(messages, 6, message);
    const BGSetup setup = { .screenSize = 0x1000,
                            .resolution = BGRES_512x256,
                            .colorMode = GX_BG_COLORMODE_16,
                            .screenBase = GX_BG_SCRBASE_0x7000,
                            .charBase = GX_BG_CHARBASE_0x04000,
                            .charSize = 0x3000,
                            .priority = 0 };
    GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
    const u32 blank_tile[8] = { 0 };
    GFL_BGSysAllocChar(1, sizeof(blank_tile), FALSE);
    GFL_BGSysLoadChar(1, blank_tile, sizeof(blank_tile), 0);
    GFL_BGSysFillScr(1, 0);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0, HEAPID_SYSTEM);
    GFL_G2DIOLoadArcNCLRDefault(arc, 5, PALTYPE_MAIN_BG, 10 * 0x20, 0x20, HEAPID_SYSTEM);
    GFL_ArcToolFree(arc);
    BmpWin_InitAllocator(HEAPID_SYSTEM);
    BmpWin *window = BmpWin_CreateDynamic(1, 1, 22, 46, 4, 10, FALSE);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 4, 0, message, font);
    BmpWin_TransferNow(window);
    BmpWin_Free(window);
    BmpWin_FreeAllocator();
    GFL_StrBufFree(message);
    GFL_MsgDataFree(messages);
    GFL_FontFree(font);
    GFL_ArcSysInit(NULL, 0);
    PcGxMainDisplay |= 1u << 9;
    return !PcGx.failed;
}

static void DrawSprite(void) {
    MtxFx44 projection;
    MTX_OrthoW(0, PC_CANVAS_HEIGHT * FX32_ONE, 0, PC_CANVAS_WIDTH * FX32_ONE, 0, 100 * FX32_ONE, FX32_ONE, &projection);
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_LoadMtx44(&projection);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_TEXTURE);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_POSITION);
    G3_PolygonAttr(0, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 31, 0);
    G3_Color(GX_RGB(31, 31, 31));
    NNS_G2dBeginRendering(&sprite_renderer);
    NNS_G2dTranslate((hardware_sprites ? 128 : PC_CANVAS_WIDTH / 2) * FX32_ONE,
                     (hardware_sprites ? 96 : PC_CANVAS_HEIGHT / 2) * FX32_ONE, 0);
    if (hardware_sprites)
        NNS_G2dDrawCellAnimation(&title_cell_animation);
    else
        NNS_G2dDrawMultiCellAnimation(&sprite_animation);
    NNS_G2dEndRendering();
    if (hardware_sprites) {
        NNS_G2dApplyOamManagerToHW(&sprite_oam);
        PcGx_DrawObjects(0, (PcViewport){ 64, 12, 256, 192 });
        NNS_G2dResetOamManagerBuffer(&sprite_oam);
    }
    if (hardware_sprites)
        NNS_G2dTickCellAnimation(&title_cell_animation, FX32_ONE);
    else
        NNS_G2dTickMCAnimation(&sprite_animation, FX32_ONE);
}

static bool LoadModel(const char *path, int model_entry, unsigned model_offset, const char *texture_path,
                      int texture_entry, const char *animation_path) {
    size_t size;
    model_file = LoadResource(path, model_entry, &size);
    if (!model_file)
        return false;
    if (model_offset > size || model_offset % 4)
        return SDL_SetError("Invalid model resource offset");
    size -= model_offset;
    /* Embedded models must own an allocation base when GFL releases their resource. */
    if (model_offset)
        SDL_memmove(model_file, (const u8 *)model_file + model_offset, size);
    const NNSG3dResFileHeader *header = model_file;
    if (size < sizeof(*header) || header->signature != NNS_G3D_SIGNATURE_BMD0 || header->fileSize > size ||
        header->byteOrder != 0xfeff || header->dataBlocks < 1 || header->dataBlocks > 2) {
        return SDL_SetError("Unsupported or incomplete BMD resource: %s", path);
    }
    NNSG3dResMdl *model = NNS_G3dGetMdlByIdx(NNS_G3dGetMdlSet(header), 0);
    if (!model)
        return SDL_SetError("BMD has no model");
    NNSG3dResTex *tex = NNS_G3dGetTex(header);
    if (texture_path) {
        texture_file = LoadResource(texture_path, texture_entry, &size);
        if (!texture_file)
            return false;
        const NNSG3dResFileHeader *texture_header = texture_file;
        if (size < sizeof(*texture_header) || texture_header->signature != NNS_G3D_SIGNATURE_BTX0 ||
            texture_header->fileSize != size) {
            return SDL_SetError("Unsupported or incomplete BTX resource");
        }
        tex = NNS_G3dGetTex(texture_header);
    }
    GFL_G3DSysCreate(TRUE, 4, FALSE, 6, 0, HEAPID_SYSTEM, NULL);
    model_system_loaded = true;
    model_resource = GFL_HeapAllocate(HEAPID_SYSTEM, GFL_G3DResGetAllocSize(), FALSE, __FILE__, __LINE__);
    GFL_G3DResSetup(model_resource, model_file);
    model_file = NULL;
    if (texture_file) {
        texture_resource = GFL_HeapAllocate(HEAPID_SYSTEM, GFL_G3DResGetAllocSize(), FALSE, __FILE__, __LINE__);
        GFL_G3DResSetup(texture_resource, texture_file);
        texture_file = NULL;
    }
    if (tex) {
        model_texture_uploaded = GFL_G3DResUploadTexData(texture_resource ? texture_resource : model_resource);
        if (!model_texture_uploaded)
            return SDL_SetError("Model texture allocation failed");
        if (!NNS_G3dBindMdlTex(model, tex) || !NNS_G3dBindMdlPltt(model, tex))
            return SDL_SetError("Model texture binding failed");
    }
    model_object =
        GFL_G3DMdlCreate(model_resource, 0, tex ? (texture_resource ? texture_resource : model_resource) : NULL);
    if (animation_path) {
        animation_file = SDL_LoadFile(animation_path, &size);
        if (!animation_file || size < sizeof(NNSG3dResFileHeader))
            return SDL_SetError("Unable to load animation");
        void *animation = NNS_G3dGetAnmByIdx(animation_file, 0);
        if (!animation)
            return SDL_SetError("Resource has no animation");
        animation_object = SDL_calloc(1, NNS_G3dAnmObjCalcSizeRequired(animation, model));
        if (!animation_object)
            return false;
        NNS_G3dAnmObjInit(animation_object, animation, model, tex);
        NNS_G3dRenderObjAddAnmObj(GFL_G3DMdlGetEngineModel(model_object), animation_object);
        animation_frames = (u16)(NNS_G3dAnmObjGetNumFrame(animation_object) / FX32_ONE);
    }
    float scale = model->info.boxPosScale / 4096.0f;
    float radius = fmaxf(model->info.boxW, fmaxf(model->info.boxH, model->info.boxD)) * scale / 4096.0f;
    if (radius < 0.01f)
        radius = 10;
    VecFx32 target = { (fx32)((model->info.boxX + model->info.boxW * 0.5f) * scale),
                       (fx32)((model->info.boxY + model->info.boxH * 0.5f) * scale),
                       (fx32)((model->info.boxZ + model->info.boxD * 0.5f) * scale) };
    VecFx32 camera = { target.x + (fx32)(radius * 4096 * 2), target.y + (fx32)(radius * 4096 * 1.5f),
                       target.z + (fx32)(radius * 4096 * 3) };
    VecFx32 up = { 0, FX32_ONE, 0 };
    NNS_G3dGlbLookAt(&camera, &up, &target);
    fx32 extent = (fx32)(radius * 4096 * 0.75f);
    NNS_G3dGlbOrtho(extent, -extent, -extent * 16 / 9, extent * 16 / 9, FX32_ONE / 16, (fx32)(radius * 4096 * 10));
    NNS_G3dGlbLightVector(GX_LIGHTID_0, 0, -2896, -2896);
    NNS_G3dGlbLightColor(GX_LIGHTID_0, GX_RGB(31, 31, 31));
    SDL_Log("Original BW2 renderer: %u nodes, %u materials, %u shapes", model->info.numNode, model->info.numMat,
            model->info.numShp);
    return !PcGx.failed;
}

static void DrawDiagnostic(PcPixel *pixels, bool panel, bool pointer, float pointer_x, float pointer_y) {
    for (int y = 0; y < PC_CANVAS_HEIGHT; y++) {
        for (int x = 0; x < PC_CANVAS_WIDTH; x++) {
            PcPixel color = { 24, 32, 48, 255 };
            if (x % 24 == 0 || y % 24 == 0) {
                color = (PcPixel){ 42, 56, 72, 255 };
            }
            if (x == 0 || y == 0 || x == PC_CANVAS_WIDTH - 1 || y == PC_CANVAS_HEIGHT - 1) {
                color = (PcPixel){ 86, 202, 206, 255 };
            }
            // RGB patches make byte order mistakes visible without any game assets.
            if (y >= 12 && y < 28 && x >= 12 && x < 60) {
                color = (PcPixel){ x < 28 ? 255 : 0, x >= 28 && x < 44 ? 255 : 0, x >= 44 ? 255 : 0, 255 };
            }
            if (panel && x >= 264 && x < 372 && y >= 36 && y < 204) {
                color = (PcPixel){ 58, 74, 94, 255 };
            }
            if (pointer && abs(x - (int)pointer_x) <= 3 && abs(y - (int)pointer_y) <= 3) {
                color = (PcPixel){ 255, 210, 80, 255 };
            }
            pixels[y * PC_CANVAS_WIDTH + x] = color;
        }
    }
}

int main(int argc, char **argv) {
    launch_arguments = argv;
    unsigned long frame_limit = 0;
    bool resize_test = false;
    bool boot = false;
    PcBootEntry entry = PC_BOOT_MOVIES;
    const char *model_path = NULL, *texture_path = NULL, *screenshot_path = NULL;
    int model_entry = -1, texture_entry = -1;
    unsigned model_offset = 0;
    const char *animation_path = NULL;
    const char *sprite_archive = NULL;
    const char *data_root = NULL;
    char intro_path[4096];
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            puts("BW2 PC native port (work in progress)\n"
                 "Usage: pokebw2-pc [--data-root EXTRACTED_FILES] [--frames N] [--screenshot FILE.bmp]\n"
                 "  --data-root DIR       Intro background, animation, and original greeting text\n"
                 "  --boot                Run the original boot process with --data-root\n"
                 "  --title               Start directly at the original title process\n"
                 "  --menu                Start directly at the original main menu\n"
                 "  --new-game            Start directly at the original new-game intro\n"
                 "  --intro-sprite NARC   Intro background and multi-cell animation\n"
                 "  --title-sprite       Exercise original title-screen OAM animation\n"
                 "  --model FILE         Original NSBMD file, or NARC with --model-entry N\n"
                 "  --model-offset N     Offset of the NSBMD inside its archive member\n"
                 "  --texture FILE       Original NSBTX file, or NARC with --texture-entry N\n"
                 "  --animation FILE     Original skeletal animation\n"
                 "Tab: toggle diagnostic panel; mouse: pointer mapping; Esc: back (quit in diagnostics).\n"
                 "--frames N exits after N presentations; --resize-test exercises the swapchain.");
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "--title-sprite") == 0) {
            hardware_sprites = true;
        } else if (strcmp(argv[i], "--menu") == 0) {
            boot = true;
            entry = PC_BOOT_MENU;
        } else if (strcmp(argv[i], "--new-game") == 0) {
            boot = true;
            entry = PC_BOOT_NEW_GAME;
        } else if (strcmp(argv[i], "--title") == 0) {
            boot = true;
            entry = PC_BOOT_TITLE;
        } else if (strcmp(argv[i], "--boot") == 0) {
            boot = true;
        } else if (strcmp(argv[i], "--data-root") == 0 && i + 1 < argc) {
            data_root = argv[++i];
        } else if (strcmp(argv[i], "--intro-sprite") == 0 && i + 1 < argc) {
            sprite_archive = argv[++i];
        } else if (strcmp(argv[i], "--animation") == 0 && i + 1 < argc) {
            animation_path = argv[++i];
        } else if (strcmp(argv[i], "--model-offset") == 0 && i + 1 < argc) {
            char *end;
            unsigned long offset = strtoul(argv[++i], &end, 0);
            if (*end || end == argv[i] || offset > UINT32_MAX)
                return EXIT_FAILURE;
            model_offset = (unsigned)offset;
        } else if ((strcmp(argv[i], "--model-entry") == 0 || strcmp(argv[i], "--texture-entry") == 0) && i + 1 < argc) {
            bool is_model = strcmp(argv[i++], "--model-entry") == 0;
            char *end;
            long entry = strtol(argv[i], &end, 10);
            if (*end || end == argv[i] || entry < 0 || entry > 65535)
                return EXIT_FAILURE;
            if (is_model)
                model_entry = (int)entry;
            else
                texture_entry = (int)entry;
        } else if (strcmp(argv[i], "--model") == 0 && i + 1 < argc) {
            model_path = argv[++i];
        } else if (strcmp(argv[i], "--texture") == 0 && i + 1 < argc) {
            texture_path = argv[++i];
        } else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (strcmp(argv[i], "--resize-test") == 0) {
            resize_test = true;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            char *end;
            const char *value = argv[++i];
            errno = 0;
            frame_limit = strtoul(value, &end, 10);
            if (value[0] < '0' || value[0] > '9' || *end || errno || frame_limit == 0 || frame_limit > 1000000) {
                fputs("--frames requires an integer from 1 to 1000000\n", stderr);
                return EXIT_FAILURE;
            }
        } else {
            fprintf(stderr, "Unknown or incomplete argument: %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    if ((animation_path || texture_path || model_entry >= 0 || model_offset) && !model_path) {
        fputs("Model options require --model\n", stderr);
        return EXIT_FAILURE;
    }
    if (texture_entry >= 0 && !texture_path) {
        fputs("--texture-entry requires --texture\n", stderr);
        return EXIT_FAILURE;
    }

    if (boot && (!data_root || model_path || sprite_archive || hardware_sprites)) {
        fputs("--boot requires --data-root and cannot be combined with resource previews\n", stderr);
        return EXIT_FAILURE;
    }
    if (data_root && !sprite_archive && !boot) {
        if (SDL_snprintf(intro_path, sizeof(intro_path), "%s/%s", data_root,
                         hardware_sprites ? "a/0/2/6" : "a/1/6/8") >= (int)sizeof(intro_path))
            return EXIT_FAILURE;
        sprite_archive = intro_path;
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "SDL initialization: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    PcVideo video;
    if (!PcVideo_Init(&video)) {
        fprintf(stderr, "Vulkan initialization: %s\n", SDL_GetError());
        PcVideo_Destroy(&video);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_Log("BW2 PC: backend=%s; one %dx%d canvas", SDL_GetGPUDeviceDriver(video.device), PC_CANVAS_WIDTH,
            PC_CANVAS_HEIGHT);
    int result = EXIT_FAILURE;
    PcPixel *pixels = NULL;
    if (boot && (!PcBoot_Start(data_root, entry) || !PcGxVideo_Init(&video))) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Boot initialization: %s", SDL_GetError());
        goto cleanup;
    }
    if (!model_path && sprite_archive) {
        GFL_G3DSysCreate(TRUE, 4, FALSE, 6, 0, HEAPID_SYSTEM, NULL);
        model_system_loaded = true;
    }
    if (model_path && (!LoadModel(model_path, model_entry, model_offset, texture_path, texture_entry, animation_path) ||
                       !PcGxVideo_Init(&video))) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer initialization: %s", SDL_GetError());
        goto cleanup;
    }
    if (sprite_archive &&
        (!LoadSprite(sprite_archive) || (data_root && !hardware_sprites && !LoadIntroText(data_root)) ||
         (!model_path && !PcGxVideo_Init(&video)))) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Sprite initialization: %s", SDL_GetError());
        goto cleanup;
    }
    pixels = SDL_malloc(PC_CANVAS_WIDTH * PC_CANVAS_HEIGHT * sizeof(*pixels));
    if (!pixels) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to allocate diagnostic canvas");
        goto cleanup;
    }
    if (boot && !PcSettings_ApplyVideo(&video, PcSettings_Get()))
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Saved display settings: %s", SDL_GetError());
    bool running = true;
    bool panel = false;
    unsigned long frames = 0;
    uint64_t game_frames = 0;
    PcGameClock game_clock;
    PcGameClock_Init(&game_clock, SDL_GetTicksNS());
    result = EXIT_SUCCESS;
    Uint64 deadline = SDL_GetTicks() + 10000;
    while (running && (frame_limit == 0 || frames < frame_limit)) {
        Uint64 frameStart = SDL_GetTicksNS();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            PcSettings_InputEvent(&event);
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                running = false;
                break;
            }
            if (PcBattleUi_HandleEvent(&video, &event) || PcMenuUi_HandleEvent(&video, &event) ||
                PcPartyUi_HandleEvent(&video, &event) || PcDexUi_HandleEvent(&video, &event) ||
                (boot && PcBoot_HandleEvent(&event)))
                continue;
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE && !boot) {
                    running = false;
                } else if (event.key.scancode == SDL_SCANCODE_TAB) {
                    panel = !panel;
                }
            }
        }
        if (!running) {
            break;
        }
        float mouse_x, mouse_y, canvas_x = 0, canvas_y = 0;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        bool pointer = SDL_GetMouseFocus() == video.window && SDL_GetKeyboardFocus() == video.window &&
                       PcVideo_MapPointer(&video, mouse_x, mouse_y, &canvas_x, &canvas_y);
        if (!boot && !model_path && !sprite_archive)
            DrawDiagnostic(pixels, panel, pointer, canvas_x, canvas_y);
        bool presented;
        unsigned updates = PcGameClock_Advance(&game_clock, SDL_GetTicksNS());
        for (unsigned update = 0; update < updates; update++) {
            PcGx_BeginFrame();
            if (boot) {
                PcBoot_Update();
                PcBoot_Draw();
            }
            if (model_path) {
                if (animation_object && animation_frames)
                    animation_object->frame = (fx32)((game_frames % animation_frames) * FX32_ONE);
                NNS_G3dGlbFlush();
                NNS_G3dDraw(GFL_G3DMdlGetEngineModel(model_object));
                NNS_G3dGeFlushBuffer();
            }
            if (sprite_archive) {
                PcGx_DrawBackgrounds(0);
                DrawSprite();
            }
            game_frames++;
        }
        /* Original draw callbacks also advance game state. Present their latest snapshot between ticks. */
        if (!((boot || model_path || sprite_archive) ? PcGxVideo_Present(&video, &presented)
                                                     : PcVideo_Present(&video, pixels, &presented))) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Present failed: %s", SDL_GetError());
            result = EXIT_FAILURE;
            break;
        }
        if (presented) {
            frames++;
            deadline = SDL_GetTicks() + 10000;
            if (resize_test && (frames == 3 || frames == 6)) {
                if (!SDL_SetWindowSize(video.window, frames == 3 ? 640 : 960, frames == 3 ? 640 : 540) ||
                    !SDL_SyncWindow(video.window)) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Resize failed: %s", SDL_GetError());
                    result = EXIT_FAILURE;
                    break;
                }
                panel = !panel;
            }
        } else if (frame_limit && SDL_GetTicks() >= deadline) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No presentation for ten seconds");
            result = EXIT_FAILURE;
            break;
        }
        PcSettings_LimitFrame(frameStart);
    }
    if (frame_limit && frames != frame_limit) {
        result = EXIT_FAILURE;
    }
    SDL_Log("Presented %lu frames; simulated %llu ticks", frames, (unsigned long long)game_frames);
    if (boot || model_path || sprite_archive) {
        SDL_Log("Original geometry output: %u vertices", PcGx.vertex_count);
        if (screenshot_path && !PcGxVideo_Screenshot(&video, screenshot_path))
            result = EXIT_FAILURE;
    }
cleanup:
    PcSettings_CloseInput();
    if (boot)
        PcBoot_Stop();
    PcUi_Shutdown();
    if (boot || model_path || sprite_archive)
        PcGxVideo_Destroy(&video);
    GFL_HeapFree(model_file);
    GFL_HeapFree(texture_file);
    SDL_free(animation_object);
    SDL_free(animation_file);
    GFL_G3DMdlFree(model_object);
    if (model_texture_uploaded)
        GFL_G3DResFreeTexData(texture_resource ? texture_resource : model_resource);
    GFL_G3DResFree(texture_resource);
    GFL_G3DResFree(model_resource);
    if (sprite_texture_key)
        NNS_GfdFreeTexVram(sprite_texture_key);
    if (sprite_palette_key)
        NNS_GfdFreePlttVram(sprite_palette_key);
    if (model_system_loaded)
        GFL_G3DSysFree();
    for (unsigned i = 0; i < SDL_arraysize(sprite_files); i++)
        GFL_HeapFree(sprite_files[i]);
    SDL_free(sprite_work);
    if (text_loaded)
        GFL_BitmapFree(g_TextRendererTempBitmap);
    if (background_loaded)
        GFL_BGSysFree();
    SDL_free(pixels);
    PcVideo_Destroy(&video);
    PcFs_Shutdown();
    PcHeap_Shutdown();
    SDL_Quit();
    return result;
}
