#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 14"
    MsgPlaceSign 0, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nThe maximum number of Boxes is now 24![f000]븁\u0000\nIn other words, you can store\n720 Pokémon!"
    MsgPlaceSign 1, 0
    MsgPlaceSignClose
    FlagSet 2675
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nWhile you are using the Xtransceiver,\npress a direction on the +Control Pad.[f000]븁\u0000\nThe appearance of the screen will\nchange in varied ways!"
    MsgPlaceSign 2, 0
    MsgPlaceSignClose
    FlagSet 2676
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
