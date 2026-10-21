#ifndef DR_GUEST_WII_PARTY_H
#define DR_GUEST_WII_PARTY_H

#include "../DrGuestWii.h"

#include <QSet>

class WiiParty : public DrGuestWii
{
  Q_OBJECT

public:
  WiiParty(QObject *parent = nullptr);

  const char *name() const override { return "Wii Party"; }
  dr_guest id() const override { return DR_GUEST_WIIPARTY; }
  const char *state(void) const override { return "wiiparty"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;

protected:
  void run() override;
  void doApplyGameData(const DrGameData &data) override;

private:
  /// Address of the mini-game's REL in the OS module list, or 0 if not loaded.
  size_t moduleAddress(unsigned module_id);

  /// Writes our characters' colors over the game's per-player color table.
  void writeColors(void);

  /// Finds the module's own copy of the player color table and recolors it.
  void writeModuleColors(size_t module);

  /// Recolors every module that has loaded since the last call; with `recolor`
  /// false it only records the modules already loaded.
  void recolorNewModules(bool recolor);

  /// Picks each player's game slot and team. Duelists go in slots 0 and 1.
  void assignSlots(void);

  /// Writes our players into the game's player table.
  void writePlayers(void);

  /// Patches the explanation screen so it doesn't offer "back" to freeplay.
  void disableBackButton(void);
  void logResults(void);

  int m_minigameFrames = 0;
  bool m_finishScheduled = false;
  /// True once the mini-game's REL has been seen loaded; it unloading means it's over.
  bool m_moduleSeen = false;
  /// Frames left before the deferred netplay hard resync; 0 = none pending.
  int m_resyncCountdown = 0;
  /// Game slot and team for each board player, from assignSlots()
  unsigned m_slotOf[4] = { 0, 1, 2, 3 };
  int32_t m_teamOf[4] = { 0, 1, 2, 3 };
  /// Ids of loaded modules already recolored; dropped when they unload so a reload is recolored
  QSet<unsigned> m_recoloredModules;
};

#endif
