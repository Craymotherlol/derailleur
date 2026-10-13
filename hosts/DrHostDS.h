#ifndef DR_HOST_DS_H
#define DR_HOST_DS_H

#include "DrHostRetro.h"
#include "../DrRetroDS.h"

/// Base for a host running a Nintendo DS game, held in a DrRetroDS.
class DrHostDS : public DrHostRetro
{
  Q_OBJECT

public:
  explicit DrHostDS(QObject *parent = nullptr)
    : DrHostRetro(parent)
  {
    setRetro(new DrRetroDS(this));
  }

  dr_host_platform platform(void) const override { return DR_HOST_PLATFORM_DS; }
};

#endif
