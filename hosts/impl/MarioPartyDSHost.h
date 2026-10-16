#ifndef DR_HOST_MARIO_PARTY_DS_H
#define DR_HOST_MARIO_PARTY_DS_H

#include "../DrHostDS.h"

#include <QString>
#include <array>
#include <atomic>

typedef enum
{
  DR_MPDS_HOST_STATE_INVALID = 0,
  DR_MPDS_HOST_STATE_BEFORE_BOARD,
  DR_MPDS_HOST_STATE_BOARD,
  DR_MPDS_HOST_STATE_ROULETTE,
  DR_MPDS_HOST_STATE_AFTER_ROULETTE,
  DR_MPDS_HOST_STATE_MINIGAME,

  DR_MPDS_HOST_STATE_SIZE
} dr_mpds_host_state;

class MarioPartyDSHost : public DrHostDS
{
  Q_OBJECT

public:
  explicit MarioPartyDSHost(QObject *parent = nullptr);

  dr_game game(void) const override { return DR_GAME_MARIOPARTYDS; }

  void writeResults(DrGuest *guest) override;
  void clearResults(void) override;

  void setCurrentTurn(unsigned turn) override;
  bool readPlayerSetup(DrPlayerArray &players) override;
  bool writePlayerSetup(const DrPlayerArray &players) override;
  dr_sync_validator_t syncValidator(void) override;
  unsigned battlePot(void) override;

private:
  void run(void);

  /* Read every port's buttons from the frontend into m_buttons and the
   * InputRecords. */
  void writeInput(void);
  void cloneStick(void);

  /* Hand m_buttons to the cave's pad_held, which the game builds the per-port
   * pads from. Only once the cave is stamped. */
  void writePads(void);

  /* Edge-detects and logs the current scene. Returns it, or -1 if unreadable. */
  int pollScene(void);

  /* Which ports' hexes to reveal: your own in netplay, a lone human's offline. */
  void updateHexShow(void);

  /* Copy the mpmod cave's code and idTable into RAM, and poke the board and
   * input hook tables. Nothing is written until the ARM9 binary has finished
   * unpacking itself. */
  void stampCave(void);
  void applyHooks(void);

  /* Tell the cave which ports have someone on them, so a human's windows and
   * menus stay on their port while a CPU slot's fall to the real DS. */
  void writeConnectedPorts(void);

  /* Reroll the shared pool, and cache/stamp one type's candidates as titles. */
  void rollMinigames(void);
  void stampTitles(dr_minigame_type type);

  /* Resolve the landed slot to a cached candidate and launch it. */
  void startMinigame(void);
  void readPlayers(DrPlayerArray &players);

  void setState(dr_mpds_host_state state);

  /* The game mode, (0x0214CA54 >> 7) & 7: 0 story, 1 Extras, 2 puzzle, 3 party,
   * 4 mini-game modes. -1 if unreadable. */
  int gameMode(void);

  /* Base of the live player context, chosen by the game mode, or 0 if no mode
   * has claimed one yet. */
  size_t context(void);

  /* Watch the ports: the first time anything is pressed on one, that seat
   * becomes a person. Seats nobody touches are left alone, and setting a seat
   * back to CPU by hand drops its latch. */
  void updateHumanPorts(void);

  /* Whose turn it is, 0-3, or -1 if no board context has one yet. */
  int turnOwner(void);

  /* Base of the record `slot` banks coins and stars on within `ctx`. Usually its
   * own, but the team rule redirects it to the team leader's. */
  size_t record(size_t ctx, unsigned slot);

  void applyPendingLoss(void);


  std::array<DrMinigameCandidate, 5> m_candidates = {};

  /* Last-seen scene, for edge-detecting board/roulette/results transitions. */
  int16_t m_lastScene = -1;

  dr_mpds_host_state m_state = DR_MPDS_HOST_STATE_INVALID;

  /* Each port's held buttons this frame, NitroSDK PAD_* bits. */
  std::array<uint16_t, 4> m_buttons = {};

  /* hex_show: per player slot, nonzero shows that player's hexes as what they
   * are, zero covers them with the owner's emblem. Stock offline view. */
  std::array<uint8_t, 4> m_hexShow = { 0, 0, 0, 0 };

  /* Ports claimed by a player. Latched until the seat is set back to CPU. */
  std::array<bool, 4> m_portActive = {};

  /* What the roulette rolled and where it landed (0-based), kept until the next
   * roulette so the launch and the results can still tell. */
  dr_minigame_type m_minigameType = DR_MINIGAME_INVALID;

  /* Set when the core loads a state, cleared by run(); written from whichever
   * thread ran the load, read on the timing thread. */
  std::atomic<bool> m_restampTitles{ false };
  int m_slot = -1;

  /* Whether the board handed off to a results scene (end of turn) rather than
   * straight back to itself (duel space). */
  bool m_toResults = true;

  int m_pendingLoss[4] = { 0, 0, 0, 0 };
  unsigned m_pendingLossFrames = 0;

};

#endif
