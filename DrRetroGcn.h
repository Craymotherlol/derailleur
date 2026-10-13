#ifndef DR_RETRO_GCN_H
#define DR_RETRO_GCN_H

#include "DrRetroDolphin.h"

/// Helper class for a DrRetro instance that plays a GameCube game
class DrRetroGcn : public DrRetroDolphin
{
  Q_OBJECT

public:
  DrRetroGcn(const QString &subdir, QObject *parent = nullptr)
    : DrRetroDolphin(subdir, parent)
  {
  }
};

#endif
