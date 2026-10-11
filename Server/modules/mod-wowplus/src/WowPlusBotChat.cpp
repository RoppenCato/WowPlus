/*
 * WowPlus bot chat: bots talk through Claude. See Settings/Docs/Bot-Chat.md.
 *
 * This file is only the game's side of it. When a real player says something a bot should
 * answer, or it is time for a bot to say something by itself, the bot's and the player's
 * details go to the bridge, a small program in its own window (Start-Bot-Chat.bat,
 * Settings/Scripts/Bot-Chat-Bridge.py). The bridge knows who each bot is, remembers earlier
 * talks, asks Claude, and sends lines back, each with the name of the bot that says it. If the
 * bridge is not running, bots simply stay quiet.
 *
 * Who answers the player:
 * - a whisper: the bot whispered to
 * - /say: a nearby bot named in the text, else the bot the player has targeted, else the
 *   closest bot if one stands right next to the player
 * - party and guild chat: a bot named in the text, else the bot that answered last (a talk
 *   that is going on), else sometimes one of them (the chance is a setting)
 * Something that is a bot command ("follow", "stay", ...) is left to the bot module, unless it
 * is a question or a whole sentence.
 *
 * Bots speaking first:
 * - a guild mate greets the player who comes online
 * - every few minutes a guild mate says something in guild chat, and sometimes another answers.
 *   It ends there unless the player joins in; while the player stays silent the pauses grow.
 * - moments: a level-up, a death, a good piece of loot, a rare or a boss killed. About one in
 *   three gets a comment from one party member or guild mate.
 * - meeting: a guild mate, or a bot the player has talked with before, that comes close out in
 *   the world waves and says something.
 * - a gift: now and then a guild mate sends a letter with a few potions.
 * - companions remark on things in party chat (asked for by WowPlusBots.cpp)
 * Bots are also handed a true rumour about a vendor's surprise stock now and then, to pass on.
 *
 * Asking Claude takes seconds, so it happens on helper threads; the answer is handed back to
 * the world thread, which makes the bots speak.
 */

#include "WowPlusBotChat.h"

#include "Channel.h"
#include "ChannelMgr.h"
#include "Chat.h"
#include "ChatHelper.h"
#include "Config.h"
#include "Creature.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "Guild.h"
#include "Item.h"
#include "Log.h"
#include "Mail.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Playerbots.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include "WowPlusItems.h"
#include "WowPlusJournal.h"

#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    enum Channel : uint8
    {
        CHANNEL_WHISPER,
        CHANNEL_SAY,
        CHANNEL_PARTY,
        CHANNEL_GUILD,
        CHANNEL_ZONE,                                       // General, Trade and the like: the reply goes to the same channel
        CHANNEL_KNOWN,                                      // not a chat: the bridge lists the bots that remember the player
        CHANNEL_MAIL                                        // not a chat: the bridge writes a letter
    };

    char const* const CHANNEL_NAMES[] = {"whisper", "say", "party", "guild", "general", "known", "mail"};
    constexpr uint32 TALK_CHANNELS = 5;

    constexpr uint32 MAX_REQUESTS_AT_ONCE = 4;
    constexpr uint32 REPLY_TIMEOUT_SECONDS = 150;
    constexpr uint32 TALK_GOES_ON_SECONDS = 120;            // the bot that answered last keeps answering this long
    constexpr uint32 MAX_TEXT_LENGTH = 250;
    constexpr uint32 COMMAND_MAX_WORDS = 4;                 // anything longer is talk, not a bot command
    constexpr uint32 TICK = 5000;                           // ms between looks at who might speak first
    constexpr uint32 MAX_PAUSE_GROWTH = 3;                  // unanswered openers stretch the pause up to this many times
    constexpr uint32 MOMENT_PAUSE = 45 * IN_MILLISECONDS;   // at least this long between two comments on moments
    constexpr float MEET_RADIUS = 20.0f;
    constexpr uint32 MEET_PAUSE = 3 * MINUTE * IN_MILLISECONDS;     // between two meetings
    constexpr time_t MEET_AGAIN_SECONDS = 3 * HOUR;         // the same bot does not greet again sooner than this

    bool enabled = true;
    std::string bridgePort = "8586";
    bool sayEnabled = true;
    bool partyEnabled = true;
    bool guildEnabled = true;
    float sayNameRadius = 30.0f;
    float sayClosestRadius = 8.0f;
    uint32 partyUnaddressedChance = 50;
    uint32 guildUnaddressedChance = 35;
    bool zoneChannelsEnabled = true;
    uint32 zoneUnaddressedChance = 25;
    bool guildOpeners = true;
    uint32 guildOpenMinMinutes = 5;
    uint32 guildOpenMaxMinutes = 10;
    uint32 guildBanterChance = 50;
    uint32 guildGreetChance = 70;
    uint32 momentChance = 33;
    uint32 meetChance = 60;
    uint32 rumourChance = 30;
    uint32 giftHours = 3;

    struct SpokenLine
    {
        std::string speaker;                                // the bot's name
        std::string text;
    };

    struct Reply
    {
        ObjectGuid player;
        Channel channel;
        std::vector<SpokenLine> lines;
        std::string channelName;                            // for CHANNEL_ZONE
    };

    struct PendingLine
    {
        uint32 delay;                                       // ms until the bot says it
        ObjectGuid player;
        Channel channel;
        SpokenLine line;
        std::string channelName;
    };

    struct LastTalk
    {
        ObjectGuid bot;
        time_t at = 0;
    };

    // What the bots around one real player are up to
    struct PlayerTalk
    {
        bool askedWhoKnows = false;
        std::unordered_set<std::string> knownBots;          // bots that remember this player from before
        std::unordered_map<ObjectGuid, time_t> met;         // bot -> when it last greeted the player in the world
        uint32 untilOpener = 0;                             // ms
        uint32 untilGreeting = 0;                           // ms, 0: no greeting due
        uint32 unanswered = 0;                              // openers since the player last spoke in guild chat
        uint32 sinceMoment = MOMENT_PAUSE;                  // ms since the last comment on a moment
        uint32 sinceMeeting = MEET_PAUSE;                   // ms since the last meeting
        uint32 untilGift = 0;                               // ms, 0: not set yet
        uint32 giftItem = 0;                                // the gift a letter is being written for
        uint32 giftCount = 0;
    };

    // Shared with the helper threads. Never freed, so a thread that outlives the shutdown cannot touch freed memory.
    std::mutex& replyMutex = *new std::mutex();
    std::vector<Reply>& replies = *new std::vector<Reply>();
    std::atomic<uint32>& requestsRunning = *new std::atomic<uint32>(0);
    std::atomic<bool>& bridgeWasMissing = *new std::atomic<bool>(false);

    // World thread only
    std::vector<PendingLine> pendingLines;
    std::unordered_map<ObjectGuid, LastTalk> lastTalk[TALK_CHANNELS];   // per channel: real player -> the bot that answered last
    std::unordered_map<ObjectGuid, PlayerTalk> playerTalk;
    uint32 sinceTick = 0;

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

    char const* GenderOf(Player* player)
    {
        return player->getGender() == GENDER_FEMALE ? "female" : "male";
    }

    // Runs on a helper thread. Posts the form to the bridge and reads the lines: "bot name<tab>text".
    void AskBridge(std::string port, std::string body, ObjectGuid player, Channel channel, std::string channelName)
    {
        std::vector<SpokenLine> lines;
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
                    {
                        inBody = line.empty();
                        continue;
                    }

                    std::size_t const tab = line.find('\t');
                    if (tab != std::string::npos && tab > 0 && tab + 1 < line.size())
                        lines.push_back({line.substr(0, tab), line.substr(tab + 1, channel == CHANNEL_MAIL ? 1000 : 255)});
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
            replies.push_back({player, channel, std::move(lines), std::move(channelName)});
        }

        --requestsRunning;
    }

    /*
     * kind: "reply"  the player said `text`
     *       "open"   the bot starts talking
     *       "greet"  the player came online
     *       "event"  something happened, described in `text`
     *       "meet"   the bot ran into the player out in the world
     *       "letter" the bot sends the gift named in `text` by mail
     *       "known"  no bot: which bots remember this player?
     */
    void Ask(Player* bot, Player* player, Channel channel, char const* kind, std::string const& text,
        std::vector<Player*> const& others, bool banter = false, std::string const& channelName = "")
    {
        if (requestsRunning >= MAX_REQUESTS_AT_ONCE)
            return;

        // The other bots present, as "name:gender:race:class:level"
        std::string otherBots;
        uint32 listed = 0;
        for (Player* other : others)
            if (other != bot && listed++ < 8)
                otherBots += (otherBots.empty() ? "" : ",") + other->GetName() + ":" + GenderOf(other) + ":" + RaceOf(other) +
                    ":" + ClassOf(other) + ":" + std::to_string(other->GetLevel());

        std::ostringstream body;
        body << "kind=" << kind
             << "&player=" << UrlEncode(player->GetName())
             << "&pclass=" << UrlEncode(ClassOf(player)) << "&prace=" << UrlEncode(RaceOf(player))
             << "&plevel=" << uint32(player->GetLevel()) << "&pzone=" << UrlEncode(ZoneOf(player))
             << "&channel=" << CHANNEL_NAMES[channel]
             << "&banter=" << (banter ? 1 : 0)
             << "&others=" << UrlEncode(otherBots)
             << "&text=" << UrlEncode(text.substr(0, MAX_TEXT_LENGTH));

        if (bot)
        {
            bool const sameGuild = player->GetGuildId() && player->GetGuildId() == bot->GetGuildId();
            bool const sameParty = player->GetGroup() && player->GetGroup() == bot->GetGroup();
            body << "&bot=" << UrlEncode(bot->GetName())
                 << "&bclass=" << UrlEncode(ClassOf(bot)) << "&brace=" << UrlEncode(RaceOf(bot))
                 << "&blevel=" << uint32(bot->GetLevel()) << "&bzone=" << UrlEncode(ZoneOf(bot))
                 << "&bgender=" << GenderOf(bot)
                 << "&guildmate=" << (sameGuild ? 1 : 0) << "&party=" << (sameParty ? 1 : 0)
                 << "&guild=" << UrlEncode(sameGuild ? player->GetGuildName() : "");

            // Something true the bot may pass on: always at hand when answering, sometimes when starting a talk
            bool const answering = std::string_view(kind) == "reply";
            if (answering || (std::string_view(kind) == "open" && roll_chance_i(int32(rumourChance))))
            {
                // A vendor's odd item or a champion's whereabouts
                std::string rumour = roll_chance_i(50) ? WowPlus::ChampionRumour(player->GetLevel()) : "";
                if (rumour.empty())
                    rumour = WowPlus::VendorRumour();
                body << "&rumour=" << UrlEncode(rumour);
            }
        }

        ++requestsRunning;
        std::thread(AskBridge, bridgePort, body.str(), player->GetGUID(), channel, channelName).detach();

        if (bot && channel != CHANNEL_WHISPER && channel < TALK_CHANNELS)
            lastTalk[channel][player->GetGUID()] = {bot->GetGUID(), time(nullptr)};
    }

    // The same test the bot module uses to decide whether a line is a command for the bot
    bool IsBotCommand(Player* bot, std::string const& text)
    {
        if (text.find('?') != std::string::npos)
            return false;                                   // a question is talk, whatever word it starts with

        if (std::count(text.begin(), text.end(), ' ') >= int32(COMMAND_MAX_WORDS))
            return false;                                   // so is a whole sentence

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

    std::vector<Player*> GuildBotsOf(Player* player)
    {
        std::vector<Player*> bots;
        if (!player->GetGuildId())
            return bots;

        std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
            if (other != player && other->IsInWorld() && IsBot(other) && other->GetGuildId() == player->GetGuildId())
                bots.push_back(other);

        return bots;
    }

    std::vector<Player*> PartyBotsOf(Player* player)
    {
        std::vector<Player*> bots;
        if (Group* group = player->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                if (Player* member = itr->GetSource())
                    if (member != player && member->IsInWorld() && IsBot(member))
                        bots.push_back(member);

        return bots;
    }

    std::vector<Player*> BotsNear(Player* player, float radius)
    {
        std::vector<Player*> bots;
        std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
            if (other != player && other->IsInWorld() && IsBot(other) && other->IsAlive() && other->GetMap() == player->GetMap() &&
                other->IsWithinDist(player, radius))
                bots.push_back(other);

        return bots;
    }

    void Speak(PendingLine const& pending)
    {
        Player* bot = ObjectAccessor::FindPlayerByName(pending.line.speaker);
        Player* player = ObjectAccessor::FindConnectedPlayer(pending.player);
        if (!bot || !player || !IsBot(bot) || !player->IsInWorld())
            return;

        std::string const& text = pending.line.text;
        switch (pending.channel)
        {
            case CHANNEL_WHISPER:
                bot->Whisper(text, LANG_UNIVERSAL, player);
                break;
            case CHANNEL_SAY:
                bot->Say(text, LANG_UNIVERSAL);
                break;
            case CHANNEL_PARTY:
                if (Group* group = bot->GetGroup(); group && group == player->GetGroup())
                {
                    WorldPacket data;
                    ChatHandler::BuildChatPacket(data, CHAT_MSG_PARTY, LANG_UNIVERSAL, bot, nullptr, text);
                    group->BroadcastPacket(&data, false);
                }
                break;
            case CHANNEL_GUILD:
                if (Guild* guild = bot->GetGuild(); guild && bot->GetGuildId() == player->GetGuildId())
                    guild->BroadcastToGuild(bot->GetSession(), false, text, LANG_UNIVERSAL);
                break;
            case CHANNEL_ZONE:
                if (ChannelMgr* channels = ChannelMgr::forTeam(bot->GetTeamId()))
                    if (::Channel* chat = channels->GetChannel(pending.channelName, bot, false))
                        chat->Say(bot->GetGUID(), text, LANG_UNIVERSAL);  // says nothing if the bot is not in the channel
                break;
            default:
                break;
        }
    }

    // The letter has been written: lines are the subject and the body. The gift was chosen when it was asked for.
    void SendGift(Reply const& reply)
    {
        Player* player = ObjectAccessor::FindConnectedPlayer(reply.player);
        if (!player || reply.lines.size() < 2)
            return;

        PlayerTalk& talk = playerTalk[player->GetGUID()];
        Player* bot = ObjectAccessor::FindPlayerByName(reply.lines[0].speaker);
        if (!bot || !IsBot(bot) || !talk.giftItem)
            return;

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        MailDraft draft(reply.lines[0].text, reply.lines[1].text);
        if (Item* item = Item::CreateItem(talk.giftItem, talk.giftCount))
        {
            item->SaveToDB(trans);
            draft.AddItem(item);
        }
        draft.SendMailTo(trans, MailReceiver(player), MailSender(bot));
        CharacterDatabase.CommitTransaction(trans);

        if (ItemTemplate const* gift = sObjectMgr->GetItemTemplate(talk.giftItem))
            WowPlus::Journal(player, WowPlus::JOURNAL_PEOPLE, "A letter from " + bot->GetName() + ", with " + std::to_string(talk.giftCount) + " x " + gift->Name1 + ".");
        talk.giftItem = 0;
    }

    uint32 NextOpenerIn(PlayerTalk const& talk)
    {
        uint32 const growth = 1 + std::min(talk.unanswered, MAX_PAUSE_GROWTH - 1);
        return urand(guildOpenMinMinutes, std::max(guildOpenMinMinutes, guildOpenMaxMinutes)) * MINUTE * IN_MILLISECONDS * growth;
    }

    uint32 NextGiftIn()
    {
        // Half to one and a half times the setting
        return urand(giftHours * 30, giftHours * 90) * MINUTE * IN_MILLISECONDS;
    }

    // Something happened to the player; one bot of the party (or else of the guild) may comment
    void Moment(Player* player, std::string const& what, bool partyFirst, bool always = false)
    {
        if (!enabled || IsBot(player))
            return;

        PlayerTalk& talk = playerTalk[player->GetGUID()];
        if (talk.sinceMoment < MOMENT_PAUSE || (!always && !roll_chance_i(int32(momentChance))))
            return;

        std::vector<Player*> const party = partyEnabled ? PartyBotsOf(player) : std::vector<Player*>();
        std::vector<Player*> const guild = guildEnabled ? GuildBotsOf(player) : std::vector<Player*>();
        bool const inParty = !party.empty() && (partyFirst || guild.empty());
        std::vector<Player*> const& bots = inParty ? party : guild;
        if (bots.empty())
            return;

        talk.sinceMoment = 0;
        Ask(bots[urand(0, bots.size() - 1)], player, inParty ? CHANNEL_PARTY : CHANNEL_GUILD, "event", what, bots);
    }

    // A guild mate, or a bot that remembers the player, comes close out in the world
    void TryMeeting(Player* player, PlayerTalk& talk)
    {
        if (!sayEnabled || talk.sinceMeeting < MEET_PAUSE || !player->IsAlive() || player->IsInCombat() || player->IsInFlight() ||
            player->GetMap()->IsDungeon() || player->HasStealthAura())
            return;

        time_t const now = time(nullptr);
        for (Player* bot : BotsNear(player, MEET_RADIUS))
        {
            bool const guildMate = player->GetGuildId() && bot->GetGuildId() == player->GetGuildId();
            if ((!guildMate && !talk.knownBots.count(bot->GetName())) || bot->IsInCombat() || bot->GetTeamId() != player->GetTeamId() ||
                (player->GetGroup() && player->GetGroup() == bot->GetGroup()))
                continue;

            auto met = talk.met.find(bot->GetGUID());
            if (met != talk.met.end() && now - met->second < MEET_AGAIN_SECONDS)
                continue;

            talk.met[bot->GetGUID()] = now;                 // seen each other, whether or not anything is said
            if (!roll_chance_i(int32(meetChance)))
                continue;

            talk.sinceMeeting = 0;
            WowPlus::Journal(player, WowPlus::JOURNAL_PEOPLE, "Ran into " + bot->GetName() + " in " + WowPlus::PlaceOf(player) + ".");
            bot->SetFacingToObject(player);
            bot->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
            Ask(bot, player, CHANNEL_SAY, "meet", "", {});
            return;
        }
    }

    // A guild mate picks a few potions for the player's level and has a letter written to go with them
    void TryGift(Player* player, PlayerTalk& talk)
    {
        std::vector<Player*> const bots = GuildBotsOf(player);
        if (bots.empty())
            return;

        // The best potions the player can already drink
        WowPlus::ItemList choice;
        for (uint32 level = player->GetLevel(); level > 0 && level + 8 > player->GetLevel() && choice.empty(); --level)
            choice = WowPlus::Consumables(level);

        if (choice.empty())
            return;

        ItemTemplate const* gift = choice[urand(0, choice.size() - 1)];
        talk.giftItem = gift->ItemId;
        talk.giftCount = std::min<uint32>(urand(2, 4), gift->GetMaxStackSize());
        Ask(bots[urand(0, bots.size() - 1)], player, CHANNEL_MAIL, "letter", std::to_string(talk.giftCount) + " x " + gift->Name1, bots);
    }

    // Bots speaking first, for every real player
    void UpdateSpeakingFirst(uint32 elapsed)
    {
        std::vector<Player*> realPlayers;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
                if (player->IsInWorld() && !IsBot(player) && !player->IsBeingTeleported())
                    realPlayers.push_back(player);
        }

        for (Player* player : realPlayers)
        {
            auto [entry, isNew] = playerTalk.try_emplace(player->GetGUID());
            PlayerTalk& talk = entry->second;
            talk.sinceMoment += elapsed;
            talk.sinceMeeting += elapsed;

            if (!talk.untilGift)
            {
                // First look at this player since they came online
                talk.untilOpener = NextOpenerIn(talk);
                talk.untilGreeting = roll_chance_i(int32(guildGreetChance)) ? urand(8, 20) * IN_MILLISECONDS : 0;
                talk.untilGift = std::max<uint32>(1, NextGiftIn());
            }

            if (!talk.askedWhoKnows)
            {
                talk.askedWhoKnows = true;
                Ask(nullptr, player, CHANNEL_KNOWN, "known", "", {});
            }

            if (meetChance)
                TryMeeting(player, talk);

            if (!guildEnabled || !player->GetGuildId())
                continue;

            if (talk.untilGreeting)
            {
                if (talk.untilGreeting > elapsed)
                    talk.untilGreeting -= elapsed;
                else
                {
                    talk.untilGreeting = 0;
                    std::vector<Player*> const bots = GuildBotsOf(player);
                    if (!bots.empty())
                        Ask(bots[urand(0, bots.size() - 1)], player, CHANNEL_GUILD, "greet", "", bots);
                }
                continue;
            }

            if (giftHours)
            {
                if (talk.untilGift > elapsed)
                    talk.untilGift -= elapsed;
                else
                {
                    talk.untilGift = std::max<uint32>(1, NextGiftIn());
                    TryGift(player, talk);
                }
            }

            if (!guildOpeners)
                continue;

            if (talk.untilOpener > elapsed)
            {
                talk.untilOpener -= elapsed;
                continue;
            }

            std::vector<Player*> const bots = GuildBotsOf(player);
            if (!bots.empty())
            {
                Ask(bots[urand(0, bots.size() - 1)], player, CHANNEL_GUILD, "open", "", bots,
                    bots.size() >= 2 && roll_chance_i(int32(guildBanterChance)));
                ++talk.unanswered;
            }
            talk.untilOpener = NextOpenerIn(talk);
        }
    }
}

void WowPlus::BotRemarksInParty(Player* bot, Player* player)
{
    if (enabled && partyEnabled)
        Ask(bot, player, CHANNEL_PARTY, "open", "", PartyBotsOf(player));
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
        guildOpeners = sConfigMgr->GetOption<bool>("WowPlus.BotChat.Guild.SpeakFirst", true);
        guildOpenMinMinutes = std::max<uint32>(1, sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Guild.SpeakFirst.MinMinutes", 5));
        guildOpenMaxMinutes = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Guild.SpeakFirst.MaxMinutes", 10);
        guildBanterChance = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Guild.SpeakFirst.AnswerChance", 50);
        guildGreetChance = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Guild.GreetChance", 70);
        zoneChannelsEnabled = sConfigMgr->GetOption<bool>("WowPlus.BotChat.Channels", true);
        zoneUnaddressedChance = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Channels.UnaddressedChance", 25);
        momentChance = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Moments.Chance", 33);
        meetChance = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Meet.Chance", 60);
        rumourChance = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Rumour.Chance", 30);
        giftHours = sConfigMgr->GetOption<uint32>("WowPlus.BotChat.Guild.GiftHours", 3);
    }

    void OnUpdate(uint32 diff) override
    {
        if (!enabled)
            return;

        std::vector<Reply> arrived;
        {
            std::lock_guard<std::mutex> lock(replyMutex);
            arrived.swap(replies);
        }

        for (Reply const& reply : arrived)
        {
            if (reply.channel == CHANNEL_KNOWN)
            {
                for (SpokenLine const& line : reply.lines)
                    playerTalk[reply.player].knownBots.insert(line.speaker);
                continue;
            }

            if (reply.channel == CHANNEL_MAIL)
            {
                SendGift(reply);
                continue;
            }

            // Lines come one after another with a pause as if typed, longer when another bot takes over
            uint32 delay = 0;
            std::string previous;
            for (SpokenLine const& line : reply.lines)
            {
                if (!previous.empty() && previous != line.speaker)
                    delay += urand(2500, 6000);

                pendingLines.push_back({delay, reply.player, reply.channel, line, reply.channelName});
                delay += 1200 + uint32(line.text.size()) * 40;
                previous = line.speaker;
            }

            // A bot that has talked with the player privately knows them from now on
            if ((reply.channel == CHANNEL_WHISPER || reply.channel == CHANNEL_SAY) && !reply.lines.empty())
                if (playerTalk[reply.player].knownBots.insert(reply.lines[0].speaker).second)
                    if (Player* player = ObjectAccessor::FindConnectedPlayer(reply.player))
                        WowPlus::Journal(player, WowPlus::JOURNAL_PEOPLE, "Got talking with " + reply.lines[0].speaker + " in " + WowPlus::PlaceOf(player) + ".");
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

        sinceTick += diff;
        if (sinceTick >= TICK)
        {
            UpdateSpeakingFirst(sinceTick);
            sinceTick = 0;
        }
    }
};

class WowPlusBotChatPlayer : public PlayerScript
{
public:
    WowPlusBotChatPlayer() : PlayerScript("WowPlusBotChatPlayer") { }

    using PlayerScript::OnPlayerCanUseChat;                 // keep the other overloads visible

    void OnPlayerLogout(Player* player) override
    {
        playerTalk.erase(player->GetGUID());
        for (auto& channel : lastTalk)
            channel.erase(player->GetGUID());
    }

    // Moments

    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (player->GetLevel() > oldLevel && !IsBot(player) && player->IsInWorld())
            Moment(player, player->GetName() + " just reached level " + std::to_string(player->GetLevel()) + ".", false,
                player->GetLevel() % 10 == 0);
    }

    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        if (!IsBot(killed) && killed->GetGroup())
            Moment(killed, killed->GetName() + " just died, killed by " + killer->GetName() + ".", true);
    }

    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override
    {
        // Another module may have sold the item already (grey loot): then there is nothing to say
        if (IsBot(player) || !WowPlus::StillCarried(player, item))
            return;

        ItemTemplate const* proto = item->GetTemplate();
        bool const worthAWord = proto->Quality >= ITEM_QUALITY_RARE &&
            (proto->Class == ITEM_CLASS_WEAPON || proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_MISC);
        if (worthAWord)
            Moment(player, player->GetName() + " just looted " + proto->Name1 + (proto->Quality >= ITEM_QUALITY_EPIC ? ", an epic item." : ", a rare (blue) item."), true);
    }

    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        if (IsBot(killer))
            return;

        uint32 const rank = killed->GetCreatureTemplate()->rank;
        if (killed->IsDungeonBoss() || killed->isWorldBoss())
            Moment(killer, "The party just killed the boss " + killed->GetName() + ".", true);
        else if (rank == CREATURE_ELITE_RARE || rank == CREATURE_ELITE_RAREELITE)
            Moment(killer, killer->GetName() + " just killed " + killed->GetName() + ", a rare enemy.", true);
    }

    // /say
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg) override
    {
        if (!enabled || !sayEnabled || type != CHAT_MSG_SAY || language == LANG_ADDON || IsBot(player))
            return true;

        std::vector<Player*> const nearby = BotsNear(player, sayNameRadius);

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
            Ask(speaker, player, CHANNEL_SAY, "reply", msg, nearby);

        return true;
    }

    // Whisper
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* receiver) override
    {
        if (!enabled || type != CHAT_MSG_WHISPER || language == LANG_ADDON || !receiver || IsBot(player) || !IsBot(receiver))
            return true;

        if (!IsBotCommand(receiver, msg))
            Ask(receiver, player, CHANNEL_WHISPER, "reply", msg, {});

        return true;
    }

    // Party chat
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Group* /*group*/) override
    {
        if (!enabled || !partyEnabled || (type != CHAT_MSG_PARTY && type != CHAT_MSG_PARTY_LEADER) || language == LANG_ADDON || IsBot(player))
            return true;

        std::vector<Player*> const bots = PartyBotsOf(player);
        if (Player* speaker = ChooseSpeaker(player, CHANNEL_PARTY, msg, bots, partyUnaddressedChance))
            if (!IsBotCommand(speaker, msg))
                Ask(speaker, player, CHANNEL_PARTY, "reply", msg, bots);

        return true;
    }

    // General, Trade, LocalDefense and the like: the bots that are in the channel and in the player's zone
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, ::Channel* channel) override
    {
        if (!enabled || !zoneChannelsEnabled || type != CHAT_MSG_CHANNEL || language == LANG_ADDON || IsBot(player) || !channel ||
            !channel->IsConstant() || channel->IsLFG())
            return true;

        std::vector<Player*> bots;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
                if (other != player && other->IsInWorld() && IsBot(other) && other->GetZoneId() == player->GetZoneId() &&
                    other->GetTeamId() == player->GetTeamId())
                    bots.push_back(other);
        }

        if (Player* speaker = ChooseSpeaker(player, CHANNEL_ZONE, msg, bots, zoneUnaddressedChance))
            if (!IsBotCommand(speaker, msg))
                Ask(speaker, player, CHANNEL_ZONE, "reply", msg, bots, false, channel->GetName());

        return true;
    }

    // Guild chat
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Guild* /*guild*/) override
    {
        if (!enabled || !guildEnabled || type != CHAT_MSG_GUILD || language == LANG_ADDON || IsBot(player) || !player->GetGuildId())
            return true;

        // The player takes part: the bots' own remarks go back to their normal pace
        if (auto talk = playerTalk.find(player->GetGUID()); talk != playerTalk.end())
        {
            talk->second.unanswered = 0;
            talk->second.untilOpener = NextOpenerIn(talk->second);
        }

        std::vector<Player*> const bots = GuildBotsOf(player);
        if (Player* speaker = ChooseSpeaker(player, CHANNEL_GUILD, msg, bots, guildUnaddressedChance))
            if (!IsBotCommand(speaker, msg))
                Ask(speaker, player, CHANNEL_GUILD, "reply", msg, bots);

        return true;
    }
};

// A bot that joins the player's party or guild says hello there
class WowPlusBotChatGroup : public GroupScript
{
public:
    WowPlusBotChatGroup() : GroupScript("WowPlusBotChatGroup") { }

    void OnAddMember(Group* group, ObjectGuid guid) override
    {
        Player* bot = ObjectAccessor::FindConnectedPlayer(guid);
        if (!enabled || !partyEnabled || !bot || !IsBot(bot) || group->isRaidGroup())
            return;

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            if (Player* member = itr->GetSource())
                if (!IsBot(member))
                {
                    Ask(bot, member, CHANNEL_PARTY, "joined", "", {});
                    return;
                }
    }
};

class WowPlusBotChatGuild : public GuildScript
{
public:
    WowPlusBotChatGuild() : GuildScript("WowPlusBotChatGuild") { }

    void OnAddMember(Guild* guild, Player* bot, uint8& /*rank*/) override
    {
        if (!enabled || !guildEnabled || !bot || !IsBot(bot))
            return;

        Player* host = nullptr;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
                if (other->IsInWorld() && !IsBot(other) && other->GetGuildId() == guild->GetId())
                {
                    host = other;
                    break;
                }
        }

        if (host)
            Ask(bot, host, CHANNEL_GUILD, "joined", "", {});
    }
};

void AddSC_WowPlusBotChat()
{
    new WowPlusBotChatGroup();
    new WowPlusBotChatGuild();
    new WowPlusBotChatWorld();
    new WowPlusBotChatPlayer();
}
