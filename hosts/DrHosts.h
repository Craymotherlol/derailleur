#ifndef DR_HOSTS_H
#define DR_HOSTS_H

#include "../DrHost.h"
#include <string>
#include <QString>

/// One playable host game.
struct DrHostEntry
{
  dr_game game;
  const char *name;
  /// Title screen under :/assets/titlescreen, or null to leave it off the picker
  const char *title;
  /// Content file under the roms directory
  const char *rom;
  /// Core it needs, or DR_CORE_INVALID for none
  dr_core core;
  /// Builds the host; `rom` overrides the stock content path when not empty
  DrHost *(*create)(QObject *parent, const std::string &rom);
};

/// Every host, terminated by an entry with DR_GAME_INVALID.
const DrHostEntry *dr_host_entries(void);

/// The entry for `game`, or null if there is none.
const DrHostEntry *dr_host_entry(dr_game game);

/// Full path of the entry's stock content.
QString dr_host_rom_path(const DrHostEntry *entry);

#endif
