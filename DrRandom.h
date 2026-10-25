#ifndef DR_RANDOM_H
#define DR_RANDOM_H

#include <cstdint>

class DrRandom
{
public:
  static DrRandom &instance(void);

  /// Restarts the sequence from `seed`.
  void seed(unsigned seed);

  /// The next value, 0-0x7FFF.
  int next(void);

  /// How many values have been drawn since the last seed.
  unsigned long count(void) const { return m_count; }

  /// Read/restore the raw state, so a netplay hard resync can realign the
  /// sequence (which is not part of the emulator savestate).
  unsigned state(void) const { return m_state; }
  void setState(unsigned state, unsigned long count);

private:
  DrRandom(void) = default;
  DrRandom(const DrRandom &) = delete;
  DrRandom &operator=(const DrRandom &) = delete;

  uint32_t m_state = 1u;
  unsigned long m_count = 0;
};

#endif
