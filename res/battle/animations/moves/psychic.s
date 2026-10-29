// MOVE_PSYCHIC's animation: member 94 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Psychic

Psychic:
    CameraMovePreset 1, 11, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    CallDfc70 4, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 48
    CallDfc70 3, 16, 0, 1, 0
    BgMove 1, 0, 40, 40, 1, 0
    BgWave 1, 0x1800, 16, 60, 0, 0
    CallDfd74 2, 0
    CallDfd74 1, 1
    CallDfd74 0, 1
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 2, EFFVM_DEFENDER, 0, -200, 80, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 1, EFFVM_DEFENDER, 40, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W050_01, 2, EFFVM_DEFENDER, 40, 200, 100, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, 0x333, 0x333, 4, 1, 1
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x7fff
    WaitFrames 20
    CallDfc70 3, 0, 16, 2, 0
    WaitFrames 10
    CallDfc70 4, 16, 0, 2, 0
    CallDfd74 1, 0
    CallDfd74 0, 0
    CallDfd74 2, 1
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    CallDfc70 3, 16, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
