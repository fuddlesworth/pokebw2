#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    WorkSetConst 0x8020, 0
    // "Oh?[f000]븁\u0000"
    SystemMsg 0, 2
    InfoMsgClose
    CallEggHatch 0x8020
    RecordAdd 9, 1
    Cmd_02C5 8
    WorkSetConst 0x8020, 0
    VMHalt
    .balign 4, 0
