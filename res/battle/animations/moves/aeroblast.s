// MOVE_AEROBLAST's animation: member 177 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Aeroblast

Aeroblast:
    CameraMovePreset 1, 9, 16, 0, 12
    CallDfc70 2, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 55, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 50, 0, 0
    SetYield TRUE
    ParticleLoad 348
    EmitterAdd 348, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 348, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    CameraMovePreset 1, 11, 32, 0, 22
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W307_01, 1, EFFVM_ATTACKER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 80, 0, 0
    MoveSE 1, 0, 1, 90, 0, 60, 20, 0, 0
    MoveSE 2, 0, 1, 115, 0, 60, 20, 0, 0
    MoveSE 2, 0, 0, -1000, 1000, 0, 80, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    LoadBg 78
    CallDfd74 2, 0
    CallDfc70 3, 0, 16, 1, 0
    WaitFrames 8
    CallDfd74 0, 1
    CallDfd74 1, 1
    WaitFrames 2
    CallDfc70 3, 16, 0, 1, 0
    WaitFrames 4
    BgPalAnim 8, 9
    WaitFrames 50
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0x7fff
    EmitterAdd 348, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 0, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 0, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 0, EFFVM_DEFENDER, 20, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 0, EFFVM_DEFENDER, 30, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x333, 2, 1, 8
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 8
    WaitFrames 40
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0x7fff
    CallDfc70 3, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    CallDfd74 1, 0
    CallDfd74 0, 0
    CallDfc70 2, 16, 0, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    CallDfc70 3, 16, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
