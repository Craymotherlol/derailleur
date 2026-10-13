#ifndef DR_GUEST_RETRO_H
#define DR_GUEST_RETRO_H

#include "../DrGuest.h"
#include "../DrRetro.h"

/// Base for a guest powered by a libretro core, held in a DrRetro.
class DrGuestRetro : public DrGuest
{
  Q_OBJECT

public:
  DrGuestRetro(QObject *parent = nullptr)
    : DrGuest(parent)
  {
  }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  bool isValid() const override { return m_valid && (!m_retro || m_retro->isValid()); }

  /// Frames to let the core boot after its first content load before the base
  /// invokes doApplyGameData(). Override per core as needed.
  virtual unsigned bootFrames() const { return 30; }

  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }
  QWidget *createWidget(QWidget *parent) override
  {
    return m_retro ? m_retro->createWidget(parent) : nullptr;
  }

protected:
  /// Applies the mini-game's quirks, then boots the core and applies `data`
  /// once it has run bootFrames().
  void boot(const DrGameData &data) override;

  /**
   * Any  behavior that gets done before loading the game itself,
   * ie. putting custom assets in place
   */
  virtual void onBeforeBoot(const DrGameData &data) { (void)data; }

  /// Keeps the core muted for the next `frames` frames, e.g. to hide a boot or
  /// transition blip, then unmutes it if the mini-game is running.
  void muteForFrames(int frames) { m_muteFrames = frames; }

  /// Installs the guest's libretro core wrapper and forwards its log. Call from
  /// the constructor.
  void setRetro(DrRetro *retro);
  DrRetro *m_retro = nullptr;

private:
  /// Per-frame work done for every retro guest before tick(): pending frame
  /// writes and loading/blip muting.
  void frameBegin(void);

  DrGameData m_pendingData; // launch data awaiting the boot countdown
  bool m_tickHookInstalled = false;
  int m_muteFrames = 0;
};

#endif
