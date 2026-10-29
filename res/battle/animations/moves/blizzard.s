// MOVE_BLIZZARD's animation: member 59 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Blizzard

Blizzard:
    CameraMovePreset 1, 11, 16, 0, 12
    CallDfc70 2, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    CallDfc70 3, 16, 0, 1, 0
    ParticleLoad 224
    McssSetVanish EFFVM_DEFENDER, TRUE
    LoadBg 30
    CallDfd74 0, 1
    CallDfd74 1, 1
    CallDfd74 2, 0
    BgMove 4, -16, 0, 120, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_DEFENDER, 0, 200, 70, 127, 127, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 50, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W319_01, 1, EFFVM_DEFENDER, 70, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_DEFENDER, 0, 100, 0, 0
    PlaySE SEQ_SE_W059_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_04R, 0, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    MoveSE 2, 0, 0, 0, 1000, 0, 20, 0, 0
    MoveSE 2, 0, 0, 1000, 0, 40, 30, 0, 0
    MoveSE 2, 0, 1, 0, 120, 0, 30, 0, 0
    MoveSE 2, 0, 1, 120, 0, 40, 40, 0, 0
    SetYield TRUE
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, Blizzard_04B4
    EmitterAdd 224, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 224, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 224, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 224, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    WaitFrames 10
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, Blizzard_06C8

Blizzard_04B4:
    EmitterAdd 224, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 224, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 224, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 224, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 224, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    WaitFrames 10

Blizzard_06C8:
    WaitFrames 20
    CallDfc70 3, 0, 16, 0, 0
    WaitFrames 2
    CallDfc70 2, 16, 0, 0, 0
    CallDfd74 0, 0
    CallDfd74 1, 0
    CallDfd74 2, 1
    Wait EFFVM_WAIT_PARTICLES
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
