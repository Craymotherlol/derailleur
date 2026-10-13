#ifndef DR_GUEST_GCN_H
#define DR_GUEST_GCN_H

#include "DrGuestDolphin.h"
#include "../DrRetroGcn.h"

/// Base for a guest running a GameCube game, held in a DrRetroGcn.
class DrGuestGcn : public DrGuestDolphin
{
  Q_OBJECT

public:
  /// `subdir` names the Dolphin instance; `disc` is the disc path without its
  /// extension.
  DrGuestGcn(const QString &subdir, const std::string &disc, QObject *parent = nullptr);
};

#endif
