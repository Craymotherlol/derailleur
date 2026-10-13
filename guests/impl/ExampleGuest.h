#ifndef DR_GUEST_EXAMPLE_GUEST_H
#define DR_GUEST_EXAMPLE_GUEST_H

#include "../DrGuestRetro.h"

class ExampleGuest : public DrGuestRetro
{
  Q_OBJECT

public:
  ExampleGuest(QObject *parent = nullptr);

  const char *name() const override { return "Example Guest"; }
  const char *rom() const override { return ""; }
  const char *state() const override { return ""; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void onBeforeBoot(const DrGameData &data) override;
  void doApplyGameData(const DrGameData &data) override;
  void run() override;

private:
  /* Any variables you want to use would go here */
};

#endif
