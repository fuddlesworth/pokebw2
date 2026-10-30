#include "nitro/math.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    s32 key;
    u32 identity;
    u8 payload[7];
} Record;

static s32 Compare(void *a, void *b) {
    Record *left = a, *right = b;
    return (left->key > right->key) - (left->key < right->key);
}

static s32 Reverse(void *a, void *b) {
    return -Compare(a, b);
}

int main(void) {
    Record records[601];
    u32 seed = 1;
    MATH_QSort(NULL, 0, sizeof(Record), Compare, NULL);
    for (u32 count = 1; count <= 601; count++) {
        for (u32 i = 0; i < count; i++) {
            seed = seed * 1664525 + 1013904223;
            records[i].key = (s32)(seed % 23) - 11;
            records[i].identity = i;
            for (u32 j = 0; j < sizeof(records[i].payload); j++)
                records[i].payload[j] = (u8)(i + j);
        }
        records[0].key = INT32_MIN;
        if (count > 1)
            records[1].key = INT32_MAX;
        for (int reverse = 0; reverse < 2; reverse++) {
            MATH_QSort(records, count, sizeof(Record), reverse ? Reverse : Compare, NULL);
            u8 seen[601] = { 0 };
            for (u32 i = 0; i < count; i++) {
                if (records[i].identity >= count || seen[records[i].identity]++)
                    return EXIT_FAILURE;
                if (i && (reverse ? Reverse : Compare)(&records[i - 1], &records[i]) > 0)
                    return EXIT_FAILURE;
                for (u32 j = 0; j < sizeof(records[i].payload); j++) {
                    if (records[i].payload[j] != (u8)(records[i].identity + j))
                        return EXIT_FAILURE;
                }
            }
        }
    }
    puts("SDK sort ordering and record integrity passed");
    return EXIT_SUCCESS;
}
