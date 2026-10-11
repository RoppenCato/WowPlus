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

A random bot in a guild led by a real player is left alone by the system that moves bot levels around, so your guild mates stay who they are and keep levelling on their own. Their gear and talents are kept between sessions. They are also kept "awake" while you are online.

Guild mates are the bots that remember you for good: see [Bot-Chat.md](Bot-Chat.md).

## Not built yet

- **Companions**: one or two bots that are especially yours, with their own extras (called to you from the hearthstone menu, things they do on their own when you rest, such as lighting a fire). The memory and guild parts they build on are done; the rest is waiting for decisions.
- **Easy control**: buttons instead of typed commands. There are existing addons made for this bot module; none is installed yet.
- **Bots in the hub and cities** (module `mod-playerbots-city-life`).
- **Tactics for old-world dungeons**: only where a dungeon turns out to play badly.
