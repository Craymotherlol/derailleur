#ifndef DR_HOST_N64_H
#define DR_HOST_N64_H

#include "DrHostRetro.h"
#include "../DrRetroN64.h"

/// Base for a host running a Nintendo 64 game, held in a DrRetroN64.
class DrHostN64 : public DrHostRetro
{
  Q_OBJECT

public:
  explicit DrHostN64(QObject *parent = nullptr)
    : DrHostRetro(parent)
  {
    setRetro(new DrRetroN64(this));
  }

  dr_host_platform platform(void) const override { return DR_HOST_PLATFORM_N64; }
};

#endif
