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

### The kinds of task

| Kind | What the player does |
|---|---|
| Kill enemies | Kill a number of up to four kinds of enemy. |
| Collect from enemies | Bring up to six kinds of item that enemies drop. |
| Bring items | Bring items that are bought, made, gathered or found. |
| Go and talk to someone | Travel from the giver to another person. |
| Kill one named enemy | One particular creature, once. |

Not possible yet: reaching a place, using an item somewhere, escorting someone, and quests that begin from an item or an object. Those need more than rows in the database.

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
| `spawns` | More of an enemy than the game has: `[ { "creature": number, "map": 1, "x": ..., "y": ..., "z": ..., "o": ... } ]`, at most forty. Each is a copy of one that is there already. `python Settings/Scripts/Quest-Tool.py --spots <creature> <count>` prints places near where the enemy lives, on the ground, ready to paste. They appear after a server restart. |
| `after` | The number of a quest that must be done first; 0 or left out for none. |
| `idea` | A note for the writer. Not used by the game. |

## How it gets into the game

`Settings/Scripts/Make-Quests.py` turns the files into rows in the game's database: the quest itself, who starts and ends it, the things said, loot that drops only while on the quest, added enemies, and **where it shows on the map**: an area around each group of the enemies to kill or to take things from, and a mark on the person it is handed in to. The map marks and added enemies are read by the server when it starts, so those need a restart. It first removes everything it wrote before (quests 9200000 to 9299999, and loot rows marked "WowPlus quest"), so what is in the game is always exactly what is in this folder. A file with a mistake is left out and named; the others still go in.

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

Written with the quest-writing skill (`.claude/skills/write-quests`). Not played yet. The stories used so far are in `Stories.md`. The tool's first test quest, "Bold Wolves of the Plains", was removed: the zone's own "Poison Water" already sends players after the same wolves.
