/*
 * WowPlus bot chat: what other parts of the module may ask of it. See WowPlusBotChat.cpp.
 */

#ifndef WOWPLUS_BOT_CHAT_H
#define WOWPLUS_BOT_CHAT_H

class Player;

namespace WowPlus
{
    // Has this bot say something by itself in party chat, as the person it is. Does nothing if
    // the bridge is not running. World thread only.
    void BotRemarksInParty(Player* bot, Player* player);
}

#endif
