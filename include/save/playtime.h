#ifndef POKEBW2_SAVE_PLAYTIME_H
#define POKEBW2_SAVE_PLAYTIME_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

#define PLAYTIME_MAX_HOURS 999

struct PlayTime {
    u16 hours;
    u8 minutes;
    u8 seconds;
    // The date and time of the last save
    union {
        u32 lastSaved;
        struct {
            u32 year : 7;
            u32 month : 4;
            u32 day : 5;
            u32 hour : 5;
            u32 minute : 6;
        };
    };
};

PlayTime *setupPlaytimeCounters(HeapID heapId);
void func_02008c40(PlayTime *time);
// Adds seconds to the play time, which stops at PLAYTIME_MAX_HOURS:59:59
void updateGameTime(PlayTime *time, u32 addSeconds);
void func_02008cdc(PlayTime *dest, const PlayTime *src);
u16 func_02008cec(PlayTime *time);
u8 func_02008cf0(PlayTime *time);
u8 func_02008cf4(PlayTime *time);
// Sets the date and time of the last save to now
void func_02008cf8(PlayTime *time);
u32 func_02008d68(PlayTime *time);
u32 func_02008d70(PlayTime *time);
u32 func_02008d78(PlayTime *time);
u32 func_02008d80(PlayTime *time);
u32 func_02008d88(PlayTime *time);
// Copy the word with the date and time of the last save out and back
void func_02008d90(PlayTime *time, u32 *copy);
void func_02008d98(PlayTime *time, u32 *copy);

#endif // POKEBW2_SAVE_PLAYTIME_H
