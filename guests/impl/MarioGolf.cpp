#include "MarioGolf.h"

#include <QDir>
#include <QImage>
#include <QPainter>
#include <QRetro.h>
#include <QString>

static const char *MG_HIRES_DIR =
  "system/Mupen64plus/hires_texture/MarioGolf64/GLideNHQ";

/* Scoreboard heads, per in-game slot. The savestate has every golfer as Plum in
 * colors 0-3, and the game caches those heads at round start, so they're fixed. */
static const char *MG_HEAD_FILES[4] = {
  "MarioGolf64#3EA9D108#2#1#7407E94C_ciByRGBA.png",
  "MarioGolf64#3EA9D108#2#1#1C383EB6_ciByRGBA.png",
  "MarioGolf64#3EA9D108#2#1#9BA6B3D4_ciByRGBA.png",
  "MarioGolf64#3EA9D108#2#1#96D02969_ciByRGBA.png"
};

/* Heads are 32x24 native. GLideN64 needs an exact integer upscale; 4x (128x96)
 * fits a 32px player icon at an exact 3x (96x96), pillarboxed left and right. */
static const int MG_HEAD_WIDTH = 32;
static const int MG_HEAD_HEIGHT = 24;
static const int MG_HIRES_SCALE = 4;

// u32 per-player selected/menu character.
static const dr_value_t MG_MENU_CHARACTER[4] = {
  { 0x8012F480, DR_VALUE_TYPE_U32 },
  { 0x8012F484, DR_VALUE_TYPE_U32 },
  { 0x8012F488, DR_VALUE_TYPE_U32 },
  { 0x8012F48C, DR_VALUE_TYPE_U32 }
};

// u8 per-player (true, in-game) character (raw per-player addresses).
static const dr_value_t MG_CHARACTER[4] = {
  { 0x801B71ED, DR_VALUE_TYPE_U8 },
  { 0x801B72A5, DR_VALUE_TYPE_U8 },
  { 0x801B735D, DR_VALUE_TYPE_U8 },
  { 0x801B7415, DR_VALUE_TYPE_U8 }
};

// u8 per-player color (raw per-player addresses, 0xB8 stride). 0=default color, 1=c-left
// 2=c-down, 3=c-right
static const dr_value_t MG_COLOR[4] = {
  { 0x801B724B, DR_VALUE_TYPE_U8 },
  { 0x801B7303, DR_VALUE_TYPE_U8 },
  { 0x801B73BB, DR_VALUE_TYPE_U8 },
  { 0x801B7473, DR_VALUE_TYPE_U8 }
};

// u8 bool per-player: is this golfer a bot? (0xB8-strided from P1).
static const dr_value_t MG_BOT[4] = {
  { 0x801B71EE, DR_VALUE_TYPE_U8 },
  { 0x801B72A6, DR_VALUE_TYPE_U8 },
  { 0x801B735E, DR_VALUE_TYPE_U8 },
  { 0x801B7416, DR_VALUE_TYPE_U8 }
};

// u32 course id (see mario_golf_course) and u32 current hole.
static const dr_value_t MG_COURSE = { 0x801B6094, DR_VALUE_TYPE_U32 };
static const dr_value_t MG_HOLE = { 0x801B6098, DR_VALUE_TYPE_S32 };

// u32 game state. The intro/loading screen covers everything up to MG_STATE_FINISHED,
// which is when the actual mini-golf gameplay becomes visible.
static const dr_value_t MG_STATE = { 0x801B6088, DR_VALUE_TYPE_U32 };
typedef enum
{
  MG_STATE_INTRO = 3,
  MG_STATE_INTRO2 = 4,
  MG_STATE_PLAYING = 6,
  MG_STATE_FINISHED = 7,
} mg_state;

// u32 whose turn it is (0-3). Drives netplay golf mode so the active golfer has 0
// input latency (priority) while everyone else waits.
static const dr_value_t MG_TURN = { 0x800FBE74, DR_VALUE_TYPE_U32 };

// u8 per-player controller port (0-3), 0xB8-strided. The block defaults to 0 (all
// golfers read controller 0), so we write each golfer its own port for local + netplay.
static const dr_value_t MG_PLAYER_PORT[4] = {
  { 0x801B71F7, DR_VALUE_TYPE_U8 },
  { 0x801B72AF, DR_VALUE_TYPE_U8 },
  { 0x801B7367, DR_VALUE_TYPE_U8 },
  { 0x801B741F, DR_VALUE_TYPE_U8 }
};

// Per-player hole state, 0xB8-strided (P1 0x801B71F0, P3 0x801B7360, P4 0x801B7418).
static const dr_value_t MG_SHOTS[4] = {
  { 0x801B71F0, DR_VALUE_TYPE_U32 },
  { 0x801B72A8, DR_VALUE_TYPE_U32 },
  { 0x801B7360, DR_VALUE_TYPE_U32 },
  { 0x801B7418, DR_VALUE_TYPE_U32 }
}; // u32 strokes
static const dr_value_t MG_SUNK[4] = {
  { 0x801B71F6, DR_VALUE_TYPE_U8 },
  { 0x801B72AE, DR_VALUE_TYPE_U8 },
  { 0x801B7366, DR_VALUE_TYPE_U8 },
  { 0x801B741E, DR_VALUE_TYPE_U8 }
}; // u8 ball sunk (see mg_sunk)
typedef enum
{
  MG_SUNK_PLAYING = 0,
  MG_SUNK_HOLED = 1,
  MG_SUNK_GAVE_UP = 3
} mg_sunk;

/* Stroke-play give-up check (0x8020c0c8): a ball that stops without holing gives up
 * the hole once strokes >= par*3, recording par*3. Rewritten to strokes >= par,
 * recording par+1, so a golfer who misses par is out the moment that ball stops. */
static const dr_value_t MG_GIVEUP_SCORE = { 0x8020C0F8, DR_VALUE_TYPE_U32 };
static const dr_value_t MG_GIVEUP_CMP = { 0x8020C0FC, DR_VALUE_TYPE_U32 };
static const uint32_t MG_GIVEUP_SCORE_ORIG = 0x00442821; // addu a1, v0, a0
static const uint32_t MG_GIVEUP_SCORE_NEW = 0x24850001;  // addiu a1, a0, 1
static const uint32_t MG_GIVEUP_CMP_ORIG = 0x0065182A;   // slt v1, v1, a1
static const uint32_t MG_GIVEUP_CMP_NEW = 0x0064182A;    // slt v1, v1, a0

// u8 bool: is a ball currently in flight? The stroke counter increments the moment the
// swing is made, so a hole can't be judged off the counter alone -- wait for this to
// pulse back to 0, meaning the ball that stroke launched has come to rest.
static const dr_value_t MG_IN_FLIGHT = { 0x801061CB, DR_VALUE_TYPE_U8 };

// u8 par for the current hole.
static const dr_value_t MG_PAR = { 0x800BAA04, DR_VALUE_TYPE_U8 };

/* s8 per-hole scorecard strokes: 0x801B71FB + slot * 0xB8 + hole * 2. The running
 * score-to-par (0x8006cd50) sums (card - par) over every hole before the current
 * one, so holes we skip by writing MG_HOLE count as 0 strokes. */
static const uint32_t MG_CARD_BASE = 0x801B71FB;
static const uint32_t MG_PLAYER_STRIDE = 0xB8;

/* Both mini-golf rows of the game's par table (0x800C1420) are par 3 throughout */
static const int MG_MINIGOLF_PAR = 3;

typedef enum
{
  MG_CHARACTER_PLUM = 0,
  MG_CHARACTER_CHARLIE = 1,
  MG_CHARACTER_PEACH = 5,
  MG_CHARACTER_BABY_MARIO = 3,
  MG_CHARACTER_LUIGI = 4,
  MG_CHARACTER_YOSHI = 10,
  MG_CHARACTER_SONNY = 6,
  MG_CHARACTER_WARIO = 7,
  MG_CHARACTER_HARRY = 8,
  MG_CHARACTER_MARIO = 9,
  MG_CHARACTER_MAPLE = 2,
  MG_CHARACTER_DONKEY_KONG = 11,
  MG_CHARACTER_BOWSER = 12,
  MG_CHARACTER_METAL_MARIO = 13,
  MG_CHARACTER_KID = 14,
  MG_CHARACTER_SHERRY = 15,
  MG_CHARACTER_AZALEA = 16,
  MG_CHARACTER_JOE = 17
} mg_character;

typedef struct
{
  mg_character character;
  uint8_t color;
} mg_char_pick;

static mg_char_pick mgCharFor(dr_character character)
{
  switch (character)
  {
  case DR_CHARACTER_MARIO: 
    return { MG_CHARACTER_MARIO, 0 };
  case DR_CHARACTER_LUIGI:
    return { MG_CHARACTER_LUIGI, 0 };
  case DR_CHARACTER_PEACH:
    return { MG_CHARACTER_PEACH, 0 };
  case DR_CHARACTER_YOSHI:
    return { MG_CHARACTER_YOSHI, 0 };
  case DR_CHARACTER_WARIO:
    return { MG_CHARACTER_WARIO, 0 };
  case DR_CHARACTER_DONKEY_KONG:
    return { MG_CHARACTER_DONKEY_KONG, 0 };
  case DR_CHARACTER_DAISY:
    return { MG_CHARACTER_AZALEA, 0 };
  case DR_CHARACTER_WALUIGI:
    return { MG_CHARACTER_HARRY, 0 };

  case DR_CHARACTER_TOAD:
    return { MG_CHARACTER_BABY_MARIO, 3 };
  case DR_CHARACTER_BOO:
    return { MG_CHARACTER_METAL_MARIO, 3 };
  case DR_CHARACTER_KOOPA_KID:
    return { MG_CHARACTER_BOWSER, 0 };

  case DR_CHARACTER_TOADETTE:
    return { MG_CHARACTER_PLUM, 2 };
  case DR_CHARACTER_BIRDO:
    return { MG_CHARACTER_YOSHI, 3 };
  case DR_CHARACTER_DRY_BONES:
    return { MG_CHARACTER_DONKEY_KONG, 2 };
  case DR_CHARACTER_HAMMER_BRO:
    return { MG_CHARACTER_HARRY, 1 };
  case DR_CHARACTER_BLOOPER:
    return { MG_CHARACTER_MAPLE, 2 };

  case DR_CHARACTER_KOOPA_KID_R:
    return { MG_CHARACTER_BOWSER, 1 };
  case DR_CHARACTER_KOOPA_KID_G:
    return { MG_CHARACTER_BOWSER, 3 };
  case DR_CHARACTER_KOOPA_KID_B:
    return { MG_CHARACTER_BOWSER, 2 };

  default:
    return { MG_CHARACTER_MARIO, 0 };
  }
}

typedef enum
{
  MG_COURSE_MINIGOLF_LUIGIS_GARDEN = 6,
  MG_COURSE_MINIGOLF_PEACHS_CASTLE = 9,
} mg_course;

MarioGolf::MarioGolf(QObject *parent)
  : DrGuestN64(parent)
{
  m_retro->init(coreId(), rom());

  /* One minigame per hole: minigame_id = course, scene_id = hole (0-17). */
  static const struct { signed id; const char *name; } courses[MG_COURSE_COUNT] = {
    { MG_COURSE_MINIGOLF_LUIGIS_GARDEN, "Luigi's Garden" },
    { MG_COURSE_MINIGOLF_PEACHS_CASTLE, "Peach's Castle" },
  };

  int n = 0;
  for (int c = 0; c < MG_COURSE_COUNT; c++)
    for (int hole = 0; hole < MG_HOLE_COUNT; hole++, n++)
    {
      const QByteArray label =
        QString("Golf: %1 %2").arg(courses[c].name).arg(hole + 1).toUtf8();
      qstrncpy(m_minigameNames[n], label.constData(), sizeof(m_minigameNames[n]));
      m_minigames[n] = { m_minigameNames[n], DR_MINIGAME_4P, courses[c].id, hole, DR_NO_QUIRKS, DR_NO_FLAGS };
    }
  m_minigames[n] = { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS };
}

const dr_mp_minigame_t *MarioGolf::minigames() const
{
  return m_minigames;
}

void MarioGolf::onBeforeBoot(const DrGameData &data)
{
  writePlayerHeads(data);
}

void MarioGolf::writePlayerHeads(const DrGameData &data)
{
  const QString destDir = QString::fromUtf8(MG_HIRES_DIR);
  const int canvasW = MG_HEAD_WIDTH * MG_HIRES_SCALE;
  const int canvasH = MG_HEAD_HEIGHT * MG_HIRES_SCALE;
  QDir().mkpath(destDir);

  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = data.players[i].control_port;
    if (slot >= 4)
      continue;

    const QImage art(dr_player_icon_32px(data.host_platform, data.players[i].character));
    if (art.isNull())
    {
      log(DR_LOG_WARN, qPrintable(QString("no 32px player icon for character %1")
        .arg(static_cast<int>(data.players[i].character))));
      continue;
    }

    /* Full canvas height, aspect kept; nearest-neighbour since 32 -> 96 is exact */
    const QImage head = art.scaledToHeight(canvasH, Qt::FastTransformation);
    QImage canvas(canvasW, canvasH, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    QPainter painter(&canvas);
    painter.drawImage((canvasW - head.width()) / 2, 0, head);
    painter.end();

    const QString dest = destDir + "/" + QString::fromUtf8(MG_HEAD_FILES[slot]);
    if (!canvas.save(dest, "PNG"))
      log(DR_LOG_WARN, qPrintable(QString("failed to write %1").arg(dest)));
  }
}

void MarioGolf::doApplyGameData(const DrGameData &data)
{
  m_minigameFrames = 0;
  m_lastTurn = -1;
  m_winners = 0;
  m_finishPending = false;
  for (unsigned i = 0; i < 4; i++)
  {
    m_slotToIndex[i] = -1;
    m_prevShots[i] = 0;
    m_prevSunk[i] = false;
    m_holeStrokes[i] = -1;
  }
  m_prevInFlight = false;
  m_shotPending = false;

  m_retro->loadState(state());

  /* Each minigame encodes its course (minigame_id) and hole (scene_id). The hole address
   * is 0-based one lower than the hole number, so the first hole writes -1. */
  if (data.minigame)
  {
    m_retro->writeValue(data.minigame->minigame_id, MG_COURSE);
    m_retro->writeValue(data.minigame->scene_id - 1, MG_HOLE);
  }

  /* Each board player's golfer + color go into the in-game slot matching their
   * controller port. m_slotToIndex maps back for scoring. */
  for (unsigned i = 0; i < 4; i++)
  {
    const unsigned slot = data.players[i].control_port;
    if (slot >= 4)
      continue;
    m_slotToIndex[slot] = static_cast<int>(i);

    const mg_char_pick pick = mgCharFor(data.players[i].character);
    m_retro->writeValue(pick.character, MG_MENU_CHARACTER[slot]);
    m_retro->writeValue(pick.character, MG_CHARACTER[slot]);
    m_retro->writeValue(pick.color, MG_COLOR[slot]);

    /* Assign the golfer in this slot to its own controller port. The block defaults to
     * 0 (all golfers -> controller 0); hold our value through init with writeForFrames. */
    m_retro->writeValueForFrames(slot, MG_PLAYER_PORT[slot], 300);

    /* Mark CPU-controlled golfers as bots. */
    m_retro->writeValueForFrames(
      data.players[i].control_type == DR_CONTROL_TYPE_CPU ? 1 : 0, MG_BOT[slot], 300);
  }

  /* Tee order. At MG_STATE 7 the game ranks honors (0x8005fe8c) off each golfer's
   * last-hole strokes, lowest first, and the next tee goes in that order. Seed those
   * strokes from board standings (stars * 1000 + coins, ties to the lower board index)
   * so the board leader tees off first. */
  int order[4];
  unsigned count = 0;
  for (unsigned i = 0; i < 4; i++)
    if (data.players[i].control_port < 4)
      order[count++] = static_cast<int>(i);
  for (unsigned a = 1; a < count; a++)
  {
    const int p = order[a];
    const int key = data.players[p].stars * 1000 + data.players[p].coins;
    unsigned b = a;
    while (b > 0 && data.players[order[b - 1]].stars * 1000 +
                    data.players[order[b - 1]].coins < key)
    {
      order[b] = order[b - 1];
      b--;
    }
    order[b] = p;
  }
  for (unsigned r = 0; r < count; r++)
  {
    const unsigned slot = data.players[order[r]].control_port;
    m_retro->writeValue(r + 1, MG_SHOTS[slot]);
  }

  /* Card every hole before this one at par so everyone starts even, and clear the
   * rest (the state carries its own first-hole strokes). */
  if (data.minigame)
    for (unsigned slot = 0; slot < 4; slot++)
      for (int hole = 0; hole < MG_HOLE_COUNT; hole++)
      {
        const dr_value_t card = { MG_CARD_BASE + slot * MG_PLAYER_STRIDE +
          static_cast<uint32_t>(hole) * 2, DR_VALUE_TYPE_U8 };
        m_retro->writeValue(hole < data.minigame->scene_id ? MG_MINIGOLF_PAR : 0, card);
      }

  /* Mercy rule: patch the give-up check before any frame runs, so the recompiler
   * only ever sees the patched words. */
  int64_t scoreWord = 0, cmpWord = 0;
  if (m_retro->readValue(&scoreWord, MG_GIVEUP_SCORE) == DR_OK &&
      m_retro->readValue(&cmpWord, MG_GIVEUP_CMP) == DR_OK &&
      scoreWord == MG_GIVEUP_SCORE_ORIG && cmpWord == MG_GIVEUP_CMP_ORIG)
  {
    m_retro->writeValue(MG_GIVEUP_SCORE_NEW, MG_GIVEUP_SCORE);
    m_retro->writeValue(MG_GIVEUP_CMP_NEW, MG_GIVEUP_CMP);
  }
  else
    log(DR_LOG_WARN, qPrintable(QString("Mario Golf: give-up check not patched "
      "(read 0x%1 0x%2)").arg(scoreWord, 0, 16).arg(cmpWord, 0, 16)));

  /* Don't reveal yet. Wait 60 frames for the loaded state to settle, force an A press
   * on P1 to advance past the intro, then reveal (startMinigame) once the game state
   * reaches MG_STATE_FINISHED -- see run(). */
  m_startupDelay = 120;
  m_aReleaseDelay = 0;
}

void MarioGolf::run()
{
  /* Release the forced P1 A press a few frames after it was applied. */
  if (m_aReleaseDelay > 0 && --m_aReleaseDelay == 0)
    core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, false);

  /* Hidden startup: run behind the loading screen until the game is fully in the
   * mini-golf state, then reveal via startMinigame(). */
  if (!m_minigameActive)
  {
    if (m_startupDelay > 0)
    {
      if (--m_startupDelay == 0)
      {
        /* Force an A press on P1 to advance past the intro. */
        core()->input()->joypads()[0].setForcedButton(RETRO_DEVICE_ID_JOYPAD_A, true);
        m_aReleaseDelay = 8;
      }
      return;
    }

    int64_t gameState = 0;
    if (m_retro->readValue(&gameState, MG_STATE) == DR_OK &&
        gameState == MG_STATE_INTRO)
      startMinigame();
    return;
  }

  m_minigameFrames++;

  /* Give whoever's turn it is 0 input latency (priority) via netplay golf mode; the
   * value is deterministic game state, so every peer toggles it on the same frame. */
  int64_t turn = 0;
  if (m_retro->readValue(&turn, MG_TURN) == DR_OK && turn < 4 &&
      static_cast<int>(turn) != m_lastTurn)
  {
    m_lastTurn = static_cast<int>(turn);
    /* The turn value is an in-game slot, and each slot reads its own controller port
     * (MG_PLAYER_PORT). Netplay peer N drives port N, so the slot is the authority
     * peer as-is -- not the board index, which Mario Party assigns ports apart from. */
    if (m_slotToIndex[turn] >= 0)
      emit golfModeRequested(static_cast<int>(turn));
  }

  if (m_finishPending)
    return;

  /* Read the hole state: strokes taken and whether each active player has sunk it. */
  int64_t par = 0;
  if (m_retro->readValue(&par, MG_PAR) != DR_OK)
    return;

  if (static_cast<int>(par) != m_prevPar)
  {
    m_prevPar = static_cast<int>(par);
    log(DR_LOG_INFO, qPrintable(QString("Mario Golf: par is now %1").arg(par)));
  }

  /* A stroke's outcome isn't known until its ball settles: the counter increments at the
   * swing, but the ball can still sink (or not) while it's in flight. Clear the pending
   * stroke on the in-flight falling edge, before the loop below can flag a new one. */
  int64_t inFlightRaw = 0;
  if (m_retro->readValue(&inFlightRaw, MG_IN_FLIGHT) == DR_OK)
  {
    const bool inFlight = (inFlightRaw != 0);
    if (m_prevInFlight && !inFlight)
      m_shotPending = false;
    m_prevInFlight = inFlight;
  }

  /* First pass: sample every slot and latch hole-outs. A stroke that appeared this frame
   * flags the ball as pending, so it must run to completion before anything below reads
   * m_shotPending -- an in-flight ball can still sink. */
  int64_t shots[4] = {};
  bool sunk[4] = {};
  for (unsigned slot = 0; slot < 4; slot++)
  {
    if (m_slotToIndex[slot] < 0)
      continue;
    m_retro->readValue(&shots[slot], MG_SHOTS[slot]);
    int64_t s = 0;
    m_retro->readValue(&s, MG_SUNK[slot]);
    /* Only a holed ball counts; a give-up (MG_SUNK_GAVE_UP) leaves strokes at par. */
    sunk[slot] = (s == MG_SUNK_HOLED);

    if (shots[slot] > static_cast<int64_t>(m_prevShots[slot]))
    {
      m_shotPending = true;
      log(DR_LOG_INFO, qPrintable(QString("Mario Golf: player %1 stroke %2")
        .arg(m_slotToIndex[slot]).arg(shots[slot])));
    }
    if (sunk[slot] && !m_prevSunk[slot])
    {
      log(DR_LOG_INFO, qPrintable(QString("Mario Golf: player %1 sunk it in %2 strokes")
        .arg(m_slotToIndex[slot]).arg(shots[slot])));
      /* Latch the hole-out stroke count. The live sunk flag is transient (and the game
       * clears it when it settles/advances the hole), so the decision below must run off
       * this latch, not the instantaneous read. */
      if (m_holeStrokes[slot] < 0)
        m_holeStrokes[slot] = static_cast<int>(shots[slot]);
    }

    m_prevShots[slot] = shots[slot];
    m_prevSunk[slot] = sunk[slot];
  }

  /* Nothing is decided while a ball is still in the air -- not the mercy rule, and not
   * the hole itself. A player who has just swung can still tie or beat the leader. */
  if (m_shotPending)
    return;

  /* A sink only counts if it is par or better; find the lowest such latched hole-out. */
  bool anyValidSink = false;
  uint32_t best = 0;
  for (unsigned slot = 0; slot < 4; slot++)
  {
    if (m_slotToIndex[slot] < 0 || m_holeStrokes[slot] < 0 ||
        static_cast<uint32_t>(m_holeStrokes[slot]) > par)
      continue;
    if (!anyValidSink || static_cast<uint32_t>(m_holeStrokes[slot]) < best)
    {
      best = static_cast<uint32_t>(m_holeStrokes[slot]);
      anyValidSink = true;
    }
  }

  /* A slot is still "unresolved" if it could still change the outcome. Once no slot is
   * unresolved, the hole is decided. A slot that has holed out (latched) is resolved. */
  bool finish = true;
  for (unsigned slot = 0; slot < 4; slot++)
  {
    if (m_slotToIndex[slot] < 0 || m_holeStrokes[slot] >= 0)
      continue; // inactive, or already holed out -> resolved

    /* Not yet holed out: can it still matter? shots[] counts strokes already taken, so a
     * player can still reach a target of N only while shots < N (their next stroke would
     * be shots+1). With a par-or-better winner up, the target is `best`; otherwise it's
     * par. shots == target means they've already used those strokes without sinking. */
    const uint32_t threshold = anyValidSink ? best : par;
    if (shots[slot] < threshold)
    {
      finish = false;
      break;
    }
  }

  if (finish)
  {
    /* Winners are the par-or-better hole-outs tied at `best` (none if it washed out). */
    m_winners = 0;
    if (anyValidSink)
      for (unsigned slot = 0; slot < 4; slot++)
        if (m_slotToIndex[slot] >= 0 && m_holeStrokes[slot] >= 0 &&
            static_cast<uint32_t>(m_holeStrokes[slot]) == best)
          m_winners |= (1u << m_slotToIndex[slot]);
  }

  if (finish)
  {
    log(DR_LOG_INFO, qPrintable(QString("Mario Golf: hole decided (par %1, anyValidSink %2, "
      "best %3, winners 0x%4) -- finishing")
      .arg(par).arg(anyValidSink ? 1 : 0).arg(best).arg(m_winners, 0, 16)));
    m_finishPending = true;
    emit golfModeRequested(-1); // done -- don't leave golf priority set on the board
    finishMinigameInFrames(240);
  }
}

dr_minigame_result_t MarioGolf::minigameResult(unsigned index)
{
  dr_minigame_result_t result = { 0, 0 };
  if (index < 4 && (m_winners & (1u << index)))
    result.coins = 10;
  return result;
}
