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
| **All Horde, really** | The module fills the world from a fixed set of accounts and falls back on Alliance characters when it runs out of Horde ones (8 of 100 did). There are now enough accounts for 150 Horde bots. | `Playerbots.RandomBotAccountCount`; `Settings/Deploy-Settings.ps1` hands the extra accounts over at start |
| **Awake where it matters** | A bot is fully awake in your zone, within 150 yards of you, in your guild or party, in combat, in a dungeon or battleground, and in a queue. Elsewhere it thinks one tenth of the time (the module's default). Making all bots always awake was tried: far-away bots handed in eight times the quests, which you never see, since the bot manager sets their levels anyway. | `Playerbots.BotActiveAlone` |
| **People you know stay** | A bot you have talked with more than once is put on your friends list. The module leaves bots on a friends list (and in your guild) at their level, so someone you met at level 12 is not level 60 next week. Remove them from the list to let them go. | `WowPlus.BotChat.KnownBecomeFriends` |
| **Small things** | Bots walk indoors, greet, and use emotes. | `Settings/playerbots.conf` |

Two things to know:

- **Gear changes show slowly.** A bot gets new gear when it levels up or is re-rolled. Bots that already wear blue keep it until then.
- **The gear rule is a small change inside the bot module** (`Server/modules/mod-playerbots/src/Bot/Factory/PlayerbotFactory.cpp`, look for "WowPlus"). When the bot module is updated, that change may have to be put back by hand.

## Your guild

Create a guild the normal way (a guild master sells the charter; bots sign it when asked) and invite bots with `/ginvite <name>`. They always accept.

- **Guild mates keep up with you.** A bot in your guild that is more than two levels below or above you is brought to your level, with gear, spells and talents to match. This is checked every half minute while you are online, so someone invited today has caught up when you next log in. (`WowPlus.Bots.Guild.KeepUp`)
- **They stay who they are.** The system that spreads bot levels leaves a real player's guild alone, and their gear and talents are kept between sessions.
- **They remember you for good**, greet you when you come online, and talk in guild chat by themselves now and then: see [Bot-Chat.md](Bot-Chat.md).

## Companions

Up to two guild mates you choose.

| Command | What it does |
|---|---|
| `.bots companion add <name>` | A guild mate becomes a companion. |
| `.bots companion remove <name>` | No longer a companion; stays in the guild. |
| `.bots companion list` | Who they are and whether they are online. |
| `.bots call` | They join your party and come to you. |
| `.bots dismiss` | They leave the party and go their own way. |

While they travel with you they do small things by themselves:

- **When you rest** (stand still for a minute): one lights a campfire, and now and then one sits down, eats, laughs, dances or says something in party chat.
- **On the road:** a gesture or a remark every three to seven minutes.

Settings: `WowPlus.Bots.Companions.*`.

## The bot bar

A small addon of our own, `WowPlusBots`, gives you one row of buttons instead of typed commands: Follow, Stay, Attack, Flee, Call, Dismiss, Away, Alone. Drag it by the word "Bots" to move it; `/botbar` hides or shows it. It is installed by `Start-Server.bat` like our other addons.

## The people behind the bots

Who each bot is comes from files in [Settings/Bots](../Bots/README.md): 300 people, each with a background, topics, and a style per game. They are written so that MapleStoryPlus can use the same files.

## Bots around you

Bots of your level range gather in the zone you are in (`Playerbots.RandomBotConcentrateInPlayerZone`). `.bots alone on` keeps them away when you want the place to yourself.

## Finding a group

The **Group** button on the bot bar (or `/board`) opens a board of people looking for a group: pick who comes along to a dungeon or raid, or post your own group and see who answers. See [Group-Board.md](Group-Board.md).

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
