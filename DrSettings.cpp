#include "DrSettings.h"

#include <QDataStream>
#include <QDir>
#include <QSettings>

const dr_setting_t DrSettings::table[DR_SETTING_SIZE] = {
  { "loading_overlay", DR_SETTING_TYPE_BOOL, 1, false, false },
  { "mute_while_loading", DR_SETTING_TYPE_BOOL, 1, false, false },
  { "character_injection", DR_SETTING_TYPE_BOOL, 0, true, true },
  { "res_scale_n64", DR_SETTING_TYPE_UNSIGNED, 2, false, false },
  { "res_scale_gcn", DR_SETTING_TYPE_UNSIGNED, 1, false, false },
  { "res_scale_ds", DR_SETTING_TYPE_UNSIGNED, 2, false, false },
  { "widescreen_hack", DR_SETTING_TYPE_BOOL, 0, false, false },
  { "resident_guests", DR_SETTING_TYPE_UNSIGNED, 0, true, false },
};

static QString settings_ini_path(void)
{
  return QDir::current().filePath("derailleur.ini");
}

static QString settings_key(const dr_setting_t *setting)
{
  return QString("settings/") + setting->name;
}

DrSettings &DrSettings::instance(void)
{
  static DrSettings settings;

  return settings;
}

DrSettings::DrSettings(void)
{
  unsigned i;

  for (i = 0; i < DR_SETTING_SIZE; i++)
    m_values[i] = m_ownValues[i] = table[i].default_value;
}

void DrSettings::setValue(dr_setting_id id, unsigned value)
{
  m_values[id] = (table[id].type == DR_SETTING_TYPE_BOOL) ? (value != 0) : value;
}

void DrSettings::load(void)
{
  QSettings s(settings_ini_path(), QSettings::IniFormat);
  unsigned i;

  for (i = 0; i < DR_SETTING_SIZE; i++)
  {
    const QVariant value = s.value(settings_key(&table[i]), m_values[i]);

    setValue(static_cast<dr_setting_id>(i),
      table[i].type == DR_SETTING_TYPE_BOOL ? value.toBool() : value.toUInt());
  }
}

void DrSettings::save(void) const
{
  QSettings s(settings_ini_path(), QSettings::IniFormat);
  unsigned i;

  for (i = 0; i < DR_SETTING_SIZE; i++)
  {
    const unsigned value = (m_fromHost && table[i].host_inherited) ? m_ownValues[i] : m_values[i];

    if (table[i].type == DR_SETTING_TYPE_BOOL)
      s.setValue(settings_key(&table[i]), value != 0);
    else
      s.setValue(settings_key(&table[i]), value);
  }
  s.sync();
}

QByteArray DrSettings::encodeInherited(void) const
{
  QByteArray payload;
  QDataStream s(&payload, QIODevice::WriteOnly);
  unsigned i;

  s.setByteOrder(QDataStream::LittleEndian);
  for (i = 0; i < DR_SETTING_SIZE; i++)
    if (table[i].host_inherited)
      s << static_cast<quint8>(i) << static_cast<quint32>(m_values[i]);

  return payload;
}

void DrSettings::applyHost(const QByteArray &payload)
{
  QDataStream s(payload);
  unsigned i;

  s.setByteOrder(QDataStream::LittleEndian);
  if (!m_fromHost)
    for (i = 0; i < DR_SETTING_SIZE; i++)
      m_ownValues[i] = m_values[i];
  m_fromHost = true;

  while (!s.atEnd())
  {
    quint8 id = 0;
    quint32 value = 0;

    s >> id >> value;
    if (s.status() != QDataStream::Ok)
      break;
    if (id < DR_SETTING_SIZE && table[id].host_inherited)
      setValue(static_cast<dr_setting_id>(id), value);
  }
}

void DrSettings::clearHost(void)
{
  unsigned i;

  if (!m_fromHost)
    return;
  for (i = 0; i < DR_SETTING_SIZE; i++)
    if (table[i].host_inherited)
      m_values[i] = m_ownValues[i];
  m_fromHost = false;
}
