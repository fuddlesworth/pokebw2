#include "timing.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "line %d: %s\n", __LINE__, #condition);                                                    \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

int main(void) {
    const unsigned rates[] = { 24, 30, 60, 120, 144, 240, 1000 };
    for (unsigned rate = 0; rate < sizeof(rates) / sizeof(rates[0]); rate++) {
        PcGameClock clock;
        PcGameClock_Init(&clock, 0);
        unsigned ticks = 0;
        for (unsigned frame = 0; frame <= rates[rate] * 10; frame++) {
            unsigned updates = PcGameClock_Advance(&clock, (uint64_t)frame * UINT64_C(1000000000) / rates[rate]);
            CHECK(updates <= PC_GAME_MAX_CATCHUP);
            ticks += updates;
        }
        /* One initial tick plus ten seconds, regardless of the presentation cadence. */
        CHECK(ticks == 601);
        CHECK(clock.phase == 0);
    }

    PcGameClock clock;
    PcGameClock_Init(&clock, 0);
    CHECK(PcGameClock_Advance(&clock, 0) == 1);
    CHECK(PcGameClock_Advance(&clock, 0) == 0);
    CHECK(PcGameClock_Advance(&clock, 16666666) == 0);
    CHECK(PcGameClock_Advance(&clock, 16666667) == 1);
    CHECK(PcGameClock_Advance(&clock, 50000000) == 2);
    /* A debugger stop discards excess full ticks, retaining the fractional frame. */
    CHECK(PcGameClock_Advance(&clock, 1060000000) == PC_GAME_MAX_CATCHUP);
    CHECK(PcGameClock_Advance(&clock, 1060000000) == 0);
    CHECK(PcGameClock_Advance(&clock, 1066666667) == 1);
    /* Long suspends cannot overflow the scaled accumulator or cause a catch-up spiral. */
    CHECK(PcGameClock_Advance(&clock, UINT64_MAX) == PC_GAME_MAX_CATCHUP);
    CHECK(PcGameClock_Advance(&clock, UINT64_MAX) == 0);
    /* A replaced clock origin must never look like an unsigned multi-century stall. */
    CHECK(PcGameClock_Advance(&clock, 0) == 0);
    CHECK(PcGameClock_Advance(&clock, 16666667) == 1);
    puts("Fixed game timing is independent of presentation rate");
    return EXIT_SUCCESS;
}
