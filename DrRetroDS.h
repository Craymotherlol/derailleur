#ifndef DR_RETRO_DS_H
#define DR_RETRO_DS_H

#include "DrRetro.h"

/// Helper class for a DrRetro instance that plays a Nintendo DS game
class DrRetroDS : public DrRetro
{
  Q_OBJECT

public:
  DrRetroDS(QObject *parent = nullptr)
    : DrRetro(parent)
  {
  }

  void setCore(QRetro *core) override
  {
    DrRetro::setCore(core);
    if (!m_core)
      return;

    /* Disallow changing the screen layout by default */
    m_core->options()->setOptionValue("melonds_number_of_screen_layouts", "1");
    m_core->options()->setOptionValue("melonds_render_mode", "opengl");
    m_core->options()->setOptionValue("melonds_screen_layout1", "left-right");

    /* Shutup stuff in the log that fires every frame */
    m_core->options()->setOptionValue("melonds_show_bios_warnings", "disabled");
    m_core->options()->setOptionValue("melonds_show_camera_state", "disabled");
    m_core->options()->setOptionValue("melonds_show_current_layout", "disabled");
    m_core->options()->setOptionValue("melonds_show_lid_state", "disabled");
    m_core->options()->setOptionValue("melonds_show_mic_state", "disabled");
    m_core->options()->setOptionValue("melonds_show_sensor_reading", "disabled");
    m_core->options()->setOptionValue("melonds_show_unsupported_features", "disabled");
  }
};

#endif
