/*
 * WowPlus bot chat: bots answer through Claude. See Settings/Docs/Bot-Chat.md.
 *
 * This file is only the game's side of it. When a real player says something a bot should
 * answer, the bot's and the player's details and the text go to the bridge, a small program in
 * its own window (Start-Bot-Chat.bat, Settings/Scripts/Bot-Chat-Bridge.py). The bridge knows who
 * each bot is, remembers earlier talks, asks Claude, and sends the lines back; the bot then
 * says them. If the bridge is not running, bots simply stay quiet.
 *
 * Who answers:
 * - a whisper: the bot whispered to
 * - /say: a nearby bot named in the text, else the bot the player has targeted, else the
 *   closest bot if one stands right next to the player
 * - party and guild chat: a bot named in the text, else the bot that answered last (a talk
 *   that is going on), else sometimes one of them (the chance is a setting)
 * Something that is a bot command ("follow", "stay", ...) is left to the bot module, unless it
 * is a question.
 *
 * Asking Claude takes seconds, so it happens on helper threads; the answer is handed back to
 * the world thread, which makes the bot speak.
 */

#include "Chat.h"
#include "ChatHelper.h"
#include "Config.h"
#include "DBCStores.h"
#include "Group.h"
#include "Guild.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Playerbots.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{
    enum Channel : uint8
    {
        CHANNEL_WHISPER,
        CHANNEL_SAY,
        CHANNEL_PARTY,
        CHANNEL_GUILD
    };

    char const* const CHANNEL_NAMES[] = {"whisper", "say", "party", "guild"};

    constexpr uint32 MAX_REQUESTS_AT_ONCE = 4;
    constexpr uint32 REPLY_TIMEOUT_SECONDS = 100;
    constexpr uint32 TALK_GOES_ON_SECONDS = 120;            // the bot that answered last keeps answering this long
    constexpr uint32 MAX_TEXT_LENGTH = 250;

    bool enabled = true;
    std::string bridgePort = "8586";
    bool sayEnabled = true;
    bool partyEnabled = true;
    bool guildEnabled = true;
    float sayNameRadius = 30.0f;
    float sayClosestRadius = 8.0f;
    uint32 partyUnaddressedChance = 50;
    uint32 guildUnaddressedChance = 35;

    struct Reply
    {
        ObjectGuid bot;
        ObjectGuid player;
        Channel channel;
        std::vector<std::string> lines;
    };

    struct PendingLine
    {
        uint32 delay;                                       // ms until the bot says it
        ObjectGuid bot;
        ObjectGuid player;
        Channel channel;
        std::string text;
    };

    struct LastTalk
    {
        ObjectGuid bot;
        time_t at = 0;
    };

    // Shared with the helper threads. Never freed, so a thread that outlives the shutdown cannot touch freed memory.
    std::mutex& replyMutex = *new std::mutex();
    std::vector<Reply>& replies = *new std::vector<Reply>();
    std::atomic<uint32>& requestsRunning = *new std::atomic<uint32>(0);
    std::atomic<bool>& bridgeWasMissing = *new std::atomic<bool>(false);

    // World thread only
    std::vector<PendingLine> pendingLines;
    std::unordered_map<ObjectGuid, LastTalk> lastTalk[4];   // per channel: real player -> the bot that answered last

    bool IsBot(Player const* player)
    {
        return player->GetSession()->IsHeadless();
    }

    std::string UrlEncode(std::string const& text)
    {
        static char const* hex = "0123456789ABCDEF";
        std::string out;
        for (unsigned char c : text)
        {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.')
                out += char(c);
            else
            {
                out += '%';
                out += hex[c >> 4];
                out += hex[c & 15];
            }
        }
        return out;
    }

    std::string ZoneOf(Player* player)
    {
        std::string zone;
        if (AreaTableEntry const* entry = sAreaTableStore.LookupEntry(player->GetZoneId()))
            zone = entry->area_name[0];
        if (AreaTableEntry const* entry = sAreaTableStore.LookupEntry(player->GetAreaId()))
            if (zone != entry->area_name[0])
                zone = std::string(entry->area_name[0]) + (zone.empty() ? "" : ", " + zone);
        return zone;
    }

    std::string ClassOf(Player* player)
    {
        ChrClassesEntry const* entry = sChrClassesStore.LookupEntry(player->getClass());
        return entry ? entry->name[0] : "";
    }

    std::string RaceOf(Player* player)
    {
        ChrRacesEntry const* entry = sChrRacesStore.LookupEntry(player->getRace());
        return entry ? entry->name[0] : "";
    }

    // Runs on a helper thread. Posts the form to the bridge and reads the reply lines.
    void AskBridge(std::string port, std::string body, ObjectGuid bot, ObjectGuid player, Channel channel)
    {
        std::vector<std::string> lines;
        try
        {
            boost::asio::ip::tcp::iostream stream;
            stream.expires_after(std::chrono::seconds(REPLY_TIMEOUT_SECONDS));
            stream.connect("127.0.0.1", port);
            if (stream)
            {
                stream << "POST /chat HTTP/1.0\r\nHost: 127.0.0.1\r\nContent-Type: application/x-www-form-urlencoded\r\n"
                       << "Content-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body << std::flush;

                std::string line;
                bool inBody = false;
                while (std::getline(stream, line))
                {
                    if (!line.empty() && line.back() == '\r')
                        line.pop_back();

                    if (!inBody)
                        inBody = line.empty();
                    else if (!line.empty())
                        lines.push_back(line.substr(0, 255));
                }
                bridgeWasMissing = false;
            }
            else if (!bridgeWasMissing.exchange(true))
                LOG_INFO("module", "WowPlus bot chat: the bridge is not running (Start-Bot-Chat.bat), bots stay quiet.");
        }
        catch (std::exception const&)
        {
        }

        if (!lines.empty())
        {
            std::lock_guard<std::mutex> lock(replyMutex);
            replies.push_back({bot, player, channel, std::move(lines)});
        }

        --requestsRunning;
    }

    void Ask(Player* bot, Player* player, Channel channel, std::string const& text, std::vector<Player*> const& others)
    {
        if (requestsRunning >= MAX_REQUESTS_AT_ONCE)
            return;

        std::string otherNames;
        for (Player* other : others)
            if (other != bot && otherNames.size() < 120)
                otherNames += (otherNames.empty() ? "" : ", ") + other->GetName();

        bool const sameGuild = player->GetGuildId() && player->GetGuildId() == bot->GetGuildId();
        bool const sameParty = player->GetGroup() && player->GetGroup() == bot->GetGroup();

        std::ostringstream body;
        body << "player=" << UrlEncode(player->GetName())
             << "&pclass=" << UrlEncode(ClassOf(player)) << "&prace=" << UrlEncode(RaceOf(player))
             << "&plevel=" << uint32(player->GetLevel()) << "&pzone=" << UrlEncode(ZoneOf(player))
             << "&bot=" << UrlEncode(bot->GetName())
             << "&bclass=" << UrlEncode(ClassOf(bot)) << "&brace=" << UrlEncode(RaceOf(bot))
             << "&blevel=" << uint32(bot->GetLevel()) << "&bzone=" << UrlEncode(ZoneOf(bot))
             << "&bgender=" << (bot->getGender() == GENDER_FEMALE ? "female" : "male")
             << "&channel=" << CHANNEL_NAMES[channel]
             << "&guildmate=" << (sameGuild ? 1 : 0) << "&party=" << (sameParty ? 1 : 0)
             << "&guild=" << UrlEncode(sameGuild ? player->GetGuildName() : "")
             << "&others=" << UrlEncode(otherNames)
             << "&text=" << UrlEncode(text.substr(0, MAX_TEXT_LENGTH));

        ++requestsRunning;
        std::thread(AskBridge, bridgePort, body.str(), bot->GetGUID(), player->GetGUID(), channel).detach();

        if (channel != CHANNEL_WHISPER)
            lastTalk[channel][player->GetGUID()] = {bot->GetGUID(), time(nullptr)};
    }

    // The same test the bot module uses to decide whether a line is a command for the bot
    bool IsBotCommand(Player* bot, std::string const& text)
    {
        if (text.find('?') != std::string::npos)
            return false;                                   // a question is talk, whatever word it starts with

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
        if (!botAI || !botAI->GetAiObjectContext())
            return false;

        AiObjectContext* context = botAI->GetAiObjectContext();
        if (context->GetTrigger(text))
            return true;

        for (std::size_t space = text.rfind(' '); space != std::string::npos && space > 0; space = text.rfind(' ', space - 1))
            if (context->GetTrigger(text.substr(0, space)))
                return true;

        return ChatHelper::parseableItem(text);
    }

    bool Mentions(std::string const& text, std::string const& name)
    {
        auto lower = [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); }); return s; };
        std::string const haystack = lower(text);
        std::string const needle = lower(name);

        for (std::size_t at = haystack.find(needle); at != std::string::npos; at = haystack.find(needle, at + 1))
        {
            bool const startsWord = at == 0 || !std::isalpha(static_cast<unsigned char>(haystack[at - 1]));
            std::size_t const end = at + needle.size();
            bool const endsWord = end >= haystack.size() || !std::isalpha(static_cast<unsigned char>(haystack[end]));
            if (startsWord && endsWord)
                return true;
        }

        return false;
    }

    // Who of these bots answers a line said to all of them. nullptr: nobody.
    Player* ChooseSpeaker(Player* player, Channel channel, std::string const& text, std::vector<Player*> const& bots, uint32 unaddressedChance)
    {
        if (bots.empty())
            return nullptr;

        for (Player* bot : bots)
            if (Mentions(text, bot->GetName()))
                return bot;

        auto last = lastTalk[channel].find(player->GetGUID());
        if (last != lastTalk[channel].end() && time(nullptr) - last->second.at <= TALK_GOES_ON_SECONDS)
            for (Player* bot : bots)
                if (bot->GetGUID() == last->second.bot)
                    return bot;

        return roll_chance_i(int32(unaddressedChance)) ? bots[urand(0, bots.size() - 1)] : nullptr;
    }

    void Speak(PendingLine const& line)
    {
        Player* bot = ObjectAccessor::FindConnectedPlayer(line.bot);
        Player* player = ObjectAccessor::FindConnectedPlayer(line.player);
        if (!bot || !player || !bot->IsInWorld() || !player->IsInWorld())
            return;

        switch (line.channel)
        {
            case CHANNEL_WHISPER:
                bot->Whisper(line.text, LANG_UNIVERSAL, player);
                break;
            case CHANNEL_SAY:
                bot->Say(line.text, LANG_UNIVERSAL);
                break;
            case CHANNEL_PARTY:
                if (Group* group = bot->GetGroup())
                {
                    WorldPacket data;
                    ChatHandler::BuildChatPacket(data, CHAT_MSG_PARTY, LANG_UNIVERSAL, bot, nullptr, line.text);
                    group->BroadcastPacket(&data, false);
                }
                break;
            case CHANNEL_GUILD:
                if (Guild* guild = bot->GetGuild())
                    guild->BroadcastToGuild(bot->GetSession(), false, line.text, LANG_UNIVERSAL);
                break;
        }
    }
}

class WowPlusBotChatWorld : public WorldScript
{
public:
    WowPlusBotChatWorld() : WorldScript("WowPlusBotChatWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("WowPlus.BotChat.Enable", true);
        bridgePort = std::to_string(sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Port", 8586));
        sayEnabled = sConfigMgr->GetOption<bool>("WowPlus.BotChat.Say", true);
        partyEnabled = sConfigMgr->GetOption<bool>("WowPlus.BotChat.Party", true);
        guildEnabled = sConfigMgr->GetOption<bool>("WowPlus.BotChat.Guild", true);
        sayClosestRadius = sConfigMgr->GetOption<float>("WowPlus.BotChat.Say.ClosestRadius", 8.0f);
        partyUnaddressedChance = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Party.UnaddressedChance", 50);
        guildUnaddressedChance = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Guild.UnaddressedChance", 35);
    }

    void OnUpdate(uint32 diff) override
    {
        std::vector<Reply> arrived;
        {
            std::lock_guard<std::mutex> lock(replyMutex);
            arrived.swap(replies);
        }

        // Several lines come one after another, with a pause as if typed
        for (Reply const& reply : arrived)
        {
            uint32 delay = 0;
            for (std::string const& text : reply.lines)
            {
                pendingLines.push_back({delay, reply.bot, reply.player, reply.channel, text});
                delay += 1200 + uint32(text.size()) * 40;
            }
        }

        for (auto line = pendingLines.begin(); line != pendingLines.end();)
        {
            if (line->delay > diff)
            {
                line->delay -= diff;
                ++line;
                continue;
            }

            Speak(*line);
            line = pendingLines.erase(line);
        }
    }
};

class WowPlusBotChatPlayer : public PlayerScript
{
public:
    WowPlusBotChatPlayer() : PlayerScript("WowPlusBotChatPlayer") { }

    using PlayerScript::OnPlayerCanUseChat;                 // keep the other overloads visible

    // /say
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg) override
    {
        if (!enabled || !sayEnabled || type != CHAT_MSG_SAY || language == LANG_ADDON || IsBot(player))
            return true;

        std::vector<Player*> nearby;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
                if (other != player && other->IsInWorld() && IsBot(other) && other->IsAlive() && other->GetMap() == player->GetMap() &&
                    other->IsWithinDist(player, sayNameRadius))
                    nearby.push_back(other);
        }

        Player* speaker = nullptr;
        for (Player* bot : nearby)
            if (Mentions(msg, bot->GetName()))
                speaker = bot;

        if (!speaker)
            if (Player* target = player->GetSelectedPlayer())
                if (target != player && IsBot(target) && target->IsWithinDist(player, sayNameRadius))
                    speaker = target;

        if (!speaker)
        {
            float closest = sayClosestRadius;
            for (Player* bot : nearby)
                if (float const distance = bot->GetDistance(player); distance < closest)
                {
                    closest = distance;
                    speaker = bot;
                }
        }

        if (speaker && !IsBotCommand(speaker, msg))
            Ask(speaker, player, CHANNEL_SAY, msg, nearby);

        return true;
    }

    // Whisper
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* receiver) override
    {
        if (!enabled || type != CHAT_MSG_WHISPER || language == LANG_ADDON || !receiver || IsBot(player) || !IsBot(receiver))
            return true;

        if (!IsBotCommand(receiver, msg))
            Ask(receiver, player, CHANNEL_WHISPER, msg, {});

        return true;
    }

    // Party chat
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Group* group) override
    {
        if (!enabled || !partyEnabled || (type != CHAT_MSG_PARTY && type != CHAT_MSG_PARTY_LEADER) || language == LANG_ADDON || IsBot(player))
            return true;

        std::vector<Player*> bots;
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            if (Player* member = itr->GetSource())
                if (member != player && member->IsInWorld() && IsBot(member))
                    bots.push_back(member);

        if (Player* speaker = ChooseSpeaker(player, CHANNEL_PARTY, msg, bots, partyUnaddressedChance))
            if (!IsBotCommand(speaker, msg))
                Ask(speaker, player, CHANNEL_PARTY, msg, bots);

        return true;
    }

    // Guild chat
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Guild* /*guild*/) override
    {
        if (!enabled || !guildEnabled || type != CHAT_MSG_GUILD || language == LANG_ADDON || IsBot(player) || !player->GetGuildId())
            return true;

        std::vector<Player*> bots;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
                if (other != player && other->IsInWorld() && IsBot(other) && other->GetGuildId() == player->GetGuildId())
                    bots.push_back(other);
        }

        if (Player* speaker = ChooseSpeaker(player, CHANNEL_GUILD, msg, bots, guildUnaddressedChance))
            if (!IsBotCommand(speaker, msg))
                Ask(speaker, player, CHANNEL_GUILD, msg, bots);

        return true;
    }
};

void AddSC_WowPlusBotChat()
{
    new WowPlusBotChatWorld();
    new WowPlusBotChatPlayer();
}
