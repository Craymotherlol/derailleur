#ifndef DR_SETTINGS_H
#define DR_SETTINGS_H

#include <QByteArray>

/// resident_guests value that never unloads a guest.
#define DR_RESIDENT_GUESTS_ALL 0xFFFFFFFFu

typedef enum
{
  DR_SETTING_LOADING_OVERLAY = 0,
  DR_SETTING_MUTE_WHILE_LOADING,
  DR_SETTING_CHARACTER_INJECTION,
  DR_SETTING_RES_SCALE_N64,
  DR_SETTING_RES_SCALE_GCN,
  DR_SETTING_RES_SCALE_DS,
  DR_SETTING_WIDESCREEN_HACK,
  DR_SETTING_RESIDENT_GUESTS,

  DR_SETTING_SIZE
} dr_setting_id;

typedef enum
{
  DR_SETTING_TYPE_BOOL = 0,
  DR_SETTING_TYPE_UNSIGNED
} dr_setting_type;

/// One setting, as derailleur.ini and netplay see it.
typedef struct
{
  /// Key under "settings/" in derailleur.ini.
  const char *name;

  dr_setting_type type;

  /// Value until the ini sets one; bools are 0 or 1.
  unsigned default_value;

  /// A netplay client uses the host's value for the session (never saved).
  bool host_inherited;

  /// Greyed out once a game is running.
  bool runtime_locked;
} dr_setting_t;

class DrSettings
{
public:
  static DrSettings &instance(void);

  /// Every setting, indexed by dr_setting_id.
  static const dr_setting_t table[DR_SETTING_SIZE];

  void load(void);
  void save(void) const;

  /// A setting's value as an unsigned (bools are 0 or 1).
  unsigned value(dr_setting_id id) const { return m_values[id]; }
  void setValue(dr_setting_id id, unsigned value);

  /// Show the loading overlay (frozen frame + bouncing icon, see DrOverlay) that
  /// covers core swaps; off shows the raw swap instead.
  bool loadingOverlay(void) const { return m_values[DR_SETTING_LOADING_OVERLAY] != 0; }

  /// Mute a guest's core while it boots and loads, up until its mini-game starts.
  bool muteWhileLoading(void) const { return m_values[DR_SETTING_MUTE_WHILE_LOADING] != 0; }

  /// Build Waluigi and Daisy into the Mario Party 1 and 2 ROMs (via partystuffer)
  /// so a player who picked them appears as themselves rather than a stand-in.
  /// It swaps those guests onto a boot-patch path that reaches the mini-game by
  /// booting a rebuilt ROM instead of loading the shipped savestate.
  bool characterInjection(void) const { return m_values[DR_SETTING_CHARACTER_INJECTION] != 0; }

  /// Internal resolution multiplier per system, 1 = that system's native. The
  /// core options they drive are per-core; see dr_apply_global_core_options.
  unsigned resScaleN64(void) const { return m_values[DR_SETTING_RES_SCALE_N64]; }
  unsigned resScaleGcn(void) const { return m_values[DR_SETTING_RES_SCALE_GCN]; }
  unsigned resScaleDs(void) const { return m_values[DR_SETTING_RES_SCALE_DS]; }

  /// Force every core to render 16:9 rather than the games' native 4:3 (see
  /// dr_apply_global_core_options). Expect stretched HUDs and pop-in at the edges.
  bool widescreenHack(void) const { return m_values[DR_SETTING_WIDESCREEN_HACK] != 0; }

  /// How many of the most recently played guests keep their emulator loaded
  /// between mini-games; older ones are unloaded and boot again when next picked.
  unsigned residentGuests(void) const { return m_values[DR_SETTING_RESIDENT_GUESTS]; }

  /// Whether the session is using a netplay host's inherited settings.
  bool fromHost(void) const { return m_fromHost; }

  /// The host_inherited settings, as [u8 dr_setting_id][u32 value] pairs.
  QByteArray encodeInherited(void) const;

  /// Uses the host's host_inherited settings for the session without saving them
  /// over the user's own; clearHost restores theirs.
  void applyHost(const QByteArray &payload);
  void clearHost(void);

private:
  DrSettings(void);
  DrSettings(const DrSettings &) = delete;
  DrSettings &operator=(const DrSettings &) = delete;

  unsigned m_values[DR_SETTING_SIZE];

  /* The user's own values while a host's are in use */
  unsigned m_ownValues[DR_SETTING_SIZE];
  bool m_fromHost = false;
};

#endif
