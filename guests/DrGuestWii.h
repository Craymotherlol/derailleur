#ifndef DR_GUEST_WII_H
#define DR_GUEST_WII_H

#include "DrGuestDolphin.h"
#include "../DrRetroWii.h"

/// Base for a guest running a Wii game, held in a DrRetroWii
class DrGuestWii : public DrGuestDolphin
{
  Q_OBJECT

public:
  /**
   * @param shortname A name, within 8 characters, used to uniquely identify
   *   this Dolphin instance
   * @param disc The disc filename without extension
   */
  DrGuestWii(const QString &subdir, const std::string &disc, QObject *parent = nullptr);

protected:
  /// Controller type put in ports 1-4 before the first frame.
  void setControllerType(DrRetroWii::dr_dolphin_controller type) { m_controllerType = type; }

  /// Applies a mini-game's Wii control layouts (quirks dolphin.control and
  /// control_team) to all four players' joypads.
  void applyControlRemap(dr_emulation_quirk_t quirks, const dr_player_t players[4]);

  /// Applies one Wii control layout to all four joypads.
  void applyControlProfile(dr_wii_control control);

private:
  DrRetroWii::dr_dolphin_controller m_controllerType = DrRetroWii::DR_DOLPHIN_CONTROLLER_DEFAULT;
};

#endif
