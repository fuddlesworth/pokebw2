#ifndef POKEBW2_BATTLE_BTLV_EFFVM_H
#define POKEBW2_BATTLE_BTLV_EFFVM_H

// Overlay 168's btlv_effvm.c (named by its string), the battle effect VM: it runs the move effect scripts, whose
// commands (swan's MOVE_SCRCMD) drive the camera, the Pokémon, the cell actors, the particles, the screen effects and
// the sounds of each move's animation. BtlvEffect_QueueCommands is swan's name; the others are ours

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/vm.h"

// What btlv_effect.c tells BtlvEffvm_Start of the effect, 0x10 bytes; BtlvEffvmParam_Clear clears it. The scripts
// read the fields as their variables 9 to 15
typedef struct {
    u8 target;    // 0x0  the move's MOVE_TARGET_*
    u8 variant;   // 0x1  which of the script's variants runs, the turn of a two-turn move
    u8 unk2;      // 0x2
    u8 shakes;    // 0x3  a thrown ball's, from SC_BALL_THROW
    u32 caught;   // 0x4  the same
    u32 critical; // 0x8  the same: a critical capture
    u16 itemNo;   // 0xc  the ball the ball effects show
} BtlvEffvmParam;

VM *BtlvEffvm_Create(TCBManager *tcbManager, HeapID heapId);
BOOL BtlvEffvm_Main(VM *vm);
void BtlvEffvm_Delete(VM *vm);
void BtlvEffvm_Start(VM *vm, u32 attacker, u32 defender, u16 move, BtlvEffvmParam *param);
void BtlvEffvm_Stop(VM *vm);
void BtlvEffvm_Resume(VM *vm);
s32 BtlvEffvm_GetScriptKind(VM *vm);
void BtlvEffvmParam_Clear(BtlvEffvmParam *param);
// PlaySE's arguments; the modulation's depth and speed are capped at 255
void BtlvEffvm_PlaySEAt(VM *vm, u32 se, u32 player, u32 pan, int pitch, int volume, int modDepth, int modSpeed,
                        int wait);
void BtlvEffvm_SlideSE(VM *vm, u32 player, u32 type, u32 param, int start, int end, int delay, int frames, int stepWait,
                       int count);
void BtlvEffvm_ReleaseVoices(VM *vm);
void BtlvEffvm_ClearLastEffect(VM *vm);
// A script command, global in the original: starts the task that opens or closes window 0 step by step
BOOL BtlvEffect_QueueCommands(VM *vm, void *env);

// Script value domains, matching include/asm/btlv_effvm.inc; storage and script encodings stay unchanged.
enum {
    EFFVM_ATTACKER = 14,
    EFFVM_ATTACKER_PARTNER = 15,
    EFFVM_DEFENDER = 16,
    EFFVM_DEFENDER_PARTNER = 17,
    EFFVM_ALL = 18,
    EFFVM_PLAYER_SIDE = 19,
    EFFVM_ENEMY_SIDE = 20,
    EFFVM_EMIT_ATTACKER = 9,
    EFFVM_EMIT_ATTACKER_MIRRORED = 10,
    EFFVM_EMIT_DEFENDER = 11,
    EFFVM_EMIT_DEFENDER_MIRRORED = 12,
    EFFVM_PAN_LEFT = 0,
    EFFVM_PAN_RIGHT = 1,
    EFFVM_PAN_CENTER = 2,
    EFFVM_WAIT_ALL = 0,
    EFFVM_WAIT_CAMERA = 1,
    EFFVM_WAIT_PARTICLES = 2,
    EFFVM_WAIT_MCSS = 3,
    EFFVM_WAIT_TRAINER_ANIM = 4,
    EFFVM_WAIT_BG = 5,
    EFFVM_WAIT_STAGE_FADE = 6,
    EFFVM_WAIT_FIELD_FADE = 7,
    EFFVM_WAIT_STAGE_FIELD_FADE = 8,
    EFFVM_WAIT_PALETTE_FADE = 9,
    EFFVM_WAIT_SE = 10,
    EFFVM_WAIT_SE_PLAYER_1 = 11,
    EFFVM_WAIT_SE_PLAYER_2 = 12,
    EFFVM_WAIT_SE_PLAYER_4 = 13,
    EFFVM_WAIT_SE_PLAYER_3 = 14,
    EFFVM_WAIT_SE_PLAYER_0 = 15,
    EFFVM_WAIT_CRY = 16,
    EFFVM_WAIT_WINDOW = 17,
    EFFVM_WAIT_PAL_ANIM = 18,
    EFFVM_WAIT_BG_WAVE = 19,
    EFFVM_CMP_EQ = 0,
    EFFVM_CMP_NE = 1,
    EFFVM_CMP_LT = 2,
    EFFVM_CMP_GT = 3,
    EFFVM_CMP_LE = 4,
    EFFVM_CMP_GE = 5,
    EFFVM_VAR_WEIGHT = 0,
    EFFVM_VAR_ATTACKER_WEIGHT = 8,
    EFFVM_VAR_TARGET = 9,
    EFFVM_VAR_VARIANT = 10,
    EFFVM_VAR_PARAM_2 = 11,
    EFFVM_VAR_SHAKES = 12,
    EFFVM_VAR_CAUGHT = 13,
    EFFVM_VAR_CRITICAL = 14,
    EFFVM_VAR_ITEM = 15,
    EFFVM_VAR_VALUE = 16,
    EFFVM_VAR_ATTACKER = 17,
    EFFVM_VAR_ATTACKER_VANISH = 18,
    EFFVM_VAR_ATTACKER_SIDE = 19,
    EFFVM_VAR_RARE = 20,
    EFFVM_VAR_ATTACKER_RARE = 28,
    EFFVM_VAR_NO_BOUNCE = 29,
    EFFVM_VAR_ATTACKER_NO_BOUNCE = 37,
    EFFVM_VAR_IS_MULTI = 38,
    EFFVM_VAR_BATTLE_STYLE = 39,
    EFFVM_VAR_TRAINER_TYPE = 40,
    EFFVM_VAR_MCSS_SCALED = 53,
    EFFVM_VAR_CAMERA_SAVED = 54,
    EFFVM_VAR_CAMERA_LOCKED = 56,
    EFFVM_VAR_DEFENDER = 57,
    EFFVM_VAR_N_POKEMON = 58,
    EFFVM_VAR_ATTACKER_N_POKEMON = 66,
    EFFVM_VAR_FAMOUS = 67,
    EFFVM_VAR_ATTACKER_FAMOUS = 75,
};

// Additional C variable meanings: per-position stop-cell flags and whether the message-idle view needs restoration.
enum {
    EFFVM_VAR_STOP_CELL_FLAGS = 44,
    EFFVM_VAR_ATTACKER_STOP_CELL_FLAGS = 52,
    EFFVM_VAR_MESSAGE_IDLE_RESTORE = 55,
};

// Emitter endpoint codes: explicit vectors, screen-space attacker, or double-table positions with optional depth
// offsets.
enum {
    EFFVM_EMIT_EXPLICIT = 8,
    EFFVM_EMIT_SCREEN_ATTACKER = 13,
    EFFVM_EMIT_DOUBLE_PLAYER_1 = 14,
    EFFVM_EMIT_DOUBLE_ENEMY_1 = 15,
    EFFVM_EMIT_DOUBLE_PLAYER_2 = 16,
    EFFVM_EMIT_DOUBLE_ENEMY_2 = 17,
    EFFVM_EMIT_DOUBLE_PLAYER_1_OFFSET = 18,
    EFFVM_EMIT_DOUBLE_ENEMY_1_OFFSET = 19,
    EFFVM_EMIT_DOUBLE_PLAYER_2_OFFSET = 20,
    EFFVM_EMIT_DOUBLE_ENEMY_2_OFFSET = 21,
};

// CameraMovePreset selectors; the ALT pairs have the same resolution in this interpreter.
enum {
    EFFVM_CAMERA_SINGLE_PLAYER = 0,
    EFFVM_CAMERA_SINGLE_ENEMY = 1,
    EFFVM_CAMERA_PLAYER_1 = 2,
    EFFVM_CAMERA_ENEMY_1 = 3,
    EFFVM_CAMERA_PLAYER_2 = 4,
    EFFVM_CAMERA_ENEMY_2 = 5,
    EFFVM_CAMERA_PLAYER_3 = 6,
    EFFVM_CAMERA_ENEMY_3 = 7,
    EFFVM_CAMERA_DEFAULT = 8,
    EFFVM_CAMERA_ATTACKER = 9,
    EFFVM_CAMERA_ATTACKER_ALT = 10,
    EFFVM_CAMERA_DEFENDER = 11,
    EFFVM_CAMERA_DEFENDER_ALT = 12,
    EFFVM_CAMERA_SAVED = 13,
    EFFVM_CAMERA_WIDE = 14,
    EFFVM_CAMERA_ATTACKER_SLOT = 15,
    EFFVM_CAMERA_DEFENDER_SLOT = 16,
    EFFVM_CAMERA_DEFAULT_ALT = 17,
    EFFVM_CAMERA_TRIPLE_ENEMY_1 = 18,
    EFFVM_CAMERA_ROTATION_BENCH = 19,
    EFFVM_CAMERA_WIDE_OR_FLAT = 20,
    EFFVM_CAMERA_ATTACKER_SIDE = 21,
};

// Camera commands set a position at once, move to it, or move by an offset.
enum {
    EFFVM_CAMERA_MOVE_SET = 0,
    EFFVM_CAMERA_MOVE_TO = 1,
    EFFVM_CAMERA_MOVE_BY = 2,
};

// Emitter paths: flat/arced movement, progressively delayed arc, flat path starting from the target or local-axis
// waves.
enum {
    EFFVM_EMITTER_STILL = 0,
    EFFVM_EMITTER_FLAT = 1,
    EFFVM_EMITTER_ARC = 2,
    EFFVM_EMITTER_ARC_DELAYED = 3,
    EFFVM_EMITTER_FLAT_FROM_TARGET = 4,
    EFFVM_EMITTER_WAVE_Y = 5,
    EFFVM_EMITTER_WAVE_Z = 6,
};

// Emitter coordinates: world, project both with side-dependent scaling, project both, or project only the start.
enum {
    EFFVM_EMITTER_WORLD = 0,
    EFFVM_EMITTER_PROJECT_MIRROR = 1,
    EFFVM_EMITTER_PROJECT = 2,
    EFFVM_EMITTER_PROJECT_START = 3,
};

// ResetMcss flattens sprites except for the depth mode.
enum {
    EFFVM_MCSS_CAMERA_FLAT = 0,
    EFFVM_MCSS_CAMERA_DEPTH = 1,
};

// PlaySE resolves the sequence player automatically; StopSE has its own stop-all selector.
enum {
    EFFVM_SE_PLAYER_AUTO = 5,
    EFFVM_SE_STOP_ALL = 5,
};

// Sound parameter automation runs once or swings between endpoints.
enum {
    EFFVM_SE_MOVE_ONCE = 0,
    EFFVM_SE_MOVE_SWING = 1,
};

// Which sound property SlideSE automates.
enum {
    EFFVM_SE_PARAM_PITCH = 0,
    EFFVM_SE_PARAM_VOLUME = 1,
    EFFVM_SE_PARAM_PAN = 2,
};

// Gauge modes0/1 hide/show; mode2 moves offscreen, other modes restore the gauge offset.
enum {
    EFFVM_GAUGE_HIDE = 0,
    EFFVM_GAUGE_SHOW = 1,
    EFFVM_GAUGE_OFFSCREEN = 2,
};

// Gauge targets use their own side/attacker/defender numbering.
enum {
    EFFVM_GAUGE_PLAYER = 0,
    EFFVM_GAUGE_ENEMY = 1,
    EFFVM_GAUGE_BOTH = 2,
    EFFVM_GAUGE_ATTACKER = 3,
    EFFVM_GAUGE_DEFENDER = 4,
};

// The ball source is a Pokemon position, the used item or the attacker.
enum {
    EFFVM_BALL_SOURCE_ITEM = 8,
    EFFVM_BALL_SOURCE_ATTACKER = 9,
};

// Particle-system slots; FindParticleId returns this count when no slot matches.
#define EFFVM_PARTICLE_CAPACITY 16

// Window directions change its packed top/bottom or left/right bounds.
enum {
    EFFVM_WINDOW_EXPAND_Y = 0,
    EFFVM_WINDOW_CONTRACT_Y = 1,
    EFFVM_WINDOW_EXPAND_X = 2,
    EFFVM_WINDOW_CONTRACT_X = 3,
};

// Orbit center and angle direction (forward increases the angle; reverse decreases it).
enum {
    EFFVM_ORBIT_ATTACKER_FORWARD = 0,
    EFFVM_ORBIT_ATTACKER_REVERSE = 1,
    EFFVM_ORBIT_DEFENDER_FORWARD = 2,
    EFFVM_ORBIT_DEFENDER_REVERSE = 3,
    EFFVM_ORBIT_ORIGIN_FORWARD = 4,
    EFFVM_ORBIT_ORIGIN_REVERSE = 5,
};

// Cry handle slots; the first slot is reused once every slot is busy.
#define EFFVM_VOICE_CAPACITY 6

// CmdMcssSetMode handles this option itself instead of passing it to MCSS.
enum {
    EFFVM_MCSS_VANISH_RESTORE_ALL = 5,
};

#endif // POKEBW2_BATTLE_BTLV_EFFVM_H
