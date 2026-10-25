#include "DrRandom.h"

DrRandom &DrRandom::instance(void)
{
  static DrRandom random;

  return random;
}

void DrRandom::seed(unsigned seed)
{
  m_state = static_cast<uint32_t>(seed);
  m_count = 0;
}

int DrRandom::next(void)
{
  m_count++;
  m_state = m_state * 1103515245u + 12345u;
  return static_cast<int>((m_state >> 16) & 0x7FFF);
}

void DrRandom::setState(unsigned state, unsigned long count)
{
  m_state = static_cast<uint32_t>(state);
  m_count = count;
}
