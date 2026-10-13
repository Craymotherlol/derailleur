#ifndef DR_RETRO_NES_H
#define DR_RETRO_NES_H

#include "DrRetro.h"

/// Helper class for a DrRetro instance that plays an NES game
class DrRetroNES : public DrRetro
{
  Q_OBJECT

public:
  /* NES PPU palette colors, for recoloring tiles */
  typedef enum
  {
    /* Grayscale */
    NES_COLOR_BLACK = 0x0F,
    NES_COLOR_DARK_GRAY = 0x00,
    NES_COLOR_LIGHT_GRAY = 0x10, /* silver */
    NES_COLOR_WHITE = 0x20,
    NES_COLOR_PALE_GRAY = 0x30,

    /* Dark ($0x) -- medium mixed with black */
    NES_COLOR_DARK_AZURE = 0x01,
    NES_COLOR_DARK_BLUE = 0x02,
    NES_COLOR_DARK_VIOLET = 0x03,
    NES_COLOR_DARK_MAGENTA = 0x04,
    NES_COLOR_DARK_ROSE = 0x05,
    NES_COLOR_DARK_RED = 0x06, /* maroon */
    NES_COLOR_DARK_ORANGE = 0x07,
    NES_COLOR_DARK_YELLOW = 0x08, /* olive */
    NES_COLOR_DARK_CHARTREUSE = 0x09,
    NES_COLOR_DARK_GREEN = 0x0A,
    NES_COLOR_DARK_SPRING = 0x0B,
    NES_COLOR_DARK_CYAN = 0x0C,

    /* Medium ($1x) */
    NES_COLOR_MEDIUM_AZURE = 0x11,
    NES_COLOR_MEDIUM_BLUE = 0x12,
    NES_COLOR_MEDIUM_VIOLET = 0x13,
    NES_COLOR_MEDIUM_MAGENTA = 0x14,
    NES_COLOR_MEDIUM_ROSE = 0x15,
    NES_COLOR_MEDIUM_RED = 0x16,
    NES_COLOR_MEDIUM_ORANGE = 0x17,
    NES_COLOR_MEDIUM_YELLOW = 0x18,
    NES_COLOR_MEDIUM_CHARTREUSE = 0x19,
    NES_COLOR_MEDIUM_GREEN = 0x1A,
    NES_COLOR_MEDIUM_SPRING = 0x1B,
    NES_COLOR_MEDIUM_CYAN = 0x1C,

    /* Light ($2x) */
    NES_COLOR_LIGHT_AZURE = 0x21,
    NES_COLOR_LIGHT_BLUE = 0x22,
    NES_COLOR_LIGHT_VIOLET = 0x23,
    NES_COLOR_LIGHT_MAGENTA = 0x24,
    NES_COLOR_LIGHT_ROSE = 0x25,
    NES_COLOR_LIGHT_RED = 0x26,
    NES_COLOR_LIGHT_ORANGE = 0x27,
    NES_COLOR_LIGHT_YELLOW = 0x28,
    NES_COLOR_LIGHT_CHARTREUSE = 0x29,
    NES_COLOR_LIGHT_GREEN = 0x2A,
    NES_COLOR_LIGHT_SPRING = 0x2B,
    NES_COLOR_LIGHT_CYAN = 0x2C,

    /* Pale ($3x) -- light mixed with white */
    NES_COLOR_PALE_AZURE = 0x31,
    NES_COLOR_PALE_BLUE = 0x32,
    NES_COLOR_PALE_VIOLET = 0x33,
    NES_COLOR_PALE_MAGENTA = 0x34,
    NES_COLOR_PALE_ROSE = 0x35,
    NES_COLOR_PALE_RED = 0x36,
    NES_COLOR_PALE_ORANGE = 0x37,
    NES_COLOR_PALE_YELLOW = 0x38,
    NES_COLOR_PALE_CHARTREUSE = 0x39,
    NES_COLOR_PALE_GREEN = 0x3A,
    NES_COLOR_PALE_SPRING = 0x3B,
    NES_COLOR_PALE_CYAN = 0x3C
  } dr_nes_color;

  DrRetroNES(QObject *parent = nullptr)
    : DrRetro(parent)
  {
    m_endianness = DR_ENDIANNESS_BIG;
  }
};

#endif
