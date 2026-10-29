// MOVE_INFERNO's animation: member 517 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Inferno

Inferno:
    LoadBg 201
    CallDfd74 2, 0
    CallDfc70 4, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    CallDfd74 0, 1
    CallDfd74 1, 1
    CallDfc70 3, 16, 0, 1, 0
    BgMove 4, 0, 4, 180, 0, 0
    BgWave 0, 0x2000, 16, 160, 0, 0
    McssSetValue EFFVM_ALL, 1
    ParticleLoad 687
    EmitterAdd 687, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W464_02, 1, EFFVM_DEFENDER, 0, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 2, EFFVM_DEFENDER, 25, -200, 100, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 1, EFFVM_DEFENDER, 50, 100, 90, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 2, EFFVM_DEFENDER, 75, -400, 127, 0, 0, 0
    PlaySE SEQ_SE_W464_02, 1, EFFVM_DEFENDER, 100, -800, 100, 0, 0, 0
    PlaySE SEQ_SE_W077_03, 2, EFFVM_DEFENDER, 125, -500, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 4, 2, 6
    EmitterAdd 687, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 16
    EmitterAdd 687, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 10, 1, 0x1f
    WaitFrames 16
    EmitterAdd 687, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    McssPalFade EFFVM_DEFENDER, 10, 0, 1, 0x1f
    WaitFrames 16
    McssPalFade EFFVM_DEFENDER, 0, 10, 1, 0x1c
    EmitterAdd 687, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    EmitterAdd 687, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CallDfc70 3, 0, 16, 1, 0
    Wait EFFVM_WAIT_PARTICLES
    CallDfd74 0, 0
    CallDfd74 1, 0
    CallDfc70 4, 16, 0, 1, 0
    McssPalFade EFFVM_DEFENDER, 10, 0, 1, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
