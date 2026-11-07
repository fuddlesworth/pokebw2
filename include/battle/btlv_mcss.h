#ifndef POKEBW2_BATTLE_BTLV_MCSS_H
#define POKEBW2_BATTLE_BTLV_MCSS_H

// Overlay 168's btlv_mcss.c (named by its string), the battle view's Pokémon sprites: it wraps the MCSS sprite system,
// loading, placing, scaling, animating and recoloring each battler's sprite, and moves them with tasks. The names are
// ours; swan has none for this file

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "struct_decls.h"
#include "system/mcss.h"

// A sprite's position: 0 and 1 the Pokémon of a single battle, 2 to 7 those of the other battles, even on the player's
// side, and from 8 on the trainers. An enum, so MWCC keeps the first test of the loops over the positions
typedef enum {
    BTLV_MCSS_POS_FIRST = 0,
    BTLV_MCSS_POS_SINGLE_PLAYER = 0, // the player's Pokémon of a single battle
    BTLV_MCSS_POS_SINGLE_ENEMY = 1,  // the opponent's
    BTLV_MCSS_POS_TRAINER = 8,
    BTLV_MCSS_POS_MAX = 14,
    // No position, as an effect's attacker or defender
    BTLV_MCSS_POS_NONE = 0xff,
} BtlvMcssPos;

// What BtlvMcss_GetFlags returns of a sprite
#define BTLV_MCSS_FLAG_SUBSTITUTE_ACTIVE (1 << 0) // substitute remains active even while its doll sprite is hidden
#define BTLV_MCSS_FLAG_RARE (1 << 1)              // shiny
#define BTLV_MCSS_FLAG_SUBSTITUTE (1 << 2)        // the sprite shows the substitute
#define BTLV_MCSS_FLAG_N_POKEMON (1 << 3)         // one of N's Pokémon
#define BTLV_MCSS_FLAG_FAMOUS (1 << 4)            // a Pokéstar fame of 4 or more

// A move of a sprite around a circle, which BtlvMcss_MoveCircle copies from pos on into its task. The alignment of
// work, here and in BtlvMcssShake, is a guess: it changes no layout, only how MWCC compiles accesses through the
// member, and with it the end callbacks load work before pos, as the original does (docs/matching.md, Loops:
// "Where a member's alignment is written")
typedef struct {
    ATTRIBUTE_ALIGN(4) BtlvMcss *work; // 0x00
    s32 pos;                           // 0x04
    s32 mode;                          // 0x08  bit 0: the other way round; bits 1-2: the plane, 0: y-z, 1: x-z, 2: x-y
    u32 start;                         // 0x0c  the circle point it starts from, 0-3; mirrored for the enemy side
    fx32 radius1;                      // 0x10
    fx32 radius2;                      // 0x14
    s32 frames;                        // 0x18  steps by turn
    s32 wait;                          // 0x1c  frames between steps
    s32 count;                         // 0x20  turns
    s32 turnWait;                      // 0x24  frames to wait after each turn
    s32 angle;                         // 0x28
    s32 speed;                         // 0x2c
    s32 waitCount;                     // 0x30
    s32 turnWaitLeft;                  // 0x34
    u32 seq;                           // 0x38  the position's move counter when the task started
} BtlvMcssCircle;

// A shake of a sprite along an axis by a sine, which BtlvMcss_Shake copies into its task
typedef struct {
    ATTRIBUTE_ALIGN(4) BtlvMcss *work; // 0x00
    s32 pos;                           // 0x04
    VecFx32 start;                     // 0x08  the position, read when the shake starts
    s32 axis;                          // 0x14  0: x, otherwise y
    fx32 angle;                        // 0x18  the sine's angle, a 16-bit angle in fx32
    fx32 speed;                        // 0x1c
    fx32 amplitude;                    // 0x20
    s32 frames;                        // 0x24
    u32 seq;                           // 0x28  the position's move counter when the task started
} BtlvMcssShake;

BtlvMcss *BtlvMcss_Create(u32 battleStyle, TCBManager *tcbMgr, HeapID heapId);
void BtlvMcss_Delete(BtlvMcss *work);
void BtlvMcss_Main(BtlvMcss *work);
void BtlvMcss_Draw(BtlvMcss *work);
void BtlvMcss_AddPokemon(BtlvMcss *work, PartyPkm *pkm, int pos);
void BtlvMcss_AddTrainer(BtlvMcss *work, u32 trainerType, int pos);
void BtlvMcss_Remove(BtlvMcss *work, int pos);
void BtlvMcss_SetSideScale(BtlvMcss *work, fx32 scaleNear, fx32 scaleFar);
void BtlvMcss_SetPosition(BtlvMcss *work, int pos, fx32 x, fx32 y, fx32 z);
void BtlvMcss_SetFlatAll(BtlvMcss *work);
void BtlvMcss_ClearFlatAll(BtlvMcss *work);
void BtlvMcss_SetFlat(BtlvMcss *work, int pos);
void BtlvMcss_ClearFlat(BtlvMcss *work, int pos);
void BtlvMcss_SetAnimPause(BtlvMcss *work, int pos, int mode);
BOOL BtlvMcss_GetVanish(BtlvMcss *work, int pos);
void BtlvMcss_SetVanish(BtlvMcss *work, int pos, int mode);
void BtlvMcss_GetDefaultPos(BtlvMcss *work, VecFx32 *out, int pos);
void BtlvMcss_GetDefaultPosForStyle(BtlvMcss *work, VecFx32 *out, int pos, u32 battleStyle);
fx32 BtlvMcss_GetDefaultScale(BtlvMcss *work, int pos, BOOL flat);
void BtlvMcss_SetShadows(BtlvMcss *work, BOOL shadows);
void BtlvMcss_SetShadowVanish(BtlvMcss *work, int pos, u8 value);
void BtlvMcss_SetAnimSpeed(BtlvMcss *work, int pos, fx32 speed);
void BtlvMcss_MovePosition(BtlvMcss *work, int pos, int type, VecFx32 *dest, s32 frames, s32 wait, s32 count);
void BtlvMcss_MoveScaleOffset(BtlvMcss *work, int pos, int type, VecFx32 *dest, s32 frames, s32 wait, s32 count);
void BtlvMcss_MoveScale(BtlvMcss *work, int pos, int type, VecFx32 *dest, s32 frames, s32 wait, s32 count);
void BtlvMcss_MoveShadowScaleOffset(BtlvMcss *work, int pos, int type, VecFx32 *dest, s32 frames, s32 wait, s32 count);
void BtlvMcss_MoveRotation(BtlvMcss *work, int pos, int type, VecFx32 *dest, s32 frames, s32 wait, s32 count);
void BtlvMcss_SwapPositions(BtlvMcss *work, int pos1, int pos2);
void BtlvMcss_Blink(BtlvMcss *work, int pos, int type, s32 wait, s32 count);
void BtlvMcss_MoveAlpha(BtlvMcss *work, int pos, int type, int alpha, s32 frames, s32 wait, s32 count);
void BtlvMcss_MoveCircle(BtlvMcss *work, BtlvMcssCircle *param);
void BtlvMcss_Shake(BtlvMcss *work, BtlvMcssShake *param);
void BtlvMcss_MoveLevel(BtlvMcss *work, int pos, int type, int level, s32 frames, s32 wait, s32 count);
BOOL BtlvMcss_IsBusy(BtlvMcss *work, int pos);
BOOL BtlvMcss_IsAnyBusy(BtlvMcss *work);
BOOL BtlvMcss_Exists(BtlvMcss *work, int pos);
void BtlvMcss_StartPaletteFade(BtlvMcss *work, int pos, u8 startEvy, u8 endEvy, u8 wait, u32 color);
void BtlvMcss_ChangePokemon(BtlvMcss *work, int pos, PartyPkm *pkm);
u16 BtlvMcss_GetWeight(BtlvMcss *work, int pos);
u32 BtlvMcss_GetFlags(BtlvMcss *work, int pos);
BOOL BtlvMcss_IsNoBounce(BtlvMcss *work, int pos);
u8 BtlvMcss_GetStopCellFlags(BtlvMcss *work, int pos);
void BtlvMcss_SetSubstitute(BtlvMcss *work, int pos, BOOL substitute, BOOL updateActiveFlag);
void BtlvMcss_Transform(BtlvMcss *work, int targetPos, int pos);
void BtlvMcss_ReloadSprite(BtlvMcss *work, int pos, const MCSSLoadInfo *info);
u32 BtlvMcss_PlayCry(BtlvMcss *work, int pos, int speed, int volume, int chorusVolume, int chorusSpeed, int reverse);
void BtlvMcss_StartRotation(BtlvMcss *work, int side, int dir, BOOL playSe);
u32 BtlvMcss_GetBall(BtlvMcss *work, int pos);
void BtlvMcss_SetupPokemonLoadInfo(BtlvMcss *work, PartyPkm *pkm, MCSSLoadInfo *info, int pos);
BOOL BtlvMcss_SetAnimation(BtlvMcss *work, int pos, int anim);
void BtlvMcss_WatchAnimationEnd(BtlvMcss *work, int pos);
BOOL BtlvMcss_IsAnimationSet(BtlvMcss *work, int pos);
void BtlvMcss_ResetPosition(BtlvMcss *work, int pos);
// byPos reverses the cells of the sprites at the positions of a double or triple battle that need it (MCSS_SetReverse),
// otherwise none
void BtlvMcss_SetReverse(BtlvMcss *work, int pos, BOOL byPos);
BOOL BtlvMcss_CheckChanged(BtlvMcss *work, int pos);
void BtlvMcss_SaveVanish(BtlvMcss *work);
void BtlvMcss_RestoreVanish(BtlvMcss *work);

// Slots of the non-single position tables and their trainers; numbering follows the table, not screen order.
enum {
    BTLV_MCSS_POS_PLAYER_1 = 2,
    BTLV_MCSS_POS_ENEMY_1 = 3,
    BTLV_MCSS_POS_PLAYER_2 = 4,
    BTLV_MCSS_POS_ENEMY_2 = 5,
    BTLV_MCSS_POS_PLAYER_3 = 6,
    BTLV_MCSS_POS_ENEMY_3 = 7,
    BTLV_MCSS_POS_TRAINER_PLAYER_1 = 8,
    BTLV_MCSS_POS_TRAINER_ENEMY_1 = 9,
    BTLV_MCSS_POS_TRAINER_PLAYER_2 = 10,
    BTLV_MCSS_POS_TRAINER_ENEMY_2 = 11,
    BTLV_MCSS_POS_TRAINER_PLAYER_3 = 12,
    BTLV_MCSS_POS_TRAINER_ENEMY_3 = 13,
};

// SetAnimPause resumes, pauses, freezes or unfreezes the sprite animation.
enum {
    BTLV_MCSS_PAUSE_RESUME = 0,
    BTLV_MCSS_PAUSE_PAUSE = 1,
    BTLV_MCSS_PAUSE_FREEZE = 2,
    BTLV_MCSS_PAUSE_UNFREEZE = 3,
};

// The private flipV-mode helper controls MCSS's flipV flag and its persistent lock.
enum {
    BTLV_MCSS_FLIP_V_CLEAR = 0,
    BTLV_MCSS_FLIP_V_SET = 1,
    BTLV_MCSS_FLIP_V_TOGGLE = 2,
    BTLV_MCSS_FLIP_V_LOCK = 3,
    BTLV_MCSS_FLIP_V_UNLOCK = 4,
};

// SetVanish visibility modes; SAVE_HIDE remembers a previously hidden sprite.
enum {
    BTLV_MCSS_VANISH_SHOW = 0,
    BTLV_MCSS_VANISH_HIDE = 1,
    BTLV_MCSS_VANISH_TOGGLE = 2,
    BTLV_MCSS_VANISH_SAVE_HIDE = 3,
    BTLV_MCSS_VANISH_RESTORE = 4,
};

// Blink toggles the flipV flag repeatedly or sets/clears it immediately.
enum {
    BTLV_MCSS_BLINK_REPEAT = 2,
    BTLV_MCSS_BLINK_SET = 3,
    BTLV_MCSS_BLINK_CLEAR = 4,
};

// MovePosition extensions: step to the default position or set it immediately.
enum {
    BTLV_MCSS_MOVE_RETURN = 5,
    BTLV_MCSS_MOVE_RESET = 6,
};

// The local ellipse center lies along positive/negative a or b from its initial point; a/b map to the chosen plane.
enum {
    BTLV_MCSS_CIRCLE_CENTER_POSITIVE_A = 0,
    BTLV_MCSS_CIRCLE_CENTER_NEGATIVE_A = 1,
    BTLV_MCSS_CIRCLE_CENTER_POSITIVE_B = 2,
    BTLV_MCSS_CIRCLE_CENTER_NEGATIVE_B = 3,
};

// The coordinate pair used after extracting the circle plane from mode bits1-2.
enum {
    BTLV_MCSS_CIRCLE_PLANE_ZY = 0,
    BTLV_MCSS_CIRCLE_PLANE_XZ = 1,
    BTLV_MCSS_CIRCLE_PLANE_XY = 2,
};

// Circle mode combines its coordinate plane with the angle direction in bit0.
enum {
    BTLV_MCSS_CIRCLE_ZY_FORWARD = 0,
    BTLV_MCSS_CIRCLE_ZY_REVERSE = 1,
    BTLV_MCSS_CIRCLE_XZ_FORWARD = 2,
    BTLV_MCSS_CIRCLE_XZ_REVERSE = 3,
    BTLV_MCSS_CIRCLE_XY_FORWARD = 4,
    BTLV_MCSS_CIRCLE_XY_REVERSE = 5,
};

// Rotation direction-table indices; these differ from StartRotation selection modes.
enum {
    BTLV_MCSS_ROTATE_THIRD = 0,
    BTLV_MCSS_ROTATE_SECOND = 1,
};

#endif // POKEBW2_BATTLE_BTLV_MCSS_H
