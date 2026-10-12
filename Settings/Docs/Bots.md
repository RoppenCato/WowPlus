# Bots: how they work here and what we changed

The bots are what makes a one-player server feel like a world. This page explains how they work, what we changed, and what is still open. Talking to bots has its own page: [Bot-Chat.md](Bot-Chat.md).

## The three kinds of bot

| Kind | What it is | How you get one |
|---|---|---|
| **Random bots** | The 100 that are always online. They have their own accounts, pick quests, grind, travel, queue for dungeons, and get gear handed to them as they level. | They are just there. Invite one to your party or guild like any player. |
| **Class bots** | A bot of the class you ask for, brought to you at your level. The "I need a healer now" tool. | `.playerbots bot addclass priest` (or any class) |
| **Alt bots** | Your own other characters, logged in as bots. They keep their real gear and progress. | `.playerbots bot add <name>` |

All three run the same AI: a list of "strategies" (tank, heal, damage, follow, loot, stay out of fire). You steer a bot by whispering it or by party chat: `follow`, `stay`, `attack`, `summon`, `co +tank` and many more.

Bots cheat a little, by design of the bot module: free food and water, free flight paths, and gear handed out on level-up.

Boss tactics exist for every Northrend dungeon, a few Outland ones and most raids. In the old-world dungeons bots fight with their general AI only.

## What we changed (2026-10-11)

| What | How it is now | Where to change it |
|---|---|---|
| **Invitations** | A bot always accepts an invitation to a party or a guild. | `Playerbots.GroupInvitationPermission` in `Settings/playerbots.conf` |
| **Levels** | About half the bots gather in the ten-level range you are in. The rest stay spread over all other ranges, so cities and roads still show all kinds of characters. | `Playerbots.LevelBrackets.*`; raise or lower `Dynamic.RealPlayerWeight` (12 now) |
| **Gear** | Each bot belongs to a wardrobe class for good: 55 percent wear all green, 35 percent green with some blue pieces, 8 percent all blue, 2 percent blue with some epics. Warriors pick plate, and so on. | `WowPlus.Bots.Gear.*` in `Settings/mod_wowplus.conf`, `Playerbots.PreferClassArmorType` |
| **Passing buffs** | Now and then a bot of your faction that is near you casts its class buff on you. | `WowPlus.Bots.Buffs.*` |
| **Being alone** | `.bots away` sends every free bot near you somewhere else, once. `.bots alone on` keeps them away until `.bots alone off`. Your party stays. | `WowPlus.Bots.Away.Radius` |
| **Talk** | The module's canned replies and joke spam are off. Bots answer through Claude instead: [Bot-Chat.md](Bot-Chat.md). | `Playerbots.RandomBotTalk`, `WowPlus.BotChat.*` |
| **The side you play** | The bots in the world are all of the faction of the character you log in on. Log in on an Alliance character and the Horde bots log out and Alliance bots take their places; it takes a few minutes until all hundred are there. The guild mates and friends of that character are always among them. When the server starts, the world fills with the side that was played last, so it is usually right already. | `WowPlus.Bots.FollowFaction`. A change inside the bot module (`RandomPlayerbotMgr`, look for "WowPlus") does the switching; `Settings/Deploy-Settings.ps1` makes sure there are enough bot characters of both factions |
| **Awake where it matters** | A bot is fully awake in your zone, within 150 yards of you, in your guild or party, in combat, in a dungeon or battleground, and in a queue. Elsewhere it thinks one tenth of the time (the module's default). Making all bots always awake was tried: far-away bots handed in eight times the quests, which you never see, since the bot manager sets their levels anyway. | `Playerbots.BotActiveAlone` |
| **People you know stay** | A bot you have talked with more than once is put on your friends list. The module leaves bots on a friends list (and in your guild) at their level, so someone you met at level 12 is not level 60 next week. Remove them from the list to let them go. | `WowPlus.BotChat.KnownBecomeFriends` |
| **The loot is yours** | When a bot joins your group the loot rule becomes "free for all", and under that rule bots do not loot. Every kill is yours, in dungeons too. | `WowPlus.Bots.PlayerLoots` |
| **Small things** | Bots walk indoors, greet, and use emotes. | `Settings/playerbots.conf` |

Two things to know:

- **Gear changes show slowly.** A bot gets new gear when it levels up or is re-rolled. Bots that already wear blue keep it until then.
- **The gear rule is a small change inside the bot module** (`Server/modules/mod-playerbots/src/Bot/Factory/PlayerbotFactory.cpp`, look for "WowPlus"). When the bot module is updated, that change may have to be put back by hand.

## Your guild

Create a guild the normal way (a guild master sells the charter; bots sign it when asked) and invite bots with `/ginvite <name>`. They always accept.

- **Guild mates keep up with you.** A bot in your guild that is more than two levels below or above you is brought to your level, with gear, spells and talents to match. This is checked every half minute while you are online, so someone invited today has caught up when you next log in. (`WowPlus.Bots.Guild.KeepUp`)
- **They stay who they are.** The system that spreads bot levels leaves a real player's guild alone, and their gear and talents are kept between sessions.
- **They remember you for good**, greet you when you come online, and talk in guild chat by themselves now and then: see [Bot-Chat.md](Bot-Chat.md).

## Building your guild: meeting people

Built on 2026-10-12, not yet tried in the game. The idea: your guild is the people you chose, one at a time. In the menu under **Build my guild**, or typed:

| Command | What happens |
|---|---|
| `.bots meet` | Someone walks up, waves and introduces themselves out loud: their first name, where they live, what they like doing in the game and when they are on, and what they are like as company. `.bots meet tank`, `healer` or `damage` asks for a role. |
| (talk) | Say something or whisper them and they answer, as any bot does. You do not have to. |
| `.bots meet` again | The one in front of you waves and leaves, and someone else comes by. Nobody comes twice until everyone has been. |
| `.bots keep` | They join your guild, wave, and go back to what they were doing, as a guild mate now. Without a guild they go on your friends list. |
| `.bots pass` | They wave and go their own way. |

They are bots that are really in the world: your side, free, near your level, not in your guild yet. If nobody near your level is free, someone from another level range becomes your level, as guild mates do. Someone you neither keep nor pass leaves after ten minutes, or when you walk 60 yards away. It works out in the world only, not in a dungeon or a fight.

You can still invite anyone you meet with `/ginvite <name>`, or anyone from the group board after a run.

## Companions

**A companion is any guild mate who is in your party.** There is nothing to choose or set up beforehand (until 2026-10-12 there was a list of two).

| How | What happens |
|---|---|
| Menu → **Guild mates** | Lists the guild mates who are online, with class and level. Click one and they join your party and come to you. |
| The guild window, or `/invite <name>` | The same. |
| **Call** on the bot bar, `.bots call` | The guild mates you travelled with last time (up to four) join your party and come to you. |
| `.bots call <name>` | That guild mate. |
| **Dismiss** on the bot bar, `.bots dismiss` | The guild mates in your party leave it and go their own way. |

While they travel with you they do small things by themselves:

- **When you rest** (stand still for a minute): one lights a campfire, and now and then one sits down, eats, laughs, dances or says something in party chat.
- **On the road:** a gesture or a remark every three to seven minutes.

People who are not guild mates (from the group board, or strangers you invite) do not do these things, and leave after the run.

Settings: `WowPlus.Bots.Companions.*`.

## The bot bar

A small addon of our own, `WowPlusBots`, gives you one row of buttons instead of typed commands: Follow, Stay, Attack, Flee, Call, Dismiss, Away, Alone. Drag it by the word "Bots" to move it; `/botbar` hides or shows it. It is installed by `Start-Server.bat` like our other addons.

## The people behind the bots

Who each bot is comes from files in [Settings/Bots](../Bots/README.md): 300 people, each with a background, topics, and a style per game. They are written so that MapleStoryPlus can use the same files.

## Bots around you

Bots of your level range gather in the zone you are in (`Playerbots.RandomBotConcentrateInPlayerZone`). `.bots alone on` keeps them away when you want the place to yourself.

## The menu

A **Menu** button (addon `WowPlus Menu`; restart the game once after it is first installed) opens a menu of everything you can ask for, so that no command has to be typed or remembered. `/wp` opens it at the mouse. Drag the button with the right mouse button; `/wp reset` puts it back, `/wp hide` hides it.

| Section | What is in it |
|---|---|
| Windows | Group board, journal, show or hide the bot bar, put the bot bar or sprint bar back in place. |
| My group | Follow, stay, attack my target, tank takes my target, run to me, come here at once, spread out, drink, buffs again, get up, release spirit, ready check, leave the group. |
| Marks on my target | Skull, moon, cross, square, star, take the mark off. |
| The person I have targeted | Invite to group, invite to guild, add to friends, make a companion, no longer a companion. |
| Companions | Call them, send them off, who are they. |
| Bots around me | Send them away once, keep them away, let them come back. |

The "My group" entries say a word in party chat that the bot module's bots act on. Follow, stay, attack and run to me were on the bot bar already; the others are the module's own commands and have not been tried here yet. To add or change entries, edit the list `MENU` at the top of `Client/AddOns/WowPlusMenu/WowPlusMenu.lua`.

## Finding a group

The **Group** button on the bot bar (or `/board`) opens a board of people looking for a group: pick who comes along to a dungeon or raid, or post your own group and see who answers. See [Group-Board.md](Group-Board.md).

## In a group: ready checks, marks, and getting back up

Built on 2026-10-12, not yet tried in the game. Code: `Server/modules/mod-wowplus/src/WowPlusGroupCare.cpp`; settings `WowPlus.Group.*`.

| What | How it works |
|---|---|
| **Ready check** | **Ready?** on the bot bar, or `/readycheck`. A bot answers "not ready" when it is dead, not with the group, below 70 percent health, or below 40 percent mana (a healer: 60). It sits down to eat or drink, one of them says why in party chat, and a line in your chat names who is not ready. The game's own ready check window shows the ticks and crosses. Before, the bot module answered "ready" every time. |
| **A healer that speaks up** | Out of combat, a healer below 30 percent mana says so (at most once in three minutes) and drinks, so you know to wait. |
| **Marks** | The skull and moon in the bot bar's title row put that mark on your target; click again to take it off. Damage dealers and tanks go for the **skull** first. A mage, hunter or balance druid in the group keeps the **moon** out of the fight (sheep, trap, sleep) and the others leave it alone. This is the bot module's own behaviour; the buttons are ours. |
| **Getting back up** | The bot module leaves a dead bot lying in a dungeon until someone resurrects it; after a wipe nobody does. Now a bot that has been dead 20 seconds stands next to you again, at half health, once you are alive and out of combat. They get up one after another and one of them says something. When someone alive in the group can resurrect, they get 60 seconds to do it first. |

To try: pull something, let the healer go low and press Ready?; mark a pack with skull and moon and watch who hits what; die on purpose in a dungeon, run back, and see whether the group is on its feet when you arrive.

## A ready-made character for trying something out

When something new needs a character of a certain class and level standing at a certain place, a bot can be handed over to your account. Done once on 2026-10-12 (Tahorn, a level 10 tauren warrior at Hulfnar Stonetotem in Bloodhoof Village). The steps, for Claude:

1. Pick a bot character of the right race and class (`characters` joined to `acore_playerbots.playerbots_account_type`). If it is not in the world, put it on a real character's friends list and restart; friends are always brought in.
2. In the server's window: `bots prepare <name> <level>`. The bot module gives it the gear, spells and talents it gives its own bots at that level. Then `saveall`.
3. Stop the server. In the database: set the character's `account` to yours, give it a new `name`, a position and a home, set `money`, and remove it from `guild_member`, `character_social` and `acore_playerbots.playerbots_random_bots`. Add one to `numchars` in `acore_auth.realmcharacters`.
4. Start the server. Extra items go by mail: `send items <name> "subject" "text" <item>` in the server's window.

## Health check

`python Settings/Scripts/Test-Bots.py` looks at the database, the logs and the bot module's source and prints pass or fail. It changes nothing. Run it after a restart, a settings change, or an update of the bot module.

| Check | What a fail means | What to do |
|---|---|---|
| World server running | Nothing answers on port 8085. | `Start-Server.bat`. |
| Bots in the world | Fewer or more bots online than `Playerbots.MinRandomBots` / `MaxRandomBots`. Normal for the first three minutes after a start. | Wait; if it stays, read `Playerbots.log`. |
| Factions | Bots of a faction that is switched off are in the world. The module fills places with the other faction when it runs out of characters. | Raise `Playerbots.RandomBotAccountCount` in `Settings/playerbots.conf` by a few and restart twice (once to make the accounts, once to use them). |
| Healers | Fewer than 10 or more than 30 in 100 have a healing spec. | The chances per spec are `Playerbots.RandomClassSpecProb.*`. |
| Bots around your level | You are online and fewer than 3 in 10 bots are within your ten levels. It takes about 15 minutes after logging in. | `Playerbots.LevelBrackets.Dynamic.RealPlayerWeight`. |
| Bot log size | Over 3000 lines an hour: the log is at debug level. | `Logger.playerbots = 4,...` in `Settings/worldserver.conf`. |
| Settings the server asked for | A module is asking for a setting that is not there. | The line names the setting. |
| World update time | One round of updating the world took over 100 ms: the server is too busy. | Fewer bots, or `Playerbots.BotActiveAlone` below 100. |
| Our changes in the bot module | One of our three changes inside `mod-playerbots` is gone, usually after updating the module. | Put it back; the check names which. They are marked "WowPlus" in the source. |

Two slower checks watch the bots for a while:

- `--watch 10` saves all characters, waits ten minutes, saves again, and lists bots that did not move, earn or level, and bots that were dead at both ends.
- `--deaths 20` looks every two minutes for twenty minutes and lists bots that were dead in more than three looks.

The line "Healers" counts by talent spec. The module's own summary in `Playerbots.log` ("tank, heal, dps") only counts a healer while it is healing in a group, so it shows 3 where there are 17.

`Settings/Scripts/Send-Server-Command.ps1 "saveall"` types a command into the world server's window; the slow checks use it.

## Not built yet

- **More for companions**: calling them from the hearthstone menu, fishing or cooking at the fire, reacting to what happens (a boss kill, a death, a level-up).
- **Bots in the cities** (module `mod-playerbots-city-life`).
- **Tactics for old-world dungeons**: only where a dungeon turns out to play badly.
