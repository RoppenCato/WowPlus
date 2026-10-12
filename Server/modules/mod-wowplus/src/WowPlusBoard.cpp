/*
 * WowPlus group board: a list of people looking for a group, to choose company from.
 *
 * The addon WowPlusGroup shows a board of a dozen people who are "looking for group": bots that
 * are really in the world, near the player's level and free, always with a couple of tanks and
 * healers among them. Each has a note in their own words (written by the bot chat bridge). The
 * list changes as people come and go.
 *
 *   - Pick someone and they join. They do not appear out of thin air: they say hello, and a minute
 *     or so later they stand at the entrance of the dungeon the player chose, or walk up to the
 *     player when no place was chosen.
 *   - Or post what is needed ("tank and healer for Shadowfang Keep") and people whisper to ask
 *     for a place; accept or decline them on the board.
 *   - A party holds five. For a raid the group is made a raid, up to WowPlus.Board.RaidCap.
 *   - When the run is over and the player has been outside for a few minutes, the people from the
 *     board say goodbye and leave. The run is written into the journal with their names.
 *
 * The bot module teleports a bot to whoever invites it. That is switched off here
 * (Playerbots.SummonWhenGroup) and done by this file instead, at once for bots invited the ordinary
 * way and for companions, later for people from the board.
 *
 * Also here, because it is about joining a group: when a bot joins a real player's group, the loot
 * rule becomes "free for all", under which bots do not loot (WowPlus.Bots.PlayerLoots).
 *
 * The addon talks to this file by whispering its own character with the prefix "WPG".
 * Settings: WowPlus.Board.*
 */

#include "CharacterCache.h"
#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "Group.h"
#include "Guild.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WowPlusBoard.h"
#include "WowPlusBotChat.h"
#include "WowPlusJournal.h"

#include <algorithm>
#include <random>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace
{
    std::string const ADDON_PREFIX = "WPG\t";
    constexpr uint32 TICK = 1000;
    constexpr uint32 JOIN_TIMEOUT = 20 * IN_MILLISECONDS;   // an invited bot that has not joined by then is forgotten
    constexpr float NEAR_ENOUGH = 100.0f;                   // a bot this close simply walks over
    constexpr float ENTRANCE_RADIUS = 60.0f;                // the player is "at the entrance" within this distance
    constexpr uint32 MAX_APPLICANTS = 4;
    constexpr uint32 APPLICANT_WAITS = 3 * MINUTE;          // seconds an applicant stays on the board
    constexpr uint32 NOTES_PER_REQUEST = 8;
    constexpr uint8 DEATH_KNIGHT_FIRST_LEVEL = 55;
    constexpr uint32 LFG_TYPE_DUNGEON_ID = 1;
    constexpr uint32 LFG_TYPE_RAID_ID = 2;
    constexpr uint32 LFG_TYPE_HEROIC_ID = 5;

    bool enabled = true;
    uint32 listSize = 14;
    uint32 levelRange = 3;
    uint32 stayMinMinutes = 4;
    uint32 stayMaxMinutes = 10;
    uint32 busyChance = 8;
    uint32 raidCap = 10;
    uint32 travelMinSeconds = 40;
    uint32 travelMaxSeconds = 110;
    uint32 leaveAfterMinutes = 5;
    uint32 postMinutes = 5;
    uint32 applicantMinSeconds = 12;
    uint32 applicantMaxSeconds = 40;
    bool playerLoots = true;

    enum Role : uint8 { ROLE_TANK_BOT, ROLE_HEALER_BOT, ROLE_DAMAGE_BOT, ROLES };
    char const* const ROLE_NAMES[ROLES] = {"tank", "healer", "damage"};

    // Notes for when the bot chat bridge is not running
    char const* const PLAIN_NOTES[ROLES][4] = {
        {"tank lfg", "Tank, up for a dungeon", "can tank", "tank looking for a run"},
        {"healer lfg", "Healer, any dungeon", "can heal", "healer looking for group"},
        {"dps lfg", "LFG anything", "looking for a dungeon run", "dps, up for anything"},
    };

    struct Entry
    {
        ObjectGuid bot;
        std::string note;
        time_t until;
        bool applicant;
        bool noteAsked;
    };

    enum MemberState : uint8 { JOINING, TRAVELLING, WAITING, WITH_PLAYER, LEAVING };

    struct Member
    {
        ObjectGuid bot;
        MemberState state;
        uint32 timer;                                       // ms
        bool fromBoard;                                     // false: invited the ordinary way, or a companion
    };

    struct Board
    {
        std::vector<Entry> entries;
        std::vector<Member> members;
        uint32 destination = 0;                             // LFGDungeons.dbc id, 0: nowhere in particular
        time_t postUntil = 0;
        bool needs[ROLES] = {false, false, false};
        uint32 nextApplicant = 0;                           // ms
        uint32 nextRole = 0;
        bool wasInside = false;                             // the player has been in a dungeon with people from the board
        uint32 outside = 0;                                 // ms since then outside
        uint32 journalMap = 0;
        bool entranceAnnounced = false;
        bool open = false;                                  // the addon window has asked for the list at least once
    };

    std::unordered_map<ObjectGuid, Board> boards;
    uint32 sinceTick = 0;

    bool IsBot(Player const* player)
    {
        return player->GetSession()->IsHeadless();
    }

    void Send(Player* player, std::string const& text)
    {
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, ADDON_PREFIX + text);
        player->SendDirectMessage(&data);
    }

    void Tell(Player* player, std::string const& text)
    {
        Send(player, "M\t" + text);
    }

    Role RoleOf(Player* bot)
    {
        if (bot->GetLevel() < 10)                           // no talents yet: by what the class is for
            return bot->getClass() == CLASS_WARRIOR ? ROLE_TANK_BOT : bot->getClass() == CLASS_PRIEST ? ROLE_HEALER_BOT : ROLE_DAMAGE_BOT;

        return PlayerbotAI::IsTank(bot, true) ? ROLE_TANK_BOT : PlayerbotAI::IsHeal(bot, true) ? ROLE_HEALER_BOT : ROLE_DAMAGE_BOT;
    }

    bool Known(Player* player, Player* bot)
    {
        return (player->GetGuildId() && player->GetGuildId() == bot->GetGuildId()) ||
            (player->GetSocial() && player->GetSocial()->HasFriend(bot->GetGUID()));
    }

    bool SafeToMove(Player* bot)
    {
        return bot->IsInWorld() && bot->IsAlive() && !bot->IsInCombat() && !bot->IsInFlight() && !bot->IsBeingTeleported() &&
            !bot->InBattleground() && !bot->GetSession()->IsLoggingOut();
    }

    // Free to be asked along: a bot of the world, on the player's side, alone, not busy somewhere it cannot leave
    bool Free(Player* bot, Player* player)
    {
        return bot != player && bot->IsInWorld() && IsBot(bot) && sRandomPlayerbotMgr.IsRandomBot(bot) &&
            bot->GetTeamId() == player->GetTeamId() && !bot->GetGroup() && bot->IsAlive() && !bot->IsBeingTeleported() &&
            !bot->InBattleground() && !bot->GetMap()->IsDungeon() && !bot->GetSession()->IsLoggingOut();
    }

    std::vector<Player*> FreeBots(Player* player)
    {
        std::vector<Player*> found;
        std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        for (auto const& [guid, other] : ObjectAccessor::GetPlayers())
            if (Free(other, player))
                found.push_back(other);
        return found;
    }

    LFGDungeonEntry const* Destination(Board const& board)
    {
        return board.destination ? sLFGDungeonStore.LookupEntry(board.destination) : nullptr;
    }

    bool IsRaid(LFGDungeonEntry const* dungeon)
    {
        if (!dungeon)
            return false;
        MapEntry const* map = sMapStore.LookupEntry(dungeon->MapID);
        return dungeon->TypeID == LFG_TYPE_RAID_ID || (map && map->IsRaid());
    }

    uint32 GroupLimit(Board const& board)
    {
        return IsRaid(Destination(board)) ? std::clamp<uint32>(raidCap, 5, 40) : 5;
    }

    uint32 GroupSize(Player* player)
    {
        return player->GetGroup() ? player->GetGroup()->GetMembersCount() : 1;
    }

    bool OnBoard(Board const& board, ObjectGuid bot)
    {
        return std::any_of(board.entries.begin(), board.entries.end(), [bot](Entry const& e) { return e.bot == bot; });
    }

    Member* MemberOf(Board& board, ObjectGuid bot)
    {
        for (Member& member : board.members)
            if (member.bot == bot)
                return &member;
        return nullptr;
    }

    // ---------------------------------------------------------------------------------------
    // The list
    // ---------------------------------------------------------------------------------------

    // Someone from another level range becomes the player's level, the way guild mates keep up
    bool BringToLevel(Player* bot, Player* player)
    {
        uint8 const level = uint8(std::clamp<int32>(int32(player->GetLevel()) + irand(-1, 1), 1, int32(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))));
        if (!SafeToMove(bot) || bot->GetMap()->IsDungeon() || (bot->getClass() == CLASS_DEATH_KNIGHT && level < DEATH_KNIGHT_FIRST_LEVEL))
            return false;

        if (bot->IsMounted())
            bot->Dismount();

        Role const was = RoleOf(bot);
        uint8 const oldLevel = bot->GetLevel();
        PlayerbotFactory factory(bot, level);
        factory.Randomize(false);
        LOG_INFO("module", "WowPlus board: {} went from level {} to {} so that {} has a {} to ask.",
            bot->GetName(), oldLevel, level, player->GetName(), ROLE_NAMES[was]);
        return true;
    }

    std::string PlainNote(Player* bot)
    {
        return PLAIN_NOTES[RoleOf(bot)][bot->GetGUID().GetCounter() % 4];
    }

    void AddEntry(Board& board, Player* bot, bool applicant)
    {
        time_t const stays = applicant ? APPLICANT_WAITS : urand(stayMinMinutes * MINUTE, std::max(stayMinMinutes, stayMaxMinutes) * MINUTE);
        board.entries.push_back({bot->GetGUID(), "", time(nullptr) + stays, applicant, false});
    }

    // Takes off who has left or found something else, and fills the board up again
    void Refill(Player* player, Board& board)
    {
        time_t const now = time(nullptr);
        std::erase_if(board.entries, [&](Entry const& entry)
        {
            Player* bot = ObjectAccessor::FindConnectedPlayer(entry.bot);
            return !bot || !Free(bot, player) || entry.until < now;
        });

        uint32 have[ROLES] = {0, 0, 0};
        uint32 listed = 0;
        for (Entry const& entry : board.entries)
            if (!entry.applicant)
            {
                ++have[RoleOf(ObjectAccessor::FindConnectedPlayer(entry.bot))];
                ++listed;
            }

        std::vector<Player*> free = FreeBots(player);
        std::erase_if(free, [&](Player* bot) { return OnBoard(board, bot->GetGUID()); });
        std::shuffle(free.begin(), free.end(), std::mt19937(std::random_device{}()));

        int32 const level = player->GetLevel();
        auto within = [&](Player* bot, int32 range) { return std::abs(int32(bot->GetLevel()) - level) <= range; };
        auto take = [&](Player* bot)
        {
            AddEntry(board, bot, false);
            ++have[RoleOf(bot)];
            ++listed;
            std::erase(free, bot);
        };

        // People the player knows come first
        for (Player* bot : std::vector<Player*>(free))
            if (listed < listSize && Known(player, bot) && within(bot, int32(levelRange)))
                take(bot);

        // Always a couple of tanks and healers: from near the player's level, then from a little further,
        // and when there is nobody, one from elsewhere who becomes the player's level
        bool brought = false;
        for (Role role : {ROLE_TANK_BOT, ROLE_HEALER_BOT})
            for (int32 range : {int32(levelRange), int32(levelRange) * 2 + 2, 100})
                for (Player* bot : std::vector<Player*>(free))
                {
                    if (have[role] >= 2 || listed >= listSize)
                        break;
                    if (RoleOf(bot) != role || !within(bot, range))
                        continue;
                    if (range == 100)
                    {
                        if (brought || !BringToLevel(bot, player))
                            continue;
                        brought = true;
                    }
                    take(bot);
                }

        for (Player* bot : std::vector<Player*>(free))
            if (listed < listSize && within(bot, int32(levelRange)))
                take(bot);
    }

    std::string GuildOf(Player* bot)
    {
        return bot->GetGuildId() ? bot->GetGuildName() : "";
    }

    void SendState(Player* player, Board const& board)
    {
        std::ostringstream state;
        state << "S\t" << board.destination << '\t' << (board.postUntil > time(nullptr) ? 1 : 0) << '\t' << GroupLimit(board)
              << '\t' << GroupSize(player);
        Send(player, state.str());
    }

    void SendList(Player* player, Board const& board)
    {
        Send(player, "C");
        for (Entry const& entry : board.entries)
        {
            Player* bot = ObjectAccessor::FindConnectedPlayer(entry.bot);
            if (!bot)
                continue;

            std::ostringstream row;
            row << "E\t" << bot->GetName() << '\t' << uint32(bot->getClass()) << '\t' << ROLE_NAMES[RoleOf(bot)] << '\t'
                << uint32(bot->GetLevel()) << '\t' << GuildOf(bot) << '\t' << (Known(player, bot) ? 1 : 0) << '\t'
                << (entry.applicant ? 1 : 0) << '\t' << (entry.note.empty() ? PlainNote(bot) : entry.note);
            Send(player, row.str());
        }
        SendState(player, board);
        Send(player, "Z");
    }

    // The dungeons and raids for the player's level, to choose where the group is going
    void SendDungeons(Player* player)
    {
        uint32 const level = player->GetLevel();
        for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
        {
            LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i);
            if (!dungeon || level < dungeon->MinLevel || level > dungeon->MaxLevel || !dungeon->Name[0] || !*dungeon->Name[0])
                continue;
            if (dungeon->TypeID != LFG_TYPE_DUNGEON_ID && dungeon->TypeID != LFG_TYPE_RAID_ID && dungeon->TypeID != LFG_TYPE_HEROIC_ID)
                continue;

            std::ostringstream row;
            row << "G\t" << dungeon->ID << '\t' << dungeon->Name[0] << (dungeon->TypeID == LFG_TYPE_HEROIC_ID ? " (heroic)" : "")
                << '\t' << (IsRaid(dungeon) ? 1 : 0);
            Send(player, row.str());
        }
    }

    // Has the notes written for the people who have none yet, a few at a time
    void AskForNotes(Player* player, Board& board)
    {
        std::string people, dungeons;
        uint32 asked = 0;
        for (Entry& entry : board.entries)
        {
            Player* bot = ObjectAccessor::FindConnectedPlayer(entry.bot);
            if (!bot || entry.noteAsked || asked >= NOTES_PER_REQUEST)
                continue;

            ChrClassesEntry const* cls = sChrClassesStore.LookupEntry(bot->getClass());
            ChrRacesEntry const* race = sChrRacesStore.LookupEntry(bot->getRace());
            people += (people.empty() ? "" : ",") + bot->GetName() + ":" + (bot->getGender() == GENDER_FEMALE ? "female" : "male") + ":" +
                (race ? race->name[0] : "") + ":" + (cls ? cls->name[0] : "") + ":" + std::to_string(bot->GetLevel()) + ":" + ROLE_NAMES[RoleOf(bot)];
            ++asked;
        }
        if (!asked)
            return;

        uint32 const level = player->GetLevel();
        for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows() && dungeons.size() < 180; ++i)
            if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i))
                if (dungeon->TypeID == LFG_TYPE_DUNGEON_ID && level >= dungeon->MinLevel && level <= dungeon->MaxLevel && dungeon->Name[0])
                    dungeons += (dungeons.empty() ? "" : ", ") + std::string(dungeon->Name[0]);

        if (!WowPlus::AskBoardNotes(player, people, dungeons))
            return;                                         // busy or off: the plain notes stay, and the next look asks again

        uint32 marked = 0;
        for (Entry& entry : board.entries)
            if (!entry.noteAsked && marked++ < asked)
                entry.noteAsked = true;
    }

    // ---------------------------------------------------------------------------------------
    // Inviting, arriving, leaving
    // ---------------------------------------------------------------------------------------

    void Follow(Player* bot, bool follow)
    {
        if (PlayerbotAI* ai = GET_PLAYERBOT_AI(bot))
            ai->ChangeStrategy(follow ? "+follow,-stay" : "+stay,-follow", BOT_STATE_NON_COMBAT);
    }

    void BringTo(Player* bot, Player* player)
    {
        if (bot->IsMounted())
            bot->Dismount();
        bot->TeleportTo(player->GetMapId(), player->GetPositionX() + frand(-2.5f, 2.5f), player->GetPositionY() + frand(-2.5f, 2.5f),
            player->GetPositionZ() + 0.5f, player->GetOrientation());
        Follow(bot, true);
    }

    bool Far(Player* bot, Player* player)
    {
        return bot->GetMapId() != player->GetMapId() || !bot->IsWithinDist(player, NEAR_ENOUGH);
    }

    void Invite(Player* player, Board& board, std::string const& name)
    {
        Player* bot = ObjectAccessor::FindPlayerByName(name);
        auto entry = std::find_if(board.entries.begin(), board.entries.end(), [&](Entry const& e) { return bot && e.bot == bot->GetGUID(); });
        if (!bot || entry == board.entries.end() || !Free(bot, player))
        {
            Tell(player, name + " is no longer looking for a group.");
            if (entry != board.entries.end())
                board.entries.erase(entry);
            return;
        }

        Group* group = player->GetGroup();
        if (group && group->GetLeaderGUID() != player->GetGUID())
        {
            Tell(player, "Only the leader of your group can invite.");
            return;
        }

        uint32 const limit = GroupLimit(board);
        if (GroupSize(player) >= limit)
        {
            Tell(player, limit > 5 ? "Your raid is full: " + std::to_string(limit) + " is the most for a raid here."
                                   : "Your party is full. Choose a raid as the place to go for a larger group.");
            return;
        }

        if (!entry->applicant && !Known(player, bot) && roll_chance_i(int32(busyChance)))
        {
            Tell(player, bot->GetName() + " has just found a group.");
            board.entries.erase(entry);
            return;
        }

        if (group && !group->isRaidGroup() && group->GetMembersCount() >= 5)
            group->ConvertToRaid();

        sPlayerbotAIConfig.SummonWhenGroup = false;         // this file brings bots over, in its own time
        if (Member* member = MemberOf(board, bot->GetGUID()))
            *member = {bot->GetGUID(), JOINING, JOIN_TIMEOUT, true};
        else
            board.members.push_back({bot->GetGUID(), JOINING, JOIN_TIMEOUT, true});
        board.entries.erase(entry);

        WorldPacket invite(CMSG_GROUP_INVITE, bot->GetName().size() + 5);
        invite << bot->GetName();
        invite << uint32(0);
        player->GetSession()->HandleGroupInviteOpcode(invite);
    }

    // The bot's travel time is over: it stands at the entrance, or walks up to the player
    void Arrive(Player* player, Board& board, Member& member, Player* bot)
    {
        LFGDungeonEntry const* dungeon = member.fromBoard ? Destination(board) : nullptr;
        AreaTriggerTeleport const* entrance = dungeon ? sObjectMgr->GetGoBackTrigger(dungeon->MapID) : nullptr;
        bool const playerInside = player->GetMap()->IsDungeon();
        bool const playerAtEntrance = entrance && player->GetMapId() == entrance->target_mapId &&
            player->GetDistance(entrance->target_X, entrance->target_Y, entrance->target_Z) < ENTRANCE_RADIUS;

        if (!entrance || playerInside || playerAtEntrance)
        {
            BringTo(bot, player);
            member.state = WITH_PLAYER;
            return;
        }

        if (bot->IsMounted())
            bot->Dismount();
        bot->TeleportTo(entrance->target_mapId, entrance->target_X + frand(-3.0f, 3.0f), entrance->target_Y + frand(-3.0f, 3.0f),
            entrance->target_Z + 0.5f, entrance->target_Orientation);
        Follow(bot, false);
        member.state = WAITING;

        if (!board.entranceAnnounced)
        {
            board.entranceAnnounced = true;
            WowPlus::BotReactsInParty(bot, player, std::string("You have arrived at the entrance of ") + dungeon->Name[0] +
                " and wait there for the others. Say so in a few words.");
        }
    }

    void UpdateMembers(Player* player, Board& board)
    {
        Group* group = player->GetGroup();
        bool const inside = player->GetMap()->IsDungeon();
        uint32 fromBoardWithPlayer = 0;

        for (auto member = board.members.begin(); member != board.members.end();)
        {
            Player* bot = ObjectAccessor::FindConnectedPlayer(member->bot);
            bool const grouped = bot && group && bot->GetGroup() == group;
            member->timer = member->timer > TICK ? member->timer - TICK : 0;

            if (!bot || !bot->IsInWorld() || (!grouped && (member->state != JOINING || !member->timer)))
            {
                member = board.members.erase(member);
                continue;
            }

            switch (member->state)
            {
                case JOINING:
                    if (!grouped)
                        break;
                    if (!Far(bot, player))
                        member->state = WITH_PLAYER;        // close by: walks over like anyone
                    else
                    {
                        Follow(bot, false);                 // stops what it was doing; on its way
                        member->state = TRAVELLING;
                        member->timer = member->fromBoard ? urand(travelMinSeconds, std::max(travelMinSeconds, travelMaxSeconds)) * IN_MILLISECONDS : 1500;
                    }
                    break;
                case TRAVELLING:
                    if (member->timer)
                        break;
                    if (!bot->IsAlive())
                        bot->ResurrectPlayer(1.0f);
                    if (SafeToMove(bot))
                        Arrive(player, board, *member, bot);
                    else
                        member->timer = 5 * IN_MILLISECONDS;
                    break;
                case WAITING:
                    if (inside || (player->GetMapId() == bot->GetMapId() && player->IsWithinDist(bot, 40.0f)))
                    {
                        if (Far(bot, player) && SafeToMove(bot))
                            BringTo(bot, player);
                        else
                            Follow(bot, true);
                        member->state = WITH_PLAYER;
                    }
                    break;
                case LEAVING:
                    if (!member->timer)
                    {
                        Player::RemoveFromGroup(group, bot->GetGUID());
                        member = board.members.erase(member);
                        continue;
                    }
                    break;
                default:
                    break;
            }

            if (member->fromBoard && (member->state == WITH_PLAYER || member->state == WAITING))
                ++fromBoardWithPlayer;
            ++member;
        }

        if (board.members.empty())
            board.entranceAnnounced = false;

        // After the run: a few minutes outside and the people from the board go their own way
        if (inside && fromBoardWithPlayer)
        {
            board.wasInside = true;
            board.outside = 0;
        }
        else if (board.wasInside && !inside && leaveAfterMinutes)
        {
            board.outside += TICK;
            if (board.outside >= leaveAfterMinutes * MINUTE * IN_MILLISECONDS)
            {
                bool said = false;
                uint32 wait = 8 * IN_MILLISECONDS;
                for (Member& member : board.members)
                    if (member.fromBoard && member.state == WITH_PLAYER)
                    {
                        Player* bot = ObjectAccessor::FindConnectedPlayer(member.bot);
                        if (bot && !said)
                        {
                            WowPlus::BotReactsInParty(bot, player, "The run is over and you are about to leave the group and go your own way. Say goodbye in a few words.");
                            said = true;
                        }
                        member.state = LEAVING;
                        member.timer = wait;
                        wait += urand(3, 9) * IN_MILLISECONDS;
                    }
                board.wasInside = false;
                board.outside = 0;
                board.journalMap = 0;
            }
        }
    }

    // People answer the player's post
    void UpdatePost(Player* player, Board& board)
    {
        if (!board.postUntil)
            return;

        if (board.postUntil <= time(nullptr) || GroupSize(player) >= GroupLimit(board))
        {
            bool const full = GroupSize(player) >= GroupLimit(board);
            board.postUntil = 0;
            Tell(player, full ? "Your group is full; your post has been taken down." : "Your post has run out.");
            SendState(player, board);
            return;
        }

        board.nextApplicant = board.nextApplicant > TICK ? board.nextApplicant - TICK : 0;
        uint32 const waiting = uint32(std::count_if(board.entries.begin(), board.entries.end(), [](Entry const& e) { return e.applicant; }));
        if (board.nextApplicant || waiting >= MAX_APPLICANTS)
            return;

        board.nextApplicant = urand(applicantMinSeconds, std::max(applicantMinSeconds, applicantMaxSeconds)) * IN_MILLISECONDS;

        // The next role that was asked for, in turn
        Role role = ROLES;
        for (uint32 i = 0; i < ROLES && role == ROLES; ++i)
            if (board.needs[(board.nextRole + i) % ROLES])
            {
                role = Role((board.nextRole + i) % ROLES);
                board.nextRole = (board.nextRole + i + 1) % ROLES;
            }
        if (role == ROLES)
            return;

        std::vector<Player*> free = FreeBots(player);
        std::shuffle(free.begin(), free.end(), std::mt19937(std::random_device{}()));
        for (Player* bot : free)
        {
            if (RoleOf(bot) != role || std::abs(int32(bot->GetLevel()) - int32(player->GetLevel())) > int32(levelRange) + 1)
                continue;

            // Someone already on the list may be the one who writes
            auto listed = std::find_if(board.entries.begin(), board.entries.end(), [&](Entry const& e) { return e.bot == bot->GetGUID(); });
            if (listed != board.entries.end())
            {
                if (listed->applicant)
                    continue;
                listed->applicant = true;
                listed->until = time(nullptr) + APPLICANT_WAITS;
            }
            else
                AddEntry(board, bot, true);

            LFGDungeonEntry const* dungeon = Destination(board);
            WowPlus::BotApplies(bot, player, std::string("The post asks for a ") + ROLE_NAMES[role] + (dungeon ? std::string(" for ") + dungeon->Name[0] : " for a group") +
                "; you are a " + ROLE_NAMES[role] + " and would like to come.");
            SendList(player, board);
            return;
        }
    }

    std::vector<std::string> Split(std::string const& text)
    {
        std::vector<std::string> parts;
        std::istringstream stream(text);
        for (std::string part; std::getline(stream, part, '\t');)
            parts.push_back(part);
        return parts;
    }

    void FromAddon(Player* player, std::string const& text)
    {
        std::vector<std::string> const parts = Split(text);
        if (parts.empty() || !enabled)
            return;

        Board& board = boards[player->GetGUID()];
        std::string const& what = parts[0];
        if (what == "L")                                    // look at the board
        {
            if (!board.open)
            {
                board.open = true;
                SendDungeons(player);
            }
            Refill(player, board);
            AskForNotes(player, board);
            SendList(player, board);
        }
        else if (what == "I" && parts.size() > 1)           // invite
        {
            Invite(player, board, parts[1]);
            SendList(player, board);
        }
        else if (what == "X" && parts.size() > 1)           // turn an applicant down
        {
            std::erase_if(board.entries, [&](Entry const& entry)
            {
                Player* bot = ObjectAccessor::FindConnectedPlayer(entry.bot);
                return bot && bot->GetName() == parts[1];
            });
            SendList(player, board);
        }
        else if (what == "D" && parts.size() > 1)           // where the group is going
        {
            uint32 const id = uint32(std::strtoul(parts[1].c_str(), nullptr, 10));
            board.destination = sLFGDungeonStore.LookupEntry(id) ? id : 0;
            SendState(player, board);
        }
        else if (what == "P" && parts.size() > 3)           // post: which roles are wanted
        {
            for (uint32 role = 0; role < ROLES; ++role)
                board.needs[role] = parts[role + 1] == "1";
            if (!board.needs[0] && !board.needs[1] && !board.needs[2])
            {
                Tell(player, "Tick at least one of tank, healer and damage.");
                return;
            }
            board.postUntil = time(nullptr) + postMinutes * MINUTE;
            board.nextApplicant = urand(applicantMinSeconds, std::max(applicantMinSeconds, applicantMaxSeconds)) * IN_MILLISECONDS / 2;
            Tell(player, "Your group is posted. People who want to come will whisper you.");
            SendState(player, board);
        }
        else if (what == "U")                               // take the post down
        {
            board.postUntil = 0;
            SendState(player, board);
        }
    }
}

void WowPlus::BoardNotesArrived(ObjectGuid playerGuid, std::vector<std::pair<std::string, std::string>> const& notes)
{
    Player* player = ObjectAccessor::FindConnectedPlayer(playerGuid);
    auto board = boards.find(playerGuid);
    if (!player || board == boards.end())
        return;

    for (auto const& [name, note] : notes)
        for (Entry& entry : board->second.entries)
            if (Player* bot = ObjectAccessor::FindConnectedPlayer(entry.bot))
                if (bot->GetName() == name && !note.empty())
                {
                    entry.note = note.substr(0, 60);
                    Send(player, "N\t" + name + "\t" + entry.note);
                }
}

class WowPlusBoardWorld : public WorldScript
{
public:
    WowPlusBoardWorld() : WorldScript("WowPlusBoardWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("WowPlus.Board.Enable", true);
        listSize = sConfigMgr->GetOption<uint32>("WowPlus.Board.People", 14);
        levelRange = sConfigMgr->GetOption<uint32>("WowPlus.Board.LevelRange", 3);
        stayMinMinutes = sConfigMgr->GetOption<uint32>("WowPlus.Board.Stay.MinMinutes", 4);
        stayMaxMinutes = sConfigMgr->GetOption<uint32>("WowPlus.Board.Stay.MaxMinutes", 10);
        busyChance = sConfigMgr->GetOption<uint32>("WowPlus.Board.BusyChance", 8);
        raidCap = sConfigMgr->GetOption<uint32>("WowPlus.Board.RaidCap", 10);
        travelMinSeconds = sConfigMgr->GetOption<uint32>("WowPlus.Board.Travel.MinSeconds", 40);
        travelMaxSeconds = sConfigMgr->GetOption<uint32>("WowPlus.Board.Travel.MaxSeconds", 110);
        leaveAfterMinutes = sConfigMgr->GetOption<uint32>("WowPlus.Board.LeaveAfterMinutes", 5);
        postMinutes = sConfigMgr->GetOption<uint32>("WowPlus.Board.Post.Minutes", 5);
        applicantMinSeconds = sConfigMgr->GetOption<uint32>("WowPlus.Board.Post.MinSeconds", 12);
        applicantMaxSeconds = sConfigMgr->GetOption<uint32>("WowPlus.Board.Post.MaxSeconds", 40);
        playerLoots = sConfigMgr->GetOption<bool>("WowPlus.Bots.PlayerLoots", true);
    }

    void OnStartup() override
    {
        if (enabled)
            sPlayerbotAIConfig.SummonWhenGroup = false;     // see the top of this file
        LOG_INFO("server.loading", ">> WowPlus board: {}", enabled ? "people looking for a group can be asked along" : "off");
    }

    void OnUpdate(uint32 diff) override
    {
        sinceTick += diff;
        if (!enabled || sinceTick < TICK)
            return;
        sinceTick = 0;

        for (auto itr = boards.begin(); itr != boards.end();)
        {
            Player* player = ObjectAccessor::FindConnectedPlayer(itr->first);
            if (!player || !player->IsInWorld())
            {
                if (!player)
                {
                    itr = boards.erase(itr);
                    continue;
                }
                ++itr;
                continue;
            }

            UpdateMembers(player, itr->second);
            UpdatePost(player, itr->second);
            ++itr;
        }
    }
};

class WowPlusBoardPlayer : public PlayerScript
{
public:
    WowPlusBoardPlayer() : PlayerScript("WowPlusBoardPlayer") { }

    using PlayerScript::OnPlayerCanUseChat;                 // keep the other overloads visible

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* /*receiver*/) override
    {
        if (language != LANG_ADDON || type != CHAT_MSG_WHISPER || msg.compare(0, ADDON_PREFIX.size(), ADDON_PREFIX) != 0)
            return true;

        FromAddon(player, msg.substr(ADDON_PREFIX.size()));
        return false;
    }

    // Into a dungeon with people from the board: a line for the journal
    void OnPlayerMapChanged(Player* player) override
    {
        auto found = boards.find(player->GetGUID());
        if (!enabled || found == boards.end() || !player->GetMap()->IsDungeon() || found->second.journalMap == player->GetMapId())
            return;

        std::vector<std::string> names;
        for (Member const& member : found->second.members)
            if (member.fromBoard && member.state != JOINING && member.state != LEAVING)
                if (Player* bot = ObjectAccessor::FindConnectedPlayer(member.bot))
                    names.push_back(bot->GetName());
        if (names.empty())
            return;

        std::string with;
        for (std::size_t i = 0; i < names.size(); ++i)
            with += (i == 0 ? "" : i + 1 == names.size() ? " and " : ", ") + names[i];

        found->second.journalMap = player->GetMapId();
        WowPlus::Journal(player, WowPlus::JOURNAL_PEOPLE, std::string("Went into ") + player->GetMap()->GetMapName() + " with " + with + ".");
    }
};

// Any bot that joins a real player's group the ordinary way is brought over at once, as the bot module did
class WowPlusBoardGroup : public GroupScript
{
public:
    WowPlusBoardGroup() : GroupScript("WowPlusBoardGroup") { }

    void OnAddMember(Group* group, ObjectGuid guid) override
    {
        Player* bot = ObjectAccessor::FindConnectedPlayer(guid);
        Player* leader = ObjectAccessor::FindConnectedPlayer(group->GetLeaderGUID());
        if (!bot || !leader || !IsBot(bot) || IsBot(leader) || group->isBGGroup())
            return;

        // The loot is the player's: with "free for all" the bot module's bots leave every corpse alone
        // (Playerbots.FreeMethodLoot = 0), where with group loot every other kill would be theirs to loot.
        if (playerLoots && group->GetLootMethod() != FREE_FOR_ALL)
        {
            group->SetLootMethod(FREE_FOR_ALL);
            group->SendUpdate();
        }

        if (!enabled || group->isLFGGroup())
            return;

        Board& board = boards[leader->GetGUID()];
        if (!MemberOf(board, guid))
            board.members.push_back({guid, JOINING, JOIN_TIMEOUT, false});
    }
};

void AddSC_WowPlusBoard()
{
    new WowPlusBoardWorld();
    new WowPlusBoardPlayer();
    new WowPlusBoardGroup();
}
