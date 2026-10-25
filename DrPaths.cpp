#include "DrPaths.h"

DrPaths &DrPaths::instance(void)
{
  static DrPaths paths;

  return paths;
}

QString DrPaths::corePath(dr_core core) const
{
  switch (core)
  {
  case DR_CORE_MUPEN64PLUSNEXT:
    return m_cores + "/mupen64plus_next_libretro" + dr_os_extension();
  case DR_CORE_DOLPHIN:
    return m_cores + "/dolphin_libretro" + dr_os_extension();
  case DR_CORE_MGBA:
    return m_cores + "/mgba_libretro" + dr_os_extension();
  case DR_CORE_FLYCAST:
    return m_cores + "/flycast_libretro" + dr_os_extension();
  case DR_CORE_FCEUMM:
    return m_cores + "/fceumm_libretro" + dr_os_extension();
  case DR_CORE_SNES9X:
    return m_cores + "/snes9x_libretro" + dr_os_extension();
  case DR_CORE_MELONDSDS:
    return m_cores + "/melondsds_libretro" + dr_os_extension();
  default:
    return {};
  }
}
