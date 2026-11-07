#include "battle/b_plist_anm.h"
#include "battle/b_plist_main.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "system/palanm.h"

// The battle party list's buttons (b_plist_anm.c, named by the ROM's embedded string): the pieces of each button in
// each of its states, cut from the list's tilemaps, drawn onto the sub screen's BG 2, and the press animation that
// draws a button pressed and then released

// Where a button is on the screen and its size, in tiles
typedef struct {
    u8 x;
    u8 y;
    u8 width;
    u8 height;
} BPlistButtonRect;

static void BPlistAnm_CopyScrnRect(u16 *dst, const u16 *src, u8 x, u8 y, u8 width, u8 height);
static u16 *BPlistAnm_GetButtonScrn(BPlistWork *work, u8 button, u8 state, u8 mode);
static void BPlistAnm_MakeButtonScrn(BPlistWork *work, u16 *buf, u8 button, u8 state, u8 mode);
static void BPlistAnm_PutButton(BPlistWork *work, u8 button, u8 state, u8 mode);
static u8 BPlistAnm_CanSwitch(BPlistWork *work);

static const BPlistButtonRect sBPlistButtonRects[32] = {
    { 0x00, 0x00, 0x10, 0x06 }, { 0x10, 0x01, 0x10, 0x06 }, { 0x00, 0x06, 0x10, 0x06 }, { 0x10, 0x07, 0x10, 0x06 },
    { 0x00, 0x0c, 0x10, 0x06 }, { 0x10, 0x0d, 0x10, 0x06 }, { 0x1b, 0x13, 0x05, 0x05 }, { 0x00, 0x01, 0x1e, 0x11 },
    { 0x00, 0x13, 0x0d, 0x05 }, { 0x0c, 0x13, 0x0d, 0x05 }, { 0x0d, 0x13, 0x0d, 0x05 }, { 0x0c, 0x13, 0x0d, 0x05 },
    { 0x00, 0x13, 0x05, 0x05 }, { 0x05, 0x13, 0x05, 0x05 }, { 0x00, 0x06, 0x10, 0x06 }, { 0x10, 0x06, 0x10, 0x06 },
    { 0x00, 0x0c, 0x10, 0x06 }, { 0x10, 0x0c, 0x10, 0x06 }, { 0x00, 0x06, 0x10, 0x06 }, { 0x10, 0x06, 0x10, 0x06 },
    { 0x00, 0x0c, 0x10, 0x06 }, { 0x10, 0x0c, 0x10, 0x06 }, { 0x00, 0x06, 0x10, 0x06 }, { 0x10, 0x06, 0x10, 0x06 },
    { 0x00, 0x0c, 0x10, 0x06 }, { 0x10, 0x0c, 0x10, 0x06 }, { 0x08, 0x12, 0x10, 0x06 }, { 0x00, 0x13, 0x1a, 0x05 },
    { 0x0b, 0x13, 0x05, 0x02 }, { 0x10, 0x13, 0x05, 0x02 }, { 0x0b, 0x15, 0x05, 0x02 }, { 0x10, 0x15, 0x05, 0x02 },
};

void BPlistAnm_CutButtonScrn(BPlistWork *work, const void *scrn) {
    BPlistAnm_CopyScrnRect(work->plateScrn[BPLIST_PLATE_IN_BATTLE][BPLIST_BUTTON_NORMAL], scrn, 0, 0, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[BPLIST_PLATE_IN_BATTLE][BPLIST_BUTTON_PRESSED], scrn, 0, 6, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[BPLIST_PLATE_IN_BATTLE][BPLIST_BUTTON_SELECTED], scrn, 0, 12, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[BPLIST_PLATE_IN_BATTLE][BPLIST_BUTTON_DISABLED], scrn, 0, 18, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[BPLIST_PLATE_RESERVE][BPLIST_BUTTON_NORMAL], scrn, 16, 0, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[BPLIST_PLATE_RESERVE][BPLIST_BUTTON_PRESSED], scrn, 16, 6, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[BPLIST_PLATE_RESERVE][BPLIST_BUTTON_SELECTED], scrn, 16, 12, 16, 6);
    BPlistAnm_CopyScrnRect(work->plateScrn[BPLIST_PLATE_RESERVE][BPLIST_BUTTON_DISABLED], scrn, 16, 18, 16, 6);
    BPlistAnm_CopyScrnRect(work->moveButtonScrn[BPLIST_BUTTON_NORMAL], scrn, 0, 39, 13, 5);
    BPlistAnm_CopyScrnRect(work->moveButtonScrn[BPLIST_BUTTON_PRESSED], scrn, 0, 44, 13, 5);
    BPlistAnm_CopyScrnRect(work->moveButtonScrn[BPLIST_BUTTON_SELECTED], scrn, 13, 39, 13, 5);
    BPlistAnm_CopyScrnRect(work->moveButtonScrn[BPLIST_BUTTON_DISABLED], scrn, 13, 44, 13, 5);
    BPlistAnm_CopyScrnRect(work->button12Scrn[BPLIST_BUTTON_NORMAL], scrn, 0, 49, 5, 5);
    BPlistAnm_CopyScrnRect(work->button12Scrn[BPLIST_BUTTON_PRESSED], scrn, 5, 49, 5, 5);
    BPlistAnm_CopyScrnRect(work->button12Scrn[BPLIST_BUTTON_SELECTED], scrn, 10, 49, 5, 5);
    BPlistAnm_CopyScrnRect(work->button12Scrn[BPLIST_BUTTON_DISABLED], scrn, 15, 49, 5, 5);
    BPlistAnm_CopyScrnRect(work->button13Scrn[BPLIST_BUTTON_NORMAL], scrn, 0, 54, 5, 5);
    BPlistAnm_CopyScrnRect(work->button13Scrn[BPLIST_BUTTON_PRESSED], scrn, 5, 54, 5, 5);
    BPlistAnm_CopyScrnRect(work->button13Scrn[BPLIST_BUTTON_SELECTED], scrn, 10, 54, 5, 5);
    BPlistAnm_CopyScrnRect(work->button13Scrn[BPLIST_BUTTON_DISABLED], scrn, 15, 54, 5, 5);
    BPlistAnm_CopyScrnRect(work->button6Scrn[BPLIST_BUTTON_NORMAL], scrn, 26, 24, 5, 5);
    BPlistAnm_CopyScrnRect(work->button6Scrn[BPLIST_BUTTON_PRESSED], scrn, 26, 29, 5, 5);
    BPlistAnm_CopyScrnRect(work->button6Scrn[BPLIST_BUTTON_SELECTED], scrn, 26, 34, 5, 5);
    BPlistAnm_CopyScrnRect(work->button6Scrn[BPLIST_BUTTON_DISABLED], scrn, 26, 39, 5, 5);
    BPlistAnm_CopyScrnRect(work->button27Scrn[BPLIST_BUTTON_NORMAL], scrn, 0, 24, 26, 5);
    BPlistAnm_CopyScrnRect(work->button27Scrn[BPLIST_BUTTON_PRESSED], scrn, 0, 29, 26, 5);
    BPlistAnm_CopyScrnRect(work->button27Scrn[BPLIST_BUTTON_SELECTED], scrn, 0, 34, 26, 5);
    BPlistAnm_CopyScrnRect(work->button28Scrn[BPLIST_BUTTON_NORMAL], scrn, 0, 59, 5, 2);
    BPlistAnm_CopyScrnRect(work->button28Scrn[BPLIST_BUTTON_PRESSED], scrn, 5, 59, 5, 2);
    BPlistAnm_CopyScrnRect(work->button28Scrn[BPLIST_BUTTON_SELECTED], scrn, 10, 59, 5, 2);
}

void BPlistAnm_CutPageScrn(BPlistWork *work, const void *scrn) {
    BPlistAnm_CopyScrnRect(work->button7Scrn[BPLIST_BUTTON_NORMAL], scrn, 0, 0, 30, 17);
    BPlistAnm_CopyScrnRect(work->button7Scrn[BPLIST_BUTTON_PRESSED], scrn, 0, 17, 30, 17);
    BPlistAnm_CopyScrnRect(work->button7Scrn[BPLIST_BUTTON_SELECTED], scrn, 0, 34, 30, 17);
    BPlistAnm_CopyScrnRect(work->button14Scrn[BPLIST_BUTTON_NORMAL], scrn, 0, 51, 16, 6);
    BPlistAnm_CopyScrnRect(work->button14Scrn[BPLIST_BUTTON_PRESSED], scrn, 16, 51, 16, 6);
    BPlistAnm_CopyScrnRect(work->button14Scrn[BPLIST_BUTTON_SELECTED], scrn, 0, 57, 16, 6);
    BPlistAnm_CopyScrnRect(work->button14Scrn[BPLIST_BUTTON_DISABLED], scrn, 16, 57, 16, 6);
}

// Copies a width by height piece of a 32-tile-wide tilemap
static void BPlistAnm_CopyScrnRect(u16 *dst, const u16 *src, u8 x, u8 y, u8 width, u8 height) {
    u16 i, j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            dst[i * width + j] = src[(y + i) * 32 + x + j];
        }
    }
}

static u16 *BPlistAnm_GetButtonScrn(BPlistWork *work, u8 button, u8 state, u8 mode) {
    switch (button) {
    case BPLIST_BUTTON_PARTY_0:
    case BPLIST_BUTTON_PARTY_1:
    case BPLIST_BUTTON_PARTY_2:
    case BPLIST_BUTTON_PARTY_3:
    case BPLIST_BUTTON_PARTY_4:
    case BPLIST_BUTTON_PARTY_5:
        if (mode == BPLIST_PLATE_IN_BATTLE) {
            return work->plateScrn[BPLIST_PLATE_IN_BATTLE][state];
        }
        return work->plateScrn[BPLIST_PLATE_RESERVE][state];
    case BPLIST_BUTTON_BACK:
        return work->button6Scrn[state];
    case BPLIST_BUTTON_SWITCH:
        return work->button7Scrn[state];
    case BPLIST_BUTTON_SELECTED_SUMMARY:
    case BPLIST_BUTTON_MOVES_SUMMARY:
    case BPLIST_BUTTON_SELECTED_MOVES:
    case BPLIST_BUTTON_SUMMARY_MOVES:
        return work->moveButtonScrn[state];
    case BPLIST_BUTTON_PREV_POKEMON:
        return work->button12Scrn[state];
    case BPLIST_BUTTON_NEXT_POKEMON:
        return work->button13Scrn[state];
    case BPLIST_BUTTON_MOVE_INFO_0:
    case BPLIST_BUTTON_MOVE_INFO_1:
    case BPLIST_BUTTON_MOVE_INFO_2:
    case BPLIST_BUTTON_MOVE_INFO_3:
    case BPLIST_BUTTON_PP_RESTORE_0:
    case BPLIST_BUTTON_PP_RESTORE_1:
    case BPLIST_BUTTON_PP_RESTORE_2:
    case BPLIST_BUTTON_PP_RESTORE_3:
    case BPLIST_BUTTON_FORGET_0:
    case BPLIST_BUTTON_FORGET_1:
    case BPLIST_BUTTON_FORGET_2:
    case BPLIST_BUTTON_FORGET_3:
    case BPLIST_BUTTON_FORGET_NEW_MOVE:
        return work->button14Scrn[state];
    case BPLIST_BUTTON_CONFIRM_FORGET:
        return work->button27Scrn[state];
    case BPLIST_BUTTON_MOVE_TAB_0:
    case BPLIST_BUTTON_MOVE_TAB_1:
    case BPLIST_BUTTON_MOVE_TAB_2:
    case BPLIST_BUTTON_MOVE_TAB_3:
        return work->button28Scrn[state];
    }
    return NULL;
}

static void BPlistAnm_MakeButtonScrn(BPlistWork *work, u16 *buf, u8 button, u8 state, u8 mode) {
    u16 *src;
    u8 width;
    int size;
    u8 idx;
    u16 tiles[2];
    u8 i, j;

    src = BPlistAnm_GetButtonScrn(work, button, state, mode);
    width = sBPlistButtonRects[button].width;
    size = width * sBPlistButtonRects[button].height;
    MI_CpuCopy8(src, buf, size * 2);

    switch (button) {
    case BPLIST_BUTTON_PARTY_0:
    case BPLIST_BUTTON_PARTY_1:
    case BPLIST_BUTTON_PARTY_2:
    case BPLIST_BUTTON_PARTY_3:
    case BPLIST_BUTTON_PARTY_4:
    case BPLIST_BUTTON_PARTY_5:
        idx = BPlistMain_GetPartySlot(work, button);
        if (work->pokemon[idx].species == 0) {
            break;
        }
        if (work->pokemon[idx].isEgg) {
            tiles[0] = buf[width * 2 + 5];
            tiles[1] = buf[width * 3 + 5];
            for (i = 0; i < 2; i++) {
                for (j = 0; j < 9; j++) {
                    buf[(i + 2) * width + j + 6] = tiles[i];
                }
            }
        } else if (work->pokemon[idx].hp == 0) {
            for (i = 0; i < size; i++) {
                buf[i] = (buf[i] & 0xfff) | (((state & 1) + 7) << 12);
            }
        } else if (BPlistMain_IsPartnerSlot(work, idx) == TRUE) {
            for (i = 0; i < size; i++) {
                buf[i] = (buf[i] & 0xfff) | (((state & 1) + 5) << 12);
            }
        }
        break;
    }
}

static void BPlistAnm_PutButton(BPlistWork *work, u8 button, u8 state, u8 mode) {
    u16 *buf = GFL_HeapAllocate(HEAPID_TAIL(work->param->heapId),
                                sBPlistButtonRects[button].width * sBPlistButtonRects[button].height * 2, FALSE,
                                "b_plist_anm.c", 517);

    BPlistAnm_MakeButtonScrn(work, buf, button, state, mode);
    GFL_BGSysLoadScrAreaAll(6, buf, sBPlistButtonRects[button].x, sBPlistButtonRects[button].y,
                            sBPlistButtonRects[button].width, sBPlistButtonRects[button].height);
    GFL_BGSysQueueScrLoad(6);
    GFL_HeapFree(buf);
}

void BPlistAnm_StartButtonAnm(BPlistWork *work, u8 button) {
    work->animMode = BPLIST_PLATE_IN_BATTLE;
    if (button <= BPLIST_BUTTON_PARTY_5 && BPlistMain_CheckPos(work, button) == BPLIST_POS_RESERVE) {
        work->animMode = BPLIST_PLATE_RESERVE;
    }
    work->animSeq = 0;
    work->animCount = 0;
    work->animButton = button;
    work->animActive = TRUE;
}

enum {
    STEP_BUTTON_LIGHT,
    STEP_BUTTON_UNLIGHT,
    STEP_BUTTON_END,
};

void BPlistAnm_MainButtonAnm(BPlistWork *work) {
    if (!work->animActive) {
        return;
    }

    switch (work->animSeq) {
    case STEP_BUTTON_LIGHT:
        BPlistAnm_PutButton(work, work->animButton, BPLIST_BUTTON_PRESSED, work->animMode);
        work->animSeq++;
        break;
    case STEP_BUTTON_UNLIGHT:
        if (work->animCount == 4) {
            BPlistAnm_PutButton(work, work->animButton, BPLIST_BUTTON_NORMAL, work->animMode);
            work->animCount = 0;
            work->animSeq++;
        } else {
            work->animCount++;
        }
        break;
    case STEP_BUTTON_END:
        if (work->animCount == 1) {
            work->animCount = 0;
            work->animSeq = STEP_BUTTON_LIGHT;
            work->animActive = FALSE;
        } else {
            work->animCount++;
        }
        break;
    }
}

void BPlistAnm_PutPageButtons(BPlistWork *work, u8 page) {
    u16 i;
    int state;

    switch (page) {
    case BPLIST_PAGE_PARTY:
        for (i = 0; i < 6; i++) {
            state = BPlistMain_CheckPos(work, i);
            if (state == BPLIST_POS_EMPTY) {
                BPlistAnm_PutButton(work, i, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_RESERVE);
            } else if (state == BPLIST_POS_IN_BATTLE) {
                BPlistAnm_PutButton(work, i, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
            } else if (state == BPLIST_POS_RESERVE) {
                BPlistAnm_PutButton(work, i, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_RESERVE);
            }
        }
        if (work->param->mode == BPLIST_MODE_MID_TURN || work->param->mode == BPLIST_MODE_FAINTED) {
            BPlistAnm_PutReturnButton(work);
        } else {
            BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        }
        break;
    case BPLIST_PAGE_SWITCH_POSITION:
        for (i = 0; i < 6; i++) {
            state = BPlistMain_CheckPos(work, i);
            if (state == BPLIST_POS_EMPTY) {
                BPlistAnm_PutButton(work, i, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_RESERVE);
            } else if (state == BPLIST_POS_IN_BATTLE) {
                BPlistAnm_PutButton(work, i, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
            } else if (state == BPLIST_POS_RESERVE) {
                BPlistAnm_PutButton(work, i, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_RESERVE);
            }
        }
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        break;
    case BPLIST_PAGE_SELECTED:
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_SWITCH, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].isEgg) {
            BPlistAnm_PutButton(work, BPLIST_BUTTON_SELECTED_SUMMARY, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_IN_BATTLE);
            BPlistAnm_PutButton(work, BPLIST_BUTTON_SELECTED_MOVES, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_IN_BATTLE);
        } else {
            BPlistAnm_PutButton(work, BPLIST_BUTTON_SELECTED_SUMMARY, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
            BPlistAnm_PutButton(work, BPLIST_BUTTON_SELECTED_MOVES, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        }
        break;
    case BPLIST_PAGE_SUMMARY:
        if (BPlistAnm_CanSwitch(work) == TRUE) {
            BPlistAnm_PutButton(work, BPLIST_BUTTON_PREV_POKEMON, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
            BPlistAnm_PutButton(work, BPLIST_BUTTON_NEXT_POKEMON, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        } else {
            BPlistAnm_PutButton(work, BPLIST_BUTTON_PREV_POKEMON, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_IN_BATTLE);
            BPlistAnm_PutButton(work, BPLIST_BUTTON_NEXT_POKEMON, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_IN_BATTLE);
        }
        BPlistAnm_PutButton(work, BPLIST_BUTTON_SUMMARY_MOVES, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        break;
    case BPLIST_PAGE_MOVES:
        if (BPlistAnm_CanSwitch(work) == TRUE) {
            BPlistAnm_PutButton(work, BPLIST_BUTTON_PREV_POKEMON, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
            BPlistAnm_PutButton(work, BPLIST_BUTTON_NEXT_POKEMON, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        } else {
            BPlistAnm_PutButton(work, BPLIST_BUTTON_PREV_POKEMON, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_IN_BATTLE);
            BPlistAnm_PutButton(work, BPLIST_BUTTON_NEXT_POKEMON, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_IN_BATTLE);
        }
        for (i = 0; i < 4; i++) {
            if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[i].move != 0) {
                BPlistAnm_PutButton(work, i + BPLIST_BUTTON_MOVE_INFO_0, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
            } else {
                BPlistAnm_PutButton(work, i + BPLIST_BUTTON_MOVE_INFO_0, BPLIST_BUTTON_DISABLED,
                                    BPLIST_PLATE_IN_BATTLE);
            }
        }
        BPlistAnm_PutButton(work, BPLIST_BUTTON_MOVES_SUMMARY, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        break;
    case BPLIST_PAGE_MOVE_INFO:
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        for (i = 0; i < 4; i++) {
            if (work->param->slot == i) {
                BPlistAnm_PutButton(work, i + BPLIST_BUTTON_MOVE_TAB_0, BPLIST_BUTTON_SELECTED, BPLIST_PLATE_IN_BATTLE);
            } else {
                BPlistAnm_PutButton(work, i + BPLIST_BUTTON_MOVE_TAB_0, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
            }
        }
        break;
    case BPLIST_PAGE_PP_RESTORE:
        for (i = 0; i < 4; i++) {
            if (work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[i].move != 0) {
                BPlistAnm_PutButton(work, i + BPLIST_BUTTON_PP_RESTORE_0, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
            } else {
                BPlistAnm_PutButton(work, i + BPLIST_BUTTON_PP_RESTORE_0, BPLIST_BUTTON_DISABLED,
                                    BPLIST_PLATE_IN_BATTLE);
            }
        }
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        break;
    case BPLIST_PAGE_FORGET_MOVE:
        BPlistAnm_PutButton(work, BPLIST_BUTTON_FORGET_0, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_FORGET_1, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_FORGET_2, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_FORGET_3, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_FORGET_NEW_MOVE, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        break;
    case BPLIST_PAGE_CONFIRM_FORGET:
        BPlistAnm_PutButton(work, BPLIST_BUTTON_CONFIRM_FORGET, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
        break;
    }
}

// The caller passes the page, which this does not use
void BPlistAnm_RestorePalette(BPlistWork *work, u8 page) {
    PaletteFade_LoadData(work->paletteFade, work->savedPalette, PALFADE_BUFFER_SUB_BG, 0xc0,
                         sizeof(work->savedPalette));
}

// Whether at least two of the party can fight
static u8 BPlistAnm_CanSwitch(BPlistWork *work) {
    u16 i;
    u16 count = 0;

    for (i = 0; i < 6; i++) {
        if (work->pokemon[i].species != 0 && !work->pokemon[i].isEgg) {
            count++;
        }
    }
    return count >= 2;
}

void BPlistAnm_PutReturnButton(BPlistWork *work) {
    u8 pos1, pos2;

    if (BPlistMain_PopSwap(work, &pos1, &pos2, FALSE) == TRUE) {
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_NORMAL, BPLIST_PLATE_IN_BATTLE);
    } else {
        BPlistAnm_PutButton(work, BPLIST_BUTTON_BACK, BPLIST_BUTTON_DISABLED, BPLIST_PLATE_IN_BATTLE);
    }
}
