#ifndef DR_GUEST_MARIO_PARTY_8_H
#define DR_GUEST_MARIO_PARTY_8_H

#include "../DrGuestWii.h"
#include <string>

class MarioParty8 : public DrGuestWii
{
  Q_OBJECT

public:
  MarioParty8(QObject *parent = nullptr);

  const char *name() const override { return "Mario Party 8"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY8; }
  const char *state(void) const override { return "mp8"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;
  dr_sync_validator_t syncValidator(void) override;

protected:
  void run() override;

  void doApplyGameData(const DrGameData &data) override;

private:
  int m_minigameFrames = 0;
  int32_t m_lastScene = -1;
};

#endif
