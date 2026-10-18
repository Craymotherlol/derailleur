#ifndef DR_GUEST_STAR_FOX_64_H
#define DR_GUEST_STAR_FOX_64_H

#include "../DrGuestN64.h"

class StarFox64 : public DrGuestN64
{
  Q_OBJECT

public:
  StarFox64(QObject *parent = nullptr);

  const char *name() const override { return "Star Fox 64"; }
  dr_guest id() const override { return DR_GUEST_STARFOX64; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "Star Fox 64 (USA) (Rev 1).z64"; }
  const char *state() const override { return "starfox64"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;
  void onBeforeBoot(const DrGameData &data) override;

private:
  /* Writes the character's head into in-game player slot `slot`'s hires icon. */
  void writePlayerIcon(unsigned slot, dr_character character);

  int m_minigameFrames = 0;
  int m_slotToIndex[4] = { -1, -1, -1, -1 }; /* in-game player slot -> board player index */
  unsigned m_winners = 0;                    /* bitmask of winning board player indices */
  int m_formOption[4] = { 0, 0, 0, 0 };      /* per slot: starting vehicle to pick (sf64_form_option), 0 = none */
  int m_soloSlot = -1;                       /* 1v3: in-game slot of the solo player, else -1 */
  bool m_shieldsPending = false;             /* Battle Royal: cut shields once play starts */
};

#endif
