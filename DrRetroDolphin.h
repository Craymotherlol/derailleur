#ifndef DR_RETRO_DOLPHIN_H
#define DR_RETRO_DOLPHIN_H

#include "DrRetro.h"
#include <QString>

/// DrRetro for a Dolphin instance: patches the library with a unique memory-arena
/// name before dlopening it, then forces the options Dolphin needs.
class DrRetroDolphin : public DrRetro
{
  Q_OBJECT

public:
  DrRetroDolphin(const QString &subdir, QObject *parent = nullptr)
    : DrRetro(parent)
    , m_subdir(subdir)
  {
  }

  /// The name of this instance, used for its memory arena and own directories.
  const QString &subdir(void) const { return m_subdir; }

  /// Extra options set when the library opens. Terminated by a null key.
  void setOptions(const dr_core_option_t *options) { m_options = options; }
  bool loadCore() override;

private:
  QString m_subdir;
  const dr_core_option_t *m_options = nullptr;
};

#endif
