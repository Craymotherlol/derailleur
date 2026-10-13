#ifndef DR_GUEST_H
#define DR_GUEST_H

#include "DrCommon.h"
#include <cstdio>
#include <string>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QWidget>

class QRetro;

/* Stable mini-game filter key: game, type, scene_id and minigame_id packed together. */
typedef quint64 dr_minigame_key_t;

dr_minigame_key_t dr_minigame_key(dr_guest game, const dr_mp_minigame_t *mg);
dr_guest dr_minigame_key_game(dr_minigame_key_t key);
dr_minigame_type dr_minigame_key_type(dr_minigame_key_t key);
signed dr_minigame_key_scene(dr_minigame_key_t key);
signed dr_minigame_key_id(dr_minigame_key_t key);
dr_minigame_key_t dr_minigame_key_make(
  dr_guest game, dr_minigame_type type, signed minigame_id, signed scene_id);

/* Disabled mini-game set as a netplay payload: [u16 count][count * u64 key]. */
QByteArray dr_minigame_filter_encode(const QSet<dr_minigame_key_t> &disabled);
QSet<dr_minigame_key_t> dr_minigame_filter_decode(const QByteArray &payload);

/// The full setup for one minigame launch, delivered to a guest in a single
/// call: which minigame, its type, and all four players. Extend with board
/// context as guests need it.
struct DrGameData
{
  const dr_mp_minigame_t *minigame = nullptr;
  dr_minigame_type type = DR_MINIGAME_INVALID;
  dr_player_t players[4] = {};

  /// The console family of the host this launch came from, so a guest drawing
  /// the board's players picks the matching art set (see dr_player_icon_32px).
  dr_host_platform host_platform = DR_HOST_PLATFORM_INVALID;

  /// Coins the board collected for a battle mini-game (see DrHost::battlePot).
  /// 0 for every other mini-game type.
  unsigned battle_pot = 0;
};

class DrGuest : public QObject
{
  Q_OBJECT

public:
  DrGuest(QObject *parent = nullptr)
    : QObject(parent)
  {
  }

  /// The guest's libretro core, or null for a guest that isn't libretro-powered.
  virtual QRetro *core() const { return nullptr; }
  virtual bool isValid() const { return m_valid; }

  virtual void pause() {}
  virtual void unpause() {}
  virtual QWidget *createWidget(QWidget *parent) { (void)parent; return nullptr; }

  /// Per-frame driver: runs the finish countdown, then the guest's run().
  void tick();

  virtual dr_minigame_result_t minigameResult(unsigned index) = 0;

  virtual dr_sync_validator_t syncValidator(void) { return { 0, 0 }; }
  virtual const dr_mp_minigame_t *minigames() const = 0;
  virtual const char *name(void) const = 0;
  virtual dr_guest id(void) const { return DR_GUEST_INVALID; }

  /// The libretro core this guest uses, if any. See `dr_core`
  virtual dr_core coreId(void) const { return DR_CORE_INVALID; }

  /// The filename of the ROM this guest uses, if any
  virtual const char *rom(void) const { return ""; }

  /// The filename of the savestate this guest loads, if any
  virtual const char *state(void) const { return ""; }

  void cancelMinigame(void) { m_minigameActive = false; }
  bool minigameActive(void) const { return m_minigameActive; }

  void applyGameData(const DrGameData &data);

protected:
  void log(unsigned level, const char *message)
  {
    /* Also echo to stderr */
    fprintf(stderr, "[guest] %s\n", message);
    emit logMessage(level, QString::fromUtf8(message));
  }
  void startMinigame(void);
  void finishMinigame(void);
  void finishMinigameInFrames(int frames) { m_finishCountdown = frames; }
  /// Whether a finishMinigameInFrames() countdown is running.
  bool finishPending(void) const { return m_finishCountdown > 0; }

  virtual void run() {}

  /**
   * Any behavior that modifies game state based on the board
   * context, done after the guest game has loaded
   */
  virtual void doApplyGameData(const DrGameData &data) = 0;

  /// Brings the game up and applies `data` to it. The default applies it
  /// straight away; a libretro guest boots its core first.
  virtual void boot(const DrGameData &data) { doApplyGameData(data); }

  bool m_valid = true;
  bool m_minigameActive = false;
  int m_finishCountdown = 0;
  const dr_mp_minigame_t *m_minigame = nullptr;

  /* The four players for the current launch, copied from the launch data by
   * applyGameData() before boot() runs. */
  dr_player_t m_players[4] = {};

  /* The host's console family for the current launch, copied alongside the
   * players. Guests that draw the board's characters read it to pick an art set. */
  dr_host_platform m_hostPlatform = DR_HOST_PLATFORM_INVALID;

signals:
  void logMessage(unsigned level, const QString &message);

  void minigameStarted();
  void minigameFinished();
  
  /**
   * Requests netplay to perform a hard resync, loading serializaed state
   * from the host for every client. Use in game-specific instances where a
   * desync is suspected.
   */
  void hardResync(void);

  /**
   * Requests netplay to give input authority to a specific player index.
   * Use in games where players take turns or only one player plays.
   */
  void golfModeRequested(int authorityPlayer);
};

#endif
