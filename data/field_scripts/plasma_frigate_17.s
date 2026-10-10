#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_SW_PLAZMASHIP_09
    SEWait
    // "Warning! Warning!\nIntruders in the vessel![f000]븀\u0000\nEveryone, please respond."
    InfoMsg 0, 2
    LastKeyWait
    InfoMsgClose_0039
    WorkSetConst 0x4149, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
