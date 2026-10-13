#include "DrStartup.h"
#include "DrDownloader.h"
#include "DrLogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

#ifndef DR_GIT_HASH
#define DR_GIT_HASH unknown
#endif
#ifndef DR_GIT_HASH_FULL
#define DR_GIT_HASH_FULL unknown
#endif
#define DR_STRINGIZE_(x) #x
#define DR_STRINGIZE(x) DR_STRINGIZE_(x)

#define DR_INI_FILE "derailleur.ini"

/* Copies one image over every texture slot listed, under Dolphin's load dir */
struct DrTextureAsset
{
  const char *dir;
  const char *source;
  const char *const *files;
};

static const char *const MP6_BATTLE_ICON_FILE[] = {
  "tex1_128x128_2bb955ca50dd26a6_14.png",
  "tex1_128x128_2f5e8d147ce5d7f5_14.png",
  "tex1_128x128_560c420a6be5402f_14.png",
  "tex1_128x128_722f65211251356e_14.png",
  "tex1_128x128_d3d80b92eda45477_14.png",
  "tex1_128x128_f75626c325e81470_14.png",

  nullptr
};

static const char *const MP7_BATTLE_ICON_FILE[] = {
  "tex1_128x104_2e3485dc3d6f3eb6_eea1d86ba1bfcd04_9.png",
  "tex1_128x104_a8173cf855b51046_b52e7bf246f8a16b_9.png",
  "tex1_128x104_aa12f37d6f11d30a_93ab94e37f5aabf6_9.png",
  "tex1_128x104_ddbc2b4774edce92_79b97c505c3eb0f3_9.png",
  "tex1_128x104_e2943b1807a2fede_fc117f3666cb6b2e_9.png",

  nullptr
};

static const char *const MP8_BATTLE_ICON_FILE[] = {
  "tex1_224x168_16c7623064db6ff8_14.png",

  nullptr
};

static const DrTextureAsset DR_TEXTURE_ASSETS[] = {
  { "GP6E01", ":/assets/minigame-icon/battle-mp6.png", MP6_BATTLE_ICON_FILE },
  { "GP7E01", ":/assets/minigame-icon/battle-mp7.png", MP7_BATTLE_ICON_FILE },
  { "RM8E01", ":/assets/minigame-icon/battle-mp8.png", MP8_BATTLE_ICON_FILE },

  { nullptr, nullptr, nullptr }
};

static void dr_attach_downloader(DrDownloader *downloader, DrLogger *logger)
{
  if (!logger)
    return;
  logger->attach(downloader, Qt::AutoConnection);
  QObject::connect(downloader, &DrDownloader::progressStarted, logger, &DrLogger::showProgress);
  QObject::connect(downloader, &DrDownloader::progressFinished, logger, &DrLogger::hideProgress);
  QObject::connect(downloader, &DrDownloader::progressUpdated, logger, &DrLogger::setProgress);
}

static void dr_log(DrLogger *logger, unsigned level, const QString &message)
{
  if (logger)
    logger->message(level, message);
}

static void dr_load_paths(DrLogger *logger)
{
  const QDir cwd = QDir::current();
  const QString ini = cwd.filePath(DR_INI_FILE);
  const bool existed = QFile::exists(ini);
  QSettings s(ini, QSettings::IniFormat);
  auto load = [&](const char *key, const QString &def) {
    if (!s.contains(key))
      s.setValue(key, def);
    return s.value(key).toString();
  };

  dr_set_roms_directory(load("paths/roms", cwd.filePath("roms")));
  dr_set_cores_directory(load("paths/cores", cwd.filePath("cores")));
  dr_set_state_directory(load("paths/state", cwd.filePath("state")));
  dr_set_save_directory(load("paths/save", cwd.filePath("save")));
  s.sync();
  dr_settings_load();

  dr_log(logger, DR_LOG_INFO, QString("%1 %2").arg(existed ? "loaded" : "created", ini));
  dr_log(logger, DR_LOG_INFO, QString("paths/roms: %1").arg(dr_roms_directory()));
  dr_log(logger, DR_LOG_INFO, QString("paths/cores: %1").arg(dr_cores_directory()));
  dr_log(logger, DR_LOG_INFO, QString("paths/state: %1").arg(dr_state_directory()));
  dr_log(logger, DR_LOG_INFO, QString("paths/save: %1").arg(dr_save_directory()));

  DrDownloader downloader;
  dr_attach_downloader(&downloader, logger);
  downloader.runBlocking(s, dr_save_directory(), dr_state_directory());
}

static void dr_install_textures(DrLogger *logger)
{
  for (const DrTextureAsset *asset = DR_TEXTURE_ASSETS; asset->dir; asset++)
  {
    const QString dir = dr_save_directory() + "/User/Load/Textures/" + asset->dir;
    unsigned copied = 0;

    if (!QDir().mkpath(dir))
    {
      dr_log(logger, DR_LOG_WARN, QString("textures: cannot create %1").arg(dir));
      continue;
    }

    for (const char *const *file = asset->files; *file; file++)
    {
      const QString dest = dir + "/" + *file;

      QFile::remove(dest);
      if (QFile::copy(asset->source, dest))
      {
        QFile::setPermissions(dest,
          QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
        copied++;
      }
      else
        dr_log(logger, DR_LOG_WARN, QString("textures: failed to write %1").arg(dest));
    }

    dr_log(logger, DR_LOG_INFO,
      QString("textures: %1 file(s) into %2").arg(copied).arg(asset->dir));
  }
}

static void dr_clear_mupen_cache(DrLogger *logger)
{
  QDir cache("system/Mupen64plus/cache");

  if (!cache.exists())
    return;
  if (cache.removeRecursively())
    dr_log(logger, DR_LOG_INFO, QString("cleared %1").arg(cache.path()));
  else
    dr_log(logger, DR_LOG_WARN, QString("failed to clear %1").arg(cache.path()));
}

void dr_startup(DrLogger *logger)
{
  dr_log(logger, DR_LOG_INFO, QString("derailleur built %1 %2").arg(__DATE__, __TIME__));
  dr_log(logger, DR_LOG_INFO, QString("build revision %1").arg(DR_STRINGIZE(DR_GIT_HASH)));
  dr_log(logger, DR_LOG_INFO, QString("git hash %1").arg(DR_STRINGIZE(DR_GIT_HASH_FULL)));
  dr_log(logger, DR_LOG_INFO,
    QString("started %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));

  dr_load_paths(logger);
  dr_install_textures(logger);
  dr_clear_mupen_cache(logger);
}

void dr_redownload_saves(DrLogger *logger)
{
  QSettings s(QDir::current().filePath(DR_INI_FILE), QSettings::IniFormat);
  DrDownloader downloader;

  dr_attach_downloader(&downloader, logger);
  downloader.downloadSaves(s, dr_save_directory(), dr_state_directory());
}

QList<DrCustomRom> dr_custom_roms(void)
{
  QList<DrCustomRom> roms;

  for (int mp = 1; mp <= 8; mp++)
  {
    const dr_game game = static_cast<dr_game>(DR_GAME_MARIOPARTY1 + mp - 1);
    const QStringList filters =
      mp <= 3 ? QStringList{ "*.z64" } : QStringList{ "*.rvz", "*.iso" };
    QDir dir(dr_roms_directory() + QString("/custom/mp%1").arg(mp));

    if (!dir.exists())
      continue;

    for (const QFileInfo &info : dir.entryInfoList(filters, QDir::Files, QDir::Name))
    {
      const QString png = dir.filePath(info.completeBaseName() + ".png");

      roms.append({ game, info.absoluteFilePath(), info.completeBaseName(),
        QFile::exists(png) ? png : QString() });
    }
  }

  return roms;
}

QString dr_custom_rom_path(dr_game game, const QString &name)
{
  for (const DrCustomRom &rom : dr_custom_roms())
    if (rom.game == game && rom.name == name)
      return rom.path;

  return QString();
}
