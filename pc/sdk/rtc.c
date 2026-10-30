#include "nitro/rtc.h"
#include "nitro/os.h"
#include <SDL3/SDL.h>

void RTC_Init(void) {
    // SDL reads the host clock directly; no ARM7 RTC service needs to start.
}
RTCResult RTC_GetDateTimeAsync(RTCDate *date, RTCTime *time, RTCCallback callback, void *arg) {
    SDL_Time ticks;
    SDL_DateTime local;
    if (!date || !time)
        return RTC_RESULT_ILLEGAL_PARAMETER;
    if (!SDL_GetCurrentTime(&ticks) || !SDL_TimeToDateTime(ticks, &local, true))
        return RTC_RESULT_FATAL_ERROR;
    if (local.year < 2000 || local.year > 2099)
        return RTC_RESULT_ILLEGAL_PARAMETER;
    *date = (RTCDate){ (u32)(local.year - 2000), (u32)local.month, (u32)local.day, (RTCWeek)local.day_of_week };
    *time = (RTCTime){ (u32)local.hour, (u32)local.minute, (u32)local.second };
    if (callback)
        callback(RTC_RESULT_SUCCESS, arg);
    return RTC_RESULT_SUCCESS;
}
BOOL OS_IsAvailableWireless(void) {
    // Nintendo DS local-wireless hardware is not exposed by the native backend.
    return FALSE;
}
RTCResult RTC_GetDate(RTCDate *date) {
    RTCTime time;
    return RTC_GetDateTimeAsync(date, &time, NULL, NULL);
}
RTCResult RTC_GetTime(RTCTime *time) {
    RTCDate date;
    return RTC_GetDateTimeAsync(&date, time, NULL, NULL);
}
OSTWLParentalControl *OS_GetParentalControlInfoPtr(void) {
    return NULL;
}
void OS_GetMacAddress(u8 *address) {
    SDL_memset(address, 0, 6);
}
void OS_GetOwnerInfo(OSOwnerInfo *info) {
    static const u16 name[] = { 'P', 'l', 'a', 'y', 'e', 'r' };
    SDL_memset(info, 0, sizeof(*info));
    info->language = 1;
    info->birthday.month = 1;
    info->birthday.day = 1;
    info->nickNameLength = SDL_arraysize(name);
    SDL_memcpy(info->nickName, name, sizeof(name));
}
s64 OS_GetOwnerRtcOffset(void) {
    // Native saves use the host RTC directly, without a firmware clock correction.
    return 0;
}
void OS_GetOwnerInfoEx(OSOwnerInfoEx *info) {
    OSOwnerInfo owner;
    OS_GetOwnerInfo(&owner);
    SDL_memset(info, 0, sizeof(*info));
    info->language = owner.language;
    info->favoriteColor = owner.favoriteColor;
    info->birthday = owner.birthday;
    info->nickNameLength = owner.nickNameLength;
    info->commentLength = owner.commentLength;
    SDL_memcpy(info->nickName, owner.nickName, sizeof(info->nickName));
    SDL_memcpy(info->comment, owner.comment, sizeof(info->comment));
}
void OS_GetLowEntropyData(u32 buffer[8]) {
    SDL_Time time = 0;
    SDL_GetCurrentTime(&time);
    Uint64 state = (Uint64)time ^ SDL_GetPerformanceCounter();
    for (unsigned i = 0; i < 8; i++)
        buffer[i] = SDL_rand_bits_r(&state);
}
