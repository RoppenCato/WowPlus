/*
 * WowPlus journal and world: what other parts of the module may ask of them.
 * See WowPlusJournal.cpp and WowPlusWorld.cpp.
 */

#ifndef WOWPLUS_JOURNAL_H
#define WOWPLUS_JOURNAL_H

#include "Define.h"

#include <string>

class Player;

namespace WowPlus
{
    // The two books of the journal
    constexpr char const* JOURNAL_WORLD = "world";          // kills, places, levels, bounties
    constexpr char const* JOURNAL_PEOPLE = "people";        // who you met, letters and gifts

    // Writes a line into a character's journal, and shows it at once if the journal addon is there.
    // Not for bots. Any thread that may touch the player.
    void Journal(Player* player, char const* book, std::string const& text);

    // The name of the area a player stands in, with its zone: "Sentinel Hill, Westfall"
    std::string PlaceOf(Player* player);

    // A true sentence about a champion near this level, for bots to pass on (WowPlusWorld.cpp).
    // Empty when there is none. World thread only.
    std::string ChampionRumour(uint8 level);
}

#endif
