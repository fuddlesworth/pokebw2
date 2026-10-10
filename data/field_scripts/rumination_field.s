#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag 331
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003B
    // "I've heard it's called Virizion, and it\ntook up against humans to protect[f000]븀\u0000\nPokémon from a war between people.[f000]븁\u0000\nIt's apparently a legendary Pokémon.[f000]븁\u0000\nMaybe it was afraid that Team Plasma's\nrising to power would ruin the world and[f000]븀\u0000\nits friends' homes would be destroyed."
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0061

L_003B:
    // "This happened around when the hero\nappeared in Opelucid City with the[f000]븀\u0000\nlegendary dragon Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    // "Right around here, you would\nhear a very sad cry.[f000]븁\u0000\nThen a bright-green Pokémon would\nrun around like the wind.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    // "I've heard it's called Virizion, and it\ntook up against humans to protect[f000]븀\u0000\nPokémon from a war between people.[f000]븁\u0000\nIt's apparently a legendary Pokémon.[f000]븁\u0000\nMaybe it was afraid that Team Plasma's\nrising to power would ruin the world and[f000]븀\u0000\nits friends' homes would be destroyed."
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 331

L_0061:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
