#ifndef POKEBW2_SAVE_ADVENTURE_H
#define POKEBW2_SAVE_ADVENTURE_H

#include "types.h"
#include "nitro/rtc.h"
#include "struct_decls.h"

// The game's clock, at 0x10 in the adventure data save block
struct AdventureTime {
    // Whether the clock was built; EvTime_Update does nothing while it is 0
    u32 valid;
    // When the clock was last checked
    RTCDate date;
    RTCTime time;
    // The number of the day it was last checked on
    s32 day;
    // When the adventure started, in seconds since 2000
    s64 startSeconds;
    // When the player first entered the Hall of Fame, or 0
    s64 seconds;
    // The minutes left of the day's countdown, at most MINUTES_PER_DAY
    u32 dayMinutes;
};

#define MINUTES_PER_DAY 1440

// The save block 0x25 (adventure_savedata.c, a guessed name)
struct AdventureSave {
    // The RTC's offset, which a changed clock no longer matches
    s64 rtcOffset;
    u8 ownerMac[6];
    // The day and month of the DS owner's birthday
    u8 birthdayMonth;
    u8 birthdayDay;
    AdventureTime time;
    u8 unk48[0x14];
};

u32 getSizeofAdventureDataBlk(void);
void initAdventureDataBlk(AdventureSave *adventure);

void setAdvTimeBlkRtcOffsetOwnerMacBdayMonthDay(AdventureSave *adventure);
BOOL hasClockNotBeenTampered(AdventureSave *adventure);
u8 getOSBirthdayMonth(AdventureSave *adventure);
u8 getOSBirthdayDay(AdventureSave *adventure);
// Starts the clock: the date and time now, no days since
void buildAdventureTimeBlk(AdventureTime *time);
BOOL hasFullDayMinutesPassed(AdventureTime *time);
// Takes minutes off the day's countdown, which stops at 0
void dayCountdownTest(AdventureTime *time, u32 minutes);
void setNewDayForCountdown(AdventureTime *time);

#endif // POKEBW2_SAVE_ADVENTURE_H
