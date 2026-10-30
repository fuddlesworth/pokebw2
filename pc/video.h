#ifndef POKEBW2_PC_VIDEO_H
#define POKEBW2_PC_VIDEO_H

#include "layout.h"
#include <SDL3/SDL.h>

typedef struct PcPixel {
    Uint8 r, g, b, a;
} PcPixel;

typedef struct PcVideo {
    SDL_Window *window;
    SDL_GPUDevice *device;
    SDL_GPUTexture *canvas;
    SDL_GPUTransferBuffer *upload;
    bool claimed;
} PcVideo;

bool PcVideo_Init(PcVideo *video);
void PcVideo_Destroy(PcVideo *video);
// The pixels are one composed canvas. Future 3D and UI passes share this presentation surface.
bool PcVideo_Present(PcVideo *video, const PcPixel *pixels, bool *presented);
bool PcVideo_MapPointer(PcVideo *video, float x, float y, float *canvas_x, float *canvas_y);

#endif
