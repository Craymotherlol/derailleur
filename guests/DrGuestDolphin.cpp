#include "DrGuestDolphin.h"

#include <QDir>
#include <QFile>
#include <QRetro.h>
#include <QRetroDirectories.h>

static QString dr_dolphin_disc_path(const QString &base)
{
  if (QFile::exists(base + ".rvz"))
    return base + ".rvz";
  if (QFile::exists(base + ".iso"))
    return base + ".iso";
  return QString();
}

DrGuestDolphin::DrGuestDolphin(
  DrRetroDolphin *retro, const std::string &disc, bool ownDirs, QObject *parent)
  : DrGuestRetro(parent)
  , m_dolphin(retro)
{
  const QString subdir = retro->subdir();
  const QString base = QString::fromStdString(disc);
  const QString discPath = dr_dolphin_disc_path(base);
  QRetroDirectories *dirs;

  m_dolphin->setParent(this);
  m_dolphin->setCore(new QRetro());
  setRetro(m_dolphin);
  dr_apply_global_core_options(core(), DR_CORE_DOLPHIN);
  m_retro->setCorePath(dr_core_path(DR_CORE_DOLPHIN).toStdString());

  /* Pretend not to support gyro/accel so the core doesn't try to use it */
  core()->setEnvironmentCallbackSupported(
    RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE, false);

  dirs = core()->directories();
  if (ownDirs)
  {
    const QString system =
      QString::fromUtf8(dirs->get(QRetroDirectories::System)) + "/" + subdir;
    const QString save =
      QString::fromUtf8(dirs->get(QRetroDirectories::Save)) + "/" + subdir;

    QDir().mkpath(system);
    QDir().mkpath(save);
    dirs->set(QRetroDirectories::System, system);
    dirs->set(QRetroDirectories::Save, save);
  }

  if (discPath.isEmpty())
  {
    log(DR_LOG_ERROR, qPrintable(QString("rom not found (tried .rvz, .iso): %1").arg(base)));
    m_valid = false;
  }
  else
    m_retro->setGamePath(discPath.toStdString());
}
