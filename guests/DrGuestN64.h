#ifndef DR_GUEST_N64_H
#define DR_GUEST_N64_H

#include "DrGuestRetro.h"
#include "../DrRetroN64.h"

/// Base for a guest running a Nintendo 64 game, held in a DrRetroN64.
class DrGuestN64 : public DrGuestRetro
{
  Q_OBJECT

public:
  DrGuestN64(QObject *parent = nullptr)
    : DrGuestRetro(parent)
  {
    setRetro(new DrRetroN64(this));
  }

  unsigned bootFrames() const override { return 16; }
};

#endif
