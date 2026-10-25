#ifndef DR_NETPLAY_H
#define DR_NETPLAY_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QWaitCondition>
#include <atomic>
#include <cstdint>
#include <functional>

#include "DrCommon.h"
#include "QRetroInput.h"

class DrInputStore;
class QEvent;
class QRetro;
class QRetroInputBackend;
class QTcpServer;
class QTcpSocket;

typedef enum
{
  DR_NETPLAY_PACKET_INVALID = 0,

  /* server -> client: { peerIndex, peerCount } */
  DR_NETPLAY_PACKET_HANDSHAKE,

  /* server -> client: var: [u8 game][u32 seed][utf8 custom rom] */
  DR_NETPLAY_PACKET_START,

  /* either direction: DrNetplayPacket */
  DR_NETPLAY_PACKET_INPUT,

  /* any peer (relayed): new input delay, 1 byte */
  DR_NETPLAY_PACKET_SET_DELAY,

  /* server -> clients: { context } stop for resync */
  DR_NETPLAY_PACKET_RESYNC_BEGIN,
  
  /* server -> clients: [u32 len][u64 frame][zstate] */
  DR_NETPLAY_PACKET_RESYNC_STATE,

  /* client -> server: build hash on connect */
  DR_NETPLAY_PACKET_VERSION,

  /* server -> clients: var-length disabled-set payload */
  DR_NETPLAY_PACKET_MINIGAME_FILTER,

  /* client -> server: I timed out, please hard-resync */
  DR_NETPLAY_PACKET_RESYNC_REQUEST,

  /* server -> clients: var-length save-directory bundle */
  DR_NETPLAY_PACKET_SAVE,

  /* any peer (relayed): cancel the active mini-game */
  DR_NETPLAY_PACKET_CANCEL,

  /* any peer (relayed): { s8 authority } */
  DR_NETPLAY_PACKET_GOLF,

  /* var: [u64 frame][opaque]; frame 0 = client request */
  DR_NETPLAY_PACKET_LAUNCH,

  /* either direction: var-length [u8 dr_guest]... the sender owns */
  DR_NETPLAY_PACKET_OWNED_GUESTS,

  /* server -> clients: var: [u8 dr_setting_id][u32 value]... host-inherited settings */
  DR_NETPLAY_PACKET_SETTINGS
} dr_netplay_packet_type;

/* Maximum peers in a session (also the per-frame input array width). */
#define DR_NETPLAY_MAX_PEERS 4

/* Maximum QRetro contexts (host + guests) tracked per peer; at most 255. A core past
 * this limit fails attachCore and logs as context -1. */
#define DR_NETPLAY_MAX_CONTEXTS 48

/* Number of frames ahead to schedule a hard resync */
#define DR_NETPLAY_RESYNC_MARGIN 60

/* How many frames ahead of the current active-context frame a debug-menu launch is
 * scheduled, so the broadcast reaches every peer before that gated frame arrives. */
#define DR_NETPLAY_LAUNCH_DELAY 60

/* Under golf mode, the input delay every peer but the authority sends with. */
#define DR_NETPLAY_GOLF_HIGH_DELAY 20

/* Input packet payload: quint64 + quint8 + quint8 + quint16 + 6 * qint16 + validator + quint32. */
#define DR_NETPLAY_PACKET_PAYLOAD_SIZE (8 + 1 + 1 + 2 + 6 * 2 + sizeof(dr_sync_validator_t) + 4)

/* Fixed, null-padded git short hash exchanged on connect for version checks. */
#define DR_NETPLAY_VERSION_HASH_LEN 16

#pragma pack(push, 1)
struct DrNetplayPacket
{
  uint64_t frame; // per-context netplay frame counter (not the libretro counter)
  uint8_t peerIndex;
  uint8_t context; // which QRetro context (host or a guest) this input is for
  uint16_t bitmask;
  int16_t leftX, leftY;
  int16_t rightX, rightY;
  int16_t l2, r2;
  dr_sync_validator_t sync; // context state at the start of frame `syncFrame`; syncFrame 0 = none
  uint32_t syncFrame;
};
#pragma pack(pop)

class DrNetplay : public QObject
{
  Q_OBJECT

public:
  explicit DrNetplay(DrInputStore *store, QObject *parent = nullptr);

  void hostSession(quint16 port, int playerCount);
  void joinSession(const QString &address, quint16 port);

  /// Clears every per-context netplay frame counter and pending input. Called
  /// at session start. Uses internal counters, never the libretro counter.
  void resetFrameCounter();

  /// Selects the foreground QRetro context (host or a guest) — the only context
  /// that is gated. Called at each foreground switch (post-warmup host, or a
  /// minigame load), which acts as that context's per-context sync point.
  void setActiveContext(QRetro *core);

  /// Freezes the currently active context at the current frame: its core stops
  /// advancing (retro_run is held off in onFrameBegin) until it is re-activated.
  /// Connect this to minigameRequested with a DirectConnection so every peer
  /// freezes the host on the same lockstep frame before the async minigame
  /// launch can run a different number of host frames on each peer.
  void freezeActiveContext();

  int inputDelay() const { return m_InputDelay.load(); }

  /// "Golf mode": one player (a peer index) sends with 0 input delay while everyone
  /// else sends with DR_NETPLAY_GOLF_HIGH_DELAY frames, giving that player responsive, priority
  /// authority in turn-based games (e.g. golf). `authorityPlayer` < 0 turns it off.
  /// Purely local (each peer applies it to its own send scheduling, which is
  /// frame-tagged and stays deterministic); safe to call any time, in or out of a
  /// session.
  void setGolfMode(int authorityPlayer);

  /// Applies golf mode locally (see setGolfMode) AND propagates it to every peer, so
  /// one peer can drive it (e.g. the host from the netplay UI). Use this instead of
  /// setGolfMode when the call happens on only one peer rather than in lockstep.
  void broadcastGolfMode(int authorityPlayer);

  /// Changes the input delay (buffer frames) locally and, during a session,
  /// propagates it to every peer (the server relays). Safe to call mid-session:
  /// each context back-fills any send gap so no peer stalls. Callable from the
  /// main (UI) thread.
  void changeInputDelay(int frames);

  bool isServer() const { return m_IsServer; }
  bool active() const { return m_Active; }

  /// Whether a netplay session is active, for code without a DrNetplay handle
  /// (e.g. guests) to gate netplay-only behavior.
  static bool sessionActive(void) { return s_Instance && s_Instance->m_Active; }

  /// Whether this instance is the server of an active netplay session.
  static bool sessionServer(void) { return sessionActive() && s_Instance->m_IsServer; }

  /// Schedules a debug-menu mini-game launch so every peer runs it on the same gated
  /// frame. `payload` is an opaque blob (the app serializes guest/mini-game/players);
  /// the server assigns a target frame DR_NETPLAY_LAUNCH_DELAY ahead and broadcasts it,
  /// a client forwards the request to the server. Every peer emits debugLaunchReady when
  /// its active context reaches that frame. No-op outside a session (caller launches
  /// directly instead).
  void requestDebugLaunch(const QByteArray &payload);

  /// Wakes any timing thread parked in waitForFrame so it can exit. Call before
  /// tearing down cores (e.g. on window close) to avoid blocking shutdown.
  void abort();

  /// Server only: tells every connected client which game to start (a dr_game
  /// value), then begins lockstep locally. `saveFilters` are the host game's save
  /// file wildcards (see DrHost::saveFilePatterns, e.g. "Mario Party 3 (USA).*");
  /// only the files they select are shipped to clients, not the whole save folder.
  /// Empty ships the entire folder (legacy fallback).
  void startGame(int gameId, const QStringList &saveFilters = QStringList(),
    const QString &customRom = QString());

  /// Server only: forces a hard resync of the active context — every client
  /// stops, receives the host's (compressed) savestate, loads it and resumes
  /// in lockstep from a fresh sync point. Safe to call mid-session.
  void requestHardResync();

  /// Trigger a hard resync from any peer: the server does it directly, a client
  /// asks the server to. No-op outside a session. Use when a peer suspects it may
  /// have diverged (e.g. a non-deterministic setup that couldn't be gated).
  void requestResync();

  /// Tell every peer to cancel the active mini-game and return to the board. The
  /// caller cancels locally; this only propagates it. No-op outside a session.
  void broadcastCancelMinigame();

  /// Sets the authoritative allowed-mini-games filter (opaque payload from
  /// DrMinigameFilter). The server broadcasts it to every client and re-sends it
  /// at session start so it is delivered at least once; clients store it for
  /// relay. Emits minigameFilterReceived locally so the caller applies it too.
  void setMinigameFilter(const QByteArray &payload);

  /// Sets the guests (dr_guest values) this install owns, reported to the other
  /// side of each connection. Call before hosting or joining.
  void setOwnedGuests(const QSet<int> &guests);

  /// Server only: sends the settings every peer must share (see DrSettings) to
  /// every client. Call when one of them changes on the host.
  void broadcastSettings(void);

  /// Re-targets an existing physical (e.g. SDL3) backend so it samples local
  /// hardware into our private joypad array instead of a core's array.
  void setLocalSource(QRetroInputBackend *backend);

  /// Installs a QRetroInputBackendShared on `core` and gates its frameBegin. `name`
  /// labels the context in logs (e.g. the guest/host name). `syncProbe` is read
  /// each frame and compared across peers (see checkSyncSample).
  void attachCore(QRetro *core, const QString &name = QString(),
    std::function<dr_sync_validator_t()> syncProbe = {});

protected:
  /// Forwards app-wide keyboard events into m_LocalInput regardless of focus.
  bool eventFilter(QObject *watched, QEvent *event) override;

signals:
  void sessionStarted(int peerIndex, int peerCount);
  void sessionError(const QString &reason);
  void peerCountChanged(int connected, int total);
  void lobbyJoined(int peerIndex, int peerCount);
  /// Emitted on a client when the server selects a game (a dr_game value).
  void startGameRequested(int gameId, const QString &customRom);
  /// Emitted (locally on set, or on a client when the host's filter arrives)
  /// with the opaque allowed-mini-games payload to apply.
  void minigameFilterReceived(QByteArray payload);
  /// Emitted when the input delay changes (locally or from a peer) so the UI
  /// can reflect the new value. Does not re-broadcast.
  void inputDelayChanged(int frames);
  /// A peer cancelled the active mini-game; the app should cancel locally and
  /// return to the board. Emitted only for cancels received from the network.
  void cancelMinigameReceived();
  /// A scheduled debug launch has reached its gated frame on this peer (emitted in
  /// lockstep across peers). The active context has already been frozen synchronously on
  /// the timing thread (like the host launch's DirectConnection freeze), so the handler
  /// only deserializes `payload` and launches the mini-game on the GUI thread.
  void debugLaunchReady(QByteArray payload);
  /// Server: the owned guests at least one connected client is missing.
  void missingGuestsChanged(QSet<int> guests);
  /// Client: the guests the host owns.
  void hostGuestsReceived(QSet<int> guests);
  /// Client: the host's shared settings have been applied to DrSettings.
  void hostSettingsReceived(void);
  /// Diagnostic log; level matches DrLogger::message (DR_LOG_*).
  void logMessage(unsigned level, const QString &msg);

private slots:
  void onNewConnection();
  void onSocketReadyRead();
  void onSocketDisconnected();

private:
  /// This peer's current send delay: 0 or DR_NETPLAY_GOLF_HIGH_DELAY under golf mode (by whether
  /// this peer is the authority), otherwise m_InputDelay.
  int effectiveDelay() const
  {
    const int authority = m_GolfAuthority.load();
    if (authority < 0)
      return m_InputDelay.load();
    return (m_PeerIndex == authority) ? 0 : DR_NETPLAY_GOLF_HIGH_DELAY;
  }

  /// "index (name)" for logs, or just the index when the context has no name.
  QString ctxLabel(int ctx) const;

  // Frame coordination (timing thread).
  void onFrameBegin(int context);
  void scheduleLaunch(const QByteArray &payload); // server: pick target frame + broadcast
  void maybeFireLaunch(quint64 frame); // fire a due launch after committing (active ctx)
  void runResync(int context);
  void sampleLocal();
  bool isFrameCompleteLocked(int context, quint64 frame) const;
  bool peerLeftLocked(int peer, int context) const;
  bool waitForFrame(int context, quint64 frame);
  void commitMergedFrame(int context, quint64 frame);

  // Networking (main thread).
  void primeContext(int context);
  void recordPacket(const DrNetplayPacket &p);
  void sendInput(const DrNetplayPacket &p);
  void writeMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload);
  void broadcast(quint8 type, const QByteArray &payload, QTcpSocket *except = nullptr);
  void writeVarMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload);
  void broadcastVar(quint8 type, const QByteArray &payload, QTcpSocket *except = nullptr);
  void handleMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload);
  void dropSession(const QString &reason);

  /// Server: recomputes the guests a client is missing and emits missingGuestsChanged.
  void updateMissingGuests(void);
  static QByteArray encodeGuests(const QSet<int> &guests);
  static QSet<int> decodeGuests(const QByteArray &payload);

  /// Server: compares `p`'s sync validator against the first sample seen for its frame
  /// and hard-resyncs on a mismatch. Called from recordPacket with m_RecvMutex held.
  void checkSyncSample(const DrNetplayPacket &p);

  static quint64 frameKey(int context, quint64 frame);
  static int payloadLength(quint8 type);
  static QByteArray encodePacket(const DrNetplayPacket &p);
  static DrNetplayPacket decodePacket(const QByteArray &b);
  static DrNetplayPacket packetFromJoypad(
    QRetroInputJoypad &jp, int peerIndex, int context, quint64 frame);
  static void applyPacketToJoypad(const DrNetplayPacket &p, QRetroInputJoypad &jp);

  static DrNetplay *s_Instance; // the one DrNetplay, for sessionActive/sessionServer

  DrInputStore *m_Store = nullptr;

  QRetroInputBackend *m_LocalSource = nullptr;
  /* Canonical local input pipeline: poll() bakes in keyboard macros/turbo/etc.
   * before we send. Remaps stay identity here; each core applies its own. */
  QRetroInput m_LocalInput;
  QRetroInputJoypad m_CommitPads[DR_NETPLAY_MAX_PEERS];

  QTcpServer *m_Server = nullptr;
  QList<QTcpSocket *> m_Sockets;
  QHash<QTcpSocket *, QByteArray> m_RxBuf;
  int m_NextAssignIndex = 1;

  bool m_IsServer = false;
  int m_PeerIndex = 0;
  int m_PeerCount = 1;
  std::atomic<int> m_InputDelay{ 2 };
  /* "Golf mode": the authority peer sends with 0 delay, everyone else with
   * DR_NETPLAY_GOLF_HIGH_DELAY. -1 = off (use m_InputDelay for everyone). See effectiveDelay. */
  std::atomic<int> m_GolfAuthority{ -1 };
  int m_TimeoutMs = 5000; /* stall this long before requesting a hard resync */

  bool m_Active = false;
  bool m_Abort = false;

  // Authoritative allowed-mini-games filter (opaque payload). Empty = all
  // allowed. The server re-sends this at session start so every client gets it
  // at least once.
  QByteArray m_MinigameFilter;

  QSet<int> m_OwnedGuests;
  QHash<QTcpSocket *, QSet<int>> m_PeerGuests; // server: each client's owned guests

  // Per-context state. attachCore() assigns each QRetro a context id in a
  // deterministic order (host first, then guests), identical across peers.
  QHash<QRetro *, int> m_ContextIds;
  QRetro *m_Contexts[DR_NETPLAY_MAX_CONTEXTS] = {}; // context id -> core, for state I/O
  QString m_ContextNames[DR_NETPLAY_MAX_CONTEXTS];  // context id -> label, for logs
  int m_ContextCount = 0;
  int m_ActiveContext = -1;
  int m_FrozenContext = -1; // context held (retro_run paused) until re-activated
  quint64 m_CtxFrame[DR_NETPLAY_MAX_CONTEXTS] = {};   // next netplay frame to consume per context
  quint64 m_CtxBarrier[DR_NETPLAY_MAX_CONTEXTS] = {}; // per-context sync-point frame (waits w/o timeout)
  quint64 m_CtxSend[DR_NETPLAY_MAX_CONTEXTS] = {};    // next local frame to send per context (delay pipeline)

  // Debug-menu launch scheduled for a specific active-context frame (guarded by
  // m_RecvMutex). -1 when none pending; m_PendingLaunchData is the opaque app payload.
  qint64 m_PendingLaunchFrame = -1;
  QByteArray m_PendingLaunchData;

  quint64 m_LocalFrame = 0; // singleplayer frame counter

  struct FrameInputs
  {
    DrNetplayPacket pkts[DR_NETPLAY_MAX_PEERS] = {};
    bool have[DR_NETPLAY_MAX_PEERS] = {};
  };
  QMutex m_RecvMutex;
  QWaitCondition m_FrameReady;
  QHash<quint64, FrameInputs> m_Received;

  // Per-peer context tracking (guarded by m_RecvMutex); see peerLeftLocked
  int m_PeerLastCtx[DR_NETPLAY_MAX_PEERS] = { -1, -1, -1, -1 };
  quint64 m_PeerCtxFrames[DR_NETPLAY_MAX_PEERS][DR_NETPLAY_MAX_CONTEXTS] = {}; // highest frame + 1; 0 = none
  quint64 m_PeerCtxEntry[DR_NETPLAY_MAX_PEERS][DR_NETPLAY_MAX_CONTEXTS] = {};  // first frame of the peer's current run (its barrier)
  quint64 m_LeftLoggedBarrier[DR_NETPLAY_MAX_CONTEXTS] = {}; // barrier + 1 already logged; 0 = none

  // Sync cross-check (server only, guarded by m_RecvMutex): the first sample seen
  // for a (context, frame), which every later peer's sample must match.
  struct SyncSample
  {
    dr_sync_validator_t sync;
    quint8 peer;
  };
  std::function<dr_sync_validator_t()> m_SyncProbes[DR_NETPLAY_MAX_CONTEXTS];
  QHash<quint64, SyncSample> m_SyncSamples;
  static quint64 syncSampleKey(int context, quint32 frame)
  {
    return (static_cast<quint64>(static_cast<quint8>(context)) << 32) | frame;
  }

  // Hard resync. m_ResyncActive is read on the timing thread; the rest is guarded
  // by m_RecvMutex. The host serializes the active core and ships it; clients park
  // until the state arrives, load it, and resume from m_ResyncFrame.
  std::atomic<bool> m_ResyncActive{ false };
  int m_ResyncCtx = -1;
  bool m_ResyncStateReady = false;
  quint64 m_ResyncFrame = 0;
  quint32 m_ResyncRandState = 0; // host's DrRandom state to restore alongside the savestate
  quint64 m_ResyncRandCount = 0;
  QByteArray m_ResyncState; // decompressed savestate awaiting load (client)
};

#endif
