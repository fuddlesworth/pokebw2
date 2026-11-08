#ifndef POKEBW2_APP_MUSICAL_MUSICAL_MCSS_H
#define POKEBW2_APP_MUSICAL_MUSICAL_MCSS_H

// Overlay 209's musical_mcss.c: the musical's own copy of MCSS (system/mcss.h), which draws the Pokémon's multi-cell
// sprites as 3D quads and reports the cells that mark where props go. Its sprites' files come from the older layout
// of the Pokémon graphics archive, which its copy of pokegra.c's file functions computes

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/mcss.h"

// The marker cells: two of kinds 4 and 5, and one for each prop position from kind 7
enum {
    MUS_POKE_DRAW_CELL_MARK4 = 4,
    MUS_POKE_DRAW_CELL_MARK5,
    MUS_POKE_DRAW_CELL_EQUIP = 7,
};

// Where a marker cell of a sprite was drawn this frame, as a draw callback gets it
typedef struct {
    // The sprite's position, y up
    VecFx32 pos;
    // The cell's offset from it, scaled, and its depth
    VecFx32 offset;
    VecFx32 scale;
    // The sprite's center of rotation
    VecFx32 center;
    u16 rotation;
    // The cell's rotation, its own plus the multi-cell's
    u16 cellRotation;
} MusicalMcssCellInfo;

// Called for each marker cell of a sprite as it is drawn, with the cell's kind and the work given to
// MusicalMcss_Add
typedef void (*MusicalMcssCellCallback)(u32 kind, MusicalMcssCellInfo *info, void *work);

MusicalMcssSys *MusicalMcss_InitSystem(u32 count, HeapID heapId);
void MusicalMcss_TermSystem(MusicalMcssSys *sys);
void MusicalMcss_UpdateSystem(MusicalMcssSys *sys);
void MusicalMcss_DrawSystem(MusicalMcssSys *sys, MusicalMcssCellCallback callback);
MusicalMcss *MusicalMcss_Add(MusicalMcssSys *sys, fx32 x, fx32 y, fx32 z, MCSSLoadInfo *info, void *work,
                             BOOL loadAtVBlank);
void MusicalMcss_Del(MusicalMcssSys *sys, MusicalMcss *mcss);
// Draws the sprites with an orthographic projection of their own
void MusicalMcss_SetOrthoMode(MusicalMcssSys *sys);
// Where the sprites' characters and palettes go in texture VRAM
void MusicalMcss_SetTexBase(MusicalMcssSys *sys, u32 base);
void MusicalMcss_SetPlttBase(MusicalMcssSys *sys, u32 base);
void MusicalMcss_SetPosition(MusicalMcss *mcss, VecFx32 *pos);
void MusicalMcss_SetScale(MusicalMcss *mcss, VecFx32 *scale);
void MusicalMcss_SetRotation(MusicalMcss *mcss, u16 rotation);
void MusicalMcss_SetFlip(MusicalMcss *mcss);
void MusicalMcss_ResetFlip(MusicalMcss *mcss);
void MusicalMcss_StopAnime(MusicalMcss *mcss);
void MusicalMcss_StartAnime(MusicalMcss *mcss);
void MusicalMcss_ChangeAnime(MusicalMcss *mcss, u16 anime);
BOOL MusicalMcss_IsHidden(MusicalMcss *mcss);
void MusicalMcss_Hide(MusicalMcss *mcss);
void MusicalMcss_Show(MusicalMcss *mcss);
// Gives dst src's position, scale, flags and the rest of its placing
void MusicalMcss_CopyState(MusicalMcss *src, MusicalMcss *dst);

// The files of a Pokémon's sprite in the musical's archive, as pokegra.c's GetPokemon*DataNo
u32 MusicalMcss_GetCharacterDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 MusicalMcss_GetPaletteDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 MusicalMcss_GetCellsDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 MusicalMcss_GetCellAnimeDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 MusicalMcss_GetMultiCellsDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 MusicalMcss_GetMultiCellAnimeDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 MusicalMcss_GetBinFileDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);

#endif // POKEBW2_APP_MUSICAL_MUSICAL_MCSS_H
