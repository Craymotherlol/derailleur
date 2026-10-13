#ifndef DR_GUEST_MARIO_PARTY_9_H
#define DR_GUEST_MARIO_PARTY_9_H

#include "../DrGuestWii.h"
#include <string>

class MarioParty9 : public DrGuestWii
{
  Q_OBJECT

public:
  MarioParty9(QObject *parent = nullptr);

  const char *name() const override { return "Mario Party 9"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY9; }
  const char *state(void) const override { return "mp9"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;
  dr_sync_validator_t syncValidator(void) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  int m_minigameFrames = 0;
  bool m_finishScheduled = false;
  /// False until the start button is clicked and we swap off the pointer layout.
  bool m_controlsApplied = false;
  int64_t m_partyPointsStart = 0;
};

#endif
