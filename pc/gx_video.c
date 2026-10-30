#include "dex_skin.h"
#include "gx_backend.h"
#include "settings.h"
#include <stddef.h>

static const Uint32 vertex_code[] =
#include "gx_vert.h"
    ;
static const Uint32 fragment_code[] =
#include "gx_frag.h"
    ;
typedef struct {
    Uint32 first, count;
    unsigned pipeline;
    bool scene;
} CaptureBatch;

static struct {
    SDL_GPUGraphicsPipeline *pipelines[3], *multisample[2][3];
    SDL_GPUBuffer *vertices, *textures, *palettes, *backgrounds, *rearPlane;
    SDL_GPUTransferBuffer *upload;
    SDL_GPUTexture *color, *depth, *msColor, *msDepth;
    unsigned samples;
    Uint32 width, height;
    PcVideo *video;
    SDL_GPUTexture *captureColor, *captureDepth;
    SDL_GPUTransferBuffer *captureDownload;
    CaptureBatch batches[PC_GX_MAX_VERTICES / 3 + 1];
    Uint32 batchCount;
    float sceneMinDepth, sceneMaxDepth;
    float sceneProjectionScaleX;
} renderer;

static void CaptureScreen(u32 control);

static const PcGxVertex rearVertices[6] = {
    { .clip = { -1, -1, 1, 1 }, .polygon = 1u << 22 }, { .clip = { 1, -1, 1, 1 }, .polygon = 1u << 22 },
    { .clip = { 1, 1, 1, 1 }, .polygon = 1u << 22 },   { .clip = { -1, -1, 1, 1 }, .polygon = 1u << 22 },
    { .clip = { 1, 1, 1, 1 }, .polygon = 1u << 22 },   { .clip = { -1, 1, 1, 1 }, .polygon = 1u << 22 },
};

static void SetViewport(SDL_GPUCommandBuffer *commands, SDL_GPURenderPass *pass, const SDL_GPUViewport *viewport,
                        Uint32 captureMode, bool scene) {
    float projectionScale[4] = { 1, 1, 1, 1 };
    struct {
        float rect[4], depth[4];
        Uint32 captureMode, spriteFilter, padding[2];
    } state = { .rect = { viewport->x, viewport->y, viewport->w, viewport->h },
                .depth = { viewport->min_depth, viewport->max_depth, scene ? 1 : 0, 0 },
                .captureMode = captureMode,
                .spriteFilter = PcSettings_Get()->spriteFilter };
    SDL_SetGPUViewport(pass, viewport);
    if (captureMode && scene && renderer.sceneProjectionScaleX > 0)
        projectionScale[0] = 1.0f / renderer.sceneProjectionScaleX;
    SDL_PushGPUVertexUniformData(commands, 0, projectionScale, sizeof(projectionScale));
    SDL_PushGPUFragmentUniformData(commands, 0, &state, sizeof(state));
}

static int DrawLayer(const PcGxVertex *vertex, float sceneDepth) {
    if (vertex->polygon & PC_GX_POLYGON_UI)
        return 3;
    if (vertex->polygon & 0xc0000000u) {
        // Compose the rear plane between the original layers behind and in front of BG0.
        float depth = (vertex->clip[2] / vertex->clip[3] + 1) * 0.5f;
        return PcGx.composite_scene && depth < sceneDepth ? 2 : 0;
    }
    return 1;
}

bool PcGxVideo_Init(PcVideo *video) {
    SDL_GPUShaderCreateInfo shader_info = { .code = (const Uint8 *)vertex_code,
                                            .code_size = sizeof(vertex_code),
                                            .entrypoint = "main",
                                            .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
                                            .num_uniform_buffers = 1 };
    SDL_GPUShader *vert = SDL_CreateGPUShader(video->device, &shader_info);
    if (!vert)
        return false;
    shader_info.code = (const Uint8 *)fragment_code;
    shader_info.code_size = sizeof(fragment_code);
    shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader_info.num_storage_buffers = 4;
    shader_info.num_uniform_buffers = 1;
    SDL_GPUShader *frag = SDL_CreateGPUShader(video->device, &shader_info);
    if (!frag) {
        SDL_ReleaseGPUShader(video->device, vert);
        return false;
    }
    SDL_GPUVertexBufferDescription buffer = { .slot = 0,
                                              .pitch = sizeof(PcGxVertex),
                                              .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX };
    SDL_GPUVertexAttribute attributes[] = {
        { .location = 0,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
          .offset = offsetof(PcGxVertex, clip) },
        { .location = 1,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
          .offset = offsetof(PcGxVertex, uv) },
        { .location = 2,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
          .offset = offsetof(PcGxVertex, color) },
        { .location = 3,
          .buffer_slot = 0,
          .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT3,
          .offset = offsetof(PcGxVertex, texture) },
    };
    SDL_GPUColorTargetDescription target = { .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM };
    SDL_GPUGraphicsPipelineCreateInfo pipeline = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .vertex_input_state = { .vertex_buffer_descriptions = &buffer,
                                .num_vertex_buffers = 1,
                                .vertex_attributes = attributes,
                                .num_vertex_attributes = 4 },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = { .fill_mode = SDL_GPU_FILLMODE_FILL,
                              .cull_mode = SDL_GPU_CULLMODE_NONE,
                              .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE },
        .multisample_state = { .sample_count = SDL_GPU_SAMPLECOUNT_1 },
        .depth_stencil_state = { .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
                                 .enable_depth_test = true,
                                 .enable_depth_write = true },
        .target_info = { .color_target_descriptions = &target,
                         .num_color_targets = 1,
                         .has_depth_stencil_target = true,
                         .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT },
    };
    for (unsigned level = 0; level < 3; level++) {
        SDL_GPUSampleCount samples = level == 2   ? SDL_GPU_SAMPLECOUNT_4
                                     : level == 1 ? SDL_GPU_SAMPLECOUNT_2
                                                  : SDL_GPU_SAMPLECOUNT_1;
        if (!SDL_GPUTextureSupportsSampleCount(video->device, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, samples) ||
            !SDL_GPUTextureSupportsSampleCount(video->device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, samples))
            continue;
        pipeline.multisample_state.sample_count = samples;
        for (int i = 0; i < 3; i++) {
            target.blend_state =
                (SDL_GPUColorTargetBlendState){ .enable_blend = i != 0,
                                                .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                                                .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                                .color_blend_op = SDL_GPU_BLENDOP_ADD,
                                                .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                                                .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                                .alpha_blend_op = SDL_GPU_BLENDOP_ADD };
            pipeline.depth_stencil_state.enable_depth_write = i != 1;
            SDL_GPUGraphicsPipeline *created = SDL_CreateGPUGraphicsPipeline(video->device, &pipeline);
            if (level)
                renderer.multisample[level - 1][i] = created;
            else
                renderer.pipelines[i] = created;
            if (!created) {
                SDL_ReleaseGPUShader(video->device, vert);
                SDL_ReleaseGPUShader(video->device, frag);
                return false;
            }
        }
    }
    SDL_ReleaseGPUShader(video->device, vert);
    SDL_ReleaseGPUShader(video->device, frag);
    if (!renderer.pipelines[2])
        return false;
    SDL_GPUBufferCreateInfo buffer_info = { .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                            .size = sizeof(PcGx.vertices) + sizeof(rearVertices) };
    renderer.vertices = SDL_CreateGPUBuffer(video->device, &buffer_info);
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    buffer_info.size = sizeof(PcGx.textures);
    renderer.textures = SDL_CreateGPUBuffer(video->device, &buffer_info);
    buffer_info.size = sizeof(PcGx.palettes);
    renderer.palettes = SDL_CreateGPUBuffer(video->device, &buffer_info);
    buffer_info.size = 2 * sizeof(PcGxBackgroundState);
    renderer.backgrounds = SDL_CreateGPUBuffer(video->device, &buffer_info);
    buffer_info.size = 256 * 192 * sizeof(PcGxRearPixel);
    renderer.rearPlane = SDL_CreateGPUBuffer(video->device, &buffer_info);
    SDL_GPUTransferBufferCreateInfo transfer_info = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                      .size = sizeof(PcGx.vertices) + sizeof(PcGx.textures) +
                                                              sizeof(PcGx.palettes) + 2 * sizeof(PcGxBackgroundState) +
                                                              sizeof(rearVertices) +
                                                              256 * 192 * sizeof(PcGxRearPixel) };
    renderer.upload = SDL_CreateGPUTransferBuffer(video->device, &transfer_info);
    renderer.video = video;
    PcGx_SetCaptureCallback(CaptureScreen);
    return renderer.vertices && renderer.textures && renderer.palettes && renderer.backgrounds && renderer.rearPlane &&
           renderer.upload && PcDexSkin_Init(video);
}
void PcGxVideo_Destroy(PcVideo *video) {
    PcGx_SetCaptureCallback(NULL);
    SDL_WaitForGPUIdle(video->device);
    PcDexSkin_Destroy(video);
    for (int i = 0; i < 3; i++)
        if (renderer.pipelines[i])
            SDL_ReleaseGPUGraphicsPipeline(video->device, renderer.pipelines[i]);
    for (unsigned level = 0; level < 2; level++)
        for (unsigned i = 0; i < 3; i++)
            if (renderer.multisample[level][i])
                SDL_ReleaseGPUGraphicsPipeline(video->device, renderer.multisample[level][i]);
    if (renderer.msColor)
        SDL_ReleaseGPUTexture(video->device, renderer.msColor);
    if (renderer.msDepth)
        SDL_ReleaseGPUTexture(video->device, renderer.msDepth);
    if (renderer.vertices)
        SDL_ReleaseGPUBuffer(video->device, renderer.vertices);
    if (renderer.textures)
        SDL_ReleaseGPUBuffer(video->device, renderer.textures);
    if (renderer.palettes)
        SDL_ReleaseGPUBuffer(video->device, renderer.palettes);
    if (renderer.backgrounds)
        SDL_ReleaseGPUBuffer(video->device, renderer.backgrounds);
    if (renderer.rearPlane)
        SDL_ReleaseGPUBuffer(video->device, renderer.rearPlane);
    if (renderer.upload)
        SDL_ReleaseGPUTransferBuffer(video->device, renderer.upload);
    if (renderer.color)
        SDL_ReleaseGPUTexture(video->device, renderer.color);
    if (renderer.depth)
        SDL_ReleaseGPUTexture(video->device, renderer.depth);
    if (renderer.captureColor)
        SDL_ReleaseGPUTexture(video->device, renderer.captureColor);
    if (renderer.captureDepth)
        SDL_ReleaseGPUTexture(video->device, renderer.captureDepth);
    if (renderer.captureDownload)
        SDL_ReleaseGPUTransferBuffer(video->device, renderer.captureDownload);
    SDL_zero(renderer);
}
static bool Resize(PcVideo *video, Uint32 width, Uint32 height) {
    unsigned samples = PcSettings_Get()->antialiasing;
    if (samples && !renderer.multisample[samples - 1][0])
        samples = 0;
    if (renderer.width == width && renderer.height == height && renderer.samples == samples)
        return true;
    SDL_GPUTextureCreateInfo info = { .type = SDL_GPU_TEXTURETYPE_2D,
                                      .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                      .width = width,
                                      .height = height,
                                      .layer_count_or_depth = 1,
                                      .num_levels = 1,
                                      .sample_count = SDL_GPU_SAMPLECOUNT_1 };
    SDL_GPUTexture *color = SDL_CreateGPUTexture(video->device, &info);
    if (!color)
        return false;
    info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    SDL_GPUTexture *depth = SDL_CreateGPUTexture(video->device, &info);
    if (!depth) {
        SDL_ReleaseGPUTexture(video->device, color);
        return false;
    }
    SDL_GPUTexture *msColor = NULL, *msDepth = NULL;
    if (samples) {
        info.sample_count = samples == 2 ? SDL_GPU_SAMPLECOUNT_4 : SDL_GPU_SAMPLECOUNT_2;
        msDepth = SDL_CreateGPUTexture(video->device, &info);
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        msColor = SDL_CreateGPUTexture(video->device, &info);
        if (!msColor || !msDepth) {
            if (msColor)
                SDL_ReleaseGPUTexture(video->device, msColor);
            if (msDepth)
                SDL_ReleaseGPUTexture(video->device, msDepth);
            SDL_ReleaseGPUTexture(video->device, color);
            SDL_ReleaseGPUTexture(video->device, depth);
            return false;
        }
    }
    if (renderer.msColor)
        SDL_ReleaseGPUTexture(video->device, renderer.msColor);
    if (renderer.msDepth)
        SDL_ReleaseGPUTexture(video->device, renderer.msDepth);
    renderer.msColor = msColor;
    renderer.msDepth = msDepth;
    renderer.samples = samples;
    if (renderer.color)
        SDL_ReleaseGPUTexture(video->device, renderer.color);
    if (renderer.depth)
        SDL_ReleaseGPUTexture(video->device, renderer.depth);
    renderer.color = color;
    renderer.depth = depth;
    renderer.width = width;
    renderer.height = height;
    return true;
}
bool PcGxVideo_Present(PcVideo *video, bool *presented) {
    *presented = false;
    if (PcGx.failed)
        return SDL_SetError("%s", PcGx.error);
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(video->device);
    if (!commands)
        return false;
    SDL_GPUTexture *swapchain = NULL;
    Uint32 width, height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, video->window, &swapchain, &width, &height)) {
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }
    if (!swapchain)
        return SDL_SubmitGPUCommandBuffer(commands);
    if (!Resize(video, width, height)) {
        SDL_SubmitGPUCommandBuffer(commands);
        return false;
    }
    Uint32 vertex_bytes = PcGx.vertex_count * sizeof(PcGxVertex) + sizeof(rearVertices);
    Uint8 *mapped = SDL_MapGPUTransferBuffer(video->device, renderer.upload, true);
    if (!mapped) {
        SDL_SubmitGPUCommandBuffer(commands);
        return false;
    }
    SDL_memcpy(mapped, PcGx.vertices, vertex_bytes - sizeof(rearVertices));
    SDL_memcpy(mapped + vertex_bytes - sizeof(rearVertices), rearVertices, sizeof(rearVertices));
    SDL_memcpy(mapped + vertex_bytes, PcGx.textures, sizeof(PcGx.textures));
    SDL_memcpy(mapped + vertex_bytes + sizeof(PcGx.textures), PcGx.palettes, sizeof(PcGx.palettes));
    PcGxBackgroundState *backgrounds = (void *)(mapped + vertex_bytes + sizeof(PcGx.textures) + sizeof(PcGx.palettes));
    for (unsigned i = 0; i < 2; i++) {
        SDL_memcpy(backgrounds[i].vram, PcGxBackground[i], sizeof(backgrounds[i].vram));
        SDL_memcpy(backgrounds[i].palette, PcGx2D[i].background_palette, sizeof(backgrounds[i].palette));
        SDL_memcpy(backgrounds[i].extended_palette, PcGx2D[i].background_extended_palette,
                   sizeof(backgrounds[i].extended_palette));
        SDL_memcpy(backgrounds[i].registers, PcGxRegisters[i].words, sizeof(backgrounds[i].registers));
        SDL_memcpy(backgrounds[i].object, PcGx2D[i].object, sizeof(backgrounds[i].object));
        SDL_memcpy(backgrounds[i].object_palette, PcGx2D[i].object_palette, sizeof(backgrounds[i].object_palette));
        SDL_memcpy(backgrounds[i].object_extended_palette, PcGx2D[i].object_extended_palette,
                   sizeof(backgrounds[i].object_extended_palette));
        SDL_memcpy(backgrounds[i].oam, PcGx2D[i].oam, sizeof(backgrounds[i].oam));
        backgrounds[i].display = i ? PcGxSubDisplay : PcGxMainDisplay;
    }
    PcGx_ReadRearPlane((PcGxRearPixel *)(void *)(backgrounds + 2));
    SDL_UnmapGPUTransferBuffer(video->device, renderer.upload);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTransferBufferLocation source = { .transfer_buffer = renderer.upload };
    SDL_GPUBufferRegion dest = { .buffer = renderer.vertices, .size = vertex_bytes };
    if (vertex_bytes)
        SDL_UploadToGPUBuffer(copy, &source, &dest, true);
    source.offset = vertex_bytes;
    dest.buffer = renderer.textures;
    dest.size = sizeof(PcGx.textures);
    SDL_UploadToGPUBuffer(copy, &source, &dest, true);
    source.offset += dest.size;
    dest.buffer = renderer.palettes;
    dest.size = sizeof(PcGx.palettes);
    SDL_UploadToGPUBuffer(copy, &source, &dest, true);
    source.offset += dest.size;
    dest.buffer = renderer.backgrounds;
    dest.size = 2 * sizeof(PcGxBackgroundState);
    SDL_UploadToGPUBuffer(copy, &source, &dest, true);
    source.offset += dest.size;
    dest.buffer = renderer.rearPlane;
    dest.size = 256 * 192 * sizeof(PcGxRearPixel);
    SDL_UploadToGPUBuffer(copy, &source, &dest, true);
    SDL_EndGPUCopyPass(copy);
    PcViewport fitted = PcSettings_Viewport(width, height);
    bool skinDrawn =
        !PcGx.battle_ui && (PcGx.dex_ui || PcGx.party_ui || PcGx.menu_layer_count) && fitted.width && fitted.height;
    if (skinDrawn) {
        if (PcGx.party_ui)
            PcPartySkin_Blit(commands, renderer.color, fitted, PcGx.party_selected, PcGx.party_marked,
                             PcGx.party_menu_row, PcGx.party_occupied, PcGx.party_sex, PcGx.party_hp_rows,
                             PcGx.party_overview, PcGx.party_overview_data);
        else if (PcGx.dex_ui)
            PcDexSkin_Blit(commands, renderer.color, fitted, PcGx.dex_selected_row, PcGx.dex_tab, PcGx.dex_filter_row);
        if (PcGx.menu_layer_count)
            PcMenuSkin_Blit(commands, renderer.color, fitted);
    }
    unsigned samples = skinDrawn ? 0 : renderer.samples;
    SDL_GPUColorTargetInfo color = { .texture = samples ? renderer.msColor : renderer.color,
                                     .resolve_texture = samples ? renderer.color : NULL,
                                     .cycle_resolve_texture = true,
                                     .clear_color = { 0, 0, 0, 1 },
                                     .load_op = skinDrawn ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_CLEAR,
                                     .store_op = samples ? SDL_GPU_STOREOP_RESOLVE : SDL_GPU_STOREOP_STORE,
                                     .cycle = !skinDrawn };
    // Keep depth stored across the multisample-to-UI pass switch; discarding depth at the switch
    // produces depth-target faults on the tested NVIDIA Vulkan driver.
    SDL_GPUDepthStencilTargetInfo depth = { .texture = samples ? renderer.msDepth : renderer.depth,
                                            .clear_depth = 1,
                                            .load_op = SDL_GPU_LOADOP_CLEAR,
                                            .store_op = SDL_GPU_STOREOP_STORE,
                                            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
                                            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
                                            .cycle = true };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &color, 1, &depth);
    SDL_GPUViewport viewport = { .x = (float)fitted.x,
                                 .y = (float)fitted.y,
                                 .w = (float)fitted.width,
                                 .h = (float)fitted.height,
                                 .min_depth = 0,
                                 .max_depth = 1 };
    if (fitted.width && fitted.height)
        SDL_SetGPUViewport(pass, &viewport);
    SDL_GPUBufferBinding binding = { .buffer = renderer.vertices };
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
    SDL_GPUBuffer *storage[] = { renderer.textures, renderer.palettes, renderer.backgrounds, renderer.rearPlane };
    SDL_BindGPUFragmentStorageBuffers(pass, 0, storage, 4);
    Uint32 captureMode = 0;
    SetViewport(commands, pass, &viewport, captureMode, false);
    SDL_GPUViewport scene_viewport = viewport;
    if (PcGx.composite_scene) {
        scene_viewport.x += (float)PcGx.scene_viewport.x * fitted.width / PC_CANVAS_WIDTH;
        scene_viewport.y += (float)PcGx.scene_viewport.y * fitted.height / PC_CANVAS_HEIGHT;
        scene_viewport.w = (float)PcGx.scene_viewport.width * fitted.width / PC_CANVAS_WIDTH;
        scene_viewport.h = (float)PcGx.scene_viewport.height * fitted.height / PC_CANVAS_HEIGHT;
        // The vertex shader maps the BGs' clip-space depth (0.5 + priority * 0.08) to [0, 1].
        // Apply the same mapping to BG0's reserved scene interval while preserving homogeneous clipping.
        float priority = (PcGxRegisters[0].halves[4] & 3) * 0.04f;
        scene_viewport.min_depth = 0.749f + priority;
        scene_viewport.max_depth = 0.751f + priority;
    }
    renderer.batchCount = 0;
    renderer.sceneMinDepth = scene_viewport.min_depth;
    renderer.sceneMaxDepth = scene_viewport.max_depth;
    renderer.sceneProjectionScaleX = PcGx.wide_scene ? PcGx.projection_scale_x : 1.0f;
    for (int layer = 0; layer < 4; layer++) {
        if (layer == 3 && (samples || PcGx.battle_ui)) {
            SDL_EndGPURenderPass(pass);
            if (PcGx.battle_ui)
                PcMenuSkin_Blit(commands, renderer.color, fitted);
            samples = 0;
            color.texture = renderer.color;
            color.resolve_texture = NULL;
            color.store_op = SDL_GPU_STOREOP_STORE;
            depth.texture = renderer.depth;
            color.load_op = SDL_GPU_LOADOP_LOAD;
            color.cycle = false;
            pass = SDL_BeginGPURenderPass(commands, &color, 1, &depth);
            SDL_SetGPUViewport(pass, &viewport);
            SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
            SDL_BindGPUFragmentStorageBuffers(pass, 0, storage, 4);
            SetViewport(commands, pass, &viewport, captureMode, false);
        }
        if (layer == 1 && PcGx.composite_scene && (PcGxMainDisplay & 0x108) == 0x108 && fitted.width && fitted.height) {
            SetViewport(commands, pass, &scene_viewport, captureMode, true);
            SDL_BindGPUGraphicsPipeline(pass, (samples ? renderer.multisample[samples - 1][1] : renderer.pipelines[1]));
            SDL_DrawGPUPrimitives(pass, 6, 1, PcGx.vertex_count, 0);
            renderer.batches[renderer.batchCount++] = (CaptureBatch){ PcGx.vertex_count, 6, 1, true };
        }
        for (int translucent = 0; fitted.width && fitted.height && translucent < 2; translucent++) {
            int active_pipeline = -1;
            int active_scene = -1;
            for (u32 i = 0; i < PcGx.vertex_count;) {
                const PcGxVertex *v = &PcGx.vertices[i];
                bool ui = (v->polygon & PC_GX_POLYGON_UI) != 0;
                int vertexLayer = DrawLayer(v, scene_viewport.max_depth);
                if (vertexLayer != layer) {
                    i += 3;
                    continue;
                }
                unsigned format = (v->texture >> 26) & 7;
                bool blend = v->color[3] < 1 || format == 1 || format == 6;
                if (blend != (translucent != 0)) {
                    i += 3;
                    continue;
                }
                bool scene = PcGx.composite_scene && !(v->polygon & (0xc0000000u | PC_GX_POLYGON_UI));
                if (scene != active_scene) {
                    SetViewport(commands, pass, scene ? &scene_viewport : &viewport, captureMode, scene);
                    active_scene = scene;
                }
                int pipeline = blend ? ((v->polygon & 0x800) ? 2 : 1) : 0;
                if (pipeline != active_pipeline) {
                    SDL_BindGPUGraphicsPipeline(
                        pass, (samples ? renderer.multisample[samples - 1][pipeline] : renderer.pipelines[pipeline]));
                    active_pipeline = pipeline;
                }
                u32 end = i + 3;
                while (end < PcGx.vertex_count) {
                    const PcGxVertex *next = &PcGx.vertices[end];
                    unsigned next_format = (next->texture >> 26) & 7;
                    bool next_blend = next->color[3] < 1 || next_format == 1 || next_format == 6;
                    bool next_scene = PcGx.composite_scene && !(next->polygon & (0xc0000000u | PC_GX_POLYGON_UI));
                    int next_pipeline = next_blend ? ((next->polygon & 0x800) ? 2 : 1) : 0;
                    if (next_blend != blend || next_scene != scene || next_pipeline != pipeline ||
                        DrawLayer(next, scene_viewport.max_depth) != layer)
                        break;
                    end += 3;
                }
                SDL_DrawGPUPrimitives(pass, end - i, 1, i, 0);
                if (!ui)
                    renderer.batches[renderer.batchCount++] = (CaptureBatch){ i, end - i, (unsigned)pipeline, scene };
                i = end;
            }
        }
    }
    SDL_EndGPURenderPass(pass);
    SDL_GPUBlitInfo blit = { .source = { .texture = renderer.color, .w = width, .h = height },
                             .destination = { .texture = swapchain, .w = width, .h = height },
                             .load_op = SDL_GPU_LOADOP_DONT_CARE,
                             .filter = SDL_GPU_FILTER_NEAREST };
    SDL_BlitGPUTexture(commands, &blit);
    if (!SDL_SubmitGPUCommandBuffer(commands))
        return false;
    *presented = true;
    return true;
}

static void CaptureScreen(u32 control) {
    PcVideo *video = renderer.video;
    if (!video || !renderer.batchCount)
        return;
    if (!renderer.captureColor) {
        SDL_GPUTextureCreateInfo info = { .type = SDL_GPU_TEXTURETYPE_2D,
                                          .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                          .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                                          .width = 256,
                                          .height = 192,
                                          .layer_count_or_depth = 1,
                                          .num_levels = 1,
                                          .sample_count = SDL_GPU_SAMPLECOUNT_1 };
        renderer.captureColor = SDL_CreateGPUTexture(video->device, &info);
        info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        renderer.captureDepth = SDL_CreateGPUTexture(video->device, &info);
        SDL_GPUTransferBufferCreateInfo download = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                                                     .size = 256 * 192 * 4 };
        renderer.captureDownload = SDL_CreateGPUTransferBuffer(video->device, &download);
    }
    if (!renderer.captureColor || !renderer.captureDepth || !renderer.captureDownload)
        goto fail;
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(video->device);
    if (!commands)
        goto fail;
    SDL_GPUColorTargetInfo color = { .texture = renderer.captureColor,
                                     .clear_color = { 0, 0, 0, 1 },
                                     .load_op = SDL_GPU_LOADOP_CLEAR,
                                     .store_op = SDL_GPU_STOREOP_STORE };
    SDL_GPUDepthStencilTargetInfo depth = { .texture = renderer.captureDepth,
                                            .clear_depth = 1,
                                            .load_op = SDL_GPU_LOADOP_CLEAR,
                                            .store_op = SDL_GPU_STOREOP_DONT_CARE,
                                            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
                                            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &color, 1, &depth);
    SDL_GPUBufferBinding binding = { .buffer = renderer.vertices };
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
    SDL_GPUBuffer *storage[] = { renderer.textures, renderer.palettes, renderer.backgrounds, renderer.rearPlane };
    SDL_BindGPUFragmentStorageBuffers(pass, 0, storage, 4);
    Uint32 mode = (control & (1u << 24)) ? 2 : 1;

    for (Uint32 i = 0; i < renderer.batchCount; i++) {
        const CaptureBatch *batch = &renderer.batches[i];
        if (mode == 2 && !batch->scene)
            continue;
        SDL_GPUViewport viewport = { .w = 256,
                                     .h = 192,
                                     .min_depth = batch->scene ? renderer.sceneMinDepth : 0,
                                     .max_depth = batch->scene ? renderer.sceneMaxDepth : 1 };
        SetViewport(commands, pass, &viewport, mode, batch->scene);
        SDL_BindGPUGraphicsPipeline(pass, renderer.pipelines[batch->pipeline]);
        SDL_DrawGPUPrimitives(pass, batch->count, 1, batch->first, 0);
    }
    SDL_EndGPURenderPass(pass);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureRegion source = { .texture = renderer.captureColor, .w = 256, .h = 192, .d = 1 };
    SDL_GPUTextureTransferInfo destination = { .transfer_buffer = renderer.captureDownload };
    SDL_DownloadFromGPUTexture(copy, &source, &destination);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    bool ok = fence && SDL_WaitForGPUFences(video->device, true, &fence, 1);
    if (ok) {
        const u8 *pixels = SDL_MapGPUTransferBuffer(video->device, renderer.captureDownload, false);
        ok = pixels != NULL;
        if (pixels) {
            u32 bank = (control >> 16) & 3;
            u32 offset = ((control >> 18) & 3) * 0x8000;
            for (u32 i = 0; i < 256 * 192; i++) {
                u16 pixel =
                    0x8000 | (pixels[i * 4] >> 3) | ((pixels[i * 4 + 1] >> 3) << 5) | ((pixels[i * 4 + 2] >> 3) << 10);
                u32 address = bank * 0x20000 + ((offset + i * 2) & 0x1ffff);
                PcGxLcdc[address] = pixel & 255;
                PcGxLcdc[address + 1] = pixel >> 8;
            }
            PcGxCapturedBanks |= 1u << bank;
            SDL_UnmapGPUTransferBuffer(video->device, renderer.captureDownload);
        }
    }
    if (fence)
        SDL_ReleaseGPUFence(video->device, fence);
    if (ok)
        return;
fail:
    PcGx.failed = true;
    SDL_snprintf(PcGx.error, sizeof(PcGx.error), "Screen capture failed: %s", SDL_GetError());
}

bool PcGxVideo_Screenshot(PcVideo *video, const char *path) {
    if (!renderer.color)
        return SDL_SetError("No rendered frame to capture");
    SDL_GPUTransferBufferCreateInfo info = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                                             .size = renderer.width * renderer.height * 4 };
    SDL_GPUTransferBuffer *download = SDL_CreateGPUTransferBuffer(video->device, &info);
    if (!download)
        return false;
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(video->device);
    if (!commands) {
        SDL_ReleaseGPUTransferBuffer(video->device, download);
        return false;
    }
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureRegion source = { .texture = renderer.color, .w = renderer.width, .h = renderer.height, .d = 1 };
    SDL_GPUTextureTransferInfo dest = { .transfer_buffer = download };
    SDL_DownloadFromGPUTexture(copy, &source, &dest);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    bool ok = fence && SDL_WaitForGPUFences(video->device, true, &fence, 1);
    if (ok) {
        void *pixels = SDL_MapGPUTransferBuffer(video->device, download, false);
        if (pixels) {
            SDL_Surface *surface = SDL_CreateSurfaceFrom(renderer.width, renderer.height, SDL_PIXELFORMAT_RGBA32,
                                                         pixels, renderer.width * 4);
            ok = surface && SDL_SaveBMP(surface, path);
            SDL_DestroySurface(surface);
            SDL_UnmapGPUTransferBuffer(video->device, download);
        } else
            ok = false;
    }
    if (fence)
        SDL_ReleaseGPUFence(video->device, fence);
    SDL_ReleaseGPUTransferBuffer(video->device, download);
    return ok;
}
