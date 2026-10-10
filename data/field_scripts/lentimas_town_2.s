#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    Random 0x400b, 5
    WorkSetConst 0x8020, 1
    WorkAdd 0x8020, 0x400b
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This quiz is difficult.[f000]븁\u0000\n“What will happen when you press SELECT\nwhile you are checking the Town Map?\"[f000]븁\u0000\nI don't know, because I don't have one!"
    ParentActorMsg MSGFILE_SCRIPT, 6, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
