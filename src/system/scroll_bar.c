#include "system/scroll_bar.h"
#include "types.h"

// A scroll bar's conversions between a value and the bar's position, with 8 bits of fraction. The file's name and
// the functions' are ours

u32 ScrollBar_GetValue(u32 max, u32 pos, u32 top, u32 bottom, u32 barSize) {
    u32 start = top + barSize / 2;
    u32 end = bottom - barSize / 2;

    if (pos <= start) {
        return 0;
    }
    if (pos >= end) {
        return max;
    }
    pos -= start;
    max <<= 8;
    return (pos * max / (end - start)) >> 8;
}

u32 ScrollBar_GetPos(u32 max, u32 value, u32 top, u32 bottom, u32 barSize) {
#ifdef POKEBW2_PC
    if (max == 0) {
        return top + barSize / 2;
    }
#endif
    u32 range = (bottom - top - barSize) << 8;

    value = (range * value / max) >> 8;
    return top + value + barSize / 2;
}
