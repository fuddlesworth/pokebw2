#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntriesEnd

Script_9:
    VMStackPush 0x4115
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMStackPushFlag 2400
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_004F
    HollowRivalCmd_0262 0, 10

L_004F:
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Nuvema Town\nThe Start of Something Big!"
    MsgPlaceSign 5, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    Cmd_02B4 2, 0x400f
    VMStackPush 0x400f
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00A4
    Cmd_02B5 2, 0
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0000's House"
    MsgPlaceSign 6, 2
    MsgPlaceSignClose
    VMJump L_00B6

L_00A4:
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "...'s House"
    MsgPlaceSign 7, 2
    MsgPlaceSignClose

L_00B6:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Cheren's House"
    MsgPlaceSign 8, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Bianca's House"
    MsgPlaceSign 9, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Juniper Pokémon Lab"
    MsgPlaceSign 10, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The power of science is amazing![f000]븁\u0000\nNow you can use infrared to trade\nPokémon and have battles--[f000]븀\u0000\nall in the blink of an eye!"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you think traveling with Pokémon\nchanges people?"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0163
    // "Me too! Traveling and thinking about many\nthings can definitely make a difference!"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0171

L_0163:
    // "You're right! It's fine to just enjoy the\njourney without overthinking it!"
    ParentActorMsg MSGFILE_SCRIPT, 3, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0171:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Know what? On my next birthday,\nProfessor Juniper is going to[f000]븀\u0000\ngive me a Pokémon as a present![f000]븁\u0000\nI'll get a Pokédex, too, of course![f000]븁\u0000\nMaybe I'll grow up to be a Pokémon\nprofessor, or a Champion![f000]븁\u0000\nI haven't decided yet, but\nI'll pick my own dream to pursue!"
    ParentActorMsg MSGFILE_SCRIPT, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
