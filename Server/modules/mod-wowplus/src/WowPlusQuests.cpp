/*
 * WowPlus quests: two kinds of task that the game's database cannot express by itself.
 *
 *   - Go to a place. The task is done when the player comes within a distance of a point.
 *   - Hear someone out. The task is done when the player speaks to a certain person, who then says
 *     a few lines to the player. This is how a quest lets someone tell a piece of the world's lore.
 *
 * Our quests are files in Settings/Quests, put into the database by Settings/Scripts/Make-Quests.py.
 * For these two kinds of task the script writes a row in the table wowplus_quest_goals and gives the
 * quest an invisible creature to "kill" (the way the game itself counts such things); this file gives
 * the player credit for that creature when the place is reached or the person is spoken to.
 *
 * The table is read when the server starts. In the server's window, "wowplus quests" reads it again.
 */

#include "Chat.h"
#include "CommandScript.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace Acore::ChatCommands;

namespace
{
    constexpr uint32 TICK = 2000;                           // ms between looks at where the players are
    constexpr uint32 LINE_PAUSE = 3500;                     // ms between two lines someone says

    struct Goal
    {
        uint32 quest;
        uint32 credit;                                      // the invisible creature the quest counts
        uint32 map;
        float x, y, radius;
        std::vector<std::string> says;
    };

    std::vector<Goal> places;
    std::unordered_map<uint32, std::vector<Goal>> talks;    // creature -> what speaking to it completes

    struct Speech
    {
        ObjectGuid speaker, listener;
        std::string line;
        uint32 delay;
    };

    std::vector<Speech> speeches;
    uint32 sinceTick = 0;

    uint32 LoadGoals()
    {
        places.clear();
        talks.clear();
        if (!WorldDatabase.Query("SHOW TABLES LIKE 'wowplus_quest_goals'"))
            return 0;

        uint32 count = 0;
        if (QueryResult result = WorldDatabase.Query("SELECT quest, credit, kind, map, x, y, radius, creature, says FROM wowplus_quest_goals"))
            do
            {
                Field* fields = result->Fetch();
                Goal goal{fields[0].Get<uint32>(), fields[1].Get<uint32>(), fields[3].Get<uint32>(), fields[4].Get<float>(),
                          fields[5].Get<float>(), fields[6].Get<float>(), {}};
                std::istringstream lines(fields[8].Get<std::string>());
                for (std::string line; std::getline(lines, line);)
                    if (!line.empty())
                        goal.says.push_back(line);

                if (fields[2].Get<uint8>() == 0)
                    places.push_back(goal);
                else
                    talks[fields[7].Get<uint32>()].push_back(goal);
                ++count;
            } while (result->NextRow());

        return count;
    }

    bool Wants(Player* player, Goal const& goal)
    {
        return player->GetQuestStatus(goal.quest) == QUEST_STATUS_INCOMPLETE && player->GetReqKillOrCastCurrentCount(goal.quest, int32(goal.credit)) == 0;
    }
}

class WowPlusQuestsWorld : public WorldScript
{
public:
    WowPlusQuestsWorld() : WorldScript("WowPlusQuestsWorld") { }

    void OnStartup() override
    {
        uint32 const count = LoadGoals();
        LOG_INFO("server.loading", ">> WowPlus quests: {} places to reach and people to hear", count);
    }

    void OnUpdate(uint32 diff) override
    {
        // What people are in the middle of saying
        for (auto speech = speeches.begin(); speech != speeches.end();)
        {
            if (speech->delay > diff)
            {
                speech->delay -= diff;
                ++speech;
                continue;
            }

            Player* listener = ObjectAccessor::FindConnectedPlayer(speech->listener);
            Creature* speaker = listener ? ObjectAccessor::GetCreature(*listener, speech->speaker) : nullptr;
            if (speaker && listener->IsInWorld())
                speaker->Whisper(speech->line, LANG_UNIVERSAL, listener);
            speech = speeches.erase(speech);
        }

        sinceTick += diff;
        if (sinceTick < TICK || places.empty())
            return;
        sinceTick = 0;

        std::vector<Player*> players;
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
                if (player->IsInWorld() && !player->GetSession()->IsHeadless() && player->IsAlive())
                    players.push_back(player);
        }

        for (Player* player : players)
            for (Goal const& goal : places)
                if (player->GetMapId() == goal.map && player->GetDistance2d(goal.x, goal.y) <= goal.radius && Wants(player, goal))
                    player->KilledMonsterCredit(goal.credit);
    }
};

// Speaking to someone a quest asks the player to hear: credit, and they say their piece
class WowPlusQuestsTalk : public AllCreatureScript
{
public:
    WowPlusQuestsTalk() : AllCreatureScript("WowPlusQuestsTalk") { }

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        auto found = talks.find(creature->GetEntry());
        if (found == talks.end() || player->GetSession()->IsHeadless())
            return false;

        for (Goal const& goal : found->second)
        {
            if (!Wants(player, goal))
                continue;

            player->KilledMonsterCredit(goal.credit);
            uint32 delay = 600;
            for (std::string const& line : goal.says)
            {
                speeches.push_back({creature->GetGUID(), player->GetGUID(), line, delay});
                delay += LINE_PAUSE + uint32(line.size()) * 25;
            }
        }
        return false;                                       // and then the game's own conversation, as always
    }
};

class WowPlusQuestsCommands : public CommandScript
{
public:
    WowPlusQuestsCommands() : CommandScript("WowPlusQuestsCommands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable wowplusTable =
        {
            { "quests", HandleReload, SEC_ADMINISTRATOR, Console::Yes },
        };

        static ChatCommandTable commandTable =
        {
            { "wowplus", wowplusTable },
        };

        return commandTable;
    }

    // wowplus quests: read the places and talks again, after Make-Quests.py has changed them
    static bool HandleReload(ChatHandler* handler)
    {
        handler->PSendSysMessage("WowPlus quests: {} places to reach and people to hear.", LoadGoals());
        return true;
    }
};

void AddSC_WowPlusQuests()
{
    new WowPlusQuestsWorld();
    new WowPlusQuestsTalk();
    new WowPlusQuestsCommands();
}
