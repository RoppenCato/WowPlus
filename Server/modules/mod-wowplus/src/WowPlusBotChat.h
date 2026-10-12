/*
 * WowPlus bot chat: what other parts of the module may ask of it. See WowPlusBotChat.cpp.
 */

#ifndef WOWPLUS_BOT_CHAT_H
#define WOWPLUS_BOT_CHAT_H

#include <string>

class Player;

namespace WowPlus
{
    // Has this bot say something by itself in party chat, as the person it is. Does nothing if
    // the bridge is not running. World thread only.
    void BotRemarksInParty(Player* bot, Player* player);

    // Has this bot say one line in party (or raid) chat about something that just happened to it or
    // the group, told in plain words: "You have arrived at the entrance of Shadowfang Keep."
    void BotReactsInParty(Player* bot, Player* player, std::string const& happened);

    // Has this bot whisper the player to ask for a place in the group the player posted. `wanted` is
    // what the post asked for and what the bot is: "a healer for Shadowfang Keep; you are a healer".
    void BotApplies(Player* bot, Player* player, std::string const& wanted);

    // Asks for the notes that people put next to their name on the group board. `people` is
    // "name:gender:race:class:level:role,..."; the answer arrives at WowPlus::BoardNotesArrived.
    // False if the bridge is busy or off; ask again later.
    bool AskBoardNotes(Player* player, std::string const& people, std::string const& dungeons);
}

#endif
