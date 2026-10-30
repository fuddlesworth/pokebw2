#ifndef POKEBW2_HOST_GX_H
#define POKEBW2_HOST_GX_H
#include "types.h"
void PcGx_Write(uptr address, u32 value);
extern u32 PcGxStatus;
extern u32 PcGxMainDisplay, PcGxSubDisplay;
extern u16 PcGxDisplay3D;
extern u16 PcGx1DotDepth;
extern u16 PcGxPower;
typedef union {
    u32 words[64];
    u16 halves[128];
} PcGxRasterState;
extern PcGxRasterState PcGxRaster;
void PcGx_ResetHardware(void);
void PcGx_InitVRAM(u32 preservedBanks);
void PcGx_ClearFifo(void);
typedef void (*PcGxCaptureCallback)(u32 control);
void PcGx_SetCaptureCallback(PcGxCaptureCallback callback);
void PcGx_Capture(void);
extern u8 PcGxLcdc[0xa4000];
extern u32 PcGxCapturedBanks;
extern BOOL PcGxWideCaptureValid;
u32 PcGx_GetPolygonCount(void);
u32 PcGx_GetVertexCount(void);
void PcGx_ReadClipMatrix(void *dest);
void PcGx_ReadVectorMatrix(void *dest);
#define PC_GX_RASTER16(address) PcGxRaster.halves[((address) & 255) / 2]
#define PC_GX_RASTER32(address) PcGxRaster.words[((address) & 255) / 4]
typedef union {
    u32 words[32];
    u16 halves[64];
} PcGx2DRegisters;
extern PcGx2DRegisters PcGxRegisters[2];
extern u8 PcGxBackground[2][524288];
#define PC_GX_REG16(address) PcGxRegisters[((address) >> 12) & 1].halves[((address) & 0xfff) / 2]
#define PC_GX_REG32(address) PcGxRegisters[((address) >> 12) & 1].words[((address) & 0xfff) / 4]
#endif
