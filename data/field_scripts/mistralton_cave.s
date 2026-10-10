#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    FlagSet 950
    FlagSet 951
    FlagSet 953
    FlagSet 955
    FlagSet 957
    FlagReset 948
    FlagReset 949
    FlagReset 952
    FlagReset 954
    FlagReset 956
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I heard there was a legendary Pokémon\nin Mistralton Cave..."
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
