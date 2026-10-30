#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/std.h"
#include "nnsys/fnd.h"
#include "system/aeabi.h"
#include "twl/mi.h"
#include <string.h>

u16 PcMiExMemControl;

u64 _u32_div_f(u32 dividend, u32 divisor) {
    if (divisor == 0)
        return dividend;
    return ((u64)(dividend % divisor) << 32) | (dividend / divisor);
}

int STD_GetStringLength(const char *str) {
    return (int)strlen(str);
}
void DC_FlushAll(void) {
    // Host allocations are cache coherent with the SDL upload path.
}

void MI_CpuFill8(void *dest, u8 value, u32 size) {
    if (size)
        memset(dest, value, size);
}
void MI_CpuFill(void *dest, u32 value, u32 size) {
    if (!size)
        return;
    u32 *words = dest;
    for (u32 i = 0; i < size / sizeof(*words); i++)
        words[i] = value;
    memcpy((u8 *)dest + size - size % sizeof(value), &value, size % sizeof(value));
}
void MIi_CpuCopy16(const void *src, void *dest, u32 size) {
    if (size)
        memcpy(dest, src, size);
}
void MI_CpuCopy8(const void *src, void *dest, u32 size) {
    if (size)
        memcpy(dest, src, size);
}
void MI_CpuCopy(const void *src, void *dest, u32 size) {
    if (size)
        memcpy(dest, src, size);
}
void MI_Copy32B(const void *src, void *dest) {
    memcpy(dest, src, 32);
}
#include <stdbool.h>
#include <stdlib.h>

// Game callbacks execute on the main thread. Preserve nested interrupt masking without ARM CPSR accesses.
static OSIntrMode interrupt_mode;
OSIntrMode OS_DisableInterrupts(void) {
    OSIntrMode previous = interrupt_mode;
    interrupt_mode |= 0x80;
    return previous;
}
OSIntrMode OS_RestoreInterrupts(OSIntrMode mask) {
    OSIntrMode previous = interrupt_mode;
    interrupt_mode = mask;
    return previous;
}
OSIntrMode OS_EnableInterrupts(void) {
    return OS_RestoreInterrupts(interrupt_mode & ~0x80u);
}

// Native CPU memory is coherent; Vulkan transfers are submitted explicitly by the backend.
/* Host uploads copy coherent CPU memory; there is no separate ARM data cache to invalidate. */
void DC_InvalidateRange(void *start, u32 size) {
    (void)start;
    (void)size;
}
void DC_FlushRange(const void *start, u32 size) {
    (void)start;
    (void)size;
}

void MI_UncompressLZ8(const void *src, void *dest) {
    const u8 *input = src;
    u8 *output = dest;
    bool extended = (input[0] & 15) != 0;
    size_t length = input[1] | ((u32)input[2] << 8) | ((u32)input[3] << 16), written = 0;
    input += 4;
    while (written < length) {
        u8 flags = *input++;
        for (unsigned bit = 0; bit < 8 && written < length; bit++) {
            if (!(flags & (128 >> bit))) {
                output[written++] = *input++;
                continue;
            }
            u32 byte = *input++, count = byte >> 4;
            if (!extended)
                count += 3;
            else if (count >= 2)
                count++;
            else {
                u32 prefix = byte & 15;
                if (count == 1)
                    prefix = (prefix << 8) | *input++;
                byte = *input++;
                count = (prefix << 4) + (byte >> 4) + (count == 1 ? 0x111 : 0x11);
            }
            u32 distance = ((byte & 15) << 8) + *input++ + 1;
            if (distance > written || count > length - written)
                abort();
            for (u32 i = 0; i < count; i++, written++)
                output[written] = output[written - distance];
        }
    }
}

void MIi_CpuSend32(const void *src, volatile void *dest, u32 size) {
    const u32 *words = src;
    for (u32 i = 0; i < size / sizeof(u32); i++) {
        if ((uptr)dest >= 0x04000400 && (uptr)dest < 0x04000600)
            PcGx_Write((uptr)dest, words[i]);
        else
            *(volatile u32 *)dest = words[i];
    }
}
void MIi_CpuCopy32(const void *src, void *dest, u32 size) {
    if (!size)
        return;
    if ((uptr)dest >= 0x04000400 && (uptr)dest < 0x04000600)
        MIi_CpuSend32(src, dest, size);
    else
        memcpy(dest, src, size);
}
void MIi_CpuCopyFast(const void *src, void *dest, u32 size) {
    MIi_CpuCopy32(src, dest, size);
}
void MI_Copy36B(const void *src, void *dest) {
    MIi_CpuCopy32(src, dest, 36);
}
void MI_Copy48B(const void *src, void *dest) {
    MIi_CpuCopy32(src, dest, 48);
}
void MI_Copy64B(const void *src, void *dest) {
    MIi_CpuCopy32(src, dest, 64);
}
void MI_Zero36B(void *dest) {
    memset(dest, 0, 36);
}
void MIi_CpuClear32(u32 value, void *dest, u32 size) {
    u32 *words = dest;
    for (u32 i = 0; i < size / sizeof(u32); i++)
        words[i] = value;
}
void MIi_CpuClear16(u16 value, void *dest, u32 size) {
    u16 *words = dest;
    for (u32 i = 0; i < size / sizeof(u16); i++)
        words[i] = value;
}
void MIi_CpuClearFast(u32 value, void *dest, u32 size) {
    MIi_CpuClear32(value, dest, size);
}
void MIi_DmaFill32(u32 dmaNo, void *dest, u32 data, u32 size, BOOL dmaEnable) {
    (void)dmaNo;
    (void)dmaEnable;
    MIi_CpuClear32(data, dest, size);
}
void MIi_DmaFill32Async(u32 dmaNo, void *dest, u32 data, u32 size, MIDmaCallback callback, void *arg, BOOL dmaEnable) {
    MIi_DmaFill32(dmaNo, dest, data, size, dmaEnable);
    if (callback)
        callback(arg);
}
void MI_NDmaFill(u32 ndmaNo, void *dest, u32 data, u32 size) {
    MIi_DmaFill32(ndmaNo, dest, data, size, TRUE);
}
void MI_NDmaFillAsync(u32 ndmaNo, void *dest, u32 data, u32 size, MINDmaCallback callback, void *arg) {
    MI_NDmaFill(ndmaNo, dest, data, size);
    if (callback)
        callback(arg);
}
void GX_SendFifo48B(const void *src, volatile void *dest) {
    MIi_CpuSend32(src, dest, 48);
}
void GX_SendFifo64B(const void *src, volatile void *dest) {
    MIi_CpuSend32(src, dest, 64);
}
void MI_SendGXCommandAsync(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    (void)dmaNo;
    MIi_CpuSend32(src, (volatile void *)0x04000400, length);
    if (callback)
        callback(arg);
}
void MI_SendGXCommandAsyncFast(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    MI_SendGXCommandAsync(dmaNo, src, length, callback, arg);
}
void MI_SendNDmaGXCommandAsync(u32 dmaNo, const void *src, u32 length, MINDmaCallback callback, void *arg) {
    MI_SendGXCommandAsync(dmaNo, src, length, callback, arg);
}
void MI_SendNDmaGXCommandAsyncFast(u32 dmaNo, const void *src, u32 length, MINDmaCallback callback, void *arg) {
    MI_SendGXCommandAsync(dmaNo, src, length, callback, arg);
}
BOOL OS_IsRunOnTwl(void) {
    return FALSE;
}
u32 OS_GetConsoleType(void) {
    // Use the retail DS feature set; native builds have no development-console overlays.
    return 0x80000001;
}
void *NNS_FndAllocFromAllocator(NNSFndAllocator *allocator, u32 size) {
    return allocator->pFunc->pfAlloc(allocator, size);
}
void NNS_FndFreeToAllocator(NNSFndAllocator *allocator, void *memBlock) {
    allocator->pFunc->pfFree(allocator, memBlock);
}
