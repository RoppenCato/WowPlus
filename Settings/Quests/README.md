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
| `9200027-masons-not-bandits.json` | Westfall, level 12, Alliance. Quartermaster Lewis at Sentinel Hill: hear the Defias Traitor and Farmer Saldean. First of three. |
| `9200028-what-they-did-with-the-land.json` | Level 15. Look over the Alexston Farmstead and into Moonbrook. |
| `9200029-the-architect-who-stayed.json` | Westfall to Stormwind, level 16. Speak with Baros Alexston at City Hall. Choice of four rewards. |
| `9200030-where-the-light-fails.json` | Westfall to Duskwood, level 18. Captain Danuvin sends you to Commander Althea Ebonlocke in Darkshire. |
| `9200031-two-kings-facing-south.json` | Loch Modan, level 12, Alliance. Jern Hornhelm in Thelsamar: stand before the statues in the Valley of Kings and ask Captain Rugelfuss whose they are. First of two. |
| `9200032-the-enemy-s-masonry.json` | Level 17. Ask Chief Engineer Hinderweir who built the Stonewrought Dam and walk out onto it. Choice of four rewards. |
| `9200033-through-dun-algaz.json` | Loch Modan to the Wetlands, level 20. Mountaineer Stormpike at Algaz Station sends you to Captain Stoutfist in Menethil Harbor. |
| `9200034-the-queen-s-people.json` | Darkshore, level 18, Alliance. Gwennyth Bly'Leggonde in Auberdine: ask Thundris about Mathystra and look upon the ruins. First of two. |
| `9200035-no-kin-of-ours.json` | Level 20. Kill 6 Stormscale Myrmidons and 5 Stormscale Sorceresses in the Ruins of Mathystra. Choice of four rewards. |
| `9200036-the-forest-road.json` | Darkshore to Ashenvale, level 20. Terenthis in Auberdine sends you to Delgren the Purifier at Maestra's Post. |
| `9200037-the-boar-god-s-blood.json` | The Barrens, level 17, Horde. Tonga Runetotem at the Crossroads: look upon Thorn Hill and ask Mangletooth, caged at Camp Taurajo, about Agamaggan. First of two. |
| `9200038-the-razorfen.json` | Level 23. Cross Agama'gor and stand before Razorfen Kraul. Choice of four rewards. |
| `9200039-the-cut-mountain.json` | The Barrens to Stonetalon, level 17. Thork at the Crossroads sends you to Braelyn Firehand at Sun Rock Retreat. |
| `9200040-timber-for-orgrimmar.json` | The Barrens to Ashenvale, level 20. Kadrak at the Mor'shan Rampart sends you to Mastok Wrilehiss at Splintertree Post. |
| `9200041-the-dry-sea.json` | The Barrens to Thousand Needles, level 25. Kirge Sternhorn at Camp Taurajo sends you to Cliffwatcher Longhorn at Freewind Post. |
| `9200042-the-closed-gate.json` | Silverpine Forest, level 16, Horde. Deathguard Podrig at the Sepulcher: ask Dalar Dawnweaver about Arugal and go to the foot of the Greymane Wall. First of two. |
| `9200043-left-outside.json` | Level 17. Kill 8 Moonrage Bloodhowlers. Choice of four rewards. |
| `9200044-south-to-the-mill.json` | Silverpine to Hillsbrad, level 20. High Executor Hadrec sends you to High Executor Darthalia at Tarren Mill. |
| `9200045-under-the-enemy-s-city.json` | Ragefire Chasm, level 15, Alliance. Master Mathias Shaw in Stormwind: kill Taragaman the Hungerer. First of two. |
| `9200046-the-invoker-and-the-satyr.json` | Ragefire Chasm, level 16, Alliance. Kill Jergosh the Invoker and Bazzalan. Choice of four rewards. |
| `9200047-a-tauren-among-thieves.json` | The Deadmines, level 18, Horde. Sage Truthseeker in Thunder Bluff: kill Mr. Smite. First of two. |
| `9200048-the-ship-under-the-hill.json` | The Deadmines, level 19, Horde. Kill Captain Greenskin and Edwin VanCleef. Choice of four rewards. |
| `9200049-the-baron-s-household.json` | Shadowfang Keep, level 22, Alliance. Archmage Malin in Stormwind: lay Baron Silverlaine and Commander Springvale to rest. First of two. |
| `9200050-dalaran-s-mistake.json` | Shadowfang Keep, level 23, Alliance. Kill Archmage Arugal. Choice of four rewards. |
| `9200051-whose-keep-is-it.json` | Redridge Mountains, level 20, Alliance. Ask Magistrate Solomon in Lakeshire how Stonewatch fell and look upon Stonewatch Keep, then return to Marshal Marris in Lakeshire. |
| `9200052-the-other-horde.json` | Redridge Mountains, level 23, Alliance. Kill 6 Blackrock Trackers and 4 Blackrock Summoners in the hills north of Lakeshire, then return to Marshal Marris. Follows 9200051. |
| `9200053-the-conjurer-s-tower.json` | Redridge Mountains, level 25, Alliance. Reach the Tower of Ilgalar in the east of Redridge and kill 6 Servants of Ilgalar, then return to Marshal Marris in Lakeshire. Follows 9200052. Choice of four rewards. |
| `9200054-the-grove-no-one-enters.json` | Duskwood, level 28, Alliance. Go to the edge of the Twilight Grove in the middle of Duskwood, then return to Viktori Prism'Antras in Darkshire. |
| `9200055-the-red-queen-s-chains.json` | Wetlands, level 22, Alliance. Look over Angerfang Encampment in the middle of the Wetlands, then return to Glorin Steelbrow in Menethil Harbor. |
| `9200056-sick-of-the-mountain.json` | Wetlands, level 25, Alliance. Kill 8 Lost Whelps in the eastern Wetlands, then return to Glorin Steelbrow in Menethil Harbor. Follows 9200055. |
| `9200057-what-crawled-away.json` | Wetlands, level 26, Alliance. Kill 4 Dragonmaw Centurions and 6 Dragonmaw Bonewarders at Angerfang Encampment, then return to Glorin Steelbrow in Menethil Harbor. Follows 9200056. Choice of four rewards. |
| `9200058-a-bridge-between-clans.json` | Wetlands, level 26, Alliance. Ask Longbraid the Grim about the Thandol Span and walk out onto it, then return to Rhag Garmason at the camp south of Dun Modr. |
| `9200059-over-the-span.json` | Wetlands, level 30, Alliance. Cross the Thandol Span into the Arathi Highlands and bring Rhag Garmason's word to Captain Nials at Refuge Pointe. |
| `9200060-a-fishing-village.json` | Hillsbrad Foothills, level 22, Alliance. Ask Marshal Redpath about Southshore's wars, then return to Innkeeper Anderson in Southshore. |
| `9200061-the-lords-of-nothing.json` | Hillsbrad Foothills, level 24, Alliance. Ask Loremaster Dibbs in Southshore what the Syndicate is and look upon the ruins of Durnholde Keep, then return to Magistrate Henry Maleb. |
| `9200062-thinning-the-peerage.json` | Hillsbrad Foothills, level 24, Alliance. Kill 6 Syndicate Shadow Mages and 5 Syndicate Rogues at Durnholde Keep, then return to Magistrate Henry Maleb in Southshore. Follows 9200061. Choice of four rewards. |
| `9200063-the-slave-of-durnholde.json` | Hillsbrad Foothills, level 26, Alliance. Stand in the middle of the ruins of Durnholde Keep, then return to Loremaster Dibbs in Southshore. Follows 9200062. Choice of four rewards. |
| `9200064-where-the-city-was.json` | Hillsbrad Foothills, level 30, Alliance. Go to the edge of the Dalaran crater, on the shore of Lordamere Lake north-west of Hillsbrad, then return to Loremaster Dibbs in Southshore. |
| `9200065-the-cradle-of-kings.json` | Hillsbrad Foothills, level 30, Alliance. Take the east road through Thoradin's Wall into the Arathi Highlands and report to Captain Nials at Refuge Pointe. |
| `9200066-where-the-warchief-wore-chains.json` | Hillsbrad Foothills, level 24, Horde. Stand in the middle of the ruins of Durnholde Keep, then return to Krusk in Tarren Mill. |
| `9200067-squatters.json` | Hillsbrad Foothills, level 25, Horde. Kill 10 Syndicate Watchmen at Durnholde Keep, then return to Krusk in Tarren Mill. Follows 9200066. |
| `9200068-foxton-s-song.json` | Hillsbrad Foothills, level 26, Horde. Go to Grommash Hold in Orgrimmar and tell Thrall that you have stood in Durnholde. Follows 9200067. Choice of four rewards. |
| `9200069-the-hammer-s-fall.json` | Hillsbrad Foothills, level 30, Horde. Travel east through Thoradin's Wall into the Arathi Highlands and report to Drum Fel at Hammerfall. |
| `9200070-a-blackrock-s-debt.json` | The Stockade, level 26, Horde. Kill Targorr the Dread in the Stockade, beneath Stormwind, then return to Eitrigg in Grommash Hold in Orgrimmar. |
| `9200071-the-mason-s-lieutenant.json` | The Stockade, level 27, Horde. Kill Bazil Thredd in the Stockade, beneath Stormwind, then return to Sage Truthseeker on the Elder Rise in Thunder Bluff. Choice of four rewards. |
| `9200072-why-we-cut.json` | Ashenvale, level 24, Horde. Ask Overseer Gorthak at the Warsong Lumber Camp how the cutting began and ask Kuray'bin near Splintertree Post about the fountain, then return to Mastok Wrilehiss at Splintertree Post. |
| `9200073-what-the-blood-left.json` | Ashenvale, level 28, Horde. Kill 6 Searing Infernals on Felfire Hill, then return to Mastok Wrilehiss at Splintertree Post. Follows 9200072. |
| `9200074-may-the-warsong-never-fade.json` | Ashenvale, level 30, Horde. Go to Grom Hellscream's monument in Demon Fall Canyon, then return to Mastok Wrilehiss at Splintertree Post. Follows 9200073. Choice of four rewards. |
| `9200075-the-forest-lord.json` | Ashenvale, level 24, Alliance. Ask Pelturas Whitemoon in Astranaar who Cenarius was, then return to Sentinel Thenysil. |
| `9200076-they-are-still-cutting.json` | Ashenvale, level 27, Alliance. Destroy 4 Warsong Shredders at the Warsong Lumber Camp, then report to Sentinel Melyria Frostshadow at Silverwing Outpost. Follows 9200075. Choice of four rewards. |
| `9200077-the-talondeep-path.json` | Ashenvale, level 24, Alliance. Take the Talondeep Path south into the Stonetalon Mountains and find Kaela Shadowspear in Windshear Crag. |
| `9200078-if-you-can-chop-it-down.json` | Stonetalon Mountains, level 22, Horde. Ask Ziz Fizziks in Windshear Crag what the Venture Company is and see what has been done to the Crag, then return to Tammra Windfield at Sun Rock Retreat. |
| `9200079-the-earthmother-s-patience.json` | Stonetalon Mountains, level 23, Horde. Kill 8 Venture Co. Loggers and 6 Venture Co. Deforesters in Windshear Crag, then return to Tammra Windfield at Sun Rock Retreat. Follows 9200078. Choice of four rewards. |
| `9200080-the-wyverns-lake.json` | Stonetalon Mountains, level 24, Horde. Go to the shore of Mirkfallon Lake, north of Sun Rock Retreat, then return to Maggran Earthbinder. |
| `9200081-the-company-s-ledger.json` | Stonetalon Mountains, level 22, Alliance. Kill 8 Venture Co. Operators and 6 Venture Co. Loggers in Windshear Crag, then return to Kaela Shadowspear. Choice of four rewards. |

Written with the quest-writing skill (`.claude/skills/write-quests`). Not played yet. The stories used so far are in `Stories.md`. The tool's first test quest, "Bold Wolves of the Plains", was removed: the zone's own "Poison Water" already sends players after the same wolves.
