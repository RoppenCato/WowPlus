/*
 * WowPlus home: what the innkeepers' conversation (WowPlusCoach.cpp) needs from it. See WowPlusHome.cpp.
 */

#ifndef WOWPLUS_HOME_H
#define WOWPLUS_HOME_H

#include "Define.h"

#include <string>

class Player;

namespace WowPlus
{
    // If this zone is a city with rooms and the player has no key to it yet: the key, the city's name and
    // what the key costs this player, in copper. False otherwise.
    bool RoomOffer(Player* player, uint32 zone, uint32& keyItem, std::string& cityName, uint32& price);

    // Takes the money and hands over the key. Tells the player what happened.
    void BuyRoomKey(Player* player, uint32 keyItem);
}

#endif
