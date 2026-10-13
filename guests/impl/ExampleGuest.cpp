#include "ExampleGuest.h"

/**
 * Pretend example for an emulated guest game in derailleur
 * Requires light knowledge of C++ and a tool for finding memory addresses
 * within emulated games.
 * Look at DrCommon.h, QRetro.h, and other guest game implementations for
 * additional guidance.
 */

/**
 * ============================================================================
 * The top of the file should contain any constants you will use throughout,
 * such as documented memory addresses and their known values, as well as the
 * list of supported mini-games...
 * ============================================================================
 */

/// Pretend player data values for 4 players
static const dr_value_t EXAMPLE_CHARACTER_P1 = { 0x80123450, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_IS_BOT_P1 = { 0x80123454, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_DIFFICULTY_P1 = { 0x80123458, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_RESULT_P1 = { 0x8012345C, DR_VALUE_TYPE_S32 };

static const dr_value_t EXAMPLE_CHARACTER_P2 = { 0x80123460, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_IS_BOT_P2 = { 0x80123464, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_DIFFICULTY_P2 = { 0x80123468, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_RESULT_P2 = { 0x8012346C, DR_VALUE_TYPE_S32 };

static const dr_value_t EXAMPLE_CHARACTER_P3 = { 0x80123470, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_IS_BOT_P3 = { 0x80123474, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_DIFFICULTY_P3 = { 0x80123478, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_RESULT_P3 = { 0x8012347C, DR_VALUE_TYPE_S32 };

static const dr_value_t EXAMPLE_CHARACTER_P4 = { 0x80123480, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_IS_BOT_P4 = { 0x80123484, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_DIFFICULTY_P4 = { 0x80123488, DR_VALUE_TYPE_S32 };
static const dr_value_t EXAMPLE_RESULT_P4 = { 0x8012348C, DR_VALUE_TYPE_S32 };

/// A pretend ID that game uses to load a specific mini-game
static const dr_value_t EXAMPLE_MINIGAME_TO_LOAD = { 0x80234560, DR_VALUE_TYPE_S32 };

/// A pretend 1-byte boolean that turns from 0 to 1 when the mini-game finishes
static const dr_value_t EXAMPLE_MINIGAME_HAS_ENDED = { 0x80234567, DR_VALUE_TYPE_U8 };

/**
 * This is where mini-games get defined for the dispatcher.
 * @param name The title of the mini-game, within 32 characters
 * @param type The type of mini-game, as defined in `dr_minigame_type`
 * @param minigame_id A unique identifier for the mini-game. Can technically be
 *   any value you want, as long as no mini-games share it.
 * @param scene_id Any value that might be otherwise helpful in your code for
 *   identifying or loading the mini-game later
 * @param quirks Any known emulation quirks to handle for this mini-game, as
 *   defined in `dr_emulation_quirk_t`.
 * @param flags Any further flags supported dy the program, as defined in
 *   `dr_minigame_flags_t`.
 */
static const dr_mp_minigame_t EXAMPLE_MINIGAMES[] =
{
  /**
   * Here are some example entries. Look at the other supported games for more examples:
   * { "My 4P Mini-Game", DR_MINIGAME_4P, 0x00, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
   * { "My 1v3 Mini-Game", DR_MINIGAME_1V3, 0x01, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
   * { "My 2v2 Mini-Game", DR_MINIGAME_2V2, 0x02, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
   * { "My Game with Sideways Control", DR_MINIGAME_4P, 0x03, 0x00, DR_WII_CONTROL(DR_WII_CONTROL_SIDEWAYS_BUTTONS), DR_NO_FLAGS },
   * { "My Lucky Game", DR_MINIGAME_4P, 0x04, 0x00, DR_NO_QUIRKS, DR_FLAG_LUCKY },
   */

  /* The list must end with this line so the dispatcher knows where the list stops */
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

/**
 * ============================================================================
 * Then, the constructor...
 * ============================================================================
 */

ExampleGuest::ExampleGuest(QObject *parent)
  : DrGuestRetro(parent)
{
  setRetro(new DrRetro(this));
  
  /**
   * Load a libretro core (without .dll or .so extension) and a ROM.
   * Before contributing a new guest game, move these to the header instead.
   * If this guest game runs a core already supported, try having your game
   * extend a helper class like `DrGuestWii` instead.
   */
  m_retro->init("example_libretro", "My Example Game (USA).bin");
}

/**
 * ============================================================================
 * Then, the typical functions guest games implement...
 * ============================================================================
 */

/**
 * Function that returns the mini-game list for this guest game.
 * Usually all we need to do is return our array above, but we can also generate one programmatically here
 */
const dr_mp_minigame_t *ExampleGuest::minigames(void) const
{
  return EXAMPLE_MINIGAMES;
}

/**
 * Any code that gets run before the emulator core loads
 * For example, moving save files around, putting custom assets in place, etc.
 */
void ExampleGuest::onBeforeBoot(const DrGameData &data)
{
}

/**
 * Any code that gets run after the emulator core loads, but before the mini-game starts
 * For example, injecting player character status, bot/human status, controller ports, etc.
 */
void ExampleGuest::doApplyGameData(const DrGameData &data)
{
  /* Typically the first thing to do is load a provided savestate */
  m_retro->loadState(state());

  /* Some values it may be okay to set just once, like this... */
  m_retro->writeValue(data.players[0].character, EXAMPLE_CHARACTER_P1);
  m_retro->writeValue(data.players[0].control_type == DR_CONTROL_TYPE_CPU, EXAMPLE_IS_BOT_P1);
  m_retro->writeValue(data.players[0].difficulty, EXAMPLE_DIFFICULTY_P1);

  m_retro->writeValue(data.players[1].character, EXAMPLE_CHARACTER_P2);
  m_retro->writeValue(data.players[1].control_type == DR_CONTROL_TYPE_CPU, EXAMPLE_IS_BOT_P2);
  m_retro->writeValue(data.players[1].difficulty, EXAMPLE_DIFFICULTY_P2);

  m_retro->writeValue(data.players[2].character, EXAMPLE_CHARACTER_P3);
  m_retro->writeValue(data.players[2].control_type == DR_CONTROL_TYPE_CPU, EXAMPLE_IS_BOT_P3);
  m_retro->writeValue(data.players[2].difficulty, EXAMPLE_DIFFICULTY_P3);

  m_retro->writeValue(data.players[3].character, EXAMPLE_CHARACTER_P4);
  m_retro->writeValue(data.players[3].control_type == DR_CONTROL_TYPE_CPU, EXAMPLE_IS_BOT_P4);
  m_retro->writeValue(data.players[3].difficulty, EXAMPLE_DIFFICULTY_P4);

  /* ...while some, if the game overwrites them, can be set for a time interval in frames. */
  m_retro->writeValueForFrames(data.minigame->minigame_id, EXAMPLE_MINIGAME_TO_LOAD, 60);

  /**
   * You can start the mini-game immediately if you like, or wait until
   * something happens in-game by checking in `run` below: loading screen finishes, etc.
   * In this example, we'll start the mini-game immediately.
   */
  startMinigame();
}

/**
 * Any code that gets run on every frame while the guest game plays
 * Typically here, you will watch for the mini-game to end, and cache results
 * data is the game runs if needed.
 */
void ExampleGuest::run(void)
{
  /* Watch any relevant values here to determine if the mini-game has ended */
  int64_t val;
  m_retro->readValue(&val, EXAMPLE_MINIGAME_HAS_ENDED);

  /* When you've determined the mini-game has ended, call `finishMinigame` */
  if (val)
    finishMinigame();
}

/**
 * Finally, any code for returning the minigame results after the mini-game has finished
 * 
 * `DR_MINIGAME_4P`:
 * `DR_MINIGAME_1V3`:
 * `DR_MINIGAME_2V2`:
 * `DR_MINIGAME_1P`:
 *   result + bonus will be awarded as coins.
 * 
 * `DR_MINIGAME_BATTLE`:
 *   result will be the placement of each player, with:
 *   0=1st, 1=2nd, 2=3rd, 3=4th
 * 
 * `DR_MINIGAME_DUEL`:
 *   result will contain a higher number for the player that won:
 *   10=winner, 0=loser
 * 
 * `DR_MINIGAME_BOWSER`:
 *   result will contain whether the player survived the mini-game:
 *   1=winner, 0=loser
 * 
 * `DR_MINIGAME_DK`:
 *   result will contain 
 * @see dr_minigame_type
 */
dr_minigame_result_t ExampleGuest::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  int64_t val;
  
  /**
   * `index` is decided by order from the host context, which may not be the
   * same as controller order. See other games for examples on how to handle this.
   */
  switch (index)
  {
  case 0:
    m_retro->readValue(&val, EXAMPLE_RESULT_P1);
    break;
  case 1:
    m_retro->readValue(&val, EXAMPLE_RESULT_P2);
    break;
  case 2:
    m_retro->readValue(&val, EXAMPLE_RESULT_P3);
    break;
  case 3:
    m_retro->readValue(&val, EXAMPLE_RESULT_P4);
    break;
  }
  result.coins = val;

  return result;
}
