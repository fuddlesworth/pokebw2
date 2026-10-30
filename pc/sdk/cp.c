#include "nitro/host_cp.h"
#include <limits.h>

PcCpValue PcCpNumerator, PcCpDenominator, PcCpSqrtParameter;
u16 PcCpDivMode, PcCpSqrtMode;
static PcCpValue quotient;
static u32 remainder;

PcCpValue *PcCp_GetQuotient(void) {
    s64 numerator = PcCpDivMode == 0 ? (s32)PcCpNumerator.words[0] : (s64)PcCpNumerator.wide;
    s64 denominator = PcCpDivMode == 2 ? (s64)PcCpDenominator.wide : (s32)PcCpDenominator.words[0];
    if (denominator == 0) {
        quotient.wide = numerator < 0 ? 1 : UINT64_MAX;
        remainder = (u32)numerator;
        if (PcCpDivMode == 0)
            quotient.wide ^= UINT64_C(0xffffffff00000000);
    } else if (numerator == INT64_MIN && denominator == -1) {
        quotient.wide = (u64)INT64_MIN;
        remainder = 0;
    } else {
        quotient.wide = (u64)(numerator / denominator);
        remainder = (u32)(numerator % denominator);
    }
    return &quotient;
}

u32 PcCp_GetRemainder(void) {
    PcCp_GetQuotient();
    return remainder;
}

u32 *PcCp_GetSqrt(void) {
    static u32 result;
    u64 value = PcCpSqrtMode == 1 ? PcCpSqrtParameter.wide : PcCpSqrtParameter.words[0];
    u64 root = 0;
    u64 bit = UINT64_C(1) << 62;
    while (bit > value)
        bit >>= 2;
    while (bit != 0) {
        if (value >= root + bit) {
            value -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    result = (u32)root;
    return &result;
}
