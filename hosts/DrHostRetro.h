#ifndef DR_HOST_RETRO_H
#define DR_HOST_RETRO_H

#include "../DrHost.h"
#include "../DrRetro.h"

/// Base for a host powered by a libretro core, held in a DrRetro.
class DrHostRetro : public DrHost
{
  Q_OBJECT

public:
  explicit DrHostRetro(QObject *parent = nullptr)
    : DrHost(parent)
  {
  }

  QRetro *core() const override { return m_retro ? m_retro->core() : nullptr; }
  bool isValid() const override { return m_valid && (!m_retro || m_retro->isValid()); }

  /// The board's content path. Defaults to the libretro core's.
  std::string gamePath() const override { return m_retro ? m_retro->gamePath() : std::string{}; }

  /// Starts the core and fits it to its widget once it is running.
  void startCore() override;
  void pause() override { if (m_retro) m_retro->pause(); }
  void unpause() override { if (m_retro) m_retro->unpause(); }

  /// A window container around the libretro core.
  QWidget *createWidget(QWidget *parent) override
  {
    return m_retro ? m_retro->createWidget(parent) : nullptr;
  }

protected:
  /// Installs the host's libretro core wrapper and forwards its log. Call from
  /// the constructor.
  void setRetro(DrRetro *retro);

  DrRetro *m_retro = nullptr;
};

#endif
