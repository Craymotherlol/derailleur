#include "DrGuest.h"

#include <QDataStream>
#include <QFile>
#include <QIODevice>
#include <QRetro.h>

/* Key layout: [63:56] game, [55:48] type, [47:32] scene_id, [31:0] minigame_id. */
dr_minigame_key_t dr_minigame_key_make(
  dr_guest game, dr_minigame_type type, signed minigame_id, signed scene_id)
{
  return (static_cast<quint64>(game & 0xFF) << 56)
    | (static_cast<quint64>(type & 0xFF) << 48)
    | (static_cast<quint64>(static_cast<quint16>(scene_id)) << 32)
    | static_cast<quint64>(static_cast<quint32>(minigame_id));
}

dr_minigame_key_t dr_minigame_key(dr_guest game, const dr_mp_minigame_t *mg)
{
  return dr_minigame_key_make(game, mg->type, mg->minigame_id, mg->scene_id);
}

dr_guest dr_minigame_key_game(dr_minigame_key_t key)
{
  return static_cast<dr_guest>((key >> 56) & 0xFF);
}

dr_minigame_type dr_minigame_key_type(dr_minigame_key_t key)
{
  return static_cast<dr_minigame_type>((key >> 48) & 0xFF);
}

signed dr_minigame_key_scene(dr_minigame_key_t key)
{
  return static_cast<qint16>((key >> 32) & 0xFFFF);
}

signed dr_minigame_key_id(dr_minigame_key_t key)
{
  return static_cast<qint32>(key & 0xFFFFFFFF);
}

QByteArray dr_minigame_filter_encode(const QSet<dr_minigame_key_t> &disabled)
{
  QByteArray out;
  QDataStream s(&out, QIODevice::WriteOnly);

  s.setByteOrder(QDataStream::LittleEndian);
  s << static_cast<quint16>(disabled.size());
  for (dr_minigame_key_t key : disabled)
    s << key;

  return out;
}

QSet<dr_minigame_key_t> dr_minigame_filter_decode(const QByteArray &payload)
{
  QSet<dr_minigame_key_t> disabled;
  QDataStream s(payload);
  quint16 count = 0;
  quint16 i;

  s.setByteOrder(QDataStream::LittleEndian);
  s >> count;
  for (i = 0; i < count && s.status() == QDataStream::Ok; i++)
  {
    dr_minigame_key_t key = 0;

    s >> key;
    if (s.status() == QDataStream::Ok)
      disabled.insert(key);
  }

  return disabled;
}

void DrGuest::applyGameData(const DrGameData &in)
{
  DrGameData data = in;
  unsigned used = 0;
  unsigned i;

  /* Guests index by port, so fall back to board order on a bad or repeated one */
  for (i = 0; i < 4; i++)
  {
    const unsigned port = data.players[i].control_port;

    if (port >= DR_CONTROL_PORT_SIZE || (used & (1u << port)))
      break;
    used |= 1u << port;
  }
  if (i < 4)
  {
    log(DR_LOG_WARN, "invalid controller ports, using board order");
    for (i = 0; i < 4; i++)
      data.players[i].control_port = i;
  }

  cancelMinigame();
  m_finishCountdown = 0;

  m_minigame = data.minigame;

  for (i = 0; i < 4; i++)
    m_players[i] = data.players[i];
  m_hostPlatform = data.host_platform;

  boot(data);
}

void DrGuest::tick()
{
  if (m_finishCountdown > 0 && --m_finishCountdown == 0)
  {
    finishMinigame();
    return;
  }
  run();
}

void DrGuest::startMinigame()
{
  m_finishCountdown = 0;
  m_minigameActive = true;

  /* Hold on this frame until the launcher makes it the netplay context */
  if (core())
    core()->pause();

  emit minigameStarted();
}

void DrGuest::finishMinigame()
{
  if (m_minigameActive)
  {
    m_minigameActive = false;
    emit minigameFinished();
  }
}
