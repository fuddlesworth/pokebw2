#include "types.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "demo/intro/intro.h"
#include "demo/intro/intro_script.h"
#include "gfl/fade.h"
#include "nitro/fx.h"
#include "text/system/game_start.h"

// The intro's scripts. The messages are those of the system message file 54, and the professor's sprite changes frames
// while she talks. Word 0 is the player's name and word 1 the rival's.

static const IntroCmdEntry sScriptBegin[] = {
    { INTRO_CMD_LOAD_GRAPHICS, { FALSE }, TRUE },
    { INTRO_CMD_LOAD_MESSAGES, { TRUE, TEXT_BANK_GAME_START }, TRUE },
    { INTRO_CMD_GOTO_MODE_SCRIPT },
};

static const IntroCmdEntry sScriptStart[] = {
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_GREETING } },
};

static const IntroCmdEntry sScriptGreeting[] = {
    { INTRO_CMD_ADD_SPRITE, { INTRO_SPRITE_PROFESSOR, SPECIES_NONE, 0, FX32_CONST(5.628) }, TRUE },
    { INTRO_CMD_ADD_SPRITE, { INTRO_SPRITE_POKEMON, SPECIES_CINCCINO, FX32_CONST(-5), FX32_CONST(-0.5) }, TRUE },
    { INTRO_CMD_SET_SPRITE_VISIBLE, { INTRO_SPRITE_POKEMON, FALSE } },
    { INTRO_CMD_SAVE_START },
    { INTRO_CMD_CLEAR_MESSAGE },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 2 } },
    { INTRO_CMD_BGM_PLAY, { SEQ_BGM_STARTING2 }, TRUE },
    { INTRO_CMD_BGM_FADE_IN, { 90 }, TRUE },
    { INTRO_CMD_WAIT, { 90 } },
    // Hi there! Welcome to the world of Pokémon!
    { INTRO_CMD_TALK, { GameStart_Text_HiThereWelcomeWorld, INTRO_SPRITE_PROFESSOR, TRUE } },
    // My name is Professor Juniper. Everyone calls me the Pokémon Professor!
    { INTRO_CMD_TALK, { GameStart_Text_NameProfessorJuniperEveryone, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_POKEMON } },
};

static const IntroCmdEntry sScriptPokemon[] = {
    { INTRO_CMD_CLEAR_MESSAGE },
    { INTRO_CMD_CLEAR_SPRITE_ANIMATION_ENDED, { INTRO_SPRITE_PROFESSOR } },
    { INTRO_CMD_MOVE_SPRITE_X, { INTRO_SPRITE_PROFESSOR, FX32_CONST(0.2), FX32_CONST(8) } },
    { INTRO_CMD_WAIT_SPRITE_ANIMATION, { INTRO_SPRITE_PROFESSOR } },
    { INTRO_CMD_SET_SPRITE_ANIMATION, { INTRO_SPRITE_PROFESSOR, 2, TRUE } },
    { INTRO_CMD_WAIT, { 52 } },
    { INTRO_CMD_SE_PLAY, { SEQ_SE_NAGERU } },
    { INTRO_CMD_WAIT, { 60 } },
    { INTRO_CMD_SE_PLAY, { SEQ_SE_KON } },
    { INTRO_CMD_WAIT, { 26 } },
    { INTRO_CMD_SET_SPRITE_ANIMATION, { INTRO_SPRITE_PROFESSOR, 1, FALSE } },
    { INTRO_CMD_SE_PLAY, { SEQ_SE_BOWA2 }, TRUE },
    { INTRO_CMD_SET_SPRITE_VISIBLE, { INTRO_SPRITE_POKEMON, TRUE }, TRUE },
    { INTRO_CMD_PARTICLES, { FX32_CONST(-0.4), FX32_CONST(0.5) }, TRUE },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, -16, 0, 6 } },
    { INTRO_CMD_POKEMON_APPEAR, { SPECIES_CINCCINO } },
    // That's right! This world is widely inhabited by mysterious creatures called Pokémon!
    { INTRO_CMD_TALK, { GameStart_Text_ThatsRightWorldWidely, INTRO_SPRITE_PROFESSOR, TRUE } },
    // Pokémon have mysterious powers. They come in many shapes and live in many different places. We humans live
    // happily with Pokémon! Living and working together, we complement each other. We help each other out to accomplish
    // difficult tasks. Having Pokémon battle one another is particularly popular, and it deepens the bonds between
    // people and Pokémon. And that is why I research Pokémon.
    { INTRO_CMD_TALK, { GameStart_Text_PokemonHaveMysteriousPowers, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_ABOUT_YOU } },
};

static const IntroCmdEntry sScriptAboutYou[] = {
    { INTRO_CMD_FADE_SPRITE, { INTRO_SPRITE_POKEMON, FALSE } },
    { INTRO_CMD_CLEAR_SPRITE_ANIMATION_ENDED, { INTRO_SPRITE_PROFESSOR } },
    { INTRO_CMD_MOVE_SPRITE_X, { INTRO_SPRITE_PROFESSOR, FX32_CONST(-0.2), 0 } },
    { INTRO_CMD_SET_SPRITE_ANIMATION, { INTRO_SPRITE_PROFESSOR, 1, FALSE } },
    // Well, that's enough from me... Could you tell me about yourself?
    { INTRO_CMD_TALK, { GameStart_Text_WellThatsEnoughFrom, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_FADE_SPRITE, { INTRO_SPRITE_PROFESSOR, FALSE } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_GENDER } },
};

static const IntroCmdEntry sScriptGender[] = {
    { INTRO_CMD_SET_MODEL_VISIBLE, { TRUE }, TRUE },
    { INTRO_CMD_OPEN_MODEL },
    // Are you a boy? Or a girl?
    { INTRO_CMD_TALK, { GameStart_Text_BoyGirl, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_CHOOSE_GENDER },
    { INTRO_CMD_IF, { INTRO_COND_PLAYER_IS_MALE } },
    // You're a boy, right?
    { INTRO_CMD_TALK, { GameStart_Text_YoureBoyRight, INTRO_SPRITE_PROFESSOR, TRUE } },
    // You're a girl, right?
    { INTRO_CMD_TALK, { GameStart_Text_YoureGirlRight, INTRO_SPRITE_PROFESSOR, TRUE } },
    // YES / NO
    { INTRO_CMD_YES_NO, { GameStart_Text_Yes, GameStart_Text_No, TRUE } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_ASK_NAME } },
    { INTRO_CMD_MODEL_BACK },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_GENDER } },
};

static const IntroCmdEntry sScriptAskName[] = {
    { INTRO_CMD_SET_RESULT, { INTRO_RESULT_ENTER_NAME } },
    // I'd like to know your name. Please tell me.
    { INTRO_CMD_TALK, { GameStart_Text_IdLikeKnowName, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_SAVE_PAUSE },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 2 } },
    { INTRO_CMD_END },
};

static const IntroCmdEntry sScriptPlayerNamed[] = {
    { INTRO_CMD_LOAD_GRAPHICS, { TRUE }, TRUE },
    { INTRO_CMD_CREATE_OBJ },
    { INTRO_CMD_ADD_SPRITE, { INTRO_SPRITE_PROFESSOR, SPECIES_NONE, 0, FX32_CONST(5.628) }, TRUE },
    { INTRO_CMD_SAVE_RESUME },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 2 } },
    { INTRO_CMD_SET_WORD, { INTRO_WORD_PLAYER_NAME, 0 }, TRUE },
    { INTRO_CMD_IF, { INTRO_COND_PLAYER_IS_MALE } },
    // Your name is {WORD 0}?
    { INTRO_CMD_TALK, { GameStart_Text_Name, INTRO_SPRITE_PROFESSOR, TRUE } },
    // Your name is {WORD 0}?
    { INTRO_CMD_TALK, { GameStart_Text_Name_2, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_WAIT, { 1 } },
    // YES / NO
    { INTRO_CMD_YES_NO, { GameStart_Text_Yes, GameStart_Text_No, TRUE } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_ASK_RIVAL_NAME } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_RENAME_PLAYER } },
};

static const IntroCmdEntry sScriptRenamePlayer[] = {
    { INTRO_CMD_SET_RESULT, { INTRO_RESULT_ENTER_NAME } },
    { INTRO_CMD_SAVE_PAUSE },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 2 } },
    { INTRO_CMD_END },
};

static const IntroCmdEntry sScriptAskRivalName[] = {
    { INTRO_CMD_SET_RESULT, { INTRO_RESULT_ENTER_RIVAL_NAME } },
    // So your name's {WORD 0}! What a wonderful name!
    { INTRO_CMD_TALK, { GameStart_Text_NamesWhatWonderfulName, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_FADE_SPRITE, { INTRO_SPRITE_PROFESSOR, FALSE } },
    { INTRO_CMD_OPEN_BAND },
    { INTRO_CMD_OBJ_FADE_IN },
    // Could you tell me a little about your friend--the older boy who lives nearby?
    { INTRO_CMD_TALK, { GameStart_Text_CouldTellLittleAbout, INTRO_SPRITE_PROFESSOR, TRUE } },
    // Would you mind telling me his name?
    { INTRO_CMD_TALK, { GameStart_Text_WouldMindTellingHis, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_SAVE_PAUSE },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 2 } },
    { INTRO_CMD_END },
};

static const IntroCmdEntry sScriptRivalNamed[] = {
    { INTRO_CMD_LOAD_GRAPHICS, { TRUE }, TRUE },
    { INTRO_CMD_CREATE_OBJ },
    { INTRO_CMD_ADD_SPRITE, { INTRO_SPRITE_PROFESSOR, SPECIES_NONE, 0, FX32_CONST(5.628) }, TRUE },
    { INTRO_CMD_SAVE_RESUME },
    { INTRO_CMD_SET_SPRITE_VISIBLE, { INTRO_SPRITE_PROFESSOR, FALSE } },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 2 } },
    { INTRO_CMD_OPEN_BAND },
    { INTRO_CMD_OBJ_FADE_IN },
    { INTRO_CMD_SET_WORD, { INTRO_WORD_RIVAL_NAME, 1 }, TRUE },
    // {WORD 1}? Did I get that right?
    { INTRO_CMD_TALK, { GameStart_Text_DidGetRight, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_WAIT, { 1 } },
    // YES / NO
    { INTRO_CMD_YES_NO, { GameStart_Text_Yes, GameStart_Text_No, TRUE } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_FAREWELL } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_RENAME_RIVAL } },
};

static const IntroCmdEntry sScriptRenameRival[] = {
    { INTRO_CMD_SET_RESULT, { INTRO_RESULT_ENTER_NAME } },
    { INTRO_CMD_SAVE_PAUSE },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 2 } },
    { INTRO_CMD_END },
};

static const IntroCmdEntry sScriptFarewell[] = {
    { INTRO_CMD_SET_RESULT, { INTRO_RESULT_DONE } },
    { INTRO_CMD_OBJ_FADE_OUT },
    { INTRO_CMD_CLOSE_BAND },
    { INTRO_CMD_FADE_SPRITE, { INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_SAVE_NOP },
    { INTRO_CMD_SAVE_WAIT },
    { INTRO_CMD_SET_WORD, { INTRO_WORD_PLAYER_NAME, 0 }, TRUE },
    // {WORD 0}! I'm going to entrust you with a Pokémon. I'm sure you will be great partners!
    { INTRO_CMD_TALK, { GameStart_Text_ImGoingEntrustPokemon, INTRO_SPRITE_PROFESSOR, TRUE } },
    // The moment you choose the Pokémon that will accompany you on your journey, your story will truly begin. During
    // your journey, you will meet many Pokémon and people with different personalities and points of view. I really
    // hope you learn what is important to you as a result of your travels... That's right! Befriend new people and
    // Pokémon and grow as a person! That's the most important goal of your journey! Let's go visit the world of
    // Pokémon!
    { INTRO_CMD_TALK, { GameStart_Text_MomentChoosePokemonWill, INTRO_SPRITE_PROFESSOR, TRUE } },
    { INTRO_CMD_FADE_SPRITE, { INTRO_SPRITE_PROFESSOR, FALSE } },
    { INTRO_CMD_GOTO, { INTRO_SCRIPT_FINISH } },
};

static const IntroCmdEntry sScriptFinish[] = {
    { INTRO_CMD_BGM_FADE_OUT, { 60 } },
    { INTRO_CMD_FADE, { FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 6 } },
    { INTRO_CMD_FREE_OBJ },
    { INTRO_CMD_END },
};

static const IntroCmdEntry *sScripts[INTRO_SCRIPT_COUNT] = {
    [INTRO_SCRIPT_BEGIN] = sScriptBegin,
    [INTRO_SCRIPT_START] = sScriptStart,
    [INTRO_SCRIPT_GREETING] = sScriptGreeting,
    [INTRO_SCRIPT_POKEMON] = sScriptPokemon,
    [INTRO_SCRIPT_ABOUT_YOU] = sScriptAboutYou,
    [INTRO_SCRIPT_GENDER] = sScriptGender,
    [INTRO_SCRIPT_ASK_NAME] = sScriptAskName,
    [INTRO_SCRIPT_PLAYER_NAMED] = sScriptPlayerNamed,
    [INTRO_SCRIPT_RENAME_PLAYER] = sScriptRenamePlayer,
    [INTRO_SCRIPT_ASK_RIVAL_NAME] = sScriptAskRivalName,
    [INTRO_SCRIPT_RIVAL_NAMED] = sScriptRivalNamed,
    [INTRO_SCRIPT_RENAME_RIVAL] = sScriptRenameRival,
    [INTRO_SCRIPT_FAREWELL] = sScriptFarewell,
    [INTRO_SCRIPT_FINISH] = sScriptFinish,
};

const IntroCmdEntry *IntroScript_Get(u32 script) {
    return sScripts[script];
}
