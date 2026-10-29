// MOVE_MOONLIGHT's animation: member 236 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Moonlight

Moonlight:
    CameraMovePreset 1, 9, 16, 0, 12
    CallDfc70 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 108
    BgMove 4, 0, 1, 120, 1, 0
    CallDfc70 4, 15, 0, 1, 0
    CallDfd74 2, 0
    CallDfd74 1, 1
    CallDfd74 0, 1
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 1, EFFVM_ATTACKER, 20, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_ATTACKER, 50, 0, 40, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 407
    EmitterAdd 407, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CallDfc70 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    CallDfc70 4, 15, 0, 1, 0
    CallDfd74 1, 0
    CallDfd74 0, 0
    CallDfd74 2, 1
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
