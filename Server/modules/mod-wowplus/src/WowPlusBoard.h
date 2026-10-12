/*
 * WowPlus group board: what other parts of the module may ask of it. See WowPlusBoard.cpp.
 */

#ifndef WOWPLUS_BOARD_H
#define WOWPLUS_BOARD_H

#include "ObjectGuid.h"

#include <string>
#include <utility>
#include <vector>

namespace WowPlus
{
    // The notes for the board have been written (bot name, note). World thread only.
    void BoardNotesArrived(ObjectGuid player, std::vector<std::pair<std::string, std::string>> const& notes);
}

#endif
