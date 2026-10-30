#include "video.h"
#include "settings.h"

typedef char PcRequiresRGBA8Pixels[sizeof(PcPixel) == 4 ? 1 : -1];

bool PcVideo_Init(PcVideo *video) {
    *video = (PcVideo){ 0 };
    video->window = SDL_CreateWindow("BW2 PC", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!video->window) {
        return false;
    }
    video->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
    if (!video->device) {
        return false;
    }
    if (SDL_strcmp(SDL_GetGPUDeviceDriver(video->device), "vulkan") != 0) {
        return SDL_SetError("The PC target requires the Vulkan GPU backend");
    }
    if (!SDL_ClaimWindowForGPUDevice(video->device, video->window)) {
        return false;
    }
    video->claimed = true;
    const char *vsync = SDL_getenv("POKEBW2_VSYNC");
    SDL_GPUPresentMode mode =
        vsync && SDL_strcmp(vsync, "0") == 0 ? SDL_GPU_PRESENTMODE_IMMEDIATE : SDL_GPU_PRESENTMODE_VSYNC;
    if (!SDL_SetGPUSwapchainParameters(video->device, video->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode)) {
        return false;
    }
    SDL_GPUTextureCreateInfo texture_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = PC_CANVAS_WIDTH,
        .height = PC_CANVAS_HEIGHT,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    video->canvas = SDL_CreateGPUTexture(video->device, &texture_info);
    if (!video->canvas) {
        return false;
    }
    SDL_GPUTransferBufferCreateInfo upload_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = PC_CANVAS_WIDTH * PC_CANVAS_HEIGHT * sizeof(PcPixel),
    };
    video->upload = SDL_CreateGPUTransferBuffer(video->device, &upload_info);
    return video->upload != NULL;
}

void PcVideo_Destroy(PcVideo *video) {
    if (video->device) {
        SDL_WaitForGPUIdle(video->device);
        if (video->upload) {
            SDL_ReleaseGPUTransferBuffer(video->device, video->upload);
        }
        if (video->canvas) {
            SDL_ReleaseGPUTexture(video->device, video->canvas);
        }
        if (video->claimed) {
            SDL_ReleaseWindowFromGPUDevice(video->device, video->window);
        }
        SDL_DestroyGPUDevice(video->device);
    }
    SDL_DestroyWindow(video->window);
    *video = (PcVideo){ 0 };
}

bool PcVideo_Present(PcVideo *video, const PcPixel *pixels, bool *presented) {
    *presented = false;
    void *mapped = SDL_MapGPUTransferBuffer(video->device, video->upload, true);
    if (!mapped) {
        return false;
    }
    SDL_memcpy(mapped, pixels, PC_CANVAS_WIDTH * PC_CANVAS_HEIGHT * sizeof(PcPixel));
    SDL_UnmapGPUTransferBuffer(video->device, video->upload);

    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(video->device);
    if (!commands) {
        return false;
    }
    SDL_GPUTexture *swapchain = NULL;
    Uint32 width, height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, video->window, &swapchain, &width, &height)) {
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }
    if (!swapchain) {
        SDL_CancelGPUCommandBuffer(commands);
        SDL_Delay(10);
        return true;
    }
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
    if (!copy) {
        // Once a swapchain texture is acquired, the command buffer must be submitted, never cancelled.
        SDL_SubmitGPUCommandBuffer(commands);
        return false;
    }
    SDL_GPUTextureTransferInfo source = { .transfer_buffer = video->upload };
    SDL_GPUTextureRegion destination = {
        .texture = video->canvas, .w = PC_CANVAS_WIDTH, .h = PC_CANVAS_HEIGHT, .d = 1
    };
    SDL_UploadToGPUTexture(copy, &source, &destination, true);
    SDL_EndGPUCopyPass(copy);

    PcViewport viewport = PcSettings_Viewport(width, height);
    SDL_GPUBlitInfo blit = {
        .source = { .texture = video->canvas, .w = PC_CANVAS_WIDTH, .h = PC_CANVAS_HEIGHT },
        .destination = { .texture = swapchain,
                         .x = viewport.x,
                         .y = viewport.y,
                         .w = viewport.width,
                         .h = viewport.height },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .clear_color = { 0, 0, 0, 1 },
        .filter = SDL_GPU_FILTER_NEAREST,
    };
    if (viewport.width && viewport.height) {
        SDL_BlitGPUTexture(commands, &blit);
    } else {
        // A sub-pixel fitted viewport still needs initialized swapchain contents.
        SDL_GPUColorTargetInfo target = {
            .texture = swapchain,
            .clear_color = { 0, 0, 0, 1 },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPURenderPass *clear = SDL_BeginGPURenderPass(commands, &target, 1, NULL);
        if (!clear) {
            SDL_SubmitGPUCommandBuffer(commands);
            return false;
        }
        SDL_EndGPURenderPass(clear);
    }
    if (!SDL_SubmitGPUCommandBuffer(commands)) {
        return false;
    }
    *presented = true;
    return true;
}

bool PcVideo_MapPointer(PcVideo *video, float x, float y, float *canvas_x, float *canvas_y) {
    int width, height, pixel_width, pixel_height;
    if (!SDL_GetWindowSize(video->window, &width, &height) ||
        !SDL_GetWindowSizeInPixels(video->window, &pixel_width, &pixel_height) || width <= 0 || height <= 0) {
        return false;
    }
    return PcViewport_MapPointer(PcSettings_Viewport(pixel_width, pixel_height), x * pixel_width / width,
                                 y * pixel_height / height, canvas_x, canvas_y);
}
