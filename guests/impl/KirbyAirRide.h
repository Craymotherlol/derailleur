#ifndef KIRBY_AIR_RIDE_H
#define KIRBY_AIR_RIDE_H

#include "../DrGuestGcn.h"
#include <string>

class KirbyAirRide : public DrGuestGcn
{
  Q_OBJECT

public:
  KirbyAirRide(QObject *parent = nullptr);

  const char *name() const override { return "Kirby Air Ride"; }
  dr_guest id() const override { return DR_GUEST_KIRBYAIRRIDE; }
  const char *state(void) const override { return "kar"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  void applyPlayers();

  int m_minigameFrames = 0;
  bool m_finishPending = false;
};

#endif
