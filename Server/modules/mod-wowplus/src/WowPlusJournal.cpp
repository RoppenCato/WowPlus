/*
 * WowPlus journal: a book that writes itself. See Settings/Docs/Ideas.md.
 *
 * Things worth remembering are written down per character (table wowplus_journal in the
 * characters database): rares, champions and bosses killed for the first time, places
 * discovered, levels reached, bounties collected, people met, letters and gifts. Other parts
 * of the module add lines with WowPlus::Journal(); this file adds the ones about places,
 * levels and bounties itself.
 *
 * The addon WowPlusJournal shows it (/journal). It asks for a book with a hidden addon
 * message and gets the newest lines back; a line written while the player is online is sent
 * at once, and the addon shows it on screen for a moment.
 *
 * Messages, prefix WPJ:
 *   addon to server: "L:<book>"
 *   server to addon: "E:<book>:<unix time>:<text>" for each line, newest first, then "D:<book>"
 *                    "N:<book>:<unix time>:<text>" for a line written just now
 */

#include "WowPlusJournal.h"

#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "Player.h"
#include "QueryResult.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
    std::string const ADDON_PREFIX = "WPJ\t";
    constexpr uint32 LINES_SENT = 100;                      // the newest lines of a book the addon gets
    constexpr std::size_t MAX_TEXT = 200;
    constexpr uint32 BOUNTY_QUEST_FIRST = 9100000;          // Settings/SQL/world/..._champions.sql
    constexpr uint32 BOUNTY_QUEST_LAST = 9199999;

    bool enabled = true;

    void Send(Player* player, std::string const& text)
    {
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, ADDON_PREFIX + text);
        player->SendDirectMessage(&data);
    }

    void SendBook(Player* player, std::string const& book)
    {
        if (book != WowPlus::JOURNAL_WORLD && book != WowPlus::JOURNAL_PEOPLE)
            return;

        if (QueryResult result = CharacterDatabase.Query("SELECT `at`, `text` FROM wowplus_journal WHERE guid = {} AND book = '{}' "
            "ORDER BY id DESC LIMIT {}", player->GetGUID().GetCounter(), book, LINES_SENT))
            do
                Send(player, "E:" + book + ":" + std::to_string((*result)[0].Get<uint32>()) + ":" + (*result)[1].Get<std::string>());
            while (result->NextRow());

        Send(player, "D:" + book);
    }
}

std::string WowPlus::PlaceOf(Player* player)
{
    std::string zone;
    if (AreaTableEntry const* entry = sAreaTableStore.LookupEntry(player->GetZoneId()))
        zone = entry->area_name[0];
    if (AreaTableEntry const* entry = sAreaTableStore.LookupEntry(player->GetAreaId()))
        if (zone != entry->area_name[0])
            return std::string(entry->area_name[0]) + (zone.empty() ? "" : ", " + zone);
    return zone;
}

void WowPlus::Journal(Player* player, char const* book, std::string const& text)
{
    if (!enabled || !player || player->GetSession()->IsHeadless() || text.empty())
        return;

    std::string line = text.substr(0, MAX_TEXT);
    std::replace(line.begin(), line.end(), '\t', ' ');
    uint32 const now = uint32(time(nullptr));

    std::string escaped = line;
    CharacterDatabase.EscapeString(escaped);
    CharacterDatabase.Execute("INSERT INTO wowplus_journal (guid, book, `at`, `text`) VALUES ({}, '{}', {}, '{}')",
        player->GetGUID().GetCounter(), book, now, escaped);

    Send(player, std::string("N:") + book + ":" + std::to_string(now) + ":" + line);
}

class WowPlusJournalWorld : public WorldScript
{
public:
    WowPlusJournalWorld() : WorldScript("WowPlusJournalWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("WowPlus.Journal.Enable", true);
    }
};

class WowPlusJournalPlayer : public PlayerScript
{
public:
    WowPlusJournalPlayer() : PlayerScript("WowPlusJournalPlayer") { }

    using PlayerScript::OnPlayerCanUseChat;                 // keep the other overloads visible

    // The addon asks for a book by whispering its own character. Nothing is passed on.
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* /*receiver*/) override
    {
        if (language != LANG_ADDON || type != CHAT_MSG_WHISPER || msg.compare(0, ADDON_PREFIX.size(), ADDON_PREFIX) != 0)
            return true;

        std::string const text = msg.substr(ADDON_PREFIX.size());
        if (enabled && text.compare(0, 2, "L:") == 0)
            SendBook(player, text.substr(2));

        return false;
    }

    // A new place
    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 xpSource) override
    {
        if (xpSource == XPSOURCE_EXPLORE && amount)
            WowPlus::Journal(player, WowPlus::JOURNAL_WORLD, "Discovered " + WowPlus::PlaceOf(player) + ".");
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (player->GetLevel() > oldLevel && player->IsInWorld())
            WowPlus::Journal(player, WowPlus::JOURNAL_WORLD, "Reached level " + std::to_string(player->GetLevel()) + " in " + WowPlus::PlaceOf(player) + ".");
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() >= BOUNTY_QUEST_FIRST && quest->GetQuestId() <= BOUNTY_QUEST_LAST)
            WowPlus::Journal(player, WowPlus::JOURNAL_WORLD, "Collected the bounty: " + quest->GetTitle() + ".");
    }
};

void AddSC_WowPlusJournal()
{
    new WowPlusJournalWorld();
    new WowPlusJournalPlayer();
}
