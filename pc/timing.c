#include "timing.h"
#include <stdbool.h>

#define NS_PER_SECOND UINT64_C(1000000000)

void PcGameClock_Init(PcGameClock *clock, uint64_t now_ns) {
    clock->previous_ns = now_ns;
    clock->phase = NS_PER_SECOND;
}

unsigned PcGameClock_Advance(PcGameClock *clock, uint64_t now_ns) {
    uint64_t elapsed = now_ns >= clock->previous_ns ? now_ns - clock->previous_ns : 0;
    clock->previous_ns = now_ns;
    /* Whole seconds already exceed the catch-up budget; discard them before multiplying. */
    bool stalled = elapsed >= NS_PER_SECOND;
    clock->phase += (elapsed % NS_PER_SECOND) * PC_GAME_HZ;
    uint64_t ticks = clock->phase / NS_PER_SECOND;
    clock->phase %= NS_PER_SECOND;
    /* Retain the fractional phase, but never repay an unbounded suspend/debugger backlog. */
    return stalled || ticks > PC_GAME_MAX_CATCHUP ? PC_GAME_MAX_CATCHUP : (unsigned)ticks;
}
