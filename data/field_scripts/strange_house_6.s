#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMSleep 16
    FlagReset 898
    ActorAdd 0
    ActorCmdExec 0, Movement_0084
    ActorCmdWait
    VMSleep 8
    // "In the dark dream...\nI heard my dad's voice...[f000]븁\u0000\nForget about the Lunar Wing...\nPlease stay here with me...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 0, 0, 2, 0
    MsgWinCloseAll
    VMSleep 16
    ActorCmdExec 0, Movement_00D8
    ActorCmdWait
    ActorDelete 0
    FlagSet 898
    WorkSetConst 0x4075, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There are markings on the wall\nwhere the portrait was hung..."
    InfoMsg 1, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Could there be another painting\nbehind the scratches?"
    InfoMsg 2, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0084:
    Move 73, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 2
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 3
    Move 70, 1
    Move 61, 3
    Move 69, 1
    Move 61, 4
    Move 70, 1
    MoveEnd

Movement_00D8:
    Move 69, 1
    Move 61, 4
    Move 70, 1
    Move 61, 3
    Move 69, 1
    Move 61, 3
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 2
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    MoveEnd
