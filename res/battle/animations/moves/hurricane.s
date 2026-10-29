// MOVE_HURRICANE's animation: member 542 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Hurricane

Hurricane:
    LoadBg 159
    CallDfd74 2, 0
    CallDfc70 4, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W542_BOUFU, 1, EFFVM_DEFENDER, 0, 0, 127, 70, 20, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 250, 0, 0, 100, 20, 0
    MoveSE 1, 0, 1, 127, 0, 150, 40, 0, 0
    PlaySE SEQ_SE_W239_01, 2, EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 0, 127, 0, 50, 0, 0
    MoveSE 2, 0, 1, 127, 0, 100, 100, 0, 0
    SetYield TRUE
    CallDfd74 0, 1
    CallDfd74 1, 1
    CallDfc70 3, 16, 0, 1, 0
    BgMove 4, 24, 0, 230, 0, 0
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    ParticleLoad 711
    OrbitEmitters 711, 2, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    OrbitEmitters 711, 4, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    OrbitEmitters 711, 1, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x2000, 0, 6, 0, 13
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 4, 2, 6
    WaitFrames 20
    OrbitEmitters 711, 2, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    OrbitEmitters 711, 4, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    OrbitEmitters 711, 1, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    WaitFrames 20
    OrbitEmitters 711, 2, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    OrbitEmitters 711, 4, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    OrbitEmitters 711, 1, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    WaitFrames 20
    OrbitEmitters 711, 2, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    OrbitEmitters 711, 4, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    OrbitEmitters 711, 1, 2, 0x2000, 0x2000, -0x1000, 120, 0, 1, 0
    WaitFrames 20
    Wait EFFVM_WAIT_PARTICLES
    CallDfc70 4, 0, 16, 1, 0
    Wait EFFVM_WAIT_ALL
    CallDfd74 0, 0
    CallDfd74 1, 0
    Wait EFFVM_WAIT_ALL
    CallDfc70 4, 16, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
