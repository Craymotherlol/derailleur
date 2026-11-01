#include "DonkeyKong64.h"
#include <QRetro.h>

typedef enum
{
  DK64_CHARACTER_DONKEY_KONG = 0x00,
  DK64_CHARACTER_DIDDY_KONG = 0x01,
  DK64_CHARACTER_LANKY_KONG = 0x02,
  DK64_CHARACTER_TINY_KONG = 0x03,
  DK64_CHARACTER_CHUNKY_KONG = 0x04,
  DK64_CHARACTER_KRUSHA = 0x05
} dk64_character_ids;


// s8 (multiplayer) arena ID
typedef enum
{
  DK64_ARENA_ONE = 0x00,
  DK64_ARENA_TWO = 0x01,
  DK64_ARENA_THREE = 0x02,
} dk64_arena_ids;

// s8 monkey smash game type
typedef enum
{
  DK64_MONKEYSMASH_SURVIVAL = 0x00,
  DK64_MONKEYSMASH_COIN_HOARD = 0x01,
  DK64_MONKEYSMASH_WINS = 0x02,
  DK64_MONKEYSMASH_TIME = 0x03,
  DK64_MONKEYSMASH_CAPTURE = 0x04,
  DK64_MONKEYSMASH_CAPTURE_PAD = 0x05,
} dk64_monkey_smash_game_type;

// u8 wins for each player on the results screen
static const dr_value_t DK64_WINS[4] = {
  { 0x80744524, DR_VALUE_TYPE_U8 },
  { 0x80744526, DR_VALUE_TYPE_U8 },
  { 0x8074452A, DR_VALUE_TYPE_U8 },
  { 0x80744528, DR_VALUE_TYPE_U8 },
};

static const dr_value_t DK64_ARCADE_LIVES = {0x8004c718, DR_VALUE_TYPE_S8};

static const dr_value_t DK64_RESULT_P1 = { 0x8012345c, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_RESULT_P2 = { 0x8012346c, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_RESULT_P3 = { 0x8012347c, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_RESULT_P4 = { 0x8012348c, DR_VALUE_TYPE_S8 };

// s8 Character
/* These can be set to 0x80 if a Player isn't playing. (except for Player 1) i.e: CPU's */
static const dr_value_t DK64_CHARACTER_P1 = { 0x8076a0e7, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_CHARACTER_P2 = { 0x8076a0e6, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_CHARACTER_P3 = { 0x8076a0e5, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_CHARACTER_P4 = { 0x8076a0e4, DR_VALUE_TYPE_S8 };

static const dr_value_t DK64_COLOR_P1 = { 0x8076a103, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_COLOR_P2 = { 0x8076a102, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_COLOR_P3 = { 0x8076a101, DR_VALUE_TYPE_S8 };
static const dr_value_t DK64_COLOR_P4 = { 0x8076a100, DR_VALUE_TYPE_S8 };


static const dr_value_t DK64_IS_MULTIPLAYER = { 0x807552ec, DR_VALUE_TYPE_S8};

// u8 Load to Map ID
static const dr_value_t DK64_MINIGAME_TO_LOAD = { 0x807444E7, DR_VALUE_TYPE_U8 };

// u8 Game State
// 0x00=Opening Logos
// 0x01=Intro Cutscene
// 0x02=DK Rap
// 0x03=DK TV (Attract Mode)
// 0x05=File Select
// 0x06=In-Game
// 0x07=Loading Game Over Cutscene
// 0x09=Game Over
// 0x0A=End Sequence (Adventure)
// 0x0B=DK Theatre
// 0x0C=DK Bonus / Bosses
// 0x0D=Snide's Bonus Games
// 0x0E=End Sequence (DK Theatre)
static const dr_value_t DK64_GAME_STATE = {0x80755318, DR_VALUE_TYPE_U8};

// u8 Multiplayer results screen
static const dr_value_t DK64_MULTI_MINIGAME_HAS_ENDED = { 0x804B210D, DR_VALUE_TYPE_U8 };

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

 /**
  * Some of the 1 Player minigames are sorted a bit by time and difficulty.
  * This is to make sure these minigames don't last very long and aren't ridiculously hard (ex: Mad Maze Maul 0x7C )
  */
static const dr_mp_minigame_t DK64_MINIGAMES[] = {
  { "Mad Maze Maul", DR_MINIGAME_1P, 0x00, 0x45, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Speedy Swing Sortie", DR_MINIGAME_1P, 0x01, 0x86, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Splish Slash Salvage", DR_MINIGAME_1P, 0x02, 0x84, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Minecart Mayhem", DR_MINIGAME_1P, 0x03, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stealthy Snoop", DR_MINIGAME_1P, 0x04, 0x7F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Big Bug Bash", DR_MINIGAME_1P, 0x05, 0x95, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Teetering Turtle Trouble", DR_MINIGAME_1P, 0x06, 0x78, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Kremling Kosh", DR_MINIGAME_1P, 0x07, 0x75, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Busy Barrel Barrage", DR_MINIGAME_1P, 0x08, 0x4F, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Beaver Bother", DR_MINIGAME_1P, 0x09, 0x88, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Peril Path Panic", DR_MINIGAME_1P, 0x0A, 0x92, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Batty Barrel Bandit", DR_MINIGAME_1P, 0x0B, 0x7A, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Stash Snatch", DR_MINIGAME_1P, 0x0C, 0x43, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Searchlight Seek", DR_MINIGAME_1P, 0x0D, 0x8B, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Krazy Kong Klamour", DR_MINIGAME_1P, 0x0E, 0x65, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG ARCADE: 25m", DR_MINIGAME_1P, 0x0F, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG ARCADE: 50m", DR_MINIGAME_1P, 0x10, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "DONKEY KONG ARCADE: 75m", DR_MINIGAME_1P, 0x11, 0x02, DR_NO_QUIRKS, DR_NO_FLAGS },

  { "Monkey Smash", DR_MINIGAME_4P, 0x12, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "Kong Battle", DR_MINIGAME_4P, 0x13, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

/**
 * ============================================================================
 * Then, the constructor...
 * ============================================================================
 */

DonkeyKong64::DonkeyKong64(QObject *parent)
  : DrGuestN64(parent)
{
  m_retro->init(coreId(), rom());
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
const dr_mp_minigame_t *DonkeyKong64::minigames(void) const
{
  return DK64_MINIGAMES;
}

/**
 * Any code that gets run before the emulator core loads
 * For example, moving save files around, putting custom assets in place, etc.
 */
void DonkeyKong64::onBeforeBoot(const DrGameData &data)
{
}

/**
 * Any code that gets run after the emulator core loads, but before the mini-game starts
 * For example, injecting player character status, bot/human status, controller ports, etc.
 */
void DonkeyKong64::doApplyGameData(const DrGameData &data)
{

  /* Set the load ID to the minigame's scene id for the game to boot into */
  m_retro->writeValueForFrames(data.minigame->scene_id, DK64_MINIGAME_TO_LOAD, 120);

  /* Set game state to "In-Game" so the title of the minigame shows up */
  m_retro->writeValueForFrames(0x06, DK64_GAME_STATE, 120); 

  // For some reason this game has two control flags?

  /** Stealing this from Kirby 64  */
  //for (unsigned i = 0; i < 4; i++)
  //{
  //  const unsigned slot = m_players[i].control_port;

  //  uint32_t character = 0;
  //  uint32_t color = 0;

  //  switch (m_players[i].character)
  //  {
  //  case DR_CHARACTER_MARIO:
  //    character = DK64_CHARACTER_DONKEY_KONG;
  //    color = 1;
  //    break;
  //  case DR_CHARACTER_LUIGI:
  //    character = DK64_CHARACTER_DIDDY_KONG;
  //    color = 2;
  //    break;
  //  case DR_CHARACTER_PEACH:
  //    character = DK64_CHARACTER_TINY_KONG;
  //    color = 1;
  //    break;
  //  case DR_CHARACTER_YOSHI:
  //    character = DK64_CHARACTER_LANKY_KONG;
  //    color = 2;
  //    break;
  //  case DR_CHARACTER_WARIO:
  //    character = DK64_CHARACTER_CHUNKY_KONG;
  //    color = 1;
  //    break;
  //  case DR_CHARACTER_DONKEY_KONG:
  //    character = DK64_CHARACTER_DONKEY_KONG; // :P
  //    color = 0;
  //    break;
  //  case DR_CHARACTER_WALUIGI:
  //    character = DK64_CHARACTER_CHUNKY_KONG;
  //    color = 3;
  //    break;
  //  case DR_CHARACTER_DAISY:
  //    character = DK64_CHARACTER_TINY_KONG;
  //    color = 2;
  //    break;
  //  default:
  //    character = DK64_CHARACTER_KRUSHA;
  //    color = 0;
  //    break;
  //  }
  //}

  startMinigame();
}

/**
 * Any code that gets run on every frame while the guest game plays
 * Typically here, you will watch for the mini-game to end, and cache results
 * data is the game runs if needed.
 */
void DonkeyKong64::run(void)
{
  /* Watch any relevant values here to determine if the mini-game has ended */
  //int64_t bonus_is_loaded;
  //m_retro->readValue(&bonus_is_loaded, );

  startMinigame();


  int64_t results_screen_is_shown;
  m_retro->readValue(&results_screen_is_shown, DK64_MULTI_MINIGAME_HAS_ENDED);

  /* When you've determined the mini-game has ended, call `finishMinigame` */
  if (results_screen_is_shown)
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
dr_minigame_result_t DonkeyKong64::minigameResult(unsigned index)
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
    m_retro->readValue(&val, DK64_RESULT_P1);
    break;
  case 1:
    m_retro->readValue(&val, DK64_RESULT_P2);
    break;
  case 2:
    m_retro->readValue(&val, DK64_RESULT_P3);
    break;
  case 3:
    m_retro->readValue(&val, DK64_RESULT_P4);
    break;
  }
  result.coins = val;

  return result;
}
