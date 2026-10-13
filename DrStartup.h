#ifndef DR_STARTUP_H
#define DR_STARTUP_H

#include "DrCommon.h"
#include <QList>
#include <QString>

class DrLogger;

/// A modified host ROM found under roms/custom/mpN.
struct DrCustomRom
{
  dr_game game;
  QString path;
  QString name; /* basename; what travels over netplay */
  QString icon; /* sibling .png, or empty */
};

/// Loads paths and settings from derailleur.ini, fetches missing data, and
/// prepares the save and system directories. Call once before anything else.
void dr_startup(DrLogger *logger);

/// Downloads the saves and states again, replacing the local copies.
void dr_redownload_saves(DrLogger *logger);

QList<DrCustomRom> dr_custom_roms(void);

/// Path of the custom ROM `name` for `game`, or empty if it isn't installed.
QString dr_custom_rom_path(dr_game game, const QString &name);

#endif
