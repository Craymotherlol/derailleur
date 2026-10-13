#include "DrHostPicker.h"
#include "DrStartup.h"
#include "hosts/DrHosts.h"

#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

#define DR_PICKER_COLUMNS 4
#define DR_PICKER_ICON_W 160
#define DR_PICKER_ICON_H 120

static QStringList dr_missing_files(const QString &rom, dr_core core)
{
  QStringList missing;

  if (!QFile::exists(rom))
    missing.append(QDir::toNativeSeparators(rom));
  if (core != DR_CORE_INVALID && !QFile::exists(dr_core_path(core)))
    missing.append(QDir::toNativeSeparators(dr_core_path(core)));

  return missing;
}

/* Grayed title screen with a circled "?" in the corner */
static QIcon dr_missing_icon(const QIcon &base, const QToolButton *btn)
{
  QPixmap pm = base.pixmap(QSize(DR_PICKER_ICON_W, DR_PICKER_ICON_H), QIcon::Disabled);
  const qreal w = pm.width() / pm.devicePixelRatio();
  const qreal h = pm.height() / pm.devicePixelRatio();
  const qreal d = 28;
  const QRectF circle(w - d - 4, h - d - 4, d, d);
  QPainter p(&pm);
  QFont font = btn->font();

  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(Qt::white, 2));
  p.setBrush(btn->palette().color(QPalette::Highlight));
  p.drawEllipse(circle);
  font.setBold(true);
  font.setPixelSize(static_cast<int>(d * 0.65));
  p.setFont(font);
  p.drawText(circle, Qt::AlignCenter, QStringLiteral("?"));
  p.end();

  return QIcon(pm);
}

DrHostPicker::DrHostPicker(QWidget *parent)
  : QScrollArea(parent)
{
  QVBoxLayout *layout;
  QLabel *label;

  m_page = new QWidget(this);
  layout = new QVBoxLayout(m_page);
  label = new QLabel(tr("Choose Host"), m_page);
  m_grid = new QGridLayout;

  label->setAlignment(Qt::AlignCenter);
  layout->addStretch();
  layout->addWidget(label);
  layout->addLayout(m_grid);
  layout->addStretch();

  for (const DrHostEntry *entry = dr_host_entries(); entry->game != DR_GAME_INVALID; entry++)
  {
    if (!entry->title)
      continue;
    addButton(QIcon(QString(":/assets/titlescreen/%1.png").arg(entry->title)),
      entry->name, dr_host_rom_path(entry), entry->core,
      [this, entry]() { emit hostChosen(entry, QString(), QString()); });
  }

  for (const DrCustomRom &rom : dr_custom_roms())
  {
    const DrHostEntry *entry = dr_host_entry(rom.game);

    if (!entry)
      continue;
    addButton(rom.icon.isEmpty()
        ? QIcon(QString(":/assets/titlescreen/%1.png").arg(entry->title))
        : QIcon(rom.icon),
      rom.name, rom.path, entry->core,
      [this, entry, rom]() { emit hostChosen(entry, rom.path, rom.name); });
  }

  setWidget(m_page);
  setWidgetResizable(true);
  setFrameShape(QFrame::NoFrame);
}

void DrHostPicker::addButton(const QIcon &icon, const QString &name, const QString &rom,
  dr_core core, std::function<void()> start)
{
  QToolButton *btn = new QToolButton(m_page);
  auto refresh = [btn, icon, rom, core]() {
    const QStringList missing = dr_missing_files(rom, core);

    btn->setIcon(missing.isEmpty() ? icon : dr_missing_icon(icon, btn));
    return missing;
  };

  btn->setText(name);
  btn->setIconSize(QSize(DR_PICKER_ICON_W, DR_PICKER_ICON_H));
  btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  refresh();

  connect(btn, &QToolButton::clicked, this, [this, name, refresh, start]() {
    const QStringList missing = refresh();

    if (missing.isEmpty())
      start();
    else
      QMessageBox::warning(this, name,
        tr("%1 can't be started because these files are missing:\n\n%2")
          .arg(name, missing.join("\n")));
  });

  m_grid->addWidget(btn, m_count / DR_PICKER_COLUMNS, m_count % DR_PICKER_COLUMNS);
  m_count++;
}
