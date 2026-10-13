#include "DrRetroDolphin.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRetro.h>
#include <QUuid>

/* Unique memory-arena name for the instance */
static QByteArray dr_dolphin_arena_tag(const QString &subdir)
{
  QByteArray base = subdir.toUtf8().left(8);
  const uint hash = qHash(subdir) & 0xFFF;

  while (base.size() < 8)
    base.append('-');
  return base + QByteArray::number(hash, 16).rightJustified(3, '0').right(3);
}

/* Copies the library with its "dolphin-emu" arena name replaced by the instance's
 * tag. See Source/Core/Common/MemArenaWin.cpp. Returns the copy's path, or empty. */
static QString dr_dolphin_patch_core(const QString &originalPath, const QString &subdir)
{
  const QByteArray needle("dolphin-emu", 12);
  const QByteArray marker("Memory::Init()");
  QFile origFile(originalPath);
  QByteArray data;
  int firstIdx = -1;

  if (!origFile.open(QIODevice::ReadOnly))
    return QString();
  data = origFile.readAll();
  origFile.close();

  for (int from = 0; (from = data.indexOf(needle, from)) >= 0; from++)
  {
    const int windowStart = from + needle.size();
    const int windowEnd = qMin(windowStart + 64, data.size());

    if (data.mid(windowStart, windowEnd - windowStart).contains(marker))
    {
      firstIdx = from;
      break;
    }
  }
  if (firstIdx < 0)
    return QString();

  const QByteArray tag = dr_dolphin_arena_tag(subdir);
  data.replace(firstIdx, tag.size(), tag);

  const QString destPath = QString("%1/dolphin_libretro_%2_%3.%4")
    .arg(QDir::tempPath(), subdir, QUuid::createUuid().toString(QUuid::Id128),
      QFileInfo(originalPath).suffix());
  QFile destFile(destPath);

  if (!destFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return QString();
  destFile.write(data);
  destFile.close();

  return destPath;
}

bool DrRetroDolphin::loadCore()
{
  const QString basePath = QString::fromStdString(corePath());
  const QString patchedPath = dr_dolphin_patch_core(basePath, m_subdir);
  const QString loadPath = patchedPath.isEmpty() ? basePath : patchedPath;
  const bool ok = m_core && m_core->loadCore(loadPath.toUtf8().constData());

  if (!ok)
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(loadPath)));
  else
  {
    /* Serialization needs single-core mode; multiple instances need fastmem off */
    m_core->options()->setOptionValue("dolphin_main_cpu_thread", "disabled");
    m_core->options()->setOptionValue("dolphin_fastmem", "disabled");

    /* No OSD or hotkeys; custom textures on; IR from the stick */
    m_core->options()->setOptionValue("dolphin_hotkey_sideways_toggle", "None");
    m_core->options()->setOptionValue("dolphin_ir_mode", "0");
    m_core->options()->setOptionValue("dolphin_load_custom_textures", "enabled");
    m_core->options()->setOptionValue("dolphin_osd_enabled", "disabled");

    /* Options persist per core in config.ini, so reset ones a guest may override */
    m_core->options()->setOptionValue("dolphin_widescreen", "enabled");

    for (const dr_core_option_t *option = m_options; option && option->key; option++)
    {
      m_core->options()->setOptionValue(option->key, option->value);
      log(DR_LOG_INFO,
        qPrintable(QString("core option: %1 = %2").arg(option->key).arg(option->value)));
    }
  }

  /* QRetro loads from its own copy, so the patched file can go */
  if (!patchedPath.isEmpty())
    QFile::remove(patchedPath);

  return ok;
}
