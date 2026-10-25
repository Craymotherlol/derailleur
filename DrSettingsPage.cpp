#include "DrSettingsPage.h"

#include "DrCommon.h"
#include "DrRetro.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

QCheckBox *DrSettingsPage::settingCheckbox(QWidget *parent, dr_setting_id id,
  const QString &text, const QString &tooltip)
{
  QCheckBox *box = new QCheckBox(text, parent);

  box->setChecked(DrSettings::instance().value(id) != 0);
  box->setToolTip(tooltip);
  connect(box, &QCheckBox::toggled, this, [this, id](bool on) {
    DrSettings::instance().setValue(id, on ? 1 : 0);
    DrSettings::instance().save();
    emit settingChanged(id);
  });
  m_controls[id] = box;

  return box;
}

QComboBox *DrSettingsPage::settingScaleCombo(QWidget *parent, dr_setting_id id,
  unsigned native_height, unsigned max_scale, unsigned recommended)
{
  QComboBox *combo = new QComboBox(parent);

  for (unsigned n = 1; n <= max_scale; n++)
  {
    QString label = tr("%1x for %2p").arg(n).arg(n * native_height);

    if (n == recommended)
      label += tr(" (recommended)");
    combo->addItem(label, n);
  }
  m_controls[id] = combo;
  refreshControl(id);

  /* A value from a newer build, or a hand-edited ini, would not be in the list. */
  if (combo->currentIndex() < 0)
    combo->setCurrentIndex(static_cast<int>(recommended) - 1);
  connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    [this, id, combo](int index) {
      DrSettings::instance().setValue(id, combo->itemData(index).toUInt());
      DrSettings::instance().save();
      emit settingChanged(id);
    });

  return combo;
}

void DrSettingsPage::refreshControl(dr_setting_id id)
{
  QWidget *control = m_controls[id];
  const QSignalBlocker blocker(control);

  if (QCheckBox *box = qobject_cast<QCheckBox *>(control))
    box->setChecked(DrSettings::instance().value(id) != 0);
  else if (QComboBox *combo = qobject_cast<QComboBox *>(control))
    combo->setCurrentIndex(combo->findData(DrSettings::instance().value(id)));
}

void DrSettingsPage::updateLocks(void)
{
  unsigned i;

  for (i = 0; i < DR_SETTING_SIZE; i++)
    if (m_controls[i])
      m_controls[i]->setEnabled(
        !(m_netplayClient && DrSettings::table[i].host_inherited) &&
        !(m_gameRunning && DrSettings::table[i].runtime_locked));
}

void DrSettingsPage::setNetplayClient(bool client)
{
  unsigned i;

  m_netplayClient = client;
  for (i = 0; i < DR_SETTING_SIZE; i++)
    if (m_controls[i])
      refreshControl(static_cast<dr_setting_id>(i));
  updateLocks();
}

void DrSettingsPage::setGameRunning(bool running)
{
  m_gameRunning = running;
  updateLocks();
}

/* A muted, wrapped caption under the control it explains. */
static QLabel *settings_note(QWidget *parent, const QString &text)
{
  QLabel *note = new QLabel(text, parent);

  note->setWordWrap(true);
  note->setEnabled(false); /* muted */

  return note;
}

DrSettingsPage::DrSettingsPage(QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  QGroupBox *graphics = new QGroupBox(tr("Graphics"), this);
  QVBoxLayout *graphicsLayout = new QVBoxLayout(graphics);

  graphicsLayout->addWidget(settingCheckbox(graphics, DR_SETTING_WIDESCREEN_HACK,
    tr("Widescreen hack"),
    tr("Render every game at 16:9 instead of the 4:3 they were made for.\n"
       "Stretches some HUDs and lets objects pop in at the edges of the screen.")));
  graphicsLayout->addWidget(settings_note(graphics,
    tr("Play Nintendo 64 and GameCube games in simulated widescreen.")));

  /* Native heights: N64 240p, GameCube/Wii 480p, DS 192p per screen. */
  QGroupBox *resolution = new QGroupBox(tr("Internal resolution"), graphics);
  QVBoxLayout *resolutionLayout = new QVBoxLayout(resolution);
  QFormLayout *resolutions = new QFormLayout();

  resolutions->addRow(tr("Nintendo 64"),
    settingScaleCombo(resolution, DR_SETTING_RES_SCALE_N64, 240, 8, 2));
  resolutions->addRow(tr("GameCube / Wii"),
    settingScaleCombo(resolution, DR_SETTING_RES_SCALE_GCN, 480, 6, 1));
  resolutions->addRow(tr("Nintendo DS"),
    settingScaleCombo(resolution, DR_SETTING_RES_SCALE_DS, 192, 8, 2));
  resolutionLayout->addLayout(resolutions);
  resolutionLayout->addWidget(settings_note(resolution,
    tr("Higher costs GPU time, and a few mini-games "
       "force their own resolution to render correctly.")));
  graphicsLayout->addWidget(resolution);
  layout->addWidget(graphics);

  QGroupBox *game = new QGroupBox(tr("Game"), this);
  QVBoxLayout *gameLayout = new QVBoxLayout(game);

  gameLayout->addWidget(settingCheckbox(game, DR_SETTING_CHARACTER_INJECTION,
    tr("BETA: Waluigi and Daisy in Mario Party 1 and 2"),
    tr("Play as injected characters in Mario Party and Mario Party 2 Mini-Games. Expect issues!")));
  gameLayout->addWidget(settings_note(
    game, tr("Play as injected characters in Mario Party and Mario Party 2 Mini-Games. Expect issues!")));
  layout->addWidget(game);

  QGroupBox *debug = new QGroupBox(tr("Debug"), this);
  QVBoxLayout *debugLayout = new QVBoxLayout(debug);

  debugLayout->addWidget(settingCheckbox(debug, DR_SETTING_LOADING_OVERLAY,
    tr("Show the loading overlay"),
    tr("Cover the switch between games with the last frame and a loading icon.\n"
       "Turn this off to see the cores swapping underneath.")));
  debugLayout->addWidget(settingCheckbox(debug, DR_SETTING_MUTE_WHILE_LOADING,
    tr("Mute game while loading"),
    tr("Silence a mini-game's core until the mini-game itself starts.\n"
       "Turn this off to hear it boot underneath the loading overlay.")));
  layout->addWidget(debug);

  QPushButton *redownloadSaves = new QPushButton(tr("Re-download saves"), this);
  redownloadSaves->setToolTip(
    tr("Fetch the latest save files from the server, overwriting your local ones.\n"
       "Saves are otherwise downloaded only on the first launch."));
  connect(redownloadSaves, &QPushButton::clicked, this,
    [this]() { emit redownloadSavesRequested(); });
  layout->addWidget(redownloadSaves);

  layout->addStretch();

  connect(this, &DrSettingsPage::settingChanged, this, [](dr_setting_id id) {
    if (id == DR_SETTING_WIDESCREEN_HACK || id == DR_SETTING_RES_SCALE_N64 ||
        id == DR_SETTING_RES_SCALE_GCN || id == DR_SETTING_RES_SCALE_DS)
      dr_reapply_global_core_options();
  });
}
