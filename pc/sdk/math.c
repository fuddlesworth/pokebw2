#include "nitro/math.h"
#include <stddef.h>

static void SwapElements(u8 *a, u8 *b, u32 width) {
    for (u32 i = 0; i < width; i++) {
        u8 byte = a[i];
        a[i] = b[i];
        b[i] = byte;
    }
}

static void SiftDown(u8 *data, size_t root, size_t count, u32 width, MATHCompareFunc compare) {
    while (root < count / 2) {
        size_t child = root * 2 + 1;
        if (child + 1 < count && compare(data + child * width, data + (child + 1) * width) < 0)
            child++;
        if (compare(data + root * width, data + child * width) >= 0)
            return;
        SwapElements(data + root * width, data + child * width, width);
        root = child;
    }
}

void MATH_QSort(void *head, u32 num, u32 width, MATHCompareFunc compare, void *stackBuffer) {
    // The SDK routine is ARM assembly. Heapsort preserves its comparator ABI without
    // a global qsort callback adapter, allocation, or the SDK's recursion scratch space.
    (void)stackBuffer;
    if (num < 2 || !width)
        return;
    u8 *data = head;
    for (size_t root = num / 2; root > 0; root--)
        SiftDown(data, root - 1, num, width, compare);
    for (size_t end = num - 1; end > 0; end--) {
        SwapElements(data, data + end * width, width);
        SiftDown(data, 0, end, width, compare);
    }
}
