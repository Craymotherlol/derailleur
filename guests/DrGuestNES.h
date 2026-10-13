#ifndef DR_GUEST_NES_H
#define DR_GUEST_NES_H

#include "DrGuestRetro.h"
#include "../DrRetroNES.h"

/// Base for a guest running an NES game, held in a DrRetroNES.
class DrGuestNES : public DrGuestRetro
{
  Q_OBJECT

public:
  DrGuestNES(QObject *parent = nullptr)
    : DrGuestRetro(parent)
  {
    setRetro(new DrRetroNES(this));
  }

  unsigned bootFrames() const override { return 16; }
};

#endif
