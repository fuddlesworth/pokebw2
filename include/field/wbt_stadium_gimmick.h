#ifndef POKEBW2_FIELD_WBT_STADIUM_GIMMICK_H
#define POKEBW2_FIELD_WBT_STADIUM_GIMMICK_H

#include "types.h"
#include "gfl/str.h"
#include "struct_decls.h"

// wbt_stadium_gimmick.c, overlay 134, named after what it does (the ROM names only the overlay's other file,
// wbt_battle.c): the field gimmick of the Pokémon World Tournament's stadium (zone 193), which script plugin 7 drives.
// Scene 0 holds the stadium's models and scene 2 the portraits of the fighters

// Whose names and portraits are shown: both Trainers', the player's alone or the opponent's alone
enum {
    WBT_STADIUM_NAMES_BOTH,
    WBT_STADIUM_NAMES_PLAYER,
    WBT_STADIUM_NAMES_OPPONENT,
};

// The crowd animation groups, named by the actors they start.
enum {
    WBT_STADIUM_CROWD_ACTOR11 = 0,
    WBT_STADIUM_CROWD_ACTOR12 = 1,
    WBT_STADIUM_CROWD_ACTORS11_12_13 = 2,
    WBT_STADIUM_CROWD_ACTOR14 = 3,
};

// The light actor pairs are 7/9 and 8/10; selection zero shows the second actor.
enum {
    WBT_STADIUM_LIGHT_PAIR_ODD = 0,
    WBT_STADIUM_LIGHT_PAIR_EVEN = 1,
};

enum {
    WBT_STADIUM_LIGHT_SECOND = 0,
    WBT_STADIUM_LIGHT_FIRST = 1,
};

// Position 20 uses the separate single-portrait scene.
enum {
    WBT_STADIUM_PORTRAIT_SINGLE_POSITION = 20,
    WBT_STADIUM_PORTRAIT_POSITION_COUNT = 21,
};

// Overlay 36's gimmick table calls the first three: setup, end and update of the gimmick
void WbtStadiumGimmick_Setup(Field *field);
void WbtStadiumGimmick_End(Field *field);
void WbtStadiumGimmick_Update(Field *field);
// Writes the names on the name plates, by a WBT_STADIUM_NAMES_*, and the files of the portraits
void WbtStadiumGimmick_SetNames(Field *field, u8 mode, StrBuf *name, StrBuf *playerName, u16 file, u16 playerFile);
// Draws the portraits and starts the screens' animations
void WbtStadiumGimmick_ShowPortraits(Field *field, u8 mode);
// Loads scene 2 with the portrait of a position (20 for a lone one) on a side
void WbtStadiumGimmick_LoadPortraitScene(Field *field, u8 position, u8 side);
// Plays the intro of actor 5
void WbtStadiumGimmick_PlayIntro(Field *field);
// Switches between the lights of actors 7 to 10
void WbtStadiumGimmick_SwitchLights(Field *field, u8 which, u8 pair);
// Plays the animations of actors 11 to 14
void WbtStadiumGimmick_PlayCrowd(Field *field, u8 which);

#endif // POKEBW2_FIELD_WBT_STADIUM_GIMMICK_H
