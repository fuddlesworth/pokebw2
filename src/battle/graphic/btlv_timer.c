// The two countdowns of a timed battle on the upper screen: the battle's time and the time to choose a command. The
// name is the ROM's string, from GFL_HeapAllocate's assert. The names are ours; swan has none for this file

#include "battle/btlv_timer.h"
#include "types.h"
#include "battle/btl_pokeparam.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_gauge.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/os.h"
#include "system/palanm.h"

// A countdown's actors: its label, the minutes' tens and ones, the colon and the seconds' tens and ones
#define TIMER_ACTOR_COUNT 6
// The highest limit, 99:59
#define TIMER_MAX_SECONDS (99 * 60 + 59)

// The label, colon and digit sequence IDs; warning graphics are one bank after the normal sequences.
enum {
    TIMER_SEQ_BATTLE_LABEL = 0,
    TIMER_SEQ_COMMAND_LABEL = 1,
    TIMER_SEQ_COLON = 2,
    TIMER_SEQ_FIRST_DIGIT = 3,
};

#define TIMER_SEQ_WARNING_OFFSET 13

// The timer's work, 0x78 bytes
struct BtlvTimer {
    ClActUnit *units[2];   // 0x00  one per countdown
    ClActor *actors[2][7]; // 0x08  the last of each row is never used
    u32 chars;             // 0x40
    u32 cellAnims;         // 0x44
    u32 palette;           // 0x48
    s32 limits[2];         // 0x4c  seconds
    u64 startTicks[2];     // 0x54
    u32 seqBase[2];        // 0x64  0, or 13 for the warning digits once the countdown reaches its warning time
    TCB *task;             // 0x6c
    TCB *task70;           // 0x70  removed with the actors, but never added
    u16 heapId;            // 0x74
};

static void BtlvTimer_DeleteActors(BtlvTimer *timer);
static void BtlvTimer_Draw(BtlvTimer *timer, int which);
static void BtlvTimer_Task(TCB *tcb, void *data);

BtlvTimer *BtlvTimer_Create(HeapID heapId) {
    BtlvTimer *timer = GFL_HeapAllocate(heapId, sizeof(BtlvTimer), TRUE, "btlv_timer.c", 0x81);
    ArcTool *arc;

    timer->heapId = heapId;
    arc = GFL_ArcSysCreateFileHandle(ARCID_BTLV, HEAPID_TAIL(timer->heapId));
    timer->units[BTLV_TIMER_BATTLE] = ClActUnit_Create(TIMER_ACTOR_COUNT, 0, timer->heapId);
    timer->units[BTLV_TIMER_COMMAND] = ClActUnit_Create(TIMER_ACTOR_COUNT, 0, timer->heapId);
    timer->chars = ClActSys_LoadChars(arc, 0x1ab, FALSE, CLACT_VRAM_MAIN, timer->heapId);
    timer->cellAnims = ClActSys_LoadCellAnims(arc, 0x1ac, 0x1ad, timer->heapId);
    timer->palette = ClActSys_LoadPltt(arc, BtlvGauge_PaletteFile(), CLACT_VRAM_MAIN, 0xe0, timer->heapId);
    PaletteFade_LoadFromVRAM(BtlvEffect_GetPaletteFade(), PALFADE_VRAM_MAIN_OBJ,
                             ClActSys_GetPlttVramAddr(timer->palette, FALSE) / 2, 0x20);
    GFL_ArcToolFree(arc);
    return timer;
}

void BtlvTimer_Delete(BtlvTimer *timer) {
    BtlvTimer_DeleteActors(timer);
    ClActSys_FreeChars(timer->chars);
    ClActSys_FreeCellAnims(timer->cellAnims);
    ClActSys_FreePltt(timer->palette);
    ClActUnit_Delete(timer->units[BTLV_TIMER_BATTLE]);
    ClActUnit_Delete(timer->units[BTLV_TIMER_COMMAND]);
    GFL_HeapFree(timer);
}

void BtlvTimer_Start(BtlvTimer *timer, s32 battleLimit, s32 commandLimit) {
    ClActorSetup setup = { 0, 0, 0, 0, 0 };
    const s32 xs[TIMER_ACTOR_COUNT] = { 0, 64, 68, 72, 78, 82 };
    const s32 ys[2] = { 128, 136 };
    const s32 sequences[2][TIMER_ACTOR_COUNT] = {
        { TIMER_SEQ_BATTLE_LABEL, TIMER_SEQ_FIRST_DIGIT, TIMER_SEQ_FIRST_DIGIT, TIMER_SEQ_COLON, TIMER_SEQ_FIRST_DIGIT,
          TIMER_SEQ_FIRST_DIGIT },
        { TIMER_SEQ_COMMAND_LABEL, TIMER_SEQ_FIRST_DIGIT, TIMER_SEQ_FIRST_DIGIT, TIMER_SEQ_COLON, TIMER_SEQ_FIRST_DIGIT,
          TIMER_SEQ_FIRST_DIGIT },
    };
    int i, j;

    for (i = 0; i < 2; i++) {
        if ((i == BTLV_TIMER_BATTLE && battleLimit != 0) || (i == BTLV_TIMER_COMMAND && commandLimit != 0)) {
            for (j = 0; j < TIMER_ACTOR_COUNT; j++) {
                setup.x = xs[j];
                setup.y = ys[i];
                setup.sequence = sequences[i][j];
                timer->actors[i][j] = ClActor_Create(timer->units[i], timer->chars, timer->palette, timer->cellAnims,
                                                     &setup, CLACT_SURFACE_MAIN, timer->heapId);
                ClActor_SetAutoAnim(timer->actors[i][j], TRUE);
            }
        }
    }
    if (battleLimit > TIMER_MAX_SECONDS) {
        battleLimit = TIMER_MAX_SECONDS;
    }
    if (commandLimit > TIMER_MAX_SECONDS) {
        commandLimit = TIMER_MAX_SECONDS;
    }
    timer->limits[BTLV_TIMER_BATTLE] = battleLimit;
    timer->limits[BTLV_TIMER_COMMAND] = commandLimit;
    timer->startTicks[BTLV_TIMER_BATTLE] = timer->startTicks[BTLV_TIMER_COMMAND] = OS_GetTick();
    BtlvTimer_SetVisible(timer, BTLV_TIMER_BATTLE, FALSE, FALSE);
    BtlvTimer_SetVisible(timer, BTLV_TIMER_COMMAND, FALSE, FALSE);
    timer->task = GFL_TCBMgrAddTask(BtlvEffect_GetTCBManager(), BtlvTimer_Task, timer, 0);
}

static void BtlvTimer_DeleteActors(BtlvTimer *timer) {
    int i, j;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < TIMER_ACTOR_COUNT; j++) {
            if (timer->actors[i][j] != NULL) {
                ClActor_Delete(timer->actors[i][j]);
                timer->actors[i][j] = NULL;
            }
        }
    }
    if (timer->task != NULL) {
        GFL_TCBRemove(timer->task);
        timer->task = NULL;
    }
    if (timer->task70 != NULL) {
        GFL_TCBRemove(timer->task70);
        timer->task70 = NULL;
    }
}

void BtlvTimer_SetVisible(BtlvTimer *timer, int which, BOOL visible, BOOL restart) {
    if (timer->actors[which][0] != NULL) {
        if (visible == TRUE && restart == TRUE) {
            timer->startTicks[which] = OS_GetTick();
            timer->seqBase[which] = 0;
            BtlvTimer_Draw(timer, which);
        }
        ClActUnit_SetActive(timer->units[which], visible);
    }
}

BOOL BtlvTimer_IsTimeUp(BtlvTimer *timer, int which) {
    s32 left = timer->limits[which] - (s32)OS_TicksToSeconds(OS_GetTick() - timer->startTicks[which]);

    return left <= 0;
}

// Draws a countdown's time left
static void BtlvTimer_Draw(BtlvTimer *timer, int which) {
    const s32 warnTimes[2] = { 300, 15 };
    int label = which == BTLV_TIMER_BATTLE ? TIMER_SEQ_BATTLE_LABEL : TIMER_SEQ_COMMAND_LABEL;
    s32 left = timer->limits[which] - (s32)OS_TicksToSeconds(OS_GetTick() - timer->startTicks[which]);
    int minutes, seconds, minuteTens, minuteOnes, secondTens, secondOnes;

    if (timer->actors[which][0] == NULL) {
        return;
    }
    if (left < 0) {
        left = 0;
    }
    minutes = left / 60;
    seconds = left % 60;
    minuteTens = minutes / 10;
    minuteOnes = minutes % 10;
    secondTens = seconds / 10;
    secondOnes = seconds % 10;
    if (left == warnTimes[which]) {
        timer->seqBase[which] = TIMER_SEQ_WARNING_OFFSET;
    }
    ClActor_SetSequence(timer->actors[which][0], label + timer->seqBase[which]);
    if (minuteTens == 0) {
        ClActor_SetVisible(timer->actors[which][1], FALSE);
    } else {
        ClActor_SetVisible(timer->actors[which][1], TRUE);
        ClActor_SetSequence(timer->actors[which][1], minuteTens + TIMER_SEQ_FIRST_DIGIT + timer->seqBase[which]);
    }
    if (minuteTens == 0 && minuteOnes == 0) {
        ClActor_SetVisible(timer->actors[which][2], FALSE);
        ClActor_SetVisible(timer->actors[which][3], FALSE);
    } else {
        ClActor_SetVisible(timer->actors[which][2], TRUE);
        ClActor_SetVisible(timer->actors[which][3], TRUE);
        ClActor_SetSequence(timer->actors[which][2], minuteOnes + TIMER_SEQ_FIRST_DIGIT + timer->seqBase[which]);
        ClActor_SetSequence(timer->actors[which][3], timer->seqBase[which] + TIMER_SEQ_COLON);
    }
    if (minuteTens == 0 && minuteOnes == 0 && secondTens == 0) {
        ClActor_SetVisible(timer->actors[which][4], FALSE);
    } else {
        ClActor_SetVisible(timer->actors[which][4], TRUE);
        ClActor_SetSequence(timer->actors[which][4], secondTens + TIMER_SEQ_FIRST_DIGIT + timer->seqBase[which]);
    }
    ClActor_SetSequence(timer->actors[which][5], secondOnes + TIMER_SEQ_FIRST_DIGIT + timer->seqBase[which]);
}

static void BtlvTimer_Task(TCB *tcb, void *data) {
    BtlvTimer *timer = data;

    BtlvTimer_Draw(timer, BTLV_TIMER_BATTLE);
    BtlvTimer_Draw(timer, BTLV_TIMER_COMMAND);
}
