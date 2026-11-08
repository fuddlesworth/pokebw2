#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_FADE_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_FADE_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Battle Recorder's fades between its screens (br_fade.c): by the master brightness, by alpha blending, or to its
// color with the palettes

// The screens a fade applies to
#define BR_FADE_DISPLAY_MAIN 0x1
#define BR_FADE_DISPLAY_SUB 0x2
#define BR_FADE_DISPLAY_BOTH (BR_FADE_DISPLAY_MAIN | BR_FADE_DISPLAY_SUB)

// How a fade is done
enum {
    // The master brightness, to black or to white
    BR_FADE_TYPE_MASTER_BRIGHT_BLACK,
    BR_FADE_TYPE_MASTER_BRIGHT_WHITE,
    BR_FADE_TYPE_ALPHA_BG012,
    // The palettes, to the color. A display of 0xf fades sub BG palettes 12 and 13 to white instead
    BR_FADE_TYPE_PLTT,
    // To white on the main screen and by alpha on the sub screen
    BR_FADE_TYPE_MASTER_BRIGHT_AND_ALPHA,
};

// Whether a fade shows the screens or hides them
enum {
    BR_FADE_DIR_IN,
    BR_FADE_DIR_OUT,
};

BrFade *BrFade_Init(HeapID heapId);
void BrFade_Exit(BrFade *p_wk);
void BrFade_Main(BrFade *p_wk);
void BrFade_StartFade(BrFade *p_wk, u32 type, u32 display, u32 dir);
// sync is the master brightness's frames per step, or the steps of the alpha and palette fades, 16 / the fade
// update frequency if 0
void BrFade_StartFadeEx(BrFade *p_wk, u32 type, u32 display, u32 dir, u32 sync);
BOOL BrFade_IsEnd(const BrFade *cp_wk);
// Copies the palettes in VRAM to the fade's buffers
void BrFade_LoadPltt(BrFade *p_wk);
void BrFade_SetColor(BrFade *p_wk, u16 color);
// Sets the screens' palettes to the color
void BrFade_FillColor(BrFade *p_wk, u32 display);
// Sets the alpha of the screens' BGs, hiding them at 0
void BrFade_SetAlpha(BrFade *p_wk, u32 display, u32 ev);
// Loads a palette into one of the fade's PaletteFade buffers
void BrFade_LoadPlttArc(BrFade *p_wk, ArcTool *handle, u32 fileId, u32 buffer, u32 offset, u32 size, HeapID heapId);

// PLTT-only override: sub BG palettes 12 and 13 fade to white.
#define BR_FADE_PLTT_SUB_BG12_13_WHITE 0xf

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_FADE_H
