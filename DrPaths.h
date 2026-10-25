#ifndef DR_PATHS_H
#define DR_PATHS_H

#include <QString>

#include "DrCommon.h"

/**
 * Where derailleur finds its ROMs, cores, savestates and saves. Loaded from
 * derailleur.ini at startup; a netplay client points the save directory at the
 * host's shipped saves for the session.
 */
class DrPaths
{
public:
  static DrPaths &instance(void);

  QString romsDirectory(void) const { return m_roms; }
  void setRomsDirectory(const QString &path) { m_roms = path; }

  QString coresDirectory(void) const { return m_cores; }
  void setCoresDirectory(const QString &path) { m_cores = path; }

  QString stateDirectory(void) const { return m_state; }
  void setStateDirectory(const QString &path) { m_state = path; }

  QString saveDirectory(void) const { return m_save; }
  void setSaveDirectory(const QString &path) { m_save = path; }

  /// The library for `core` in the cores directory; empty for an unknown core.
  QString corePath(dr_core core) const;

private:
  DrPaths(void) = default;
  DrPaths(const DrPaths &) = delete;
  DrPaths &operator=(const DrPaths &) = delete;

  QString m_roms = "roms";
  QString m_cores = "cores";
  QString m_state = "state";
  QString m_save = "save";
};

#endif
