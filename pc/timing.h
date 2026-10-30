#ifndef POKEBW2_PC_TIMING_H
#define POKEBW2_PC_TIMING_H

#include <stdint.h>

enum { PC_GAME_HZ = 60, PC_GAME_MAX_CATCHUP = 5 };

typedef struct {
    uint64_t previous_ns;
    uint64_t phase;
} PcGameClock;

/* The first advance produces one frame, including when no time has elapsed. */
void PcGameClock_Init(PcGameClock *clock, uint64_t now_ns);
unsigned PcGameClock_Advance(PcGameClock *clock, uint64_t now_ns);

#endif
