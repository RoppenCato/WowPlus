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

Who each bot is comes from files in [Settings/Bots](../Bots/README.md): a list of 240 people, topics, and a style per game. They are written so that MapleStoryPlus can use the same files.

## Bots around you

Bots of your level range gather in the zone you are in (`Playerbots.RandomBotConcentrateInPlayerZone`). `.bots alone on` keeps them away when you want the place to yourself.

## Not built yet

- **More for companions**: calling them from the hearthstone menu, fishing or cooking at the fire, reacting to what happens (a boss kill, a death, a level-up).
- **Bots in the hub and cities** (module `mod-playerbots-city-life`).
- **Tactics for old-world dungeons**: only where a dungeon turns out to play badly.