#ifndef DR_RETRO_N64_H
#define DR_RETRO_N64_H

#include "DrRetro.h"

/// Helper class for a DrRetro instance that plays a Nintendo 64 game
class DrRetroN64 : public DrRetro
{
  Q_OBJECT

public:
  DrRetroN64(QObject *parent = nullptr)
    : DrRetro(parent)
  {
    m_endianness = DR_ENDIANNESS_WORDFLIPPED;
  }

  void setCore(QRetro *core) override
  {
    DrRetro::setCore(core);
    if (!m_core)
      return;

    m_core->options()->setOptionValue("mupen64plus-EnableTexCoordBounds", "True");

    /* Assign C-Buttons to individual libretro buttons */
    m_core->options()->setOptionValue("mupen64plus-alt-map", "True");

    /* Use Rumble Pak by default */
    m_core->options()->setOptionValue("mupen64plus-pak1", "rumble");
    m_core->options()->setOptionValue("mupen64plus-pak2", "rumble");
    m_core->options()->setOptionValue("mupen64plus-pak3", "rumble");
    m_core->options()->setOptionValue("mupen64plus-pak4", "rumble");

    /* Allow custom textures (for games that use asset replacement) */
    m_core->options()->setOptionValue("mupen64plus-txHiresEnable", "True");
    m_core->options()->setOptionValue("mupen64plus-txHiresFullAlphaChannel", "True");

    for (unsigned port = 0; port < DR_CONTROL_PORT_SIZE; port++)
    {
      /**
       * libretro Mupen64 lays out the Nintendo 64 buttons in an odd order.
       * We normalize them here so they feel natural across system shifts.
       */
      m_core->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_Y); // B = B
      m_core->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_B); // A = A
      m_core->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_A); // Y = C??

      m_core->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_L, RETRO_DEVICE_ID_JOYPAD_L2); // L1 = Z
      m_core->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_R, RETRO_DEVICE_ID_JOYPAD_R2); // R1 = R
    }
  }
};

#endif
