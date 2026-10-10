#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    FlagSet 690
    WorkSetConst 0x8020, 0
    RTCGetSeason 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_002B
    FlagReset 690

L_002B:
    WorkSetConst 0x8020, 0
    VMHalt
    .balign 4, 0
