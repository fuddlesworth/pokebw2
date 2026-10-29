// MOVE_WONDER_ROOM's animation: member 472 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WonderRoom

WonderRoom:
    LoadBg 195
    CallDfd74 2, 0
    CallDfc70 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W470_WA, 1, EFFVM_PAN_CENTER, 0, 470, 70, 0, 0, 0
    MoveSE 1, 0, 1, 70, 0, 80, 30, 0, 0
    PlaySE SEQ_SE_W054_01, 2, EFFVM_PAN_CENTER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    CallDfd74 0, 1
    CallDfd74 1, 1
    McssSetValue EFFVM_ALL, 1
    CallDfc70 3, 15, 0, 1, 0
    BgMove 1, 0, -16, 100, 0, 0
    McssSetVanish EFFVM_ALL, TRUE
    McssScale EFFVM_ALL, 2, -0x1000, -0x1000, 1, 0, 40
    McssMoveXY EFFVM_ALL, 2, 0x800, 0, 60, 0, 1
    Wait EFFVM_WAIT_BG
    SetYield FALSE
    CallDfc70 3, 0, 15, 1, 0
    CallDfc70 2, 15, 0, 1, 0
    CallDfd74 0, 0
    CallDfd74 1, 0
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_ALL, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
