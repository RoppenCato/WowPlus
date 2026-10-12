# Quests of our own

Every quest we add is one file in this folder. Nothing else has to be kept: the files are checked and put into the game at every server start, and a file that is deleted is gone from the game at the next start.

There are two ways to make one, and they produce the same kind of file.

## 1. The quest tool (a page in your browser)

Double-click `Quest-Tool.bat` in the main folder. The database must be running (the server starts it). The page opens at `http://127.0.0.1:8590`; close the black window to stop the tool.

| Step | What you do | What the tool does |
|---|---|---|
| 1. Level and side | Type the quest's level; choose Horde, Alliance or both. | Fills in the lowest level (three below) and no highest level. Change them if you like; they stop following once you have. |
| 2. Zone | Choose from the list. | Zones that suit the level come first. The map on the right is drawn from the server's own map files: land shaded by height, water blue. |
| 3. Giver | Click a name, or a gold dot on the map. | Lists the people of your side who stand in the zone, those who give quests already first. "Hand in to" stays the same person unless you pick another. |
| 4. The task | Choose the kind, click an enemy, set how many. | Lists the zone's enemies by level and shows where they are as red dots. For collecting, it shows what the enemy drops; any item can be made to drop while the player is on the quest. |
| 5. The words | Write them, or type your idea in a sentence and press "Write the texts for me". | Claude writes title, offer, task line, and the two things said afterwards, in the game's voice. Edit freely. |
| 6. The reward | Tick gear and extras. | Suggests the money a quest of that level usually pays, experience by size, and gear of the right level. Each ticked item is "choose one" or "always given". |
| Save | "Save quest". | Checks it and writes the file here. Problems are listed instead. |
| Play | "Put all quests into the game". | Writes all quests into the game's database and tells a running server to read them again. If one does not show up, restart the server. |

"Saved quests" at the bottom opens one again; saving writes over it.

The page makes the five kinds below. Two more kinds, new named people and new named enemies exist only in the file format for now (see "Writing the file by hand").

### The kinds of task

| Kind | What the player does |
|---|---|
| Kill enemies | Kill a number of up to four kinds of enemy. |
| Collect from enemies | Bring up to six kinds of item that enemies drop. |
| Bring items | Bring items that are bought, made, gathered or found. |
| Go and talk to someone | Travel from the giver to another person. |
| Kill one named enemy | One particular creature, once. |

In a file, two more: **go to a place** and **hear someone out** (the person says a few lines to you when you speak to them).

Not possible yet: using an item somewhere, picking something up off the ground, escorting someone, and quests that begin from an item or an object.

## 2. Writing the file by hand (how Claude does it)

Look things up, write the file, check it:

```
python Settings/Scripts/Quest-Tool.py --zones 10            zones that suit level 10
python Settings/Scripts/Quest-Tool.py --zone Mulgore        people (with their numbers) and enemies there
python Settings/Scripts/Quest-Tool.py --rewards 10          usual money, experience, gear ideas
python Settings/Scripts/Make-Quests.py --check              is every file right?
python Settings/Scripts/Make-Quests.py --reload             into the game, and tell the running server
```

A file is named `<number>-<a-few-words>.json`. The number is the quest's `id`: the next free one from 9200001.

```json
{
  "id": 9200001,
  "title": "Bold Wolves of the Plains",
  "level": 6,
  "min_level": 3,
  "max_level": 0,
  "side": "horde",
  "zone": 215,
  "type": "kill",
  "giver": 2948,
  "turn_in": 2948,
  "objectives": [
    { "creature": 2958, "count": 8 }
  ],
  "texts": {
    "details": "What the giver says when offering it. $N is the player's name.\n\nAn empty line starts a new paragraph.",
    "objectives": "Kill 8 Prairie Wolves, then return to Mull Thunderhorn in Bloodhoof Village.",
    "progress": "Said while it is not done yet. May be left out.",
    "completion": "Said when it is done."
  },
  "reward": {
    "money": 147,
    "xp_difficulty": 5,
    "items": [ { "item": 2455, "count": 3 } ],
    "choices": [ { "item": 2817, "count": 1 }, { "item": 10549, "count": 1 } ]
  }
}
```

| Field | Meaning |
|---|---|
| `level`, `min_level`, `max_level` | The quest's level, the lowest level that can take it, and the highest (0: none). |
| `side` | `horde`, `alliance` or `both`. |
| `zone` | The zone's number; the quest is listed under it in the quest log. |
| `type` | `kill`, `collect`, `fetch`, `deliver` or `slay`. |
| `giver`, `turn_in` | Creature numbers. The same for most quests; different for `deliver`. |
| `objectives` | `{ "creature": number, "count": n }` to kill, `{ "item": number, "count": n }` to bring. At most four kinds of creature and six kinds of item. |
| `drops_from` | Inside an item objective: `[ { "creature": number, "chance": 40 } ]` makes that creature drop the item, only for players on the quest. The creature must have loot of its own. |
| `texts` | `details`, `objectives` and `completion` are needed; `progress` and `return` (the line in the log once the task is done; "Return to ..." if left out) are not. |
| `reward.money` | In copper (100 copper is a silver). |
| `reward.xp_difficulty` | 0 none, 5 the usual amount for the level, 8 the most. |
| `reward.items`, `reward.choices` | Always given (at most four) and choose-one (at most six). |
| `"place"` objective | `{ "place": { "map": 0, "x": ..., "y": ..., "radius": 40 }, "text": "Look into the Fargodeep Mine" }`: done when the player comes within that many yards of the point. `text` is the line in the quest log. |
| `"talk"` objective | `{ "talk": creature number, "text": "Ask Brog Hamfist about kobolds", "says": ["...", "..."] }`: done when the player speaks to that person, who then says the lines (one to six, each at most 250 letters) to the player alone. This is how a quest lets someone tell a piece of lore. Kinds of enemy, places and talks share four slots per quest. A quest of type `visit` has only places and talks; any other type may add them. |
| `creatures` | People and enemies with names of their own: `[ { "id": 9210001, "kind": "person", "like": 1975, "name": "Edrin Halloway", "title": "Fargodeep Miner", "map": 0, "x": ..., "y": ..., "z": ..., "o": ... } ]`. Each is a copy of the creature `like` (its looks, faction and, for an enemy, its abilities and loot) under a new name at one place. `kind` is `person` (can give quests and be spoken to) or `enemy` (optional `level`, `health` as a multiplier, `size`, `rank`, `respawn` in seconds). Ids run from 9210001 and must be unique across all files. Once made, any quest can use the id as giver, hand-in, enemy or talk. They appear after a server restart. |
| `spawns` | More of an enemy than the game has: `[ { "creature": number, "map": 1, "x": ..., "y": ..., "z": ..., "o": ... } ]`, at most forty. Each is a copy of one that is there already. `python Settings/Scripts/Quest-Tool.py --spots <creature> <count>` prints places near where the enemy lives, on the ground, ready to paste. They appear after a server restart. |
| `after` | The number of a quest that must be done first; 0 or left out for none. |
| `idea` | A note for the writer. Not used by the game. |

## Adding enemies without crowding

A quest may add more of an enemy (`spawns`), but a place must not fill up. Every added enemy has to keep these rules, which the check enforces and the place picker (`Quest-Tool.py --spots`) follows:

| Rule | Number | Why |
|---|---|---|
| Room to the nearest other creature | at least 18 yards | So that pulling one does not pull its neighbour. |
| Creatures within 40 yards, itself not counted | at most 6 | The game's own busy spots are about that full; more feels like a swarm. |
| Distance from anyone players talk to | at least 60 yards | No enemies next to quest givers, vendors or guards. |
| How many are added | at most twice what the game has of that enemy | A handful more, never a new population. |

Enemies added by other quest files count as neighbours too. A quest whose added enemies break a rule is left out of the game and named by `Make-Quests.py --check`, until they are moved. The numbers are at the top of `Settings/Scripts/Make-Quests.py`.

The picker looks further out from where the enemy lives when the nearby ground is full, so a crowded camp grows at its edges instead of in its middle.

## Where quests are needed

`Zone-Audit.md` in this folder surveys every zone: quests per side, how much of the levelling they carry, and which enemies and places no quest uses. Its first part says where to write next. `python Settings/Scripts/Make-Zone-Audit.py` makes the lists again after quests have been added.

## How it gets into the game

`Settings/Scripts/Make-Quests.py` turns the files into rows in the game's database: the quest itself, who starts and ends it, the things said, loot that drops only while on the quest, added enemies, people and enemies with names of their own, places to reach and people to hear (the table `wowplus_quest_goals`, acted on by `Server/modules/mod-wowplus/src/WowPlusQuests.cpp`), and **where it shows on the map**: an area around each group of the enemies to kill or to take things from, and a mark on the person it is handed in to. The map marks and added enemies are read by the server when it starts, so those need a restart. It first removes everything it wrote before (quests 9200000 to 9299999, and loot rows marked "WowPlus quest"), so what is in the game is always exactly what is in this folder. A file with a mistake is left out and named; the others still go in.

`Settings/Deploy-Settings.ps1` runs it at every server start.

## What is here

| File | Quest |
|---|---|
| `9200002-the-wandering-years.json` | Mulgore, level 8, Horde. Hulfnar Stonetotem in Bloodhoof Village tells of the nomad years and sends you to Morin Cloudstalker on the road east. First of three. |
| `9200003-scouts-of-the-galak.json` | Level 9. Morin Cloudstalker: kill 2 Galak Centaur and 2 Galak Outrunners on the plains to the east and bring 3 Centaur Bracers. |
| `9200004-a-debt-remembered.json` | Level 9. Bring word to Baine Bloodhoof, who tells what the centaur once did to him. Choice of four rewards. |
| `9200005-the-swiftest-prey.json` | Mulgore, level 7, Horde. Vorn Skyseer in Bloodhoof Village: kill 8 Adult Plainstriders north-west of the village, and hear of Apa'ro, the white stag. First of three. |
| `9200006-antlers-among-the-stars.json` | Level 8. Bring 6 Long Elegant Feathers from the swoops to the north; how the stag was caught in the stars and the moon found him. |
| `9200007-the-moon-s-mercy.json` | Level 10. Give a Kodo Bull to the north a clean end, then speak with Gennia Runetotem, who tells whose child came of the stag and the moon. Choice of four rewards. Adds four kodo bulls. |
| `9200008-what-lives-in-the-fargodeep.json` | Elwynn Forest, level 6, Alliance. Edrin Halloway, a miner in Goldshire (a new person): ask Brog Hamfist about kobolds and look into the Fargodeep Mine. First of three. |
| `9200009-the-darkness-took-them.json` | Level 7. Bring 6 Melted Candles from the kobolds of the Fargodeep and the Jasperlode. |
| `9200010-under-the-whip.json` | Level 9. Kill Yarrik Whipjaw, a Riverpaw taskmaster on the ridge above the Fargodeep (a new enemy). Choice of four rewards. |
| `9200011-arms-for-the-militia.json` | Elwynn to Westfall, level 9. Quartermaster Hudson at Westbrook Garrison sends you to Quartermaster Lewis at Sentinel Hill. |
| `9200012-word-for-three-corners.json` | Elwynn to Redridge, level 10. Drake Lindgren at Eastvale Logging Camp sends you to Guard Parker at the Three Corners. |
| `9200013-older-than-the-forge.json` | Dun Morogh, level 7, Alliance. Yori Crackhelm in Kharanos: ask Senir Whitebeard about the Frostmane and look over Shimmer Ridge. First of two. |
| `9200014-the-rememberer.json` | Level 9. Kill Jin'zakk the Rememberer at the foot of Frostmane Hold (a new enemy) and 5 Frostmane Snowstriders. Choice of four rewards. |
| `9200015-troggs-in-the-diggings.json` | Dun Morogh to Loch Modan, level 10. Prospector Gehn at Gol'Bolar Quarry sends you to Mountaineer Cobbleflint at the Valley of Kings. |
| `9200016-once-of-our-blood.json` | Teldrassil, level 6, Alliance. Orenthil Whisperwind in Dolanaar: ask Moon Priestess Amara what a satyr is, and hear out Zenn Foulhoof. First of two. |
| `9200017-the-gardener-of-starbreeze.json` | Level 8. Kill Xalthas the Gentle, a satyr on the slope above Starbreeze Village (a new enemy). Choice of four rewards. |
| `9200018-hands-for-auberdine.json` | Teldrassil to Darkshore, level 10. Daryn Lightwind in Rut'theran Village sends you to Thundris Windweaver, the elder of Auberdine. |
| `9200019-sails-from-the-east.json` | Durotar, level 7, Horde. Magga in Razor Hill: ask Master Gadrin about the Admiral's ships and look over Tiragarde Keep. First of three. |
| `9200020-the-admiral-s-last-order.json` | Level 8. Take the Musty Letter from Quartermaster Tidewell on the shore below Tiragarde Keep (a new enemy). |
| `9200021-for-the-darkspear.json` | Durotar to Orgrimmar, level 9. Bring the letter to Vol'jin in Grommash Hold. Choice of four rewards. |
| `9200022-bigger-game.json` | Durotar to the Barrens, level 11. Vel'rin Fang in Sen'jin Village sends you to Sergra Darkthorn at the Crossroads. |
| `9200023-the-first-landing.json` | Tirisfal Glades, level 9, Horde. Claire Willower in Brill: ask Cain Firesong what Tirisfal means and look into Venomweb Vale. First of two. |
| `9200024-what-the-tide-kept.json` | Level 9. Bring 4 Elven Cup Relics from the Vile Fin murlocs of the north coast. Choice of four rewards. |
| `9200025-the-archbishop-s-grave.json` | Tirisfal Glades, level 8, Horde. Dark Cleric Beryl in Brill sends you to the grave at Faol's Rest. |
| `9200026-thin-on-the-ground.json` | Tirisfal to Silverpine, level 10. Deathguard Linnea sends you to High Executor Hadrec at the Sepulcher. |

Written with the quest-writing skill (`.claude/skills/write-quests`). Not played yet. The stories used so far are in `Stories.md`. The tool's first test quest, "Bold Wolves of the Plains", was removed: the zone's own "Poison Water" already sends players after the same wolves.
