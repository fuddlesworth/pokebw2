#include "dex_skin.h"
#include "gx_backend.h"
#include <librsvg/rsvg.h>

static const Uint32 skinVertexCode[] =
#include "skin_vert.h"
    ;
static const Uint32 skinFragmentCode[] =
#include "skin_frag.h"
    ;
static SDL_GPUGraphicsPipeline *skinPipeline;
static SDL_GPUSampler *skinSampler;

enum { DEX_SKIN_WIDTH = 1280, DEX_SKIN_HEIGHT = 720 };
enum {
    SKIN_HEADER,
    SKIN_CENTER,
    SKIN_INFORMATION,
    SKIN_DESCRIPTION,
    SKIN_STATS,
    SKIN_FOOTER,
    SKIN_ROW,
    SKIN_SELECTION,
    SKIN_TABS,
    SKIN_TOP_JUNCTION,
    SKIN_HEADER_CORNER,
    SKIN_FILTER_ROW,
    SKIN_FORM_CONTROLS,
    SKIN_PARTY_HEADER,
    SKIN_PARTY_ROW,
    SKIN_PARTY_HEADING,
    SKIN_PARTY_GENDER,
    SKIN_PARTY_HP,
    SKIN_PARTY_STATS,
    SKIN_PARTY_MOVES,
    SKIN_PROFILE,
    SKIN_MENU_BASE,
    SKIN_COUNT = SKIN_MENU_BASE + PC_MENU_SKIN_COUNT
};

static const char *const skinNames[SKIN_COUNT] = {
    "dex/header",
    "dex/center",
    "dex/information-card",
    "dex/description-card",
    "dex/stats",
    "dex/footer",
    "dex/row",
    "dex/selection",
    "dex/tabs",
    "dex/top-junction",
    "dex/header-corner",
    "dex/filter-row",
    "dex/form-controls",
    "party/header",
    "party/row",
    "party/heading",
    "party/gender",
    "party/hp",
    "party/stats",
    "party/moves",
    "menu/profile",
    "party/header",
    "menu/footer",
    "menu/bag",
    "menu/summary",
    "menu/mail",
    "menu/trainer",
    "menu/signature-pad",
    "menu/save",
    "menu/tab",
    "menu/row",
    "menu/light-row",
    "menu/button",
    "menu/marking",
    "menu/trainer-badge",
    "menu/greeting-row",
    "menu/save-progress",
    "battle/panel",
    "battle/enemy-hp",
    "battle/player-hp",
    "battle/move",
    "battle/action",
    "battle/icons",
    "battle/cursor",
    "battle/notice",
    "battle/keys",
    "menu/settings",
    "menu/settings-selection",
};

static struct {
    SDL_GPUTexture *texture;
    Uint32 width, height;
} skin[SKIN_COUNT];

static SDL_GPUTexture *LoadTexture(PcVideo *video, const char *name, Uint32 *width, Uint32 *height) {
    char path[1024];
    if (SDL_snprintf(path, sizeof(path), "%sassets/%s.svg", SDL_GetBasePath(), name) >= (int)sizeof(path)) {
        SDL_SetError("Pokédex skin path is too long");
        return NULL;
    }
    GError *error = NULL;
    RsvgHandle *document = rsvg_handle_new_from_file(path, &error);
    if (!document) {
        SDL_SetError("Cannot load %s: %s", path, error ? error->message : "invalid SVG");
        g_clear_error(&error);
        return NULL;
    }
    double svgWidth, svgHeight;
    if (!rsvg_handle_get_intrinsic_size_in_pixels(document, &svgWidth, &svgHeight) || svgWidth < 1 || svgHeight < 1 ||
        svgWidth > DEX_SKIN_WIDTH || svgHeight > DEX_SKIN_HEIGHT) {
        SDL_SetError("Invalid SVG dimensions: %s", path);
        g_object_unref(document);
        return NULL;
    }
    // Rasterize vectors at twice the design size for clean edges at common desktop resolutions.
    cairo_surface_t *bitmap =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)(svgWidth * 2), (int)(svgHeight * 2));
    cairo_t *context = cairo_create(bitmap);
    RsvgRectangle bounds = { 0, 0, svgWidth * 2, svgHeight * 2 };
    bool rendered = rsvg_handle_render_document(document, context, &bounds, &error);
    cairo_destroy(context);
    g_object_unref(document);
    cairo_surface_flush(bitmap);
    SDL_Surface *surface = NULL;
    if (rendered && cairo_surface_status(bitmap) == CAIRO_STATUS_SUCCESS) {
        SDL_Surface *loaded =
            SDL_CreateSurfaceFrom((int)bounds.width, (int)bounds.height, SDL_PIXELFORMAT_ARGB8888,
                                  cairo_image_surface_get_data(bitmap), cairo_image_surface_get_stride(bitmap));
        if (loaded) {
            surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(loaded);
        }
    } else {
        SDL_SetError("Cannot render %s: %s", path,
                     error ? error->message : cairo_status_to_string(cairo_surface_status(bitmap)));
    }
    g_clear_error(&error);
    cairo_surface_destroy(bitmap);
    if (!surface)
        return NULL;

    *width = (Uint32)surface->w;
    *height = (Uint32)surface->h;
    SDL_GPUTextureCreateInfo info = { .type = SDL_GPU_TEXTURETYPE_2D,
                                      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                      .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                      .width = *width,
                                      .height = *height,
                                      .layer_count_or_depth = 1,
                                      .num_levels = 1,
                                      .sample_count = SDL_GPU_SAMPLECOUNT_1 };
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(video->device, &info);
    SDL_GPUTransferBufferCreateInfo uploadInfo = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                   .size = *width * *height * 4 };
    SDL_GPUTransferBuffer *upload = texture ? SDL_CreateGPUTransferBuffer(video->device, &uploadInfo) : NULL;
    bool okay = upload != NULL;
    if (okay) {
        Uint8 *pixels = SDL_MapGPUTransferBuffer(video->device, upload, false);
        okay = pixels != NULL;
        if (okay) {
            for (Uint32 y = 0; y < *height; y++)
                SDL_memcpy(pixels + y * *width * 4, (Uint8 *)surface->pixels + y * surface->pitch, *width * 4);
            SDL_UnmapGPUTransferBuffer(video->device, upload);
        }
    }
    if (okay) {
        SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(video->device);
        okay = commands != NULL;
        if (okay) {
            SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
            okay = copy != NULL;
            if (okay) {
                SDL_GPUTextureTransferInfo source = { .transfer_buffer = upload };
                SDL_GPUTextureRegion destination = { .texture = texture, .w = *width, .h = *height, .d = 1 };
                SDL_UploadToGPUTexture(copy, &source, &destination, false);
                SDL_EndGPUCopyPass(copy);
                okay = SDL_SubmitGPUCommandBuffer(commands);
            } else {
                SDL_CancelGPUCommandBuffer(commands);
            }
        }
    }
    if (okay)
        SDL_WaitForGPUIdle(video->device);
    if (upload)
        SDL_ReleaseGPUTransferBuffer(video->device, upload);
    SDL_DestroySurface(surface);
    if (!okay) {
        if (texture)
            SDL_ReleaseGPUTexture(video->device, texture);
        return NULL;
    }
    return texture;
}

bool PcDexSkin_Init(PcVideo *video) {
    SDL_GPUShaderCreateInfo shader = { .code = (const Uint8 *)skinVertexCode,
                                       .code_size = sizeof(skinVertexCode),
                                       .entrypoint = "main",
                                       .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                       .stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                       .num_uniform_buffers = 1 };
    SDL_GPUShader *vert = SDL_CreateGPUShader(video->device, &shader);
    shader.code = (const Uint8 *)skinFragmentCode;
    shader.code_size = sizeof(skinFragmentCode);
    shader.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader.num_uniform_buffers = 0;
    shader.num_samplers = 1;
    SDL_GPUShader *frag = SDL_CreateGPUShader(video->device, &shader);
    if (vert && frag) {
        SDL_GPUColorTargetDescription target = { .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                                 .blend_state = {
                                                     .enable_blend = true,
                                                     .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                                                     .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                                     .color_blend_op = SDL_GPU_BLENDOP_ADD,
                                                     .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                                                     .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                                     .alpha_blend_op = SDL_GPU_BLENDOP_ADD } };
        SDL_GPUGraphicsPipelineCreateInfo pipeline = { .vertex_shader = vert,
                                                       .fragment_shader = frag,
                                                       .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                                                       .target_info = { .color_target_descriptions = &target,
                                                                        .num_color_targets = 1 } };
        skinPipeline = SDL_CreateGPUGraphicsPipeline(video->device, &pipeline);
        SDL_GPUSamplerCreateInfo sampler = { .min_filter = SDL_GPU_FILTER_LINEAR,
                                             .mag_filter = SDL_GPU_FILTER_LINEAR,
                                             .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                             .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE };
        skinSampler = SDL_CreateGPUSampler(video->device, &sampler);
    }
    if (vert)
        SDL_ReleaseGPUShader(video->device, vert);
    if (frag)
        SDL_ReleaseGPUShader(video->device, frag);
    if (!skinPipeline || !skinSampler) {
        PcDexSkin_Destroy(video);
        return false;
    }
    for (Uint32 i = 0; i < SKIN_COUNT; i++) {
        skin[i].texture = LoadTexture(video, skinNames[i], &skin[i].width, &skin[i].height);
        if (!skin[i].texture) {
            PcDexSkin_Destroy(video);
            return false;
        }
    }
    return true;
}

void PcDexSkin_Destroy(PcVideo *video) {
    if (skinPipeline)
        SDL_ReleaseGPUGraphicsPipeline(video->device, skinPipeline);
    if (skinSampler)
        SDL_ReleaseGPUSampler(video->device, skinSampler);
    skinPipeline = NULL;
    skinSampler = NULL;
    for (Uint32 i = 0; i < SKIN_COUNT; i++) {
        if (skin[i].texture)
            SDL_ReleaseGPUTexture(video->device, skin[i].texture);
        skin[i].texture = NULL;
    }
}

static void BlitRegion(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *target, PcViewport viewport, Uint32 piece,
                       Uint32 sx, Uint32 sy, Uint32 sw, Uint32 sh, float x, float y, float width, float height,
                       bool clear) {
    // SVG pixels are premultiplied by Cairo. Source-over preserves transparent overlays.
    SDL_GPUColorTargetInfo color = { .texture = target,
                                     .clear_color = { 0, 0, 0, 1 },
                                     .load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD,
                                     .store_op = SDL_GPU_STOREOP_STORE };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &color, 1, NULL);
    SDL_GPUViewport area = {
        .x = viewport.x, .y = viewport.y, .w = viewport.width, .h = viewport.height, .min_depth = 0, .max_depth = 1
    };
    SDL_SetGPUViewport(pass, &area);
    SDL_BindGPUGraphicsPipeline(pass, skinPipeline);
    // Round shared endpoints, not extents, so sliced edges remain joined at fractional window scales.
    float left = SDL_roundf(x * viewport.width / DEX_SKIN_WIDTH);
    float top = SDL_roundf(y * viewport.height / DEX_SKIN_HEIGHT);
    float right = SDL_roundf((x + width) * viewport.width / DEX_SKIN_WIDTH);
    float bottom = SDL_roundf((y + height) * viewport.height / DEX_SKIN_HEIGHT);
    float rect[8] = { left / viewport.width,           top / viewport.height,
                      (right - left) / viewport.width, (bottom - top) / viewport.height,
                      (float)sx / skin[piece].width,   (float)sy / skin[piece].height,
                      (float)sw / skin[piece].width,   (float)sh / skin[piece].height };
    SDL_PushGPUVertexUniformData(commands, 0, rect, sizeof(rect));
    SDL_GPUTextureSamplerBinding binding = { skin[piece].texture, skinSampler };
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

static void Blit(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *target, PcViewport viewport, Uint32 piece, float x,
                 float y, float width, float height, bool clear) {
    BlitRegion(commands, target, viewport, piece, 0, 0, skin[piece].width, skin[piece].height, x, y, width, height,
               clear);
}

void PcDexSkin_Blit(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *target, PcViewport viewport, Uint32 selectedRow,
                    Uint32 tab, Uint32 filterRow) {
    Blit(commands, target, viewport, SKIN_HEADER, 0, 0, 1280, 81, true);
    Blit(commands, target, viewport, SKIN_CENTER, 375, 81, 421, 565, false);
    Blit(commands, target, viewport, SKIN_INFORMATION, 796, 144, 484, 280, false);
    Blit(commands, target, viewport, SKIN_DESCRIPTION, 796, 424, 484, 222, false);
    if (tab == 0 && filterRow >= 7)
        Blit(commands, target, viewport, SKIN_STATS, 855, 354, 400, 54, false);
    if (tab == 2 && filterRow >= 7)
        Blit(commands, target, viewport, SKIN_FORM_CONTROLS, 850, 295, 398, 114, false);
    if (filterRow < 7) {
        static const float rows[] = { 258, 300, 342, 384, 453, 495, 537 };
        BlitRegion(commands, target, viewport, SKIN_FILTER_ROW, 0, filterRow < 4 ? 0 : 64, 800, 64, 848,
                   rows[filterRow] - 3, 400, 32, false);
    }
    for (Uint32 row = 0; row < 9; row++)
        Blit(commands, target, viewport, SKIN_ROW, 0, 90 + row * 61, 375, 61, false);
    if (selectedRow < 9)
        Blit(commands, target, viewport, SKIN_SELECTION, 0, 90 + selectedRow * 61, 375, 61, false);
    Blit(commands, target, viewport, SKIN_TOP_JUNCTION, 356, 81, 44, 18, false);
    Blit(commands, target, viewport, SKIN_FOOTER, 0, 646, 1280, 74, false);
    BlitRegion(commands, target, viewport, SKIN_TABS, 0, (tab < 3 ? tab : 0) * 86, 972, 86, 794, 101, 486, 43, false);
    Blit(commands, target, viewport, SKIN_HEADER_CORNER, 1256, 81, 24, 24, false);
}

void PcPartySkin_Blit(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *target, PcViewport viewport, Uint32 selected,
                      Uint32 marked, Uint32 menuRow, Uint32 occupied, Uint32 sex, Uint32 hpRows, bool overview,
                      bool overviewData) {
    Blit(commands, target, viewport, SKIN_PARTY_HEADER, 0, 0, 1280, 81, true);
    Blit(commands, target, viewport, SKIN_CENTER, 375, 81, 421, 565, false);
    Blit(commands, target, viewport, SKIN_INFORMATION, 796, 144, 484, 280, false);
    Blit(commands, target, viewport, SKIN_DESCRIPTION, 796, 424, 484, 222, false);
    if (overviewData) {
        Blit(commands, target, viewport, SKIN_PARTY_STATS, 850, 252, 398, 150, false);
        Blit(commands, target, viewport, SKIN_PARTY_MOVES, 850, 474, 398, 136, false);
    }
    for (Uint32 row = 0; row < 6; row++) {
        Uint32 state = !(occupied & (1u << row)) ? 3 : row == selected ? 1 : row == marked ? 2 : 0;
        BlitRegion(commands, target, viewport, SKIN_PARTY_ROW, 0, state * 178, 750, 178, 0, 90 + row * 89, 375, 89,
                   false);
        if (hpRows & (1u << row))
            Blit(commands, target, viewport, SKIN_PARTY_HP, 122, 133 + row * 89, 220, 11, false);
    }
    if (menuRow < 5)
        BlitRegion(commands, target, viewport, SKIN_FILTER_ROW, 0, 64, 800, 64, 848, 447 + menuRow * 34, 400, 32,
                   false);
    Blit(commands, target, viewport, SKIN_TOP_JUNCTION, 356, 81, 44, 18, false);
    Blit(commands, target, viewport, SKIN_FOOTER, 0, 646, 1280, 74, false);
    BlitRegion(commands, target, viewport, SKIN_PARTY_HEADING, 0, overview ? 86 : 0, 972, 86, 794, 101, 486, 43, false);
    if (sex < 2)
        BlitRegion(commands, target, viewport, SKIN_PARTY_GENDER, sex * 64, 0, 64, 64, 1208, 254, 32, 32, false);
    Blit(commands, target, viewport, SKIN_HEADER_CORNER, 1256, 81, 24, 24, false);
}

void PcMenuSkin_Add(PcMenuSkinAsset asset, float x, float y, float width, float height, Uint32 state) {
    if ((unsigned)asset >= PC_MENU_SKIN_COUNT || width <= 0 || height <= 0 ||
        PcGx.menu_layer_count >= SDL_arraysize(PcGx.menu_layers))
        return;
    PcGx.menu_layers[PcGx.menu_layer_count++] = (PcMenuSkinLayer){ asset, x, y, width, height, state };
}

void PcMenuSkin_Blit(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *target, PcViewport viewport) {
    for (u32 i = 0; i < PcGx.menu_layer_count; i++) {
        const PcMenuSkinLayer *layer = &PcGx.menu_layers[i];
        u32 piece = SKIN_MENU_BASE + layer->asset;
        bool clear = i == 0 && !PcGx.battle_ui && !PcGx.party_ui && !PcGx.dex_ui;
        if (layer->asset == PC_MENU_SUMMARY || layer->asset == PC_MENU_TRAINER || layer->asset == PC_MENU_SAVE) {
            Blit(commands, target, viewport, SKIN_PROFILE, layer->x, layer->y, layer->width, layer->height, clear);
            clear = false;
        }
        if (layer->asset == PC_BATTLE_KEY) {
            u32 size = skin[piece].height;
            if (layer->state == 0) {
                // Keep keycap corners round when a longer key name widens the middle.
                float edge = 6 * layer->height / size;
                BlitRegion(commands, target, viewport, piece, 0, 0, 6, size, layer->x, layer->y, edge, layer->height,
                           false);
                BlitRegion(commands, target, viewport, piece, 6, 0, size - 12, size, layer->x + edge, layer->y,
                           layer->width - 2 * edge, layer->height, false);
                BlitRegion(commands, target, viewport, piece, size - 6, 0, 6, size, layer->x + layer->width - edge,
                           layer->y, edge, layer->height, false);
            } else {
                BlitRegion(commands, target, viewport, piece, (layer->state % 5) * size, 0, size, size, layer->x,
                           layer->y, layer->width, layer->height, false);
            }
            continue;
        }
        if (layer->asset == PC_BATTLE_ICON) {
            u32 size = skin[piece].height;
            BlitRegion(commands, target, viewport, piece, (layer->state % 22) * size, 0, size, size, layer->x, layer->y,
                       layer->width, layer->height, false);
            continue;
        }
        PcMenuSkinGeometry geometry = PcMenuSkin_GetGeometry(layer->asset);
        u32 states = geometry.states;
        u32 height = skin[piece].height / states;
        if (geometry.leftCap || geometry.rightCap) {
            // Rendering and hit testing share these SVG measurements.
            float sourceScale = height / geometry.height;
            u32 leftSource = (u32)(geometry.leftCap * sourceScale);
            u32 rightSource = (u32)(geometry.rightCap * sourceScale);
            float scale = layer->height / height;
            float left = leftSource * scale, right = rightSource * scale;
            if (left + right > layer->width) {
                float fit = layer->width / (left + right);
                left *= fit;
                right *= fit;
            }
            u32 sourceY = (layer->state % states) * height;
            u32 width = skin[piece].width;
            BlitRegion(commands, target, viewport, piece, 0, sourceY, leftSource, height, layer->x, layer->y, left,
                       layer->height, false);
            BlitRegion(commands, target, viewport, piece, leftSource, sourceY, width - leftSource - rightSource, height,
                       layer->x + left, layer->y, layer->width - left - right, layer->height, false);
            BlitRegion(commands, target, viewport, piece, width - rightSource, sourceY, rightSource, height,
                       layer->x + layer->width - right, layer->y, right, layer->height, false);
            continue;
        }
        BlitRegion(commands, target, viewport, piece, 0, (layer->state % states) * height, skin[piece].width, height,
                   layer->x, layer->y, layer->width, layer->height, clear);
    }
}
