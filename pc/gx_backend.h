#ifndef POKEBW2_PC_GX_BACKEND_H
#define POKEBW2_PC_GX_BACKEND_H
#include "menu_skin.h"
#include "nitro/gx.h"
#include "video.h"

enum { PC_GX_MAX_VERTICES = 262144, PC_GX_TEX_BYTES = 524288, PC_GX_PLTT_BYTES = 98304 };
// Bits 24-29 belong to the DS polygon ID, including particle IDs. Bits 21 and 23 are unused by the hardware.
// Native UI colors are independent of the original screen's brightness and blend registers.
enum { PC_GX_POLYGON_WIDE_CAPTURE = 1u << 21, PC_GX_POLYGON_UI = 1u << 23 };
enum { PC_GX_POLYGON_WIPE = PC_GX_POLYGON_WIDE_CAPTURE | PC_GX_POLYGON_UI };
typedef struct {
    float clip[4], uv[2], color[4];
    u32 texture, palette, polygon;
} PcGxVertex;
typedef struct {
    PcGxVertex vertices[PC_GX_MAX_VERTICES];
    u32 vertex_count;
    bool composite_scene;
    bool wide_scene;
    float projection_scale_x;
    bool battle_ui;
    bool dex_ui;
    bool party_ui;
    u32 menu_layer_count;
    PcMenuSkinLayer menu_layers[64];
    bool party_overview;
    bool party_overview_data;
    u32 party_selected;
    u32 party_marked;
    u32 party_menu_row;
    u32 party_occupied;
    u32 party_sex;
    u32 party_hp_rows;
    u32 dex_selected_row;
    u32 dex_tab;
    u32 dex_filter_row;
    PcViewport scene_viewport;
    u8 textures[PC_GX_TEX_BYTES], palettes[PC_GX_PLTT_BYTES];
    u16 clear_color[256 * 256], clear_depth[256 * 256];
    bool failed;
    char error[128];
} PcGxFrame;
extern PcGxFrame PcGx;
extern u32 PcGxWideCapture[PC_CANVAS_WIDTH * PC_CANVAS_HEIGHT];
bool PcGx_WideCaptureActive(void);
// Decoded rear-plane pixels are shared by the display and capture passes.
typedef struct {
    u32 color;
    float depth;
} PcGxRearPixel;
void PcGx_ReadRearPlane(PcGxRearPixel pixels[256 * 192]);
typedef struct {
    u8 object[262144], object_palette[512], object_extended_palette[8192], oam[1024];
    u8 background_palette[512], background_extended_palette[32768];
} PcGx2DMemory;
typedef struct {
    u8 vram[524288], palette[512], extended_palette[32768];
    u8 object[262144], object_palette[512], object_extended_palette[8192], oam[1024];
    u32 registers[32], display;
} PcGxBackgroundState;
extern PcGx2DMemory PcGx2D[2];
void PcGx_BeginFrame(void);
void PcGx_DrawBackgrounds(unsigned engine);
void PcGx_DrawObjects(unsigned engine, PcViewport viewport);
void PcGx_DrawBannerObjects(unsigned engine, PcViewport viewport);
void PcGx_DrawBackgroundsInViewport(unsigned engine, PcViewport viewport, unsigned source_width,
                                    unsigned source_height);
void PcGx_DrawBackgroundLayers(unsigned engine, PcViewport viewport, unsigned source_width, unsigned source_height,
                               unsigned layers);
void PcGx_DrawBackgroundRegion(unsigned engine, PcViewport viewport, SDL_FRect source, unsigned layers);
bool PcGxVideo_Init(PcVideo *video);
void PcGxVideo_Destroy(PcVideo *video);
bool PcGxVideo_Present(PcVideo *video, bool *presented);
bool PcGxVideo_Screenshot(PcVideo *video, const char *path);
#endif
