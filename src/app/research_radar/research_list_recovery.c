#include "types.h"
#include "app/research_radar/research_list_recovery.h"
#include "gfl/heap.h"

struct ResearchListRecovery {
    u8 cursor;
    s32 scrollY;
    s32 bgScroll;
};

ResearchListRecovery *ResearchListRecovery_Create(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(ResearchListRecovery), TRUE, "research_list_recovery.c", 32);
}

void ResearchListRecovery_Delete(ResearchListRecovery *recovery) {
    GFL_HeapFree(recovery);
}

void ResearchListRecovery_Init(ResearchListRecovery *recovery) {
    recovery->cursor = 0;
    recovery->scrollY = 0;
    recovery->bgScroll = -24;
}

u8 ResearchListRecovery_GetCursor(ResearchListRecovery *recovery) {
    return recovery->cursor;
}

s32 ResearchListRecovery_GetScrollY(ResearchListRecovery *recovery) {
    return recovery->scrollY;
}

s32 ResearchListRecovery_GetBGScroll(ResearchListRecovery *recovery) {
    return recovery->bgScroll;
}

void ResearchListRecovery_SetCursor(ResearchListRecovery *recovery, u8 cursor) {
    recovery->cursor = cursor;
}

void ResearchListRecovery_SetScrollY(ResearchListRecovery *recovery, s32 scrollY) {
    recovery->scrollY = scrollY;
}

void ResearchListRecovery_SetBGScroll(ResearchListRecovery *recovery, s32 bgScroll) {
    recovery->bgScroll = bgScroll;
}
