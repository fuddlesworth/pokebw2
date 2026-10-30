#ifndef POKEBW2_HOST_CP_H
#define POKEBW2_HOST_CP_H
#include "types.h"

typedef union {
    u64 wide;
    u32 words[2];
} PcCpValue;
extern PcCpValue PcCpNumerator, PcCpDenominator, PcCpSqrtParameter;
extern u16 PcCpDivMode, PcCpSqrtMode;
PcCpValue *PcCp_GetQuotient(void);
u32 PcCp_GetRemainder(void);
u32 *PcCp_GetSqrt(void);
#endif
