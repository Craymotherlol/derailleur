#include "DrLogger.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QRetro.h>
#include <memory>
#include <QVBoxLayout>

DrLogger::DrLogger(QWidget *parent)
  : QWidget(parent)
{
  m_text = new QPlainTextEdit(this);
  m_text->setReadOnly(true);
  m_text->setMaximumBlockCount(1000);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_text);

  /* Busy progress row pinned to the bottom, hidden until a task drives it. */
  m_progressRow = new QWidget(this);
  QHBoxLayout *prow = new QHBoxLayout(m_progressRow);
  prow->setContentsMargins(4, 2, 4, 2);
  m_progressLabel = new QLabel(m_progressRow);
  m_progress = new QProgressBar(m_progressRow);
  m_progress->setRange(0, 0); // indeterminate / busy
  m_progress->setTextVisible(false);
  prow->addWidget(m_progressLabel);
  prow->addWidget(m_progress, 1);
  m_progressRow->hide();
  layout->addWidget(m_progressRow);

  m_file.setFileName("derailleur.log");
  if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    qWarning("DrLogger: failed to open derailleur.log");
}

void DrLogger::showProgress(const QString &label)
{
  m_progressLabel->setText(label);
  m_progress->setRange(0, 0); // busy until the first byte-progress arrives
  m_progressRow->show();
}

void DrLogger::hideProgress()
{
  m_progressRow->hide();
}

void DrLogger::setProgress(qint64 received, qint64 total)
{
  if (total > 0)
  {
    /* Scale to a percentage so a huge file can't overflow the int range. */
    m_progress->setRange(0, 100);
    m_progress->setValue(static_cast<int>(received * 100 / total));
  }
  else
    m_progress->setRange(0, 0); // unknown size -> keep it busy
}

/* Lines carrying any of these are dropped before they reach the view or the file. */
static const char *const DR_LOG_NOISE[] = {
  /* Dolphin shader compilation spam */
  "might be used before being initialized",

  /* melonDS DS spam */
  "retro_get_memory_data",

  nullptr
};

bool DrLogger::isNoise(const QString &message)
{
  for (const char *const *noise = DR_LOG_NOISE; *noise; noise++)
    if (message.contains(QLatin1String(*noise)))
      return true;

  return false;
}

void DrLogger::message(unsigned level, const QString &message)
{
  static const char *prefixes[] = { "INFO", "WARN", "ERROR" };
  static const char *colors[] = { "#4a90d9", "#e0a000", "#e05252" };
  const char *prefix = level < 3 ? prefixes[level] : "LOG";
  const char *color = level < 3 ? colors[level] : "#9aa0a6";

  if (message.length() < 20)
    return;

  if (isNoise(message))
    return;

  QString line = QString("[%1] %2").arg(prefix).arg(message);

  /* Color the line by level in the view; the file stays plain text. */
  m_text->appendHtml(
    QString("<span style=\"color:%1;\">%2</span>").arg(color, line.toHtmlEscaped()));

  if (m_file.isOpen())
  {
    m_file.write((line + '\n').toUtf8());
    m_file.flush();
  }
}

static unsigned dr_retro_log_level(int retro_level)
{
  return retro_level >= RETRO_LOG_ERROR ? DR_LOG_ERROR
       : retro_level == RETRO_LOG_WARN  ? DR_LOG_WARN
                                        : DR_LOG_INFO;
}

void DrLogger::attachCore(QRetro *core)
{
  if (!core)
    return;

  qRegisterMetaType<QRetroMessageEntry>();

  /* Collapses repeated lines on the emulation thread */
  auto lastMsg = std::make_shared<QString>();
  auto repeats = std::make_shared<int>(0);
  connect(core, &QRetro::onCoreLog, this,
    [this, lastMsg, repeats](int level, const QString &msg) {
      if (isNoise(msg))
        return;
      if (msg == *lastMsg)
      {
        ++*repeats;
        return;
      }
      const int skipped = *repeats;
      const unsigned lvl = dr_retro_log_level(level);

      *repeats = 0;
      *lastMsg = msg;
      QMetaObject::invokeMethod(this,
        [this, lvl, msg, skipped]() {
          if (skipped > 0)
            message(DR_LOG_INFO, QStringLiteral("[core] (previous line repeated %1x)").arg(skipped));
          message(lvl, QStringLiteral("[core] ") + msg);
        },
        Qt::QueuedConnection);
    },
    Qt::DirectConnection);

  connect(core, &QRetro::onCoreMessage, this,
    [this](const QRetroMessageEntry &entry) {
      message(dr_retro_log_level(entry.level), QStringLiteral("[msg] ") + entry.message);
    },
    Qt::QueuedConnection);
}
