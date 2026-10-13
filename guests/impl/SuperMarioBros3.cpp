#include "SuperMarioBros3.h"

#include <QFile>

/// u8 The current Vs. mode map
static const dr_value_t SMB3_VS_MAP = { 0x7F24, DR_VALUE_TYPE_U8 };

/// u8 bool -- 0 if Mario won, 1 if Luigi won. Set once the game has fully faded to black
static const dr_value_t SMB3_VS_WINNER = { 0x073E, DR_VALUE_TYPE_U8 };

/// u8 bool -- Set to 1 when the mini-game is finished
static const dr_value_t SMB3_VS_GAME_OVER = { 0x007D, DR_VALUE_TYPE_U8 };

/// u8 [4] -- PPU Mario colors. Typically 00 16 36 12
static const size_t SMB3_VS_MARIO_PPU_ADDR = 0x80003010;

/// u8 [4] -- PPU Luigi colors. Typically 00 1A 36 0C
static const size_t SMB3_VS_LUIGI_PPU_ADDR = 0x80003014;

typedef struct
{
  uint8_t invisible;
  uint8_t hat;
  uint8_t skin;
  uint8_t overalls;
} smb3_player_colors_t;

typedef enum
{
  SMB3_VS_MAP_SPINY = 0,  /* Spiny only */
  SMB3_VS_MAP_FIGHTER_FLY = 1,  /* Fighter Fly only */
  SMB3_VS_MAP_SPINY_FIGHTER_FLY = 2,  /* Spiny and Fighter Fly */
  SMB3_VS_MAP_STATIC_COINS = 3,  /* Static coins */
  SMB3_VS_MAP_SPINY_SIDESTEPPER = 4,  /* Spiny and Sidestepper */
  SMB3_VS_MAP_FIGHTER_FLY_SIDESTEPPER = 5,  /* Fighter Fly and Sidestepper */
  SMB3_VS_MAP_SIDESTEPPER = 6,  /* Sidestepper only */
  SMB3_VS_MAP_COIN_FOUNTAIN = 7,  /* Coin Fountain (Fountain battlefield) */
  SMB3_VS_MAP_SPINY_ALT = 8,  /* Spiny only (alt) */
  SMB3_VS_MAP_FIGHTER_FLY_ALT = 9,  /* Fighter Fly only (alt) */
  SMB3_VS_MAP_SIDESTEPPER_ALT = 10, /* Sidestepper only (alt) */
  SMB3_VS_MAP_LADDERS = 11  /* Ladder and [?] blocks (Ladders battlefield) */
} smb3_vs_map;

static const dr_mp_minigame_t SMB3_MINIGAMES[] = {
  { "SMB3: Vs. Spinies", DR_MINIGAME_DUEL, SMB3_VS_MAP_SPINY, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "SMB3: Vs. Fighter Flies", DR_MINIGAME_DUEL, SMB3_VS_MAP_FIGHTER_FLY, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "SMB3: Vs. Spinies & Flies", DR_MINIGAME_DUEL, SMB3_VS_MAP_SPINY_FIGHTER_FLY, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "SMB3: Get the Coins", DR_MINIGAME_DUEL, SMB3_VS_MAP_STATIC_COINS, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "SMB3: Vs. Spinies & Crabs", DR_MINIGAME_DUEL, SMB3_VS_MAP_SPINY_SIDESTEPPER, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "SMB3: Vs. Flies & Crabs", DR_MINIGAME_DUEL, SMB3_VS_MAP_FIGHTER_FLY_SIDESTEPPER, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "SMB3: Vs. Crabs", DR_MINIGAME_DUEL, SMB3_VS_MAP_SIDESTEPPER, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "SMB3: Avoid the Flames", DR_MINIGAME_DUEL, SMB3_VS_MAP_COIN_FOUNTAIN, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },
  { "SMB3: Hidden Blocks", DR_MINIGAME_DUEL, SMB3_VS_MAP_LADDERS, 0x00, DR_NO_QUIRKS, DR_NO_FLAGS },

  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS },
};

SuperMarioBros3::SuperMarioBros3(QObject *parent)
  : DrGuestNES(parent)
{
  m_retro->init(coreId(), rom());
  core()->setBilinearFilter(false);
  core()->setIntegerScaling(true);
}

const dr_mp_minigame_t *SuperMarioBros3::minigames() const
{
  return SMB3_MINIGAMES;
}

static smb3_player_colors_t smb3_color(dr_character c)
{
  switch (c)
  {
  case DR_CHARACTER_LUIGI: /* 00 1A 36 0C */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_MEDIUM_GREEN, DrRetroNES::NES_COLOR_PALE_RED, DrRetroNES::NES_COLOR_DARK_CYAN };
  case DR_CHARACTER_WARIO: /* 00 28 37 04 */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_LIGHT_YELLOW, DrRetroNES::NES_COLOR_PALE_ORANGE, DrRetroNES::NES_COLOR_DARK_MAGENTA };
  case DR_CHARACTER_WALUIGI: /* 00 14 37 0F */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_MEDIUM_MAGENTA, DrRetroNES::NES_COLOR_PALE_ORANGE, DrRetroNES::NES_COLOR_BLACK };
  case DR_CHARACTER_YOSHI: /* 00 1B 1A 20 */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_MEDIUM_SPRING, DrRetroNES::NES_COLOR_MEDIUM_GREEN, DrRetroNES::NES_COLOR_WHITE };
  case DR_CHARACTER_DONKEY_KONG: /* 00 07 37 07 */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_DARK_ORANGE, DrRetroNES::NES_COLOR_PALE_ORANGE, DrRetroNES::NES_COLOR_DARK_ORANGE };
  case DR_CHARACTER_PEACH: /* 00 25 37 15 */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_LIGHT_ROSE, DrRetroNES::NES_COLOR_PALE_ORANGE, DrRetroNES::NES_COLOR_MEDIUM_ROSE };
  case DR_CHARACTER_DAISY: /* 00 27 37 17 */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_LIGHT_ORANGE, DrRetroNES::NES_COLOR_PALE_ORANGE, DrRetroNES::NES_COLOR_MEDIUM_ORANGE };
  case DR_CHARACTER_BOO: /* 00 20 30 00 */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_WHITE, DrRetroNES::NES_COLOR_PALE_GRAY, DrRetroNES::NES_COLOR_DARK_GRAY };
  case DR_CHARACTER_TOAD: /* 00 26 30 16 */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_LIGHT_RED, DrRetroNES::NES_COLOR_PALE_GRAY, DrRetroNES::NES_COLOR_MEDIUM_RED };
  case DR_CHARACTER_KOOPA_KID: /* 00 0A 27 0B */
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_DARK_GREEN, DrRetroNES::NES_COLOR_LIGHT_ORANGE, DrRetroNES::NES_COLOR_DARK_SPRING };
  case DR_CHARACTER_MARIO: /* 00 16 36 12 */
  default:
    return { DrRetroNES::NES_COLOR_DARK_GRAY, DrRetroNES::NES_COLOR_MEDIUM_RED, DrRetroNES::NES_COLOR_PALE_RED, DrRetroNES::NES_COLOR_MEDIUM_BLUE };
  }
}

void SuperMarioBros3::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;

  m_retro->loadState(state());

  /* Write battle map */
  uint8_t map = static_cast<uint8_t>(data.minigame->minigame_id);
  m_retro->writeValueForFrames(map, SMB3_VS_MAP, 30);

  /* Recolor the two players */
  const size_t ppuAddr[2] = { SMB3_VS_MARIO_PPU_ADDR, SMB3_VS_LUIGI_PPU_ADDR };
  m_slotToPlayer[0] = m_slotToPlayer[1] = -1;
  core()->input()->clearPortRoutes();
  unsigned slot = 0;
  for (unsigned i = 0; i < 4 && slot < 2; i++)
  {
    if (!dr_team_type_participates(m_players[i].team_type))
      continue;
    smb3_player_colors_t colors = smb3_color(m_players[i].character);
    m_retro->writeForFrames(ppuAddr[slot], &colors, sizeof(colors), 30);
    m_slotToPlayer[slot] = static_cast<int>(i);

    /* Route this NES slot (0 = Mario, 1 = Luigi) to read the player's physical controller port */
    if (m_players[i].control_port < DR_CONTROL_PORT_SIZE)
      core()->input()->setPortRoute(slot,
        static_cast<int>(m_players[i].control_port));

    slot++;
  }

  /* Mute for the first 30 frames to hide a beep */
  muteForFrames(30);
  startMinigame();
}

void SuperMarioBros3::run()
{
  if (!m_minigameActive)
    return;

  m_minigameFrames++;

  /* Wait some frames before waiting for the game to end */
  int64_t over = 0;
  if (m_minigameFrames >= 30 &&
      m_retro->readValue(&over, SMB3_VS_GAME_OVER) == DR_OK && over)
    finishMinigame();
}

dr_minigame_result_t SuperMarioBros3::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  int64_t winner = 0;

  /* 0=Mario win, 1=Luigi won */
  if (m_retro->readValue(&winner, SMB3_VS_WINNER) == DR_OK &&
      winner < 2 && m_slotToPlayer[winner] == static_cast<int>(index))
    result.coins = 10;

  return result;
}
