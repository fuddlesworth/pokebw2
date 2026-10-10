#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag 474
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0085
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The legendary Pokémon that\ncreated the Unova region, Reshiram.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    ActorMsgClose
    CallPokemonPreview 643, 0, 0, 0
    PokeDexRegist 0, 643
    // "Reshiram has been registered\nin the Pokédex."
    SystemMsg 1, 0
    MsgWaitAdvance
    InfoMsgClose
    // "And the other legendary\nPokémon, Zekrom.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    ActorMsgClose
    CallPokemonPreview 644, 0, 0, 0
    PokeDexRegist 0, 644
    // "Zekrom has been registered\nin the Pokédex."
    SystemMsg 3, 0
    MsgWaitAdvance
    InfoMsgClose
    // "If you have these Pokémon,\nit's so easy to control[f000]븀\u0000\nthe Unova region, right?"
    ParentActorMsg MSGFILE_SCRIPT, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet 474
    VMJump L_0099

L_0085:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you have these Pokémon,\nit's so easy to control[f000]븀\u0000\nthe Unova region, right?"
    ParentActorMsg MSGFILE_SCRIPT, 4, 0, 0
    LastKeyWait
    ActorMsgClose

L_0099:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
