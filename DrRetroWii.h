#ifndef DR_RETRO_WII_H
#define DR_RETRO_WII_H

#include "DrRetroDolphin.h"

/// Helper class for a DrRetro instance that plays a Wii game
class DrRetroWii : public DrRetroDolphin
{
  Q_OBJECT

public:
  /**
   * Controller type the Dolphin core puts in each port. These match the core's
   * controller descriptions (see controllerDesc), since its device ids
   * overlap between the GameCube and Wii lists.
   */
  typedef enum
  {
    DR_DOLPHIN_CONTROLLER_DEFAULT = 0,

    DR_DOLPHIN_CONTROLLER_GAMECUBE,
    DR_DOLPHIN_CONTROLLER_WIIMOTE,
    DR_DOLPHIN_CONTROLLER_WIIMOTE_SIDEWAYS,
    DR_DOLPHIN_CONTROLLER_WIIMOTE_NUNCHUK,
    DR_DOLPHIN_CONTROLLER_WIIMOTE_CLASSIC,
    DR_DOLPHIN_CONTROLLER_WIIMOTE_CLASSIC_PRO,

    DR_DOLPHIN_CONTROLLER_SIZE
  } dr_dolphin_controller;

  /// The Dolphin core's description for `type`, or null for the core's default.
  static const char *controllerDesc(dr_dolphin_controller type)
  {
    switch (type)
    {
    case DR_DOLPHIN_CONTROLLER_GAMECUBE:
      return "GameCube Controller";
    case DR_DOLPHIN_CONTROLLER_WIIMOTE:
      return "WiiMote";
    case DR_DOLPHIN_CONTROLLER_WIIMOTE_SIDEWAYS:
      return "WiiMote (sideways)";
    case DR_DOLPHIN_CONTROLLER_WIIMOTE_NUNCHUK:
      return "WiiMote + Nunchuk";
    case DR_DOLPHIN_CONTROLLER_WIIMOTE_CLASSIC:
      return "WiiMote + Classic Controller";
    case DR_DOLPHIN_CONTROLLER_WIIMOTE_CLASSIC_PRO:
      return "WiiMote + Classic Controller Pro";
    default:
      return nullptr;
    }
  }

  DrRetroWii(const QString &subdir, QObject *parent = nullptr)
    : DrRetroDolphin(subdir, parent)
  {
  }
};

#endif
