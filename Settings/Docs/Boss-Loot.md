# Boss loot and the loot book

Built on 2026-10-12. The lists and the server part are made and checked; a boss has not been killed in the game with it yet, and the loot book window has not been opened yet.

## What a boss drops now

Every dungeon and raid boss the game counts as an encounter (409 of them, in 72 dungeons and raids) drops:

| | What | How often |
|---|---|---|
| 1 | **The game's own loot**, untouched. | as always |
| 2 | **One more piece of its own gear**, picked the way the game picks: likelier items are likelier. | every kill (`WowPlus.BossLoot.ExtraChance`, 100) |
| 3 | **One of its new drops.** | 4 kills in 10 (`WowPlus.BossLoot.NewChance`, 40) |

Number 2 and 3 take the place of the surprise roll that other enemies get, which was aimed at your level and mostly at your class. **Boss loot does not look at who you are.** What you cannot use is for another character, a guild mate or the auction house.

Bosses far below your level (grey to you) give only the game's own loot, like every enemy.

## The new drops

Each boss has about ten drops it does not have in the real game: **one picked for each class** (nine below level 55, where there are no death knights).

- They are real items from elsewhere in the game: other dungeons, world drops, chests.
- They match the boss: the same quality as most of its own gear, and about the same item level. A dungeon boss that drops blues gets new blues, also when it has one epic among them. Naxxramas for ten gets epics of Naxxramas-for-ten level.
- "Picked for a class" means the armour type the class wears, a weapon it can use, and stats it is after. It is a rough reading, so now and then something is an odd fit.
- No arena gear.
- **Which items is decided by the world seed** (`WowPlus.World.Seed`): the same at every start, different in another world. Heroic and 25-player versions of a boss have their own lists.

Example, Mr. Smite in the Deadmines: he keeps Smite's Reaver, Thief's Blade and Smite's Mighty Hammer, and can also drop Boahn's Fang (warrior), Black Pearl Ring (paladin), Serpent's Shoulders (hunter), Serpent Gloves (priest), Blackened Defias Belt (druid) and four more.

## The loot book

`/lootbook`, or **Loot book** in the menu. The addon is `WowPlus Loot Book`; restart the game once after it is first installed.

- **Left:** dungeons or raids (the two buttons above), in order of level. It opens at a dungeon for your level.
- **Middle:** the bosses of the place you clicked.
- **Right:** the loot: the game's own with its chance, then "New on this server" with the class each was picked for. Buttons below switch between normal and heroic, or the raid sizes.
- **Find an item:** type three letters or more and it lists every boss that drops something by that name.
- Hover for the item's tooltip. Shift-click links it in chat, Ctrl-click tries it on. The game has to have seen an item once for those two; hovering does that.

The chances are worked out from the game's loot tables and are close, not exact: items that share a "one of these" group are shown with their share of it.

## How it is made

`Settings/Scripts/Make-Boss-Loot.py` reads the game's loot tables and the world seed and writes two things from the same lists, so that the book and the game always agree:

| For | Where |
|---|---|
| The game | the table `acore_world.wowplus_boss_loot`, read by `Server/modules/mod-wowplus/src/WowPlusLoot.cpp` when the server starts |
| The book | `Client/AddOns/WowPlusLootBook/Data.lua` |

`Settings/Deploy-Settings.ps1` runs it at every server start (about 15 seconds). It needs Python, which the bot chat needs as well. Without Python the server starts anyway and uses the lists from last time.

`python Settings/Scripts/Make-Boss-Loot.py --boss smite` prints one boss.

**After changing the seed**, or anything else that changes the lists: restart the server, then restart the game (or `/reload`) so that the book reads the new file.

## Settings (`Settings/mod_wowplus.conf`)

| Setting | Default | What it is |
|---|---|---|
| `WowPlus.BossLoot.Enable` | 1 | 0: bosses get the ordinary surprise roll instead, as before. |
| `WowPlus.BossLoot.ExtraChance` | 100 | Percent chance of one more piece of the boss's own gear. |
| `WowPlus.BossLoot.NewChance` | 40 | Percent chance of one of the boss's new drops. |

## Not covered

- Bosses whose loot is in a chest rather than on the body (some later raid encounters) are listed in the book only if the game ties loot to the creature, and get no extra drops.
- In a group, the extra items go straight into your bags, as surprise loot always has in a group.

## To try

1. `/lootbook`: does the window open at a dungeon for your level, and do item names and icons show?
2. Kill a dungeon boss a few times: is there one more piece each time, and a new one now and then that matches the book?
