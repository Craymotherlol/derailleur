#ifndef DR_GUEST_MARIO_PARTY_GCN_H
#define DR_GUEST_MARIO_PARTY_GCN_H

#include "../DrGuestGcn.h"
#include <string>

struct MpGcnConfig
{
  std::string game;  /* disc path without extension */
  std::string state; /* state name, see DrGuest::state() */

  int scene_miniexplain;
  int scene_miniresults;

  dr_value_t scene;
  dr_value_t minigame;

  /// The game's RNG state, sampled for netplay desync checks. 0 = unknown.
  dr_value_t rng;
  dr_value_t timer;

  dr_value_t character[4];
  dr_value_t controller[4];
  dr_value_t difficulty[4];
  dr_value_t team[4];
  dr_value_t bot[4];

  /// The secondary coin amount gained from a mini-game
  dr_value_t bonus_result[4];

  /// The primary coin amount gained from a mini-game, usually 0 or 10
  dr_value_t result[4];

  /// Board coins and stars, copied in from the host so the mini-game shows what
  /// the board has. Read at the width each game declares. 0 address = don't write.
  dr_value_t coins[4];
  dr_value_t stars[4];

  /// dr_character -> the game's own character id, flagged with whether the game
  /// really has that character or is only lending a slot (see dr_character_id_t).
  dr_character_id_t (*char_from_dr)(dr_character character);

  /// How many characters the game has, numbered from 0. Bounds the search for a
  /// free slot when a stand-in's preferred character is already taken.
  unsigned roster_size;
  const dr_mp_minigame_t *minigames;
};

class MarioPartyGcn : public DrGuestGcn
{
  Q_OBJECT

public:
  MarioPartyGcn(const MpGcnConfig &config, const QString &subdir, QObject *parent = nullptr);

  const char *state(void) const override { return m_config.state.c_str(); }

  dr_minigame_result_t minigameResult(unsigned index) override;
  dr_sync_validator_t syncValidator(void) override;
  const dr_mp_minigame_t *minigames() const override;
  void doApplyGameData(const DrGameData &data) override;

  const MpGcnConfig &config() const { return m_config; }

protected:
  void run() override;
  void applyPlayers();
  MpGcnConfig m_config;
  int32_t m_lastScene = -1;
  int m_minigameFrames = 0;

  int m_slotOf[4] = { 0, 1, 2, 3 }; // board player index -> game slot
};

#endif
