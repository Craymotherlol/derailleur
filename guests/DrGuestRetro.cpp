#include "DrGuestRetro.h"

#include <QRetro.h>
#include <QRetroAudio.h>

void DrGuestRetro::boot(const DrGameData &data)
{
  /* Nothing to boot without a libretro core */
  if (!m_retro || !m_retro->core())
  {
    doApplyGameData(data);
    return;
  }

  m_retro->applyQuirks(data.minigame);
  onBeforeBoot(data);

  if (!m_tickHookInstalled)
  {
    m_tickHookInstalled = true;
    connect(core(), &QRetro::frameBegin, this, [this]() { frameBegin(); }, Qt::DirectConnection);
  }

  m_pendingData = data;
  if (!m_retro->boot(bootFrames(), [this]() { doApplyGameData(m_pendingData); }))
    log(DR_LOG_ERROR, "failed to boot core");
}

void DrGuestRetro::frameBegin(void)
{
  QRetroAudio *audio = core()->audio();

  if (!m_retro->booting())
    m_retro->tickFrameWrites();

  /* Silence the core until the mini-game is flagged as having started */
  if (audio)
  {
    if (m_muteFrames > 0)
    {
      audio->setMute(true);
      if (--m_muteFrames == 0 && m_minigameActive)
        audio->setMute(false);
    }
    else if (!m_minigameActive && dr_settings_get().mute_while_loading)
      audio->setMute(true);
  }

  tick();
}

void DrGuestRetro::setRetro(DrRetro *retro)
{
  m_retro = retro;
  if (m_retro)
    connect(m_retro, &DrRetro::logMessage, this, &DrGuest::logMessage);
}
