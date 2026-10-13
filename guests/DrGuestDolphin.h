#ifndef DR_GUEST_DOLPHIN_H
#define DR_GUEST_DOLPHIN_H

#include "DrGuestRetro.h"
#include "../DrRetroDolphin.h"
#include <string>

/// Base for a game running in its own Dolphin instance. Use DrGuestGcn or
/// DrGuestWii rather than this directly.
class DrGuestDolphin : public DrGuestRetro
{
  Q_OBJECT

public:
  /// Takes ownership of `retro`, whose subdir names the instance. `disc` is the
  /// disc path without its extension (.rvz or .iso is found). `ownDirs` gives the
  /// instance its own system and save directories under the subdir.
  DrGuestDolphin(DrRetroDolphin *retro, const std::string &disc, bool ownDirs,
    QObject *parent = nullptr);

  dr_core coreId(void) const override { return DR_CORE_DOLPHIN; }
  unsigned bootFrames(void) const override { return 120; }

protected:
  /// Extra core options set when the library opens. Terminated by a null key.
  void setOptions(const dr_core_option_t *options) { m_dolphin->setOptions(options); }

private:
  DrRetroDolphin *m_dolphin = nullptr;
};

#endif
