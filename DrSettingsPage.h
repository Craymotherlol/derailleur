#ifndef DR_SETTINGS_PAGE_H
#define DR_SETTINGS_PAGE_H

#include <QWidget>

#include "DrSettings.h"

class QCheckBox;
class QComboBox;

/**
 * The "Settings" tool page: user-facing options backed by DrSettings. Toggling a
 * control updates the setting and persists it to derailleur.ini immediately.
 */
class DrSettingsPage : public QWidget
{
  Q_OBJECT

public:
  explicit DrSettingsPage(QWidget *parent = nullptr);

  /// Re-reads every control from DrSettings, and greys out the host_inherited
  /// ones while this is a netplay client (see DrSettings::table).
  void setNetplayClient(bool client);

  /// Greys out the runtime_locked settings while a game is running.
  void setGameRunning(bool running);

signals:
  /// The user asked to re-download the save files (see DrDownloader::downloadSaves).
  void redownloadSavesRequested();

  /// The user changed a setting (already written and saved).
  void settingChanged(dr_setting_id id);

private:
  /// A checkbox bound to a bool setting.
  QCheckBox *settingCheckbox(QWidget *parent, dr_setting_id id, const QString &text,
    const QString &tooltip);

  /// A resolution dropdown bound to an unsigned setting: entry N is labelled for
  /// N times `native_height`.
  QComboBox *settingScaleCombo(QWidget *parent, dr_setting_id id, unsigned native_height,
    unsigned max_scale, unsigned recommended);

  /// Shows `id`'s current value in its control without emitting.
  void refreshControl(dr_setting_id id);

  /// Enables each control unless its setting is held by the host or the running game.
  void updateLocks(void);

  QWidget *m_controls[DR_SETTING_SIZE] = {};
  bool m_netplayClient = false;
  bool m_gameRunning = false;
};

#endif
