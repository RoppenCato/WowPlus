# Where quests are needed

A survey of every zone: how many quests it has for each side, how they are spread, how much of the levelling they carry, and which enemies and places no quest uses. The goal is many ways to level: at every stretch of levels there should be several zones that are a full choice by themselves.

The first part is written by hand and is what to act on. Everything below the line is made by `python Settings/Scripts/Make-Zone-Audit.py` from the game's data and should be made again after quests are added.

## Findings (2026-10-12)

**The measure.** A zone's "share" is the experience its solo quests give, as a part of what its levels cost, at this server's quest rate (1.5). A zone at 70% or more is called a full choice: with kills and discoveries on top, you can level there without leaving. 70 zones were looked at; cities and level-cap zones are listed but not judged.

**Where the game is rich.** Levels 1 to 20: every starting zone and the zone after it carries itself (Mulgore 130%, Durotar 93%, Tirisfal 132%, the Barrens 119%, Silverpine 71%, Ghostlands 118%; for the Alliance likewise). Outland and Northrend are well filled almost everywhere.

**Where it is thin.** The old world from 21 to 60, for both sides:

| Levels | Full choices, Horde | Full choices, Alliance | What there is |
|---|---|---|---|
| 21-25 | 1 | 0 | The Barrens' upper end, Stonetalon (46%), Ashenvale (34%). |
| 26-30 | 0 | 0 | Ashenvale, Hillsbrad (55%), Thousand Needles (39%). For the Alliance: Wetlands (43%), Duskwood (67%). |
| 31-35 | 0 | 0 | Hillsbrad, Thousand Needles, Desolace (49%). **The narrowest stretch in the game.** |
| 36-40 | 2 | 2 | Dustwallow Marsh and Stranglethorn carry it. |
| 41-45 | 1 | 1 | Stranglethorn; Swamp of Sorrows (23%) and Badlands (45%) do not. |
| 46-50 | 0 | 0 | Feralas (57%), the Hinterlands (54%), Searing Gorge (42%), Tanaris (35%). Many zones, none full. |
| 51-60 | 2-3 | 2-3 | Un'Goro, Felwood and Silithus are full. Azshara (25%), Blasted Lands (15%), Burning Steppes (33%), Western Plaguelands, Winterspring and Eastern Plaguelands are not. |

That is the real game's shape: from 20 on you were meant to hop between zones and run dungeons. For a single-player world where the wish is to pick a zone and stay, it is the gap to fill.

**Where the room is.** Thin zones differ in how much they offer to write with:

| Zone | Levels | Share (Horde / Alliance) | Kinds of enemy no quest uses | Note |
|---|---|---|---|---|
| Azshara | 52-58 | 25% / 20% | 51 | The emptiest large zone: naga, satyrs, Timbermaw, blue dragonkin, whole named places with nothing asked. |
| Ashenvale | 24-30 | 34% / 64% | 27 | Horde side is thin; stags, bears, furbolgs and the Warsong peons are unused. |
| Thousand Needles | 29-37 | 39% / 23% | 28 | Thin for both; sits in the narrowest stretch. |
| Desolace | 33-38 | 49% / 57% | 24 | The centaur clans and the kodo graveyard. |
| Swamp of Sorrows | 38-44 | 23% / 13% | 16 | Very thin for both. |
| Burning Steppes | 53-54 | 33% / 39% | 23 | Small level range; ten quests would fill it. |
| Eastern Plaguelands | 56-60 | 58% / 54% | 48 | Nearly full, and a great deal unused. |
| Western Plaguelands | 55-60 | 40% / 41% | 27 | |
| Stonetalon Mountains | 21-26 | 46% / 40% | 14 | The first thin zone a Horde character meets. |
| Blasted Lands | 50-57 | 15% / 19% | 4 | Thin, but little to write with. |
| Tanaris | 47-60 | 35% / 35% | 1 | Its quests are spread over thirteen levels, so the number is worse than the zone feels. Everything there is used already. |

**What "enough" would take.** Bringing every thin zone to 70% for both sides is about 600 quests. That is not the plan. The plan is choices:

1. **First, one full choice per stretch that has none**, for the side being played (Horde): Ashenvale or Stonetalon (21-30), Thousand Needles (31-35), the Hinterlands or Feralas (46-50). About 12 to 30 quests each; a chain of three is about a tenth of a zone's need at those levels.
2. **Then a second choice per stretch**, picked for how much unused material the zone has: Desolace, Swamp of Sorrows, Azshara.
3. **Alliance** when an Alliance character is played; the same list applies with Redridge, Wetlands, Alterac and Hillsbrad added.

A reasonable size for one sitting of the quest skill is two or three chains (six to nine quests) in one zone. Rewards: about one quest in three should offer gear to choose.

**Dungeons and raids** have their own section below, sorted by the level of the place, with every quest named. What stands out:

| What | Where |
|---|---|
| Dungeons one side has no quests for at all | Deadmines and the Stormwind Stockade for the Horde; Ragefire Chasm and Shadowfang Keep for the Alliance. With bots, either side can run any of them, so these are four dungeons with loot and no story for half the players. |
| Old dungeons with few quests for a side | Scarlet Monastery (Alliance 2), Razorfen Kraul (4 each, two of them filed outside). |
| Northrend dungeons with one or two quests | Halls of Stone (1), Azjol-Nerub, the Old Kingdom, Violet Hold, Halls of Lightning, Utgarde Pinnacle, the Culling of Stratholme, the Forge of Souls (2 each). They were built for the dungeon finder, not for stories. |
| Outland dungeons with one or two | The Botanica and the Escape from Durnholde (1), Auchenai Crypts, Sethekk Halls, the Slave Pens (2). |
| Well supplied | Blackrock Depths (26), Blackrock Spire (40), Stratholme (31), Scholomance (21), Dire Maul (22), Gnomeregan, Uldaman, Zul'Farrak, Maraudon, Sunken Temple. |

A fair aim is **three quests per side for every dungeon**, at least one of them given outside so that there is a reason to set out. For the Horde character being played that means Deadmines and the Stockade first, then the thin Northrend dungeons when the levels come. Our quest files cannot yet point at creatures inside a dungeon on the map picker, but kill, collect and slay quests work there like anywhere.

**How far to trust the lists.** "Enemies no quest uses" is found by looking at what quests ask to be killed and at whose loot holds the items they ask for. It misses enemies used through a made item or a scripted event, and it lists a few that are there for atmosphere. "Places nothing is asked of you" are named parts of a zone with ten or more enemies and none of them touched by a quest. Both are leads to check with `Quest-Tool.py --zone` and `--quests`, not facts.

**Our own quests so far:** six, all in Mulgore, which was already full (130%). They were made to try the tool and the skill. The next ones should go where the survey points.

<!-- Made by the script from here on -->

Made by `Settings/Scripts/Make-Zone-Audit.py` on 2026-10-12. Run it again after adding quests.

## How to read it

- **Levels**: the middle half of the zone's quests, which is where you usually are when you level there.
- **Quests (share)**: quests a player of that side can get, and in brackets how much of the experience those levels cost that the zone's solo quests give, at this server's quest rate (1.5). 100% means the quests alone carry you through the zone's levels; kills and discoveries come on top.
- **Add**: how many ordinary quests would bring that side to 70%. That is the mark where a zone is a full choice by itself. "–" means the side has no business there.
- Not counted: holiday quests, daily and repeatable ones, quests nobody gives, and (for the share) dungeon, raid and group quests.

## All zones by level

| Levels | Zone | Continent | Horde quests (share) | Alliance quests (share) | Ours | Enemies no quest uses | Add |
|---|---|---|---|---|---|---|---|
| 5-9 | Eversong Woods | Eastern Kingdoms | 73 (215%) | – |  | 10 | enough |
| 5-10 | Azuremyst Isle | Kalimdor | – | 65 (132%) |  | 17 | enough |
| 5-10 | Dun Morogh | Eastern Kingdoms | 15 (2%) passing through | 66 (160%) | 3 | 13 | enough |
| 5-10 | Elwynn Forest | Eastern Kingdoms | – | 53 (113%) | 5 | 8 | enough |
| 5-10 | Mulgore | Kalimdor | 46 (130%) | 3 (8%) passing through | 6 | 9 | enough |
| 5-10 | Teldrassil | Kalimdor | 3 (14%) passing through | 56 (243%) | 3 | 13 | enough |
| 5-10 | Tirisfal Glades | Eastern Kingdoms | 51 (146%) | – | 4 | 7 | enough |
| 5-11 | Durotar | Kalimdor | 47 (103%) | 3 (7%) passing through | 4 | 14 | enough |
| 12-17 | Silverpine Forest | Eastern Kingdoms | 44 (71%) | 4 (7%) passing through |  | 11 | enough |
| 12-18 | Ghostlands | Eastern Kingdoms | 63 (118%) | – |  | 10 | enough |
| 12-18 | Westfall | Eastern Kingdoms | 7 (11%) passing through | 36 (94%) |  | 7 | enough |
| 14-18 | Bloodmyst Isle | Kalimdor | – | 74 (130%) |  | 7 | enough |
| 14-20 | Darkshore | Kalimdor | 11 (18%) passing through | 73 (102%) |  | 18 | enough |
| 14-21 | The Barrens | Kalimdor | 94 (119%) | 22 (22%) passing through |  | 21 | enough |
| 14-55 | Darnassus | Kalimdor | – | 21 |  | 0 | city |
| 15-18 | Loch Modan | Eastern Kingdoms | – | 41 (115%) |  | 4 | enough |
| 15-22 | Redridge Mountains | Eastern Kingdoms | – | 37 (43%) |  | 3 | A +21 |
| 21-26 | Stonetalon Mountains | Kalimdor | 31 (46%) | 25 (40%) |  | 14 | H +14, A +18 |
| 23-50 | Thunder Bluff | Kalimdor | 32 | – |  | 0 | city |
| 24-30 | Ashenvale | Kalimdor | 29 (34%) | 59 (64%) |  | 27 | H +25, A +5 |
| 24-30 | Wetlands | Eastern Kingdoms | – | 40 (43%) |  | 20 | A +19 |
| 25-30 | Duskwood | Eastern Kingdoms | 4 (8%) passing through | 73 (67%) |  | 3 | A +2 |
| 27-34 | Hillsbrad Foothills | Eastern Kingdoms | 43 (55%) | 18 (21%) |  | 2 | H +14, A +44 |
| 28-41 | Stormwind City | Eastern Kingdoms | – | 72 |  | 0 | city |
| 29-37 | Thousand Needles | Kalimdor | 53 (39%) | 34 (23%) |  | 28 | H +32, A +48 |
| 33-38 | Desolace | Kalimdor | 37 (49%) | 40 (57%) |  | 24 | H +15, A +10 |
| 33-60 | Orgrimmar | Kalimdor | 85 | 4 |  | 0 | city |
| 34-40 | Alterac Mountains | Eastern Kingdoms | 3 (9%) passing through | 15 (21%) |  | 5 | A +39 |
| 36-39 | Dustwallow Marsh | Kalimdor | 68 (113%) | 75 (115%) |  | 16 | enough |
| 36-56 | Undercity | Eastern Kingdoms | 28 | – |  | 0 | city |
| 37-40 | Arathi Highlands | Eastern Kingdoms | 37 (67%) | 26 (52%) |  | 13 | H +2, A +9 |
| 37-43 | Stranglethorn Vale | Eastern Kingdoms | 109 (78%) | 114 (76%) |  | 12 | enough |
| 38-44 | Swamp of Sorrows | Eastern Kingdoms | 20 (23%) | 14 (13%) |  | 16 | H +38, A +46 |
| 39-43 | Badlands | Eastern Kingdoms | 38 (45%) | 51 (53%) |  | 5 | H +15, A +10 |
| 43-60 | Ironforge | Eastern Kingdoms | – | 77 |  | 0 | city |
| 44-48 | Feralas | Kalimdor | 35 (57%) | 46 (60%) |  | 2 | H +8, A +6 |
| 45-49 | The Hinterlands | Eastern Kingdoms | 32 (54%) | 22 (35%) |  | 0 | H +10, A +21 |
| 47-50 | Searing Gorge | Eastern Kingdoms | 22 (42%) | 29 (56%) |  | 9 | H +14, A +7 |
| 47-60 | Tanaris | Kalimdor | 86 (35%) | 83 (35%) |  | 1 | H +64, A +65 |
| 50-57 | Blasted Lands | Eastern Kingdoms | 19 (15%) | 20 (19%) |  | 4 | H +54, A +51 |
| 50-60 | The Exodar | Kalimdor | – | 6 |  | 0 | city |
| 52-54 | Un'Goro Crater | Kalimdor | 46 (113%) | 44 (106%) |  | 1 | enough |
| 52-58 | Azshara | Kalimdor | 30 (25%) | 22 (20%) |  | 51 | H +38, A +43 |
| 53-54 | Burning Steppes | Eastern Kingdoms | 9 (33%) | 11 (39%) |  | 23 | H +10, A +8 |
| 54-55 | Felwood | Kalimdor | 32 (125%) | 33 (113%) |  | 0 | enough |
| 54-57 | Western Plaguelands | Eastern Kingdoms | 57 (73%) | 55 (73%) |  | 27 | enough |
| 56-59 | Winterspring | Kalimdor | 38 (47%) | 45 (57%) |  | 19 | H +12, A +7 |
| 56-60 | Eastern Plaguelands | Eastern Kingdoms | 61 (58%) | 54 (54%) |  | 48 | H +9, A +11 |
| 57-60 | Moonglade | Kalimdor | 8 | 8 |  | 1 | druid sanctuary |
| 60-60 | Silithus | Kalimdor | 65 (182%) | 65 (182%) |  | 4 | enough |
| 60-60 | Silvermoon City | Eastern Kingdoms | 5 | – |  | 0 | city |
| 61-62 | Hellfire Peninsula | Outland | 101 (161%) | 88 (138%) |  | 21 | enough |
| 62-64 | Zangarmarsh | Outland | 63 (68%) | 65 (70%) |  | 24 | H +2 |
| 64-65 | Terokkar Forest | Outland | 98 (113%) | 94 (111%) |  | 52 | enough |
| 65-70 | Shattrath City | Outland | 40 | 41 |  | 25 | city |
| 66-67 | Nagrand | Outland | 96 (92%) | 82 (78%) |  | 18 | enough |
| 66-68 | Blade's Edge Mountains | Outland | 100 (70%) | 97 (67%) |  | 48 | H +1, A +3 |
| 68-70 | Netherstorm | Outland | 153 (72%) | 153 (72%) |  | 38 | enough |
| 69-70 | Shadowmoon Valley | Outland | 133 (72%) | 133 (72%) |  | 59 | enough |
| 70-70 | Deadwind Pass | Eastern Kingdoms | 32 (28%) | 32 (28%) |  | 1 | H +34, A +34 |
| 70-80 | Isle of Quel'Danas | Eastern Kingdoms | 12 | 12 |  | 21 | daily quests at the level cap |
| 71-71 | Borean Tundra | Northrend | 170 (268%) | 145 (224%) |  | 53 | enough |
| 71-71 | Howling Fjord | Northrend | 95 (155%) | 123 (177%) |  | 79 | enough |
| 72-74 | Dragonblight | Northrend | 151 (78%) | 141 (69%) |  | 61 | A +2 |
| 74-75 | Grizzly Hills | Northrend | 88 (66%) | 96 (78%) |  | 34 | H +5 |
| 74-80 | Dalaran | Northrend | 5 | 5 |  | 3 | city |
| 75-77 | Zul'Drak | Northrend | 108 (53%) | 108 (53%) |  | 23 | H +26, A +26 |
| 76-77 | Sholazar Basin | Northrend | 82 (68%) | 82 (68%) |  | 22 | H +2, A +2 |
| 79-80 | The Storm Peaks | Northrend | 96 (157%) | 95 (153%) |  | 56 | enough |
| 80-80 | Icecrown | Northrend | 166 | 164 |  | 122 | level cap |

## Choices per level

For every five levels: the zones a side can level in (at least 3 quests there), and how many of those carry 70% or more by themselves. Few full choices means the stretch is narrow.

| Levels | Horde zones | of which full | Alliance zones | of which full |
|---|---|---|---|---|
| 1-5 | 4: Eversong Woods, Mulgore, Tirisfal Glades, Durotar | 4 | 4: Azuremyst Isle, Dun Morogh, Elwynn Forest, Teldrassil | 4 |
| 6-10 | 4: Eversong Woods, Mulgore, Tirisfal Glades, Durotar | 4 | 4: Azuremyst Isle, Dun Morogh, Elwynn Forest, Teldrassil | 4 |
| 11-15 | 4: Durotar, Silverpine Forest, Ghostlands, The Barrens | 4 | 5: Westfall, Bloodmyst Isle, Darkshore, Loch Modan, Redridge Mountains | 4 |
| 16-20 | 3: Silverpine Forest, Ghostlands, The Barrens | 3 | 5: Westfall, Bloodmyst Isle, Darkshore, Loch Modan, Redridge Mountains | 4 |
| 21-25 | 3: The Barrens, Stonetalon Mountains, Ashenvale | 1 | 5: Redridge Mountains, Stonetalon Mountains, Ashenvale, Wetlands, Duskwood | 0 |
| 26-30 | 4: Stonetalon Mountains, Ashenvale, Hillsbrad Foothills, Thousand Needles | 0 | 6: Stonetalon Mountains, Ashenvale, Wetlands, Duskwood, Hillsbrad Foothills, Thousand Needles | 0 |
| 31-35 | 3: Hillsbrad Foothills, Thousand Needles, Desolace | 0 | 4: Hillsbrad Foothills, Thousand Needles, Desolace, Alterac Mountains | 0 |
| 36-40 | 7: Thousand Needles, Desolace, Dustwallow Marsh, Arathi Highlands, Stranglethorn Vale, Swamp of Sorrows, Badlands | 2 | 8: Thousand Needles, Desolace, Alterac Mountains, Dustwallow Marsh, Arathi Highlands, Stranglethorn Vale, Swamp of Sorrows, Badlands | 2 |
| 41-45 | 5: Stranglethorn Vale, Swamp of Sorrows, Badlands, Feralas, The Hinterlands | 1 | 5: Stranglethorn Vale, Swamp of Sorrows, Badlands, Feralas, The Hinterlands | 1 |
| 46-50 | 5: Feralas, The Hinterlands, Searing Gorge, Tanaris, Blasted Lands | 0 | 5: Feralas, The Hinterlands, Searing Gorge, Tanaris, Blasted Lands | 0 |
| 51-55 | 7: Tanaris, Blasted Lands, Un'Goro Crater, Azshara, Burning Steppes, Felwood, Western Plaguelands | 3 | 7: Tanaris, Blasted Lands, Un'Goro Crater, Azshara, Burning Steppes, Felwood, Western Plaguelands | 3 |
| 56-60 | 7: Tanaris, Blasted Lands, Azshara, Western Plaguelands, Winterspring, Eastern Plaguelands, Silithus | 2 | 7: Tanaris, Blasted Lands, Azshara, Western Plaguelands, Winterspring, Eastern Plaguelands, Silithus | 2 |
| 61-65 | 3: Hellfire Peninsula, Zangarmarsh, Terokkar Forest | 2 | 3: Hellfire Peninsula, Zangarmarsh, Terokkar Forest | 3 |
| 66-70 | 5: Nagrand, Blade's Edge Mountains, Netherstorm, Shadowmoon Valley, Deadwind Pass | 4 | 5: Nagrand, Blade's Edge Mountains, Netherstorm, Shadowmoon Valley, Deadwind Pass | 3 |
| 71-75 | 5: Borean Tundra, Howling Fjord, Dragonblight, Grizzly Hills, Zul'Drak | 3 | 5: Borean Tundra, Howling Fjord, Dragonblight, Grizzly Hills, Zul'Drak | 3 |
| 76-80 | 3: Zul'Drak, Sholazar Basin, The Storm Peaks | 1 | 3: Zul'Drak, Sholazar Basin, The Storm Peaks | 1 |

## Dungeons and raids

Quests that belong to a dungeon or raid, by the level of the place. They are not counted in the zones' shares above. A place with fewer than 3 quests for a side gives that side little reason to go beyond the loot.

| Levels | Place | Kind | Horde quests | Alliance quests | Experience, Horde | Experience, Alliance | Give gear | Note |
|---|---|---|---|---|---|---|---|---|
| 16-16 | Ragefire Chasm | dungeon | 6 | 0 | 14,445 | 0 | 3 | few for Alliance |
| 17-21 | Wailing Caverns | dungeon | 9 | 5 | 22,957 | 16,425 | 4 |  |
| 18-20 | Deadmines | dungeon | 0 | 5 | 0 | 14,625 | 5 | few for Horde |
| 22-27 | Blackfathom Deeps | dungeon | 6 | 5 | 19,852 | 22,350 | 6 |  |
| 25-27 | Shadowfang Keep | dungeon | 3 | 0 | 14,250 | 0 | 3 | few for Alliance |
| 26-27 | Stormwind Stockade | dungeon | 0 | 6 | 0 | 29,025 | 3 | few for Horde |
| 26-30 | Razorfen Kraul | dungeon | 4 | 4 | 19,875 | 20,625 | 5 |  |
| 30-30 | Ashenvale (which dungeon or wing is not told) | dungeon | 1 | 0 | 5,475 | 0 | 1 | few for Horde and Alliance |
| 30-34 | Gnomeregan | dungeon | 12 | 20 | 36,045 | 67,732 | 9 |  |
| 33-42 | Scarlet Monastery | dungeon | 5 | 2 | 43,425 | 18,600 | 6 | few for Alliance |
| 36-37 | Razorfen Downs | dungeon | 8 | 4 | 54,855 | 23,355 | 5 |  |
| 41-44 | Uldaman | dungeon | 9 | 13 | 75,825 | 113,047 | 4 |  |
| 45-47 | Zul'Farrak | dungeon | 9 | 9 | 94,462 | 95,587 | 3 |  |
| 46-46 | Thousand Needles (which dungeon or wing is not told) | dungeon | 0 | 1 | 0 | 862 | 0 | few for Horde and Alliance |
| 47-51 | Maraudon | dungeon | 8 | 8 | 110,775 | 110,775 | 9 |  |
| 51-53 | Sunken Temple | dungeon | 9 | 8 | 120,660 | 118,200 | 6 |  |
| 52-52 | Blackrock Mountain (which dungeon or wing is not told) | dungeon | 1 | 1 | 11,010 | 11,010 | 0 | few for Horde and Alliance |
| 53-53 | Un'Goro Crater (which dungeon or wing is not told) | dungeon | 1 | 0 | 8,550 | 0 | 1 | few for Horde and Alliance |
| 54-59 | Blackrock Depths | dungeon | 26 | 19 | 385,829 | 269,692 | 14 |  |
| 58-60 | Tanaris (which dungeon or wing is not told) | raid | 3 | 3 | 48,900 | 48,900 | 1 |  |
| 60-60 | Ahn'Qiraj Temple | raid | 71 | 71 | 386,700 | 386,700 | 70 |  |
| 60-60 | Azshara (which dungeon or wing is not told) | raid | 1 | 1 | 21,450 | 21,450 | 1 | few for Horde and Alliance |
| 60-60 | Blackrock Spire | dungeon | 40 | 39 | 738,494 | 710,444 | 37 |  |
| 60-60 | Blackwing Lair | raid | 2 | 2 | 35,775 | 35,775 | 1 | few for Horde and Alliance |
| 60-60 | Burning Steppes (which dungeon or wing is not told) | dungeon | 1 | 1 | 10,725 | 10,725 | 0 | few for Horde and Alliance |
| 60-60 | Dire Maul | dungeon | 22 | 22 | 387,802 | 387,802 | 19 |  |
| 60-60 | Dustwallow Marsh (which dungeon or wing is not told) | raid | 4 | 0 | 57,300 | 0 | 0 | few for Alliance |
| 60-60 | Eastern Plaguelands (which dungeon or wing is not told) | dungeon | 1 | 1 | 14,325 | 14,325 | 0 | few for Horde and Alliance |
| 60-60 | Eastern Plaguelands (which dungeon or wing is not told) | raid | 1 | 2 | 17,850 | 39,300 | 2 | few for Horde and Alliance |
| 60-60 | Molten Core | raid | 4 | 4 | 75,075 | 75,075 | 1 |  |
| 60-60 | Moonglade (which dungeon or wing is not told) | dungeon | 1 | 1 | 21,450 | 21,450 | 1 | few for Horde and Alliance |
| 60-60 | Moonglade (which dungeon or wing is not told) | raid | 2 | 2 | 35,775 | 35,775 | 1 | few for Horde and Alliance |
| 60-60 | Naxxramas | raid | 17 | 17 | 35,775 | 35,775 | 15 |  |
| 60-60 | Ruins of Ahn'Qiraj | raid | 1 | 1 | 21,450 | 21,450 | 1 | few for Horde and Alliance |
| 60-60 | Scholomance | dungeon | 21 | 21 | 263,250 | 263,250 | 25 |  |
| 60-60 | Silithus (which dungeon or wing is not told) | raid | 10 | 10 | 137,482 | 137,482 | 9 |  |
| 60-60 | Stratholme | dungeon | 31 | 30 | 597,075 | 575,625 | 37 |  |
| 60-60 | Zul'Gurub | raid | 67 | 67 | 953,212 | 953,212 | 66 |  |
| 62-62 | Hellfire Peninsula (which dungeon or wing is not told) | dungeon | 1 | 1 | 30,150 | 30,150 | 0 | few for Horde and Alliance |
| 63-70 | Hellfire Citadel: The Shattered Halls | dungeon | 7 | 8 | 234,900 | 272,850 | 9 |  |
| 65-65 | Coilfang: The Underbog | dungeon | 4 | 4 | 132,000 | 132,000 | 3 |  |
| 66-70 | Auchindoun: Mana-Tombs | dungeon | 4 | 4 | 130,200 | 130,200 | 3 |  |
| 67-68 | Auchindoun: Auchenai Crypts | dungeon | 2 | 1 | 70,950 | 34,950 | 1 | few for Horde and Alliance |
| 68-68 | The Escape From Durnholde | dungeon | 1 | 1 | 18,000 | 18,000 | 1 | few for Horde and Alliance |
| 68-70 | Caverns of Time (which dungeon or wing is not told) | dungeon | 6 | 6 | 151,575 | 151,575 | 0 |  |
| 69-69 | Auchindoun: Sethekk Halls | dungeon | 2 | 2 | 73,800 | 73,800 | 2 | few for Horde and Alliance |
| 69-69 | Shadowmoon Valley (which dungeon or wing is not told) | dungeon | 1 | 1 | 36,900 | 36,900 | 0 | few for Horde and Alliance |
| 70-70 | Auchindoun (which dungeon or wing is not told) | dungeon | 4 | 4 | 128,100 | 128,100 | 1 |  |
| 70-70 | Auchindoun: Shadow Labyrinth | dungeon | 4 | 4 | 150,750 | 150,750 | 1 |  |
| 70-70 | Black Temple | raid | 2 | 2 | 57,000 | 57,000 | 1 | few for Horde and Alliance |
| 70-70 | Coilfang: Serpentshrine Cavern | raid | 1 | 1 | 28,500 | 28,500 | 0 | few for Horde and Alliance |
| 70-70 | Coilfang: The Slave Pens | dungeon | 2 | 2 | 28,500 | 28,500 | 0 | few for Horde and Alliance |
| 70-70 | Coilfang: The Steamvault | dungeon | 4 | 4 | 132,825 | 132,825 | 1 |  |
| 70-70 | Karazhan | raid | 9 | 9 | 132,825 | 132,825 | 2 |  |
| 70-70 | Magister's Terrace | dungeon | 3 | 3 | 94,875 | 94,875 | 1 |  |
| 70-70 | Magtheridon's Lair | raid | 1 | 1 | 28,500 | 28,500 | 1 | few for Horde and Alliance |
| 70-70 | Shattrath City (which dungeon or wing is not told) | dungeon | 4 | 4 | 123,450 | 123,450 | 0 |  |
| 70-70 | Tempest Keep | raid | 6 | 6 | 189,825 | 189,825 | 3 |  |
| 70-70 | Tempest Keep: The Botanica | dungeon | 1 | 1 | 37,950 | 37,950 | 0 | few for Horde and Alliance |
| 70-70 | The Battle for Mount Hyjal | raid | 1 | 1 | 28,500 | 28,500 | 0 | few for Horde and Alliance |
| 70-70 | The Black Temple (which dungeon or wing is not told) | raid | 1 | 1 | 23,700 | 23,700 | 0 | few for Horde and Alliance |
| 70-70 | Zul'Aman | raid | 8 | 8 | 101,475 | 101,475 | 3 |  |
| 71-71 | The Nexus | dungeon | 4 | 4 | 241,200 | 241,200 | 5 |  |
| 71-71 | Utgarde Keep | dungeon | 3 | 2 | 165,825 | 120,600 | 5 | few for Alliance |
| 74-74 | Azjol-Nerub | dungeon | 2 | 2 | 124,500 | 124,500 | 2 | few for Horde and Alliance |
| 74-75 | Drak'Tharon Keep | dungeon | 3 | 3 | 171,675 | 171,675 | 3 |  |
| 76-76 | Ahn'kahet: The Old Kingdom | dungeon | 2 | 2 | 126,900 | 126,900 | 1 | few for Horde and Alliance |
| 77-77 | Violet Hold | dungeon | 2 | 2 | 67,425 | 67,425 | 1 | few for Horde and Alliance |
| 78-78 | Gundrak | dungeon | 3 | 3 | 194,400 | 194,400 | 3 |  |
| 78-78 | Halls of Stone | dungeon | 1 | 1 | 64,800 | 64,800 | 1 | few for Horde and Alliance |
| 80-80 | Halls of Lightning | dungeon | 2 | 2 | 132,300 | 132,300 | 2 | few for Horde and Alliance |
| 80-80 | Halls of Reflection | dungeon | 3 | 3 | 107,475 | 107,475 | 2 |  |
| 80-80 | Icecrown (which dungeon or wing is not told) | dungeon | 1 | 1 | 33,075 | 33,075 | 0 | few for Horde and Alliance |
| 80-80 | Icecrown Citadel | raid | 32 | 32 | 1,116,150 | 1,116,150 | 24 |  |
| 80-80 | Pit of Saron | dungeon | 4 | 4 | 173,625 | 173,625 | 2 |  |
| 80-80 | The Culling of Stratholme | dungeon | 2 | 2 | 132,300 | 132,300 | 1 | few for Horde and Alliance |
| 80-80 | The Eye of Eternity | raid | 2 | 2 | 115,800 | 115,800 | 2 | few for Horde and Alliance |
| 80-80 | The Forge of Souls | dungeon | 2 | 2 | 74,400 | 74,400 | 2 | few for Horde and Alliance |
| 80-80 | The Oculus | dungeon | 4 | 4 | 264,600 | 264,600 | 2 |  |
| 80-80 | The Sunwell | raid | 4 | 2 | 0 | 0 | 0 | few for Alliance |
| 80-80 | Ulduar | raid | 18 | 18 | 1,025,400 | 1,025,400 | 5 |  |
| 80-80 | Utgarde Pinnacle | dungeon | 2 | 2 | 132,300 | 132,300 | 2 | few for Horde and Alliance |
| 82-82 | The Ruby Sanctum | raid | 3 | 3 | 87,750 | 87,750 | 1 |  |

### The quests of each dungeon and raid

**Ragefire Chasm** (dungeon, 16-16): Hidden Enemies (16, Horde); Returning the Lost Satchel (16, Horde); Searching for the Lost Satchel (16, Horde); Slaying the Beast (16, Horde); The Power to Destroy... (16, Horde); The Power to Destroy... (16, Horde).

**Wailing Caverns** (dungeon, 17-21): Hamuul Runetotem (16, Horde); Nara Wildmane (16, Horde); Deviate Hides (17); Serpentbloom (18, Horde); Smart Drinks (18); Trouble at the Docks (18); Deviate Eradication (21); Leaders of the Fang (22, Horde); The Glowing Shard (26).

**Deadmines** (dungeon, 18-20): Red Silk Bandanas (17, Alliance); Collecting Memories (18, Alliance); Oh Brother. . . (20, Alliance); Underground Assault (20, Alliance); The Defias Brotherhood (22, Alliance).

**Blackfathom Deeps** (dungeon, 22-27): Allegiance to the Old Gods (22, Horde); The Essence of Aku'Mai (22, Horde); Trouble in the Deeps (22, Horde); Knowledge in the Deeps (23, Alliance); In Search of Thaelrid (24, Alliance); Researching the Corruption (24, Alliance); Twilight Falls (25, Alliance); Allegiance to the Old Gods (26, Horde); Amongst the Ruins (27, Horde); Blackfathom Villainy (27, Alliance); Blackfathom Villainy (27, Horde).

**Shadowfang Keep** (dungeon, 25-27): Deathstalkers in Shadowfang (25, Horde); The Book of Ur (26, Horde); Arugal Must Die (27, Horde).

**Stormwind Stockade** (dungeon, 26-27): What Comes Around... (25, Alliance); Crime and Punishment (26, Alliance); Quell The Uprising (26, Alliance); The Color of Blood (26, Alliance); The Fury Runs Deep (27, Alliance); The Stockade Riots (29, Alliance).

**Razorfen Kraul** (dungeon, 26-30): Blueleaf Tubers (26); Going, Going, Guano! (26, Horde); A Vengeful Fate (27, Horde); The Crone of the Kraul (27, Alliance); Mortality Wanes (30, Alliance); Willix the Importer (30).

**Ashenvale (which dungeon or wing is not told)** (dungeon, 30-30): Baron Aquanis (30, Horde).

**Gnomeregan** (dungeon, 30-34): Save Techbot's Brain! (26, Alliance); Tinkmaster Overspark (26, Alliance); Gnogaine (27, Alliance); The Day After (27, Alliance); Castpipe's Task (28, Alliance); A Fine Mess (30); Data Rescue (30, Alliance); Essential Artificials (30, Alliance); Gyrodrillmatic Excavationators (30, Alliance); Klockmort's Essentials (30, Alliance); The Only Cure is More Green Glow (30, Alliance); The Sparklematic 5200! (30); The Sparklematic 5200! (30); The Sparklematic 5200! (30); The Sparklematic 5200! (30); The Sparklematic 5200! (30); The Sparklematic 5200! (30); Grime-Encrusted Ring (34); Return of the Ring (34, Alliance); Return of the Ring (34, Horde); Chief Engineer Scooty (35, Horde); Gnomer-gooooone! (35, Horde); Rig Wars (35, Horde); The Grand Betrayal (35, Alliance).

**Scarlet Monastery** (dungeon, 33-42): Hearts of Zeal (33, Horde); Vorrel's Revenge (33, Horde); Compendium of the Fallen (38, Horde); Mythology of the Titans (38, Alliance); In the Name of the Light (40, Alliance); Into The Scarlet Monastery (42, Horde); Into The Scarlet Monastery (42, Horde).

**Razorfen Downs** (dungeon, 36-37): A Host of Evil (35); An Unholy Alliance (36, Horde); An Unholy Alliance (36, Horde); An Unholy Alliance (36, Horde); An Unholy Alliance (36, Horde); Bring the End (37, Horde); Bring the Light (37, Alliance); Extinguishing the Idol (37); Scourge of the Downs (37).

**Uldaman** (dungeon, 41-44): A Sign of Hope (35, Alliance); Amulet of Secrets (40, Alliance); The Hidden Chamber (40, Alliance); The Lost Dwarves (40, Alliance); Lore for a Price (41, Alliance); Necklace Recovery (41, Horde); Necklace Recovery, Take 2 (41, Horde); The Shattered Necklace (41, Alliance); Back to Uldaman (42, Alliance); Translating the Journal (42); Translating the Journal (42, Horde); Find the Gems (43, Alliance); Deliver the Gems (44, Horde); Find the Gems and Power Source (44, Horde); Necklace Recovery, Take 3 (44, Horde); Restoring the Necklace (44, Alliance); Restoring the Necklace (44, Alliance); The Lost Tablets of Will (45, Alliance); The Platinum Discs (47); The Platinum Discs (47, Horde).

**Zul'Farrak** (dungeon, 45-47): Scarab Shells (45); The Spider God (45, Horde); Tran'rek (45); Troll Temper (45); Tabetha's Task (46); Tiara of the Deep (46); Divino-matic Rod (47); Nekrum's Medallion (47, Alliance); The Prophecy of Mosh'aru (47); Gahz'rilla (50).

**Thousand Needles (which dungeon or wing is not told)** (dungeon, 46-46): The Brassbolts Brothers (46, Alliance).

**Maraudon** (dungeon, 47-51): Shadowshard Fragments (42, Horde); Shadowshard Fragments (42, Alliance); Twisted Evils (47); Vyletongue Corruption (47, Horde); Vyletongue Corruption (47, Alliance); The Pariah's Instructions (48); Legends of Maraudon (49); The Scepter of Celebras (49); Corruption of Earth and Seed (51, Horde); Corruption of Earth and Seed (51, Alliance); Seed of Life (51).

**Sunken Temple** (dungeon, 51-53): Into The Temple of Atal'Hakkar (50, Alliance); The Temple of Atal'Hakkar (50, Horde); Into the Depths (51); Secret of the Circle (51); The Sunken Temple (51, Horde); Haze of Evil (52, Alliance); Zapper Fuel (52, Horde); Jammal'an the Prophet (53); The God Hakkar (53); The Essence of Eranikus (55); Eranikus, Tyrant of the Dream (60).

**Blackrock Mountain (which dungeon or wing is not told)** (dungeon, 52-52): Disharmony of Flame (52, Horde); Overmaster Pyron (52, Alliance).

**Un'Goro Crater (which dungeon or wing is not told)** (dungeon, 53-53): Bloodpetal Zapper (53, Horde).

**Blackrock Depths** (dungeon, 54-59): Commander Gor'shak (52, Horde); Dark Iron Legacy (52); Dark Iron Legacy (52); KILL ON SIGHT: Dark Iron Dwarves (52, Horde); Ribbly Screwspigot (53); Yuka Screwspigot (53); KILL ON SIGHT: High Ranking Dark Iron Officials (54, Horde); The Eastern Kingdoms (54, Horde); The Last Element (54, Horde); The Love Potion (54); What Is Going On? (54, Horde); What Is Going On? (54, Horde); Hurley Blackbreath (55, Alliance); Lost Thunderbrew Recipe (55, Horde); Ragnar Thunderbrew (55, Alliance); The Heart of the Mountain (55); The Spectral Chalice (55); Vivian Lagrave (55, Horde); Disharmony of Fire (56, Horde); Incendius! (56, Alliance); The Good Stuff (56, Alliance); A Taste of Flame (58); Grark Lorkrub (58, Horde); Operation: Death to Angerforge (58, Horde); Precarious Predicament (58, Horde); The Rise of the Machines (58, Horde); Kharan Mighthammer (59, Alliance); Kharan's Tale (59, Alliance); The Bearer of Bad News (59, Alliance); The Fate of the Kingdom (59, Alliance); The Princess's Surprise (59, Alliance); The Royal Rescue (59, Horde); The Challenge (60); The Princess Saved? (60, Horde); Three Kings of Flame (60).

**Tanaris (which dungeon or wing is not told)** (raid, 58-60): The Hand of Rastakhan (58); The Isle of Dread! (60); The Only Prescription (60).

**Ahn'Qiraj Temple** (raid, 60-60): Avenger's Breastplate (60); Avenger's Crown (60); Avenger's Greaves (60); Avenger's Legguards (60); Avenger's Pauldrons (60); Band of Unending Life (60); Band of Vaulted Secrets (60); Band of Veiled Shadows (60); Blade of Eternal Justice (60); Blade of Vaulted Secrets (60); C'Thun's Legacy (60); Cape of Eternal Justice (60); Cloak of Unending Life (60); Cloak of Veiled Shadows (60); Cloak of the Gathering Storm (60); Cloak of the Unseen Path (60); Conqueror's Breastplate (60); Conqueror's Crown (60); Conqueror's Greaves (60); Conqueror's Legguards (60); Conqueror's Spaulders (60); Dagger of Veiled Shadows (60); Deathdealer's Boots (60); Deathdealer's Helm (60); Deathdealer's Leggings (60); Deathdealer's Spaulders (60); Deathdealer's Vest (60); Doomcaller's Circlet (60); Doomcaller's Footwraps (60); Doomcaller's Mantle (60); Doomcaller's Robes (60); Doomcaller's Trousers (60); Drape of Vaulted Secrets (60); Enigma Boots (60); Enigma Circlet (60); Enigma Leggings (60); Enigma Robes (60); Enigma Shoulderpads (60); Footwraps of the Oracle (60); Gavel of Infinite Wisdom (60); Genesis Boots (60); Genesis Helm (60); Genesis Shoulderpads (60); Genesis Trousers (60); Genesis Vest (60); Hammer of the Gathering Storm (60); Kris of Unspoken Names (60); Mace of Unending Life (60); Mantle of the Oracle (60); Ring of Eternal Justice (60); Ring of Infinite Wisdom (60); Ring of Unspoken Names (60); Ring of the Gathering Storm (60); Scythe of the Unseen Path (60); Shroud of Infinite Wisdom (60); Shroud of Unspoken Names (60); Signet of the Unseen Path (60); Stormcaller's Diadem (60); Stormcaller's Footguards (60); Stormcaller's Hauberk (60); Stormcaller's Leggings (60); Stormcaller's Pauldrons (60); Striker's Diadem (60); Striker's Footguards (60); Striker's Hauberk (60); Striker's Leggings (60); Striker's Pauldrons (60); The Savior of Kalimdor (60); Tiara of the Oracle (60); Trousers of the Oracle (60); Vestments of the Oracle (60).

**Azshara (which dungeon or wing is not told)** (raid, 60-60): The Wrath of Neptulon (60).

**Blackrock Spire** (dungeon, 60-60): The Final Tablets (58); Bijou's Belongings (59, Horde); Bijou's Belongings (59, Alliance); Bijou's Reconnaissance Report (59); En-Ay-Es-Tee-Why (59); Kibler's Exotic Pets (59); Message to Maxwell (59); Operative Bijou (59, Horde); Put Her Down (59, Alliance); The Pack Mistress (59, Horde); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Alliance); An Earnest Proposition (60, Alliance); An Earnest Proposition (60, Alliance); An Earnest Proposition (60, Alliance); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Alliance); Anthion's Parting Words (60, Alliance); Blackhand's Command (60); Blood of the Black Dragon Champion (60, Horde); Delivery to Ridgewell (60, Alliance); Doomrigger's Clasp (60, Alliance); Egg Collection (60); Egg Freezing (60); Eye of the Emberseer (60); Final Preparations (60); Finkle Einhorn, At Your Service! (60); For The Horde! (60, Horde); General Drakkisath's Command (60, Alliance); General Drakkisath's Demise (60, Alliance); Just Compensation (60, Horde); Just Compensation (60, Horde); Just Compensation (60, Horde); Just Compensation (60, Horde); Just Compensation (60, Alliance); Just Compensation (60, Alliance); Just Compensation (60, Alliance); Just Compensation (60, Alliance); Maxwell's Mission (60, Alliance); Mayara Brightwing (60, Alliance); Mea Culpa, Lord Valthalak (60); Mother's Milk (60); Oculus Illusions (60, Horde); Seal of Ascension (60); Seal of Ascension (60); The Darkstone Tablet (60, Horde); The Demon Forge (60); The Left Piece of Lord Valthalak's Amulet (60); The Matron Protectorate (60); The Right Piece of Lord Valthalak's Amulet (60); Tinkee Steamboil (60); Urok Doomhowl (60); Vivian Lagrave and the Darkstone Tablet (60, Horde); Warlord's Command (60, Horde).

**Blackwing Lair** (raid, 60-60): Nefarius's Corruption (60); Only One May Rise (60).

**Burning Steppes (which dungeon or wing is not told)** (dungeon, 60-60): Leonid Barthalomew (60).

**Dire Maul** (dungeon, 60-60): Camp Mojache (57, Horde); Feathermoon Stronghold (57, Alliance); Lethtendris's Web (57, Alliance); Lethtendris's Web (57, Horde); Pusillin and the Elder Azj'Tordin (58); Anthion's Old Friend (60); Codex of Defense (60); Elven Legends (60, Horde); Elven Legends (60, Alliance); Falrin's Vendetta (60); Free Knot! (60); Frost Shock and You (60); Garona: A Study on Stealth and Treachery (60); Harnessing Shadows (60); Holy Bologna: What the Light Won't Tell You (60); The Arcanist's Cookbook (60); The Emerald Dream... (60); The Gordok Ogre Suit (60); The Gordok Taste Test (60, Alliance); The Greatest Race of Hunters (60); The Instigator's Enchantment (60); The Left Piece of Lord Valthalak's Amulet (60); The Light and How To Swing It (60); The Madness Within (60); The Right Piece of Lord Valthalak's Amulet (60); The Treasure of the Shen'dralar (60, Alliance); The Treasure of the Shen'dralar (60, Horde); Unfinished Gordok Business (60, Alliance); Unfinished Gordok Business (60, Alliance).

**Dustwallow Marsh (which dungeon or wing is not told)** (raid, 60-60): The Test of Skulls, Axtroz (60, Horde); The Test of Skulls, Chronalis (60, Horde); The Test of Skulls, Scryer (60, Horde); The Test of Skulls, Somnus (60, Horde).

**Eastern Plaguelands (which dungeon or wing is not told)** (dungeon, 60-60): The Restless Souls (60).

**Eastern Plaguelands (which dungeon or wing is not told)** (raid, 60-60): Order Must Be Restored (60, Alliance); The Battle of Darrowshire (60).

**Molten Core** (raid, 60-60): Attunement to the Core (60, Horde); Attunement to the Core (60, Alliance); Hands of the Enemy (60); Scrying Goggles? No Problem! (60); The Molten Core (60).

**Moonglade (which dungeon or wing is not told)** (dungeon, 60-60): Shards of the Felvine (60).

**Moonglade (which dungeon or wing is not told)** (raid, 60-60): The Nightmare Manifests (60); The Nightmare's Corruption (60).

**Naxxramas** (raid, 60-60): Echoes of War (60); Glacial Cloak (60); Glacial Gloves (60); Glacial Vest (60); Glacial Wrists (60); Icebane Bracers (60); Icebane Breastplate (60); Icebane Gauntlets (60); Icy Scale Bracers (60); Icy Scale Breastplate (60); Icy Scale Gauntlets (60); Omarion's Handbook (60); Polar Bracers (60); Polar Gloves (60); Polar Tunic (60); Redemption Headpiece (60); The Fall of Kel'Thuzad (60).

**Ruins of Ahn'Qiraj** (raid, 60-60): The Fall of Ossirian (60).

**Scholomance** (dungeon, 60-60): Plagued Hatchlings (58); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Alliance); Barov Family Fortune (60, Horde); Barov Family Fortune (60, Alliance); Betina Bigglezink (60); Dawn's Gambit (60); Doctor Theolen Krastinov, the Butcher (60); Just Compensation (60, Horde); Just Compensation (60, Alliance); Kirtonos the Herald (60); Krastinov's Bag of Horrors (60); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Horde); Saving the Best for Last (60, Alliance); Saving the Best for Last (60, Alliance); Saving the Best for Last (60, Alliance); Saving the Best for Last (60, Alliance); Saving the Best for Last (60, Alliance); Saving the Best for Last (60, Alliance); Saving the Best for Last (60, Alliance); Saving the Best for Last (60, Alliance); Saving the Best for Last (60, Alliance); The Left Piece of Lord Valthalak's Amulet (60); The Lich, Ras Frostwhisper (60); The Right Piece of Lord Valthalak's Amulet (60).

**Silithus (which dungeon or wing is not told)** (raid, 60-60): Armaments of War (60); Armaments of War (60); Armaments of War (60); Armaments of War (60); Armaments of War (60); Armaments of War (60); Armaments of War (60); Armaments of War (60); Bor Wishes to Speak (60); Scepter of the Council (60).

**Stratholme** (dungeon, 60-60): Above and Beyond (60); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Horde); An Earnest Proposition (60, Alliance); An Earnest Proposition (60, Alliance); An Earnest Proposition (60, Alliance); An Earnest Proposition (60, Alliance); An Earnest Proposition (60, Alliance); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Horde); Anthion's Parting Words (60, Alliance); Anthion's Parting Words (60, Alliance); Anthion's Parting Words (60, Alliance); Anthion's Parting Words (60, Alliance); Anthion's Parting Words (60, Alliance); Anthion's Parting Words (60, Alliance); Aurius' Reckoning (60); Dead Man's Plea (60); Houses of the Holy (60); Just Compensation (60, Horde); Just Compensation (60, Horde); Just Compensation (60, Horde); Just Compensation (60, Horde); Just Compensation (60, Alliance); Just Compensation (60, Alliance); Just Compensation (60, Alliance); Just Compensation (60, Alliance); Menethil's Gift (60); Of Love and Family (60); Ramstein (60, Horde); The Active Agent (60); The Archivist (60); The Flesh Does Not Lie (60); The Great Fras Siabi (60); The Left Piece of Lord Valthalak's Amulet (60); The Medallion of Faith (60); The Restless Souls (60); The Right Piece of Lord Valthalak's Amulet (60); The Truth Comes Crashing Down (60).

**Zul'Gurub** (raid, 60-60): Confront Yeh'kinya (58); A Collection of Heads (60); Enchanted South Seas Kelp (60); Enchanted South Seas Kelp (60); Enchanted South Seas Kelp (60); Kezan's Taint (60); Kezan's Taint (60); Kezan's Taint (60); Kezan's Unstoppable Taint (60); Maelstrom's Wrath (60); Nat's Measuring Tape (60); Paragons of Power: The Augur's Belt (60); Paragons of Power: The Augur's Bracers (60); Paragons of Power: The Augur's Hauberk (60); Paragons of Power: The Confessor's Bindings (60); Paragons of Power: The Confessor's Mantle (60); Paragons of Power: The Confessor's Wraps (60); Paragons of Power: The Demoniac's Mantle (60); Paragons of Power: The Demoniac's Robes (60); Paragons of Power: The Demoniac's Wraps (60); Paragons of Power: The Freethinker's Armguards (60); Paragons of Power: The Freethinker's Belt (60); Paragons of Power: The Freethinker's Breastplate (60); Paragons of Power: The Haruspex's Belt (60); Paragons of Power: The Haruspex's Bracers (60); Paragons of Power: The Haruspex's Tunic (60); Paragons of Power: The Illusionist's Mantle (60); Paragons of Power: The Illusionist's Robes (60); Paragons of Power: The Illusionist's Wraps (60); Paragons of Power: The Madcap's Bracers (60); Paragons of Power: The Madcap's Mantle (60); Paragons of Power: The Madcap's Tunic (60); Paragons of Power: The Predator's Belt (60); Paragons of Power: The Predator's Bracers (60); Paragons of Power: The Predator's Mantle (60); Paragons of Power: The Vindicator's Armguards (60); Paragons of Power: The Vindicator's Belt (60); Paragons of Power: The Vindicator's Breastplate (60); Pristine Enchanted South Seas Kelp (60); Strength of Mount Mugamba (60); Strength of Mount Mugamba (60); Strength of Mount Mugamba (60); The All-Seeing Eye of Zuldazar (60); The Eye of Zuldazar (60); The Eye of Zuldazar (60); The Eye of Zuldazar (60); The Heathen's Brand (60); The Heathen's Brand (60); The Heathen's Brand (60); The Hero's Brand (60); The Jewel of Kajaro (60); The Maelstrom's Tendril (60); The Maelstrom's Tendril (60); The Maelstrom's Tendril (60); The Pebble of Kajaro (60); The Pebble of Kajaro (60); The Pebble of Kajaro (60); The Perfect Poison (60); The Rage of Mugamba (60); The Unmarred Vision of Voodress (60); Vision of Voodress (60); Vision of Voodress (60); Vision of Voodress (60); Zandalarian Shadow Mastery Talisman (60); Zandalarian Shadow Talisman (60); Zandalarian Shadow Talisman (60); Zandalarian Shadow Talisman (60).

**Hellfire Peninsula (which dungeon or wing is not told)** (dungeon, 62-62): Dark Tidings (62, Alliance); Dark Tidings (62, Horde).

**Hellfire Citadel: The Shattered Halls** (dungeon, 63-70): Weaken the Ramparts (62, Horde); Weaken the Ramparts (62, Alliance); Heart of Rage (63, Alliance); Heart of Rage (63, Horde); The Blood is Life (63, Alliance); The Blood is Life (63, Horde); Fel Embers (70, Alliance); Imprisoned in the Citadel (70, Alliance); Imprisoned in the Citadel (70, Horde); Pride of the Fel Horde (70, Alliance); Pride of the Fel Horde (70, Horde); Tear of the Earthmother (70); The Will of the Warchief (70, Horde); Turning the Tide (70, Alliance).

**Coilfang: The Underbog** (dungeon, 65-65): Bring Me A Shrubbery! (65); Lost in Action (65); Oh, It's On! (65); Stalk the Stalker (65).

**Auchindoun: Mana-Tombs** (dungeon, 66-70): Safety Is Job One (66); Someone Else's Hard Work Pays Off (66); Undercutting the Competition (66); Stasis Chambers of the Mana-Tombs (70).

**Auchindoun: Auchenai Crypts** (dungeon, 67-68): Everything Will Be Alright (67); Auchindoun... (68, Horde).

**The Escape From Durnholde** (dungeon, 68-68): Nice Hat... (68).

**Caverns of Time (which dungeon or wing is not told)** (dungeon, 68-70): Escape from Durnholde (68); Old Hillsbrad (68); Taretha's Diversion (68); The Black Morass (70); The Master's Touch (70); The Opening of the Dark Portal (70).

**Auchindoun: Sethekk Halls** (dungeon, 69-69): Brother Against Brother (69); Terokk's Legacy (69).

**Shadowmoon Valley (which dungeon or wing is not told)** (dungeon, 69-69): Fresh from the Mechanar (69).

**Auchindoun (which dungeon or wing is not told)** (dungeon, 70-70): Find Spy To'gun (70); The Codex of Blood (70); The Soul Devices (70); Trouble at Auchindoun (70).

**Auchindoun: Shadow Labyrinth** (dungeon, 70-70): The Lexicon Demonica (69); Entry Into Karazhan (70); Into the Heart of the Labyrinth (70); The Book of Fel Names (70).

**Black Temple** (raid, 70-70): Redemption of the Ashtongue (70); The Fall of the Betrayer (70).

**Coilfang: Serpentshrine Cavern** (raid, 70-70): The Vials of Eternity (70).

**Coilfang: The Slave Pens** (dungeon, 70-70): The Cudgel of Kar'desh (70); The Mark of Vashj (70).

**Coilfang: The Steamvault** (dungeon, 70-70): Orders from Lady Vashj (70); Preparing for War (70); The Warlord's Hideout (70); Underworld Loam (70).

**Karazhan** (raid, 70-70): A Demonic Presence (70); Assessing the Situation (70); Chamber of Secrets (70); Kalynna's Request (70); Keanna's Log (70); Medivh's Journal (70); Nightbane (70); The Master's Terrace (70); The Shade of Aran (70).

**Magister's Terrace** (dungeon, 70-70): Hard to Kill (70); Magisters' Terrace (70); The Scryer's Scryer (70).

**Magtheridon's Lair** (raid, 70-70): Trial of the Naaru: Magtheridon (70).

**Shattrath City (which dungeon or wing is not told)** (dungeon, 70-70): The Second and Third Fragments (70); Trial of the Naaru: Mercy (70); Trial of the Naaru: Strength (70); Trial of the Naaru: Tenacity (70).

**Tempest Keep** (raid, 70-70): Harbinger of Doom (70); How to Break Into the Arcatraz (70); Kael'thas and the Verdant Sphere (70); Revered Among the Sha'tar (70); Ruse of the Ashtongue (70); Seer Udalo (70).

**Tempest Keep: The Botanica** (dungeon, 70-70): Capturing the Keystone (70).

**The Battle for Mount Hyjal** (raid, 70-70): An Artifact From the Past (70).

**The Black Temple (which dungeon or wing is not told)** (raid, 70-70): Seek Out the Ashtongue (70).

**Zul'Aman** (raid, 70-70): A Troll Among Trolls (70); Blood of the Warlord (70); Hex Lord? Hah! (70); Oooh, Shinies! (70); Playin' With Dolls (70); Promises, Promises... (70); Tuskin' Raiders (70); X Marks... Your Doom! (70).

**The Nexus** (dungeon, 71-71): Have They No Shame? (71, Alliance); Have They No Shame? (71, Horde); Postponing the Inevitable (71); Prisoner of War (71); Quickening (71).

**Utgarde Keep** (dungeon, 71-71): A Score to Settle (71, Horde); Disarmament (71, Alliance); Disarmament (71, Horde); Ingvar Must Die! (71, Horde); Into Utgarde! (71, Alliance).

**Azjol-Nerub** (dungeon, 74-74): Death to the Traitor King (74); Don't Forget the Eggs! (74).

**Drak'Tharon Keep** (dungeon, 74-75): Head Games (74); Search and Rescue (74); Cleansing Drak'Tharon (75).

**Ahn'kahet: The Old Kingdom** (dungeon, 76-76): Funky Fungi (76); The Faceless Ones (76).

**Violet Hold** (dungeon, 77-77): Containment (77); Discretion is Key (77).

**Gundrak** (dungeon, 78-78): For Posterity (78); Gal'darah Must Pay (78); One of a Kind (78).

**Halls of Stone** (dungeon, 78-78): Halls of Stone (78).

**Halls of Lightning** (dungeon, 80-80): Diametrically Opposed (80); Whatever it Takes! (80).

**Halls of Reflection** (dungeon, 80-80): Frostmourne (80, Alliance); Frostmourne (80, Horde); The Halls Of Reflection (80, Alliance); The Halls Of Reflection (80, Horde); Wrath of the Lich King (80, Alliance); Wrath of the Lich King (80, Horde).

**Icecrown (which dungeon or wing is not told)** (dungeon, 80-80): Tempering The Blade (80, Alliance); Tempering The Blade (80, Horde).

**Icecrown Citadel** (raid, 80-80): A Feast of Souls (80); Blood Infusion (80); Choose Your Path (80); Empowerment (80); Frost Infusion (80); Jaina's Locket (80); Mograine's Reunion (80); Muradin's Lament (80); Path of Courage (80); Path of Courage (80); Path of Courage (80); Path of Destruction (80); Path of Destruction (80); Path of Destruction (80); Path of Might (80); Path of Might (80); Path of Might (80); Path of Vengeance (80); Path of Vengeance (80); Path of Vengeance (80); Path of Wisdom (80); Path of Wisdom (80); Path of Wisdom (80); Personal Property (80); Shadow's Edge (80); Shadowmourne... (80); Sylvanas' Vengeance (80); The Lich King's Last Stand (80); The Lightbringer's Redemption (80); The Sacred and the Corrupt (80); The Splintered Throne (80); Unholy Infusion (80).

**Pit of Saron** (dungeon, 80-80): Deliverance from the Pit (80, Alliance); Deliverance from the Pit (80, Horde); Reforging The Sword (80, Alliance); Reforging The Sword (80, Horde); The Path to the Citadel (80, Alliance); The Path to the Citadel (80, Horde); The Pit of Saron (80, Horde); The Pit of Saron (80, Alliance).

**The Culling of Stratholme** (dungeon, 80-80): A Royal Escort (80); Dispelling Illusions (80).

**The Eye of Eternity** (raid, 80-80): Heroic Judgment at the Eye of Eternity (80); Judgment at the Eye of Eternity (80).

**The Forge of Souls** (dungeon, 80-80): Echoes of Tortured Souls (80, Alliance); Echoes of Tortured Souls (80, Horde); Inside the Frozen Citadel (80, Horde); Inside the Frozen Citadel (80, Alliance).

**The Oculus** (dungeon, 80-80): A Unified Front (80); A Wing and a Prayer (80); Mage-Lord Urom (80); The Struggle Persists (80).

**The Sunwell** (raid, 80-80): The Purification of Quel'Delar (80, Alliance); The Purification of Quel'Delar (80, Horde); The Purification of Quel'Delar (80, Horde); The Purification of Quel'Delar (80, Alliance); The Purification of Quel'Delar (80, Horde); The Purification of Quel'Delar (80, Horde).

**Ulduar** (raid, 80-80): Algalon (80); All Is Well That Ends Well (80); Ancient History (80); Archivum Data Disc (80); Freya's Sigil (80); Heroic: Algalon (80); Heroic: All Is Well That Ends Well (80); Heroic: Archivum Data Disc (80); Heroic: Freya's Sigil (80); Heroic: Hodir's Sigil (80); Heroic: Mimiron's Sigil (80); Heroic: The Celestial Planetarium (80); Heroic: Thorim's Sigil (80); Hodir's Sigil (80); Mimiron's Sigil (80); The Celestial Planetarium (80); Thorim's Sigil (80); Val'anyr, Hammer of Ancient Kings (80).

**Utgarde Pinnacle** (dungeon, 80-80): Junk in My Trunk (80); Vengeance Be Mine! (80).

**The Ruby Sanctum** (raid, 82-82): Assault on the Sanctum (82); The Twilight Destroyer (82); Trouble at Wyrmrest (82).

## Zone by zone

### Eversong Woods (5-9, Eastern Kingdoms)

Going from level 5 to 10 costs 22,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 73 | 0 | 49,114 | 215% | 23 | 1:2 2:6 3:3 4:5 5:8 6:10 7:6 8:3 9:12 10:14 11:3 20:1 | nothing; it carries itself |

**Enemies no quest uses:** Crazed Dragonhawk (7-8, 125 of them), Wretched Urchin (4-5, 67 of them), Feral Dragonhawk Hatchling (5-6, 50 of them), Eversong Green Keeper (7-9, 37 of them), Angershade (7-8, 19 of them), Amani Shadowpriest (8-10, 19 of them), Rotlimb Cannibal (6-7, 13 of them), Golden Crawler (5-6, 12 of them), Eversong Tender (5-7, 7 of them), Duskwither Apprentice (10, 4 of them).

**Places nothing is asked of you:** The Living Wood (41 enemies).

### Azuremyst Isle (5-10, Kalimdor)

Going from level 5 to 11 costs 30,400 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 65 | 1 | 39,996 | 132% | 23 | 1:4 2:5 3:2 4:4 5:6 6:4 7:4 8:9 9:5 10:15 11:5 12:1 20:1 | nothing; it carries itself |

**Enemies no quest uses:** Root Thresher (7-8, 97 of them), Nightstalker (8-9, 81 of them), Mutated Owlkin (3-4, 37 of them), Timberstrider (6-8, 32 of them), Greater Timberstrider (7-9, 31 of them), Timberstrider Fledgling (5-6, 29 of them), Flaming Aspect (13-15, 18 of them), Nestlewood Owlkin (3-4, 17 of them), Raving Owlbeast (9-10, 16 of them), Bristlelimb Windcaller (7-9, 16 of them), and 7 more.

**Places nothing is asked of you:** Nestlewood Hills (39 enemies), Emberglade (20 enemies), The Sacred Grove (18 enemies), Nestlewood Thicket (15 enemies), Wildwind Peak (12 enemies), Geezle's Camp (11 enemies).

### Dun Morogh (5-10, Eastern Kingdoms)

Going from level 5 to 11 costs 30,400 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 15 | 12 | 464 | 2% | 6 | 1:1 20:2 30:7 34:2 35:3 | only passing through |
| Alliance | 66 | 20 | 48,558 | 160% | 26 | 1:3 2:1 3:3 4:2 5:7 6:3 7:4 8:2 9:4 10:7 11:3 12:2 20:2 22:1 23:1 26:2 27:2 28:1 30:12 34:2 35:1 75:1 | nothing; it carries itself |

**Enemies no quest uses:** Winter Wolf (7-8, 23 of them), Rockjaw Ambusher (9-10, 18 of them), Juvenile Snow Leopard (5-6, 15 of them), Snow Tracker Wolf (6-7, 12 of them), Dark Iron Spy (9-10, 12 of them), Frostmane Hideskinner (9-10, 11 of them), Rockjaw Raider (3-4, 10 of them), Frostmane Shadowcaster (9-10, 8 of them), Frostmane Novice (3-4, 8 of them), Frostmane Troll (7-8, 7 of them), and 3 more.

### Elwynn Forest (5-10, Eastern Kingdoms)

Going from level 5 to 11 costs 30,400 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 53 | 1 | 34,221 | 113% | 23 | 1:2 2:3 3:1 4:4 5:4 6:7 7:7 8:1 9:5 10:17 11:1 20:1 | nothing; it carries itself |

**Enemies no quest uses:** Mangy Wolf (5-6, 85 of them), Gray Forest Wolf (7-8, 55 of them), Defias Cutpurse (5-6, 32 of them), Forest Spider (5-6, 15 of them), Thuros Lightfingers (11, 8 of them), Mine Spider (8-9, 8 of them), Gruff Swiftbite (12, 5 of them), Fedfennel (12, 4 of them).

### Mulgore (5-10, Kalimdor)

Going from level 5 to 11 costs 30,400 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 46 | 0 | 39,558 | 130% | 21 | 1:1 2:2 3:3 4:3 5:4 6:4 7:5 8:8 9:2 10:8 11:1 12:3 14:1 20:1 | nothing; it carries itself |
| Alliance | 3 | 0 | 2,392 | 8% | 1 | 8:1 12:1 20:1 | only passing through |

**Enemies no quest uses:** Windfury Sorceress (9-10, 47 of them), Windfury Matriarch (10-11, 46 of them), Venture Co. Laborer (6-7, 14 of them), Venture Co. Hireling (5-6, 11 of them), Kodo Calf (7-8, 8 of them), Venture Co. Taskmaster (7-8, 7 of them), Redrock Earth Spirit (1, 5 of them), Kodo Matriarch (11-12, 4 of them), "Squealer" Thornmantle (5, 4 of them).

### Teldrassil (5-10, Kalimdor)

Going from level 5 to 11 costs 30,400 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 3 | 0 | 4,379 | 14% | 1 | 10:1 12:1 60:1 | only passing through |
| Alliance | 56 | 0 | 73,777 | 243% | 23 | 1:2 2:1 3:2 4:5 5:8 6:4 7:5 8:4 9:5 10:7 11:4 12:4 47:1 55:1 56:1 57:1 60:1 | nothing; it carries itself |

**Enemies no quest uses:** Gnarlpine Defender (7-8, 51 of them), Gnarlpine Warrior (6-7, 34 of them), Gnarlpine Augur (8-9, 25 of them), Gnarlpine Pathfinder (9-10, 14 of them), Gnarlpine Gardener (5-6, 13 of them), Vicious Grell (7, 11 of them), Gnarlpine Avenger (9-10, 11 of them), Shadow Sprite (5-6, 10 of them), Gnarlpine Ursa (5-6, 10 of them), Rascal Sprite (5-6, 8 of them), and 3 more.

**Places nothing is asked of you:** Starbreeze Village (25 enemies).

### Tirisfal Glades (5-10, Eastern Kingdoms)

Going from level 5 to 11 costs 30,400 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 51 | 0 | 44,341 | 146% | 21 | 1:2 2:2 3:2 4:2 5:6 6:3 7:4 8:7 9:7 10:6 11:4 12:3 13:1 20:1 55:1 | nothing; it carries itself |

**Enemies no quest uses:** Hungering Dead (7-8, 10 of them), Tirisfal Farmer (6-7, 9 of them), Scarlet Vanguard (10-11, 9 of them), Tirisfal Farmhand (5-6, 8 of them), Scarlet Neophyte (10-11, 5 of them), Shambling Horror (8-9, 4 of them), Lost Soul (6-7, 4 of them).

**Places nothing is asked of you:** Terrace of Repose (13 enemies), Gunther's Retreat (12 enemies), Whispering Gardens (11 enemies).

### Durotar (5-11, Kalimdor)

Going from level 5 to 12 costs 39,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 47 | 0 | 40,195 | 103% | 23 | 1:3 2:1 3:2 4:3 5:5 7:7 8:5 9:6 10:2 11:3 12:7 13:1 14:1 20:1 | nothing; it carries itself |
| Alliance | 3 | 0 | 2,722 | 7% | 1 | 9:1 11:1 20:1 | only passing through |

**Enemies no quest uses:** Elder Mottled Boar (8-9, 120 of them), Dire Mottled Boar (6-7, 114 of them), Clattering Scorpid (5-6, 71 of them), Armored Scorpid (7-8, 62 of them), Bloodtalon Taillasher (6-8, 58 of them), Venomtail Scorpid (9-10, 42 of them), Bloodtalon Scythemaw (8-10, 36 of them), Felstalker (3-4, 16 of them), Dustwind Harpy (7-8, 16 of them), Dustwind Pillager (7-8, 12 of them), and 4 more.

**Places nothing is asked of you:** Razorwind Canyon (23 enemies).

### Silverpine Forest (12-17, Eastern Kingdoms)

Going from level 12 to 18 costs 78,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 44 | 0 | 55,394 | 71% | 15 | 10:5 11:5 12:4 14:6 15:5 16:8 17:3 18:3 20:3 21:1 22:1 | nothing; it carries itself |
| Alliance | 4 | 0 | 5,625 | 7% | 1 | 14:1 17:1 18:2 | only passing through |

**Enemies no quest uses:** Moonrage Bloodhowler (15-16, 33 of them), Ravenclaw Slave (11-12, 26 of them), Reef Frenzy (14-15, 16 of them), Moonrage Darkrunner (11-12, 14 of them), Rot Hide Gladerunner (11-12, 11 of them), Ravenclaw Raider (12-13, 11 of them), Ravenclaw Servant (13-14, 9 of them), Rot Hide Mystic (12-13, 8 of them), Ravenclaw Champion (14-15, 8 of them), Corrupt Water Spirit (19-20, 8 of them), and 1 more.

**Places nothing is asked of you:** Olsen's Farthing (21 enemies), The Decrepit Ferry (11 enemies).

### Ghostlands (12-18, Eastern Kingdoms)

Going from level 12 to 19 costs 95,900 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 63 | 4 | 113,480 | 118% | 22 | 10:11 11:4 12:6 13:2 14:6 15:5 16:7 17:4 18:4 19:2 20:7 21:4 70:1 | nothing; it carries itself |

**Enemies no quest uses:** Deatholme Necromancer (18-19, 19 of them), Arcane Reaver (15-16, 12 of them), Risen Creeper (9-10, 11 of them), Stonewing Tracker (16-17, 10 of them), Gangled Flesheater (10-11, 9 of them), Deatholme Darkmage (17-18, 8 of them), Phantasmal Watcher (12-14, 5 of them), Catlord Corpse (18, 5 of them), Rotting Crawler (12-13, 4 of them), Dr. Whitherlimb (18, 4 of them).

**Places nothing is asked of you:** Hatchet Hills (35 enemies), Dawnstar Spire (11 enemies), Amani Pass (10 enemies).

### Westfall (12-18, Eastern Kingdoms)

Going from level 12 to 19 costs 95,900 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 7 | 0 | 10,560 | 11% | 3 | 16:5 19:1 20:1 | only passing through |
| Alliance | 36 | 1 | 90,270 | 94% | 13 | 9:1 10:5 12:4 13:2 14:3 15:2 16:5 17:1 18:6 19:1 20:1 44:5 | nothing; it carries itself |

**Enemies no quest uses:** Coyote (10-11, 43 of them), Shore Crawler (17-18, 22 of them), Coyote Packleader (11-12, 21 of them), Sand Crawler (13-14, 17 of them), Sea Crawler (15-16, 12 of them), Kobold Digger (12-13, 7 of them), Dust Devil (18-19, 7 of them).

### Bloodmyst Isle (14-18, Kalimdor)

Going from level 14 to 19 costs 75,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 74 | 8 | 97,648 | 130% | 26 | 10:4 11:2 12:3 13:4 14:7 15:6 16:10 17:6 18:20 19:5 20:6 21:1 | nothing; it carries itself |

**Enemies no quest uses:** Brown Bear (9-10, 24 of them), Wrathscale Shorestalker (12-13, 21 of them), Wrathscale Screamer (12-13, 20 of them), Bristlelimb Shaman (10-11, 18 of them), Bristlelimb Warrior (10-11, 17 of them), Nazzivus Summoner (13, 16 of them), Elekk (10, 4 of them).

**Places nothing is asked of you:** Bristlelimb Enclave (32 enemies).

### Darkshore (14-20, Kalimdor)

Going from level 14 to 21 costs 115,200 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 11 | 0 | 20,535 | 18% | 5 | 12:1 14:2 15:1 16:1 17:2 18:1 20:2 22:1 | only passing through |
| Alliance | 73 | 1 | 117,201 | 102% | 23 | 1:1 10:4 12:5 13:5 14:10 15:5 16:5 17:7 18:7 19:3 20:12 21:1 22:2 24:2 28:2 31:2 | nothing; it carries itself |

**Enemies no quest uses:** Foreststrider Fledgling (11-13, 98 of them), Thistle Bear (11-12, 81 of them), Foreststrider (14-16, 48 of them), Coastal Frenzy (14-16, 46 of them), Stormscale Myrmidon (18-19, 34 of them), Blackwood Warrior (16-17, 27 of them), Moonkin (12-13, 26 of them), Stormscale Sorceress (19-20, 23 of them), Raging Moonkin (12-14, 22 of them), Stormscale Warrior (20-21, 19 of them), and 8 more.

**Places nothing is asked of you:** Ruins of Mathystra (77 enemies).

### The Barrens (14-21, Kalimdor)

Going from level 14 to 22 costs 137,600 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 94 | 5 | 163,715 | 119% | 29 | 10:10 11:1 12:2 13:3 14:10 15:9 16:8 17:1 18:9 19:4 20:10 21:3 22:2 23:1 24:4 25:5 26:4 27:3 29:2 30:2 33:1 | nothing; it carries itself |
| Alliance | 22 | 4 | 30,712 | 22% | 10 | 14:7 16:3 18:4 20:1 21:1 24:1 25:1 26:1 27:1 30:2 | only passing through |

**Enemies no quest uses:** Hecklefang Hyena (15-16, 108 of them), Barrens Giraffe (15-16, 75 of them), Venture Co. Drudger (14-15, 51 of them), Wandering Barrens Giraffe (18-19, 37 of them), Hecklefang Stalker (22-23, 37 of them), Hecklefang Snarler (18-19, 28 of them), Lost Barrens Kodo (14-15, 27 of them), Slimeshell Makrura (18-19, 26 of them), Burning Blade Bruiser (10-11, 26 of them), Dreadmaw Crocolisk (9-11, 24 of them), and 11 more.

**Places nothing is asked of you:** Razorfen Downs (90 enemies), Ratchet (48 enemies), Fray Island (21 enemies).

### Darnassus (14-55, Kalimdor)

Going from level 14 to 56 costs 2,663,000 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 21 | 0 | 77,615 | 3% | 3 | 10:2 11:1 12:2 14:1 20:1 50:4 53:1 54:1 55:3 60:4 73:1 | not a levelling zone |

**Enemies no quest uses:** none of note.

### Loch Modan (15-18, Eastern Kingdoms)

Going from level 15 to 19 costs 62,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 41 | 1 | 72,067 | 115% | 16 | 10:4 11:2 12:3 14:1 15:12 16:1 17:2 18:8 19:1 20:3 22:1 39:1 43:1 44:1 | nothing; it carries itself |

**Enemies no quest uses:** Loch Frenzy (12-13, 26 of them), Young Threshadon (19-20, 21 of them), Dark Iron Insurgent (18-19, 7 of them), Lord Condar (15-16, 4 of them).

### Redridge Mountains (15-22, Eastern Kingdoms)

Going from level 15 to 23 costs 149,300 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 37 | 2 | 63,494 | 43% | 14 | 14:4 15:5 16:2 17:1 18:7 20:3 21:4 22:1 23:3 24:1 25:3 26:3 | 21 quests, about 41,016 experience, 7 with gear |

**Enemies no quest uses:** Redridge Thrasher (14-15, 12 of them), Servant of Ilgalar (24-25, 11 of them), Redridge Alpha (19-20, 7 of them).

### Stonetalon Mountains (21-26, Kalimdor)

Going from level 21 to 27 costs 158,500 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 31 | 0 | 72,404 | 46% | 8 | 17:2 18:6 19:3 20:1 21:3 22:2 23:2 25:3 26:3 27:2 28:2 29:2 | 14 quests, about 38,546 experience, 5 with gear |
| Alliance | 25 | 0 | 62,617 | 40% | 5 | 17:1 21:9 22:6 26:2 27:4 28:3 | 18 quests, about 48,333 experience, 6 with gear |

**Enemies no quest uses:** Windshear Digger (21-22, 46 of them), Deepmoss Webspinner (19-20, 45 of them), Raging Cliff Stormer (18-19, 29 of them), Bloodfury Windcaller (24-25, 25 of them), Cliff Stormer (15-17, 17 of them), Grimtotem Sorcerer (15-16, 16 of them), Bloodfury Storm Witch (26-27, 15 of them), Pridewing Wyvern (21-22, 14 of them), Pridewing Consort (22-23, 14 of them), Pridewing Skyhunter (23-24, 10 of them), and 4 more.

**Places nothing is asked of you:** Windshear Mine (53 enemies).

### Thunder Bluff (23-50, Kalimdor)

Going from level 23 to 51 costs 1,816,100 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 32 | 1 | 109,491 | 6% | 9 | 8:1 10:1 11:1 15:1 18:2 20:1 23:3 25:1 29:2 30:2 37:1 47:1 50:8 55:2 60:4 73:1 | not a levelling zone |

**Enemies no quest uses:** none of note.

### Ashenvale (24-30, Kalimdor)

Going from level 24 to 31 costs 227,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 29 | 2 | 77,047 | 34% | 12 | 19:2 20:2 21:1 23:1 24:6 25:1 26:2 27:4 30:6 32:1 34:1 44:1 70:1 | 25 quests, about 82,413 experience, 9 with gear |
| Alliance | 59 | 1 | 145,678 | 64% | 15 | 18:1 19:3 20:5 21:6 22:1 23:1 24:3 25:5 27:2 28:7 29:2 30:15 32:5 34:1 44:1 70:1 | 5 quests, about 13,782 experience, 2 with gear |

**Enemies no quest uses:** Wildthorn Stalker (20-21, 109 of them), Ashenvale Bear (21-22, 91 of them), Wild Buck (18-19, 81 of them), Elder Shadowhorn Stag (26-27, 78 of them), Shadowhorn Stag (22-23, 70 of them), Ghostpaw Runner (19-20, 68 of them), Elder Ashenvale Bear (25-26, 42 of them), Spined Crawler (20-21, 36 of them), Ghostpaw Howler (23-24, 34 of them), Clattering Crawler (19-20, 32 of them), and 17 more.

**Places nothing is asked of you:** The Ruins of Stardust (43 enemies), Falfarren River (42 enemies), Iris Lake (35 enemies), Fire Scar Shrine (32 enemies), The Ruins of Ordil'Aran (22 enemies), Bough Shadow (21 enemies), The Howling Vale (13 enemies), Forest Song (11 enemies).

### Wetlands (24-30, Eastern Kingdoms)

Going from level 24 to 31 costs 227,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 40 | 0 | 98,114 | 43% | 13 | 21:5 22:2 23:1 24:4 25:5 26:7 27:2 28:1 29:2 30:5 31:5 35:1 | 19 quests, about 61,346 experience, 7 with gear |

**Enemies no quest uses:** Wetlands Crocolisk (23-24, 52 of them), Bluegill Raider (28-29, 42 of them), Highland Razormaw (27-28, 37 of them), Leech Stalker (21-22, 35 of them), Crimson Whelp (25-26, 26 of them), Bluegill Muckdweller (23-24, 23 of them), Mirelow (25, 22 of them), Highland Raptor (23-24, 22 of them), Elder Razormaw (29, 22 of them), Lost Whelp (24-25, 21 of them), and 10 more.

**Places nothing is asked of you:** Thelgen Rock (53 enemies), Dragonmaw Gates (49 enemies), Raptor Ridge (47 enemies).

### Duskwood (25-30, Eastern Kingdoms)

Going from level 25 to 31 costs 200,600 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 4 | 0 | 15,840 | 8% | 1 | 42:4 | only passing through |
| Alliance | 73 | 1 | 134,561 | 67% | 16 | 20:5 21:2 23:1 24:8 25:14 26:2 27:1 28:14 29:8 30:13 31:2 32:2 45:1 | 2 quests, about 5,859 experience, 1 with gear |

**Enemies no quest uses:** Commander Felstrom (32, 10 of them), Fenros (32, 8 of them), Lord Malathrom (31, 7 of them).

**Places nothing is asked of you:** Raven Hill (13 enemies).

### Hillsbrad Foothills (27-34, Eastern Kingdoms)

Going from level 27 to 35 costs 326,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 43 | 0 | 180,427 | 55% | 11 | 20:2 22:4 24:3 25:2 26:2 27:2 28:4 30:5 32:3 33:2 34:3 35:1 36:2 37:1 39:1 40:2 41:1 42:1 43:2 | 14 quests, about 48,333 experience, 5 with gear |
| Alliance | 18 | 0 | 68,625 | 21% | 6 | 27:2 29:1 30:1 31:1 32:5 33:1 34:2 36:2 37:2 40:1 | 44 quests, about 160,135 experience, 15 with gear |

**Enemies no quest uses:** Mudsnout Gnoll (26-27, 12 of them), Mudsnout Shaman (27-28, 8 of them).

### Stormwind City (28-41, Eastern Kingdoms)

Going from level 28 to 42 costs 759,900 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 72 | 5 | 264,857 | 35% | 8 | 2:3 15:3 22:2 24:6 28:4 29:2 30:5 31:10 32:1 33:4 35:2 36:1 38:5 40:2 41:2 43:2 45:1 50:3 52:1 55:1 60:11 80:1 | not a levelling zone |

**Enemies no quest uses:** none of note.

### Thousand Needles (29-37, Kalimdor)

Going from level 29 to 38 costs 436,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 53 | 1 | 168,584 | 39% | 11 | 25:2 26:2 27:1 28:7 29:4 30:4 31:2 33:1 35:6 36:6 37:6 38:1 39:1 41:5 43:1 44:4 | 32 quests, about 137,176 experience, 11 with gear |
| Alliance | 34 | 1 | 100,019 | 23% | 2 | 27:1 30:4 31:2 33:1 35:2 36:5 37:6 38:1 39:1 41:5 43:1 44:4 46:1 | 48 quests, about 205,741 experience, 16 with gear |

**Enemies no quest uses:** Pesterhide Hyena (26-27, 46 of them), Needles Cougar (27-28, 41 of them), Pesterhide Snarler (28-29, 36 of them), Galak Mauler (27-28, 22 of them), Galak Marauder (26-27, 22 of them), Crag Stalker (25-26, 21 of them), Elder Cloud Serpent (27-29, 20 of them), Highperch Wyvern (28-29, 17 of them), Grimtotem Stomper (26-27, 17 of them), Galak Stormer (26-27, 17 of them), and 18 more.

**Places nothing is asked of you:** Highperch (27 enemies), The Screeching Canyon (24 enemies), Windbreak Canyon (23 enemies), Camp E'thok (22 enemies), Roguefeather Den (15 enemies).

### Desolace (33-38, Kalimdor)

Going from level 33 to 39 costs 341,700 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 37 | 1 | 168,360 | 49% | 15 | 31:2 32:4 33:7 34:1 35:6 36:3 37:4 38:4 39:2 40:1 42:2 60:1 | 15 quests, about 70,830 experience, 5 with gear |
| Alliance | 40 | 0 | 194,610 | 57% | 14 | 31:1 32:4 33:8 34:1 35:6 36:2 37:3 38:2 39:8 40:3 42:2 | 10 quests, about 44,580 experience, 4 with gear |

**Enemies no quest uses:** Dread Swoop (32-33, 98 of them), Raging Thunder Lizard (33-34, 77 of them), Gritjaw Basilisk (31-32, 77 of them), Hulking Gritjaw Basilisk (35-36, 57 of them), Dread Flyer (36-37, 49 of them), Elder Thunder Lizard (37-38, 40 of them), Lesser Infernal (36-37, 16 of them), Whirlwind Stormwalker (35-37, 15 of them), Dying Kodo (35-36, 15 of them), Carrion Horror (35-37, 15 of them), and 14 more.

### Orgrimmar (33-60, Kalimdor)

Going from level 33 to 61 costs 3,103,600 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 85 | 36 | 342,250 | 11% | 44 | 12:2 15:1 16:1 20:2 21:2 30:2 33:5 35:1 43:1 48:3 50:1 52:1 53:3 54:1 55:2 56:2 60:53 74:1 80:1 | not a levelling zone |
| Alliance | 4 | 0 | 68,767 | 2% | 1 | 20:1 48:1 56:1 80:1 | not a levelling zone |

**Enemies no quest uses:** none of note.

### Alterac Mountains (34-40, Eastern Kingdoms)

Going from level 34 to 41 costs 438,200 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 3 | 0 | 38,340 | 9% | 2 | 34:1 70:2 | only passing through |
| Alliance | 15 | 0 | 93,060 | 21% | 3 | 33:1 34:4 36:2 38:1 40:5 70:2 | 39 quests, about 213,680 experience, 13 with gear |

**Enemies no quest uses:** Snapjaw (30-31, 66 of them), Giant Yeti (33-34, 34 of them), Mountain Lion (32-33, 28 of them), Hulking Mountain Lion (33-34, 26 of them), Mountain Yeti (32-33, 18 of them).

**Places nothing is asked of you:** Dalaran Crater (37 enemies), Growless Cave (32 enemies), Chillwind Point (24 enemies), Ravenholdt Manor (22 enemies), Lordamere Lake (17 enemies), Lordamere Internment Camp (14 enemies), Misty Shore (13 enemies), The Headland (12 enemies).

### Dustwallow Marsh (36-39, Kalimdor)

Going from level 36 to 40 costs 257,500 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 68 | 7 | 290,961 | 113% | 22 | 35:12 36:5 37:11 38:5 39:12 40:6 41:9 45:1 60:7 | nothing; it carries itself |
| Alliance | 75 | 1 | 295,289 | 115% | 21 | 35:18 36:8 37:16 38:11 39:14 40:5 41:2 60:1 | nothing; it carries itself |

**Enemies no quest uses:** Mottled Drywallow Crocolisk (38-39, 60 of them), Drywallow Snapper (37-38, 48 of them), Theramore Deserter (37, 36 of them), Firemane Flamecaller (41-42, 36 of them), Noxious Flayer (35-36, 22 of them), Bloodfen Raptor (36-37, 17 of them), Bloodfen Screecher (36-37, 16 of them), Murk Thresher (36-37, 14 of them), Noxious Reaver (37-38, 13 of them), Defias Diver (37-38, 13 of them), and 6 more.

**Places nothing is asked of you:** Mudsprocket (17 enemies).

### Undercity (36-56, Eastern Kingdoms)

Going from level 36 to 57 costs 2,161,500 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 28 | 1 | 193,791 | 9% | 5 | 8:1 15:3 21:2 36:1 39:1 44:1 45:2 46:1 47:1 50:1 52:5 55:2 56:2 60:4 74:1 | not a levelling zone |

**Enemies no quest uses:** none of note.

### Arathi Highlands (37-40, Eastern Kingdoms)

Going from level 37 to 41 costs 273,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 37 | 1 | 181,747 | 67% | 10 | 32:1 33:1 34:4 35:2 37:5 38:2 40:15 41:1 42:4 44:1 50:1 | 2 quests, about 9,423 experience, 1 with gear |
| Alliance | 26 | 1 | 142,680 | 52% | 9 | 31:1 35:2 36:1 37:2 38:1 39:6 40:10 42:1 44:1 50:1 | 9 quests, about 48,490 experience, 3 with gear |

**Enemies no quest uses:** Plains Creeper (32-33, 113 of them), Giant Plains Creeper (35-36, 43 of them), Young Mesa Buzzard (31-32, 18 of them), Boulderfist Ogre (32-33, 17 of them), Rumbling Exile (38-39, 16 of them), Boulderfist Enforcer (33-34, 16 of them), Mesa Buzzard (34-35, 14 of them), Cresting Exile (38-39, 14 of them), Burning Exile (38-39, 14 of them), Thundering Exile (38-39, 12 of them), and 3 more.

**Places nothing is asked of you:** Circle of West Binding (15 enemies), Faldir's Cove (10 enemies).

### Stranglethorn Vale (37-43, Eastern Kingdoms)

Going from level 37 to 44 costs 521,500 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 109 | 34 | 405,067 | 78% | 63 | 30:1 31:3 32:1 33:2 34:3 35:3 36:6 37:6 38:3 40:7 41:11 42:10 43:5 44:5 45:3 46:3 50:2 51:1 55:1 58:3 60:30 | nothing; it carries itself |
| Alliance | 114 | 35 | 394,177 | 76% | 65 | 30:1 31:3 32:2 33:4 34:2 35:4 36:4 37:13 38:4 39:1 40:9 41:8 42:10 43:6 44:4 45:1 46:1 50:2 51:1 55:1 58:3 60:30 | nothing; it carries itself |

**Enemies no quest uses:** Jungle Thunderer (37-38, 41 of them), Southern Sand Crawler (40-41, 24 of them), Skymane Gorilla (41, 19 of them), Sharptooth Frenzy (31-32, 18 of them), Jaguero Stalker (41, 14 of them), Saltwater Crocolisk (35-36, 13 of them), Mistvale Gorilla (32-33, 11 of them), Kurmokk (42, 11 of them), Silverback Patriarch (42-43, 9 of them), Bloodscalp Tiger (34-35, 6 of them), and 2 more.

**Places nothing is asked of you:** Zul'Gurub (29 enemies), Yojamba Isle (21 enemies).

### Swamp of Sorrows (38-44, Eastern Kingdoms)

Going from level 38 to 45 costs 550,700 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 20 | 0 | 128,977 | 23% | 3 | 35:2 36:1 38:2 39:1 40:1 42:2 43:3 44:2 45:2 50:1 55:3 | 38 quests, about 256,513 experience, 13 with gear |
| Alliance | 14 | 0 | 72,232 | 13% | 2 | 35:2 36:1 37:2 38:2 39:1 40:1 42:2 43:1 55:2 | 46 quests, about 313,258 experience, 16 with gear |

**Enemies no quest uses:** Deathstrike Tarantula (40-41, 38 of them), Tangled Horror (40-41, 26 of them), Silt Crawler (40-41, 24 of them), Molt Thorn (42, 19 of them), Lost One Muckdweller (36-37, 13 of them), Shadow Panther (39, 12 of them), Lost One Hunter (36-37, 12 of them), Swampwalker (38-39, 11 of them), Lost One Fisherman (35-36, 9 of them), Sawfin Frenzy (38-40, 8 of them), and 6 more.

**Places nothing is asked of you:** Misty Valley (15 enemies).

### Badlands (39-43, Eastern Kingdoms)

Going from level 39 to 44 costs 392,900 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 38 | 9 | 175,050 | 45% | 11 | 36:1 37:5 39:2 40:8 41:3 42:7 43:2 44:4 45:1 47:2 50:1 52:1 54:1 | 15 quests, about 99,980 experience, 5 with gear |
| Alliance | 51 | 14 | 206,550 | 53% | 21 | 35:3 36:1 37:5 38:3 39:1 40:13 41:3 42:9 43:3 44:4 45:3 47:2 50:1 | 10 quests, about 68,480 experience, 4 with gear |

**Enemies no quest uses:** Ridge Stalker (36-37, 25 of them), Ridge Stalker Patriarch (40-41, 22 of them), Siege Golem (40, 14 of them), 7:XT (41, 7 of them), Broken Tooth (37, 4 of them).

### Ironforge (43-60, Eastern Kingdoms)

Going from level 43 to 61 costs 2,456,100 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 77 | 37 | 198,395 | 8% | 46 | 15:2 30:3 31:2 33:1 35:1 37:1 43:1 46:1 47:1 50:6 52:7 54:3 55:2 60:46 | not a levelling zone |

**Enemies no quest uses:** none of note.

### Feralas (44-48, Kalimdor)

Going from level 44 to 49 costs 505,400 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 35 | 1 | 287,160 | 57% | 11 | 42:1 43:6 44:1 45:9 46:2 47:3 48:4 49:1 50:6 51:1 52:1 | 8 quests, about 66,620 experience, 3 with gear |
| Alliance | 46 | 0 | 303,214 | 60% | 14 | 43:6 44:6 45:4 46:5 47:9 48:7 49:3 50:4 51:2 | 6 quests, about 50,566 experience, 2 with gear |

**Enemies no quest uses:** Gordok Ogre-Mage (52-53, 13 of them), Gordok Enforcer (53-54, 5 of them).

**Places nothing is asked of you:** Oneiros (21 enemies), Jademir Lake (21 enemies), Dream Bough (21 enemies).

### The Hinterlands (45-49, Eastern Kingdoms)

Going from level 45 to 50 costs 529,500 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 32 | 4 | 286,140 | 54% | 11 | 42:1 43:1 44:1 45:2 47:3 48:11 49:1 50:6 51:4 55:2 | 10 quests, about 84,510 experience, 4 with gear |
| Alliance | 22 | 0 | 187,290 | 35% | 2 | 43:4 44:1 45:2 46:4 47:3 48:5 50:2 51:1 | 21 quests, about 183,360 experience, 7 with gear |

**Enemies no quest uses:** none of note.

**Places nothing is asked of you:** Seradane (48 enemies).

### Searing Gorge (47-50, Eastern Kingdoms)

Going from level 47 to 51 costs 453,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 22 | 0 | 191,272 | 42% | 11 | 45:1 46:1 47:1 48:5 49:5 50:8 52:1 | 14 quests, about 125,898 experience, 5 with gear |
| Alliance | 29 | 0 | 251,647 | 56% | 13 | 43:1 45:1 46:5 47:1 48:5 49:5 50:10 52:1 | 7 quests, about 65,523 experience, 3 with gear |

**Enemies no quest uses:** Searing Lava Spider (45-47, 26 of them), Muck Splash (47-49, 9 of them), Thorium Brotherhood Lookout (50-52, 6 of them), Enslaved Archaeologist (46-47, 6 of them), Slave Master Blackheart (50, 5 of them), Shadowsilk Poacher (47-50, 5 of them), Rekk'tilac (48, 5 of them), Faulty War Golem (46, 5 of them), Highlord Mastrogonde (51, 4 of them).

### Tanaris (47-60, Kalimdor)

Going from level 47 to 61 costs 2,080,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 86 | 14 | 733,121 | 35% | 32 | 43:1 44:3 45:11 46:8 47:5 48:7 49:6 50:4 51:1 53:1 54:3 57:1 58:1 60:34 | 64 quests, about 722,949 experience, 22 with gear |
| Alliance | 83 | 14 | 718,661 | 35% | 30 | 43:1 44:3 45:10 46:4 47:7 48:6 49:7 50:4 51:1 53:1 54:3 57:1 58:1 60:34 | 65 quests, about 737,409 experience, 22 with gear |

**Enemies no quest uses:** Spirit Healer (60, 6 of them).

### Blasted Lands (50-57, Eastern Kingdoms)

Going from level 50 to 58 costs 1,120,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 19 | 2 | 171,855 | 15% | 10 | 50:7 55:2 57:4 58:1 60:5 | 54 quests, about 612,215 experience, 18 with gear |
| Alliance | 20 | 2 | 208,635 | 19% | 10 | 50:5 54:1 55:2 56:1 57:5 58:1 60:5 | 51 quests, about 575,435 experience, 17 with gear |

**Enemies no quest uses:** Felguard Elite (61, 42 of them), Manahound (60, 14 of them), Doomguard Commander (61, 13 of them), Felguard Sentry (54-55, 5 of them).

**Places nothing is asked of you:** The Tainted Scar (76 enemies).

### The Exodar (50-60, Kalimdor)

Going from level 50 to 61 costs 1,747,900 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Alliance | 6 | 0 | 28,492 | 2% | 0 | 50:2 60:4 | not a levelling zone |

**Enemies no quest uses:** none of note.

### Un'Goro Crater (52-54, Kalimdor)

Going from level 52 to 55 costs 411,000 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 46 | 2 | 463,110 | 113% | 11 | 50:1 51:3 52:11 53:16 54:4 55:8 56:2 78:1 | nothing; it carries itself |
| Alliance | 44 | 2 | 437,355 | 106% | 9 | 50:1 51:3 52:12 53:13 54:4 55:8 56:2 78:1 | nothing; it carries itself |

**Enemies no quest uses:** Spirit Healer (60, 4 of them).

**Places nothing is asked of you:** Marshal's Refuge (16 enemies).

### Azshara (52-58, Kalimdor)

Going from level 52 to 59 costs 1,038,900 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 30 | 5 | 261,840 | 25% | 3 | 47:2 49:1 52:9 53:2 54:1 57:2 58:6 60:7 | 38 quests, about 465,390 experience, 13 with gear |
| Alliance | 22 | 5 | 204,180 | 20% | 3 | 47:2 49:1 52:1 53:2 54:1 57:2 58:6 60:7 | 43 quests, about 523,050 experience, 15 with gear |

**Enemies no quest uses:** Mosshoof Courser (52-53, 121 of them), Thunderhead Skystormer (50-52, 65 of them), Coralshell Lurker (53-54, 45 of them), Thunderhead Consort (52-54, 42 of them), Thunderhead Stagwing (48-50, 41 of them), Spitelash Serpent Guard (48-49, 41 of them), Timbermaw Shaman (50-51, 31 of them), Great Wavethrasher (53-54, 30 of them), Timbermaw Ursa (51-52, 29 of them), Mosshoof Runner (45-47, 29 of them), and 41 more.

**Places nothing is asked of you:** Legash Encampment (119 enemies), Ursolan (106 enemies), Bitter Reaches (88 enemies), Lake Mennar (77 enemies), Temple of Arkkoran (72 enemies), The Ruined Reaches (61 enemies), Bay of Storms (61 enemies), Jagged Reef (52 enemies).

### Burning Steppes (53-54, Eastern Kingdoms)

Going from level 53 to 55 costs 279,500 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 9 | 1 | 91,620 | 33% | 1 | 50:1 52:1 53:1 54:4 58:1 60:1 | 10 quests, about 104,030 experience, 4 with gear |
| Alliance | 11 | 1 | 109,020 | 39% | 1 | 50:1 52:2 53:2 54:3 56:1 58:1 60:1 | 8 quests, about 86,630 experience, 3 with gear |

**Enemies no quest uses:** Black Dragonspawn (52-53, 20 of them), Black Wyrmkin (53-54, 18 of them), Scalding Broodling (53-54, 16 of them), Flamescale Wyrmkin (57-58, 15 of them), Flamekin Torcher (54-56, 15 of them), Flamekin Rager (54-56, 15 of them), Black Broodling (51-52, 14 of them), Flamescale Dragonspawn (56-57, 12 of them), Deathlash Scorpid (54-55, 12 of them), Venomtip Scorpid (52-53, 11 of them), and 13 more.

**Places nothing is asked of you:** Flame Crest (13 enemies).

### Felwood (54-55, Kalimdor)

Going from level 54 to 56 costs 290,700 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 32 | 0 | 362,489 | 125% | 12 | 48:1 51:2 52:2 54:4 55:15 56:1 57:2 58:3 59:1 60:1 | nothing; it carries itself |
| Alliance | 33 | 0 | 327,711 | 113% | 15 | 48:1 51:1 52:2 54:7 55:15 56:2 57:2 58:2 60:1 | nothing; it carries itself |

**Enemies no quest uses:** none of note.

### Western Plaguelands (54-57, Eastern Kingdoms)

Going from level 54 to 58 costs 604,600 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 57 | 12 | 440,662 | 73% | 14 | 50:4 52:3 53:4 54:2 55:11 56:7 57:3 58:6 60:17 | nothing; it carries itself |
| Alliance | 55 | 13 | 442,252 | 73% | 14 | 52:4 53:3 54:2 55:12 56:7 57:3 58:6 60:18 | nothing; it carries itself |

**Enemies no quest uses:** Scarlet Worker (55-57, 56 of them), Searing Ghoul (55-56, 40 of them), Soulless Ghoul (54-55, 39 of them), Scarlet Paladin (55-56, 37 of them), Plague Lurker (54-55, 32 of them), Carrion Lurker (52-53, 31 of them), Diseased Black Bear (51-52, 27 of them), Scarlet Lumberjack (54-56, 25 of them), Scarlet Sentinel (55-56, 24 of them), Rotting Cadaver (53-54, 24 of them), and 17 more.

**Places nothing is asked of you:** Hearthglen (122 enemies), Caer Darrow (29 enemies), The Bulwark (25 enemies), Mardenholde Keep (21 enemies).

### Winterspring (56-59, Kalimdor)

Going from level 56 to 60 costs 651,700 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 38 | 5 | 304,505 | 47% | 14 | 52:2 54:3 56:10 57:2 58:4 59:5 60:12 | 12 quests, about 151,685 experience, 4 with gear |
| Alliance | 45 | 5 | 372,815 | 57% | 15 | 52:2 54:2 56:12 57:3 58:6 59:8 60:12 | 7 quests, about 83,375 experience, 3 with gear |

**Enemies no quest uses:** Shardtooth Mauler (55-56, 55 of them), Elder Shardtooth (57-58, 45 of them), Winterspring Owl (54-56, 44 of them), Frostsaber Stalker (59-60, 42 of them), Winterspring Screecher (57-59, 37 of them), Shardtooth Bear (53-54, 34 of them), Ragged Owlbeast (53-55, 30 of them), Suffering Highborne (54-55, 26 of them), Frostsaber Huntress (58-59, 25 of them), Raging Owlbeast (54-56, 22 of them), and 9 more.

**Places nothing is asked of you:** The Ruins of Kel'Theril (27 enemies).

### Eastern Plaguelands (56-60, Eastern Kingdoms)

Going from level 56 to 61 costs 941,700 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 61 | 9 | 547,393 | 58% | 17 | 55:8 56:9 57:2 58:7 60:35 | 9 quests, about 111,797 experience, 3 with gear |
| Alliance | 54 | 6 | 510,389 | 54% | 15 | 55:8 56:10 57:3 58:6 60:27 | 11 quests, about 148,801 experience, 4 with gear |

**Enemies no quest uses:** Scarlet Warder (53-54, 59 of them), Knight of the Ebon Blade (56-58, 45 of them), Scarlet Enchanter (53-55, 44 of them), Scarlet Cleric (54-55, 44 of them), Risen Drudge (50, 44 of them), Stitched Horror (57-58, 40 of them), Cannibal Ghoul (54-55, 31 of them), Scarlet Praetorian (56-57, 30 of them), Putrid Gargoyle (54-56, 30 of them), Hate Shrieker (55-57, 30 of them), and 38 more.

**Places nothing is asked of you:** Acherus: The Ebon Hold (208 enemies), Tyr's Hand (182 enemies), Light's Hope Chapel (56 enemies), Mazra'Alor (13 enemies).

### Moonglade (57-60, Kalimdor)

Going from level 57 to 61 costs 787,700 experience.

*Not a place to level (druid sanctuary); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 8 | 3 | 57,600 | 7% | 3 | 55:1 57:2 60:5 | not a levelling zone |
| Alliance | 8 | 3 | 57,600 | 7% | 3 | 55:1 57:2 60:5 | not a levelling zone |

**Enemies no quest uses:** Moonglade Warden (65, 33 of them).

**Places nothing is asked of you:** Nighthaven (42 enemies), Stormrage Barrow Dens (14 enemies).

### Silithus (60-60, Kalimdor)

Going from level 60 to 61 costs 290,000 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 65 | 17 | 526,671 | 182% | 36 | 55:3 57:5 58:2 59:3 60:52 | nothing; it carries itself |
| Alliance | 65 | 17 | 526,671 | 182% | 36 | 55:3 57:5 58:2 59:3 60:52 | nothing; it carries itself |

**Enemies no quest uses:** Cenarion Outrider (60, 12 of them), Spirit Healer (60, 5 of them), Cyclone Warrior (57, 5 of them), Brood of Nozdormu (62, 5 of them).

### Silvermoon City (60-60, Eastern Kingdoms)

Going from level 60 to 61 costs 290,000 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 5 | 0 | 30,202 | 10% | 0 | 21:1 60:4 | not a levelling zone |

**Enemies no quest uses:** none of note.

### Hellfire Peninsula (61-62, Outland)

Going from level 61 to 63 costs 666,000 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 101 | 14 | 1,070,115 | 161% | 39 | 60:9 61:41 62:28 63:18 70:5 | nothing; it carries itself |
| Alliance | 88 | 11 | 922,281 | 138% | 34 | 58:3 60:8 61:37 62:17 63:18 70:5 | nothing; it carries itself |

**Enemies no quest uses:** Deranged Helboar (60-61, 87 of them), Heckling Fel Sprite (57-58, 64 of them), Starving Helboar (58-59, 63 of them), Wrathguard (58-59, 44 of them), Fel Handler (58-59, 31 of them), Forge Camp Legionnaire (60-61, 30 of them), Netherhound (58, 29 of them), Dreghood Brute (56-57, 29 of them), Sister of Grief (60-61, 18 of them), Ghostly Denizen (59-60, 18 of them), and 11 more.

**Places nothing is asked of you:** The Legion Front (120 enemies).

### Zangarmarsh (62-64, Outland)

Going from level 62 to 65 costs 1,163,000 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 63 | 7 | 790,095 | 68% | 23 | 61:1 62:18 63:12 64:29 65:3 | 2 quests, about 24,005 experience, 1 with gear |
| Alliance | 65 | 7 | 817,545 | 70% | 23 | 62:23 63:12 64:27 65:3 | nothing; it carries itself |

**Enemies no quest uses:** Swamp Gas (60, 100 of them), Bogstrok Hatchling (63, 85 of them), Wrekt Slave (62, 67 of them), Sporeggar Spawn (60, 63 of them), Lagoon Eel (61-62, 43 of them), Bloodthirsty Marshfang (61-62, 37 of them), Marshrock Threshalisk (61-62, 26 of them), Ragestone Threshalisk (62-63, 21 of them), Sporelok (61-62, 20 of them), Ironspine Threshalisk (63-64, 19 of them), and 14 more.

### Terokkar Forest (64-65, Outland)

Going from level 64 to 66 costs 903,000 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 98 | 23 | 1,024,230 | 113% | 37 | 62:2 63:12 64:24 65:33 66:2 67:3 68:2 69:3 70:17 | nothing; it carries itself |
| Alliance | 94 | 23 | 1,003,635 | 111% | 36 | 62:2 63:13 64:23 65:29 66:2 67:3 68:2 69:3 70:17 | nothing; it carries itself |

**Enemies no quest uses:** Warp Hunter (64-65, 79 of them), Blackwind Sabercat (70-71, 69 of them), Skettis Surger (70-71, 52 of them), Razorthorn Ravager (70, 52 of them), Infested Root-Walker (64-65, 44 of them), Dampscale Devourer (63-64, 43 of them), Time-Lost Skettis High Priest (71, 36 of them), Scorpid Bonecrawler (64-65, 36 of them), Royal Teromoth (63-64, 36 of them), Ancient Draenei Spirit (1-70, 36 of them), and 42 more.

**Places nothing is asked of you:** Razorthorn Rise (68 enemies), Tuurem (46 enemies), The Barrier Hills (37 enemies), Blackwind Lake (37 enemies), Refugee Caravan (32 enemies), Blackwind Landing (24 enemies), Sha'tari Base Camp (14 enemies), Derelict Caravan (13 enemies).

### Shattrath City (65-70, Outland)

Going from level 65 to 71 costs 4,475,800 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 40 | 6 | 332,820 | 7% | 10 | 63:1 64:3 65:8 67:4 70:24 | not a levelling zone |
| Alliance | 41 | 6 | 342,195 | 8% | 10 | 63:1 64:3 65:8 67:4 70:25 | not a levelling zone |

**Enemies no quest uses:** Shattered Sun Marksman (68-70, 19 of them), Shattrath City Peacekeeper (70, 18 of them), Skettis Outcast (60, 15 of them), Scryer Arcanist (61, 14 of them), Neophyte Combatant (64-65, 12 of them), Aldor Anchorite (70, 10 of them), Shattered Sun Trainee (68-70, 9 of them), Peasant Refugee (60, 9 of them), Injured Refugee (70, 9 of them), Aldor Vindicator (70, 8 of them), and 15 more.

**Places nothing is asked of you:** Lower City (46 enemies), Aldor Rise (16 enemies).

### Nagrand (66-67, Outland)

Going from level 66 to 68 costs 1,112,000 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 96 | 24 | 1,024,515 | 92% | 27 | 65:12 66:24 67:33 68:21 70:6 | nothing; it carries itself |
| Alliance | 82 | 24 | 870,600 | 78% | 26 | 65:14 66:23 67:31 68:8 70:6 | nothing; it carries itself |

**Enemies no quest uses:** Windy Cloud (60, 63 of them), Multiphase Disturbance (70, 27 of them), Deathshadow Spellbinder (70-71, 22 of them), Deathshadow Warlock (70-71, 20 of them), Raging Fire-Soul (70-71, 17 of them), Crashing Wave-Spirit (70-71, 16 of them), Storming Wind-Ripper (70-71, 15 of them), Dark Worg (64-65, 12 of them), Rumbling Earth-Heart (70-71, 10 of them), Deathshadow Archon (70-71, 9 of them), and 8 more.

**Places nothing is asked of you:** Elemental Plateau (59 enemies), Aeris Landing (18 enemies).

### Blade's Edge Mountains (66-68, Outland)

Going from level 66 to 69 costs 1,760,000 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 100 | 12 | 1,230,900 | 70% | 26 | 64:1 65:3 66:16 67:29 68:22 69:1 70:27 72:1 | 1 quests, about 1,100 experience, 1 with gear |
| Alliance | 97 | 13 | 1,186,170 | 67% | 28 | 65:2 66:14 67:33 68:19 69:1 70:27 72:1 | 3 quests, about 45,830 experience, 1 with gear |

**Enemies no quest uses:** Insidious Familiar (70-71, 29 of them), Felstorm Corruptor (66-67, 29 of them), Expedition Warden (70, 26 of them), Wyrm from Beyond (66, 23 of them), Evergrove Druid (70, 21 of them), Rip-Blade Ravager (63, 20 of them), Vekh'nir Stormcaller (65-66, 18 of them), Vekh'nir Keeneye (65-66, 18 of them), Vekh'nir Dreadhawk (65-66, 17 of them), Searing Elemental (67-68, 16 of them), and 38 more.

**Places nothing is asked of you:** Raven's Wood (32 enemies), Razaan's Landing (24 enemies), Ogri'la (23 enemies), Skyguard Outpost (14 enemies), Toshley's Station (13 enemies).

### Netherstorm (68-70, Outland)

Going from level 68 to 71 costs 2,888,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 153 | 24 | 2,070,780 | 72% | 44 | 68:39 69:58 70:55 71:1 | nothing; it carries itself |
| Alliance | 153 | 24 | 2,070,780 | 72% | 44 | 68:39 69:58 70:55 71:1 | nothing; it carries itself |

**Enemies no quest uses:** Farahlon Lasher (68-69, 70 of them), Phase Hunter (67-68, 66 of them), Talbuk Doe (68-69, 42 of them), Mana Snapper (67-68, 33 of them), Craghide Basilisk (68-69, 33 of them), Gan'arg Mekgineer (68-69, 25 of them), Sundered Rumbler (67-68, 23 of them), Cyber-Rage Forgelord (68-69, 23 of them), Warp Monstrosity (68-69, 22 of them), Talbuk Sire (68-69, 20 of them), and 28 more.

**Places nothing is asked of you:** Area 52 (83 enemies), Eco-Dome Skyperch (51 enemies), Gyro-Plank Bridge (14 enemies), Cosmowrench (13 enemies).

### Shadowmoon Valley (69-70, Outland)

Going from level 69 to 71 costs 2,240,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 133 | 27 | 1,608,330 | 72% | 28 | 69:31 70:101 71:1 | nothing; it carries itself |
| Alliance | 133 | 27 | 1,608,330 | 72% | 28 | 69:31 70:101 71:1 | nothing; it carries itself |

**Enemies no quest uses:** Murkblood Miner (70, 115 of them), Enraged Earth Spirit (68-69, 52 of them), Wrathwalker (67-68, 46 of them), Enraged Fire Spirit (68-69, 46 of them), Disobedient Dragonmaw Peon (68-69, 45 of them), Enraged Air Spirit (69-70, 43 of them), Deathforged Infernal (68, 39 of them), Eclipsion Dragonhawk (67-68, 35 of them), Domesticated Felboar (65-66, 35 of them), Rocknail Ripper (68-69, 33 of them), and 49 more.

**Places nothing is asked of you:** Netherwing Fields (89 enemies), Coilskar Cistern (70 enemies), Invasion Point: Cataclysm (64 enemies), Shattered Plains (42 enemies), Sanctum of the Stars (18 enemies), The Altar of Damnation (17 enemies), Oronok's Farm (12 enemies), Eclipsion Fields (10 enemies).

### Deadwind Pass (70-70, Eastern Kingdoms)

Going from level 70 to 71 costs 1,523,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 32 | 9 | 432,225 | 28% | 18 | 70:32 | 34 quests, about 634,435 experience, 12 with gear |
| Alliance | 32 | 9 | 432,225 | 28% | 18 | 70:32 | 34 quests, about 634,435 experience, 12 with gear |

**Enemies no quest uses:** Unliving Resident (70, 20 of them).

**Places nothing is asked of you:** The Vice (19 enemies), Karazhan (15 enemies).

### Isle of Quel'Danas (70-80, Eastern Kingdoms)

Going from level 70 to 81 costs 15,965,800 experience.

*Not a place to level (daily quests at the level cap); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 12 | 0 | 101,700 | 1% | 4 | 62:1 70:9 80:2 | not a levelling zone |
| Alliance | 12 | 0 | 101,700 | 1% | 4 | 62:1 70:9 80:2 | not a levelling zone |

**Enemies no quest uses:** Skeletal Ravager (69-70, 218 of them), Dawnblade Summoner (70, 36 of them), Wretched Fiend (69, 31 of them), Darkspine Myrmidon (70-71, 30 of them), Frenzied Ghoul (69-70, 28 of them), Dawnblade Reservist (70, 28 of them), Dawnblade Blood Knight (70, 27 of them), Wretched Devourer (69, 22 of them), Darkspine Siren (70-71, 21 of them), Erratic Sentry (70, 18 of them), and 11 more.

**Places nothing is asked of you:** The Dead Scar (256 enemies), Dawnstar Village (98 enemies), Sun's Reach Harbor (27 enemies), Shattered Sun Staging Area (21 enemies), The Dawning Square (16 enemies), Sunwell Plateau (14 enemies), The Dawnchaser (10 enemies), The Bloodoath (10 enemies).

### Borean Tundra (71-71, Northrend)

Going from level 71 to 72 costs 1,539,600 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 170 | 4 | 4,126,575 | 268% | 51 | 71:126 72:41 74:1 80:2 | nothing; it carries itself |
| Alliance | 145 | 4 | 3,451,800 | 224% | 49 | 71:107 72:36 80:2 | nothing; it carries itself |

**Enemies no quest uses:** Marsh Caribou (70-71, 182 of them), Wooly Rhino Calf (67, 118 of them), Carrion Condor (70-71, 72 of them), Oiled Fledgeling (69, 65 of them), Steam Rager (70-71, 59 of them), Vengeful Taunka Spirit (70-71, 56 of them), Beryl Reclaimer (69-70, 44 of them), Kaskala Defender (70, 40 of them), Oil-stained Wolf (70-71, 39 of them), Marsh Fawn (66-67, 37 of them), and 43 more.

**Places nothing is asked of you:** Steam Springs (55 enemies), Unu'pe (32 enemies), Farshire Mine (19 enemies), Coast of Echoes (16 enemies), D.E.H.T.A. Encampment (13 enemies).

### Howling Fjord (71-71, Northrend)

Going from level 71 to 72 costs 1,539,600 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 95 | 4 | 2,388,000 | 155% | 28 | 70:2 71:81 72:12 | nothing; it carries itself |
| Alliance | 123 | 4 | 2,732,400 | 177% | 30 | 70:16 71:97 72:10 | nothing; it carries itself |

**Enemies no quest uses:** Shoveltusk Forager (69, 167 of them), Decomposing Ghoul (68, 131 of them), Fanggore Worg (70-71, 92 of them), Fjord Crawler (70, 60 of them), Blockade Pirate (70, 53 of them), Lion Seal Whelp (68-69, 49 of them), Fjord Hawk (68-70, 44 of them), Spotted Hippogryph (69-70, 43 of them), Iron Rune Worker (69, 35 of them), Iron Rune Guardian (69-70, 34 of them), and 69 more.

**Places nothing is asked of you:** Skorn (130 enemies), Cauldros Isle (82 enemies), Shattered Straits (73 enemies), Kamagua (43 enemies), Halgrind (33 enemies), Balejar Watch (20 enemies).

### Dragonblight (72-74, Northrend)

Going from level 72 to 75 costs 4,715,400 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 151 | 12 | 3,657,975 | 78% | 50 | 72:71 73:24 74:44 75:8 80:4 | nothing; it carries itself |
| Alliance | 141 | 10 | 3,265,950 | 69% | 46 | 72:53 73:31 74:47 75:6 80:4 | 2 quests, about 34,830 experience, 1 with gear |

**Enemies no quest uses:** Snowfall Elk (71-72, 195 of them), Arcane Wyrm (71-72, 72 of them), Wretched Ghoul (80, 59 of them), Mindless Ghoul (70, 52 of them), Arctic Grizzly (72-73, 48 of them), Arctic Grizzly Cub (69-70, 47 of them), Arctic Ram (72-73, 46 of them), Tundra Scavenger (72-73, 45 of them), Emaciated Mammoth Calf (70, 31 of them), Moonrest Highborne (71-72, 28 of them), and 51 more.

**Places nothing is asked of you:** Wyrmrest Temple (90 enemies), The Court of Skulls (45 enemies), Coldwind Pass (24 enemies), Bronze Dragonshrine (16 enemies), Angrathar the Wrathgate (16 enemies).

### Grizzly Hills (74-75, Northrend)

Going from level 74 to 76 costs 3,192,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 88 | 7 | 2,105,400 | 66% | 29 | 73:1 74:37 75:50 | 5 quests, about 129,070 experience, 2 with gear |
| Alliance | 96 | 2 | 2,479,500 | 78% | 33 | 73:6 74:35 75:55 | nothing; it carries itself |

**Enemies no quest uses:** Highland Mustang (74-75, 91 of them), Famished Scourge Troll (73, 77 of them), Earthen Warrior (73-74, 35 of them), Venture Co. Straggler (73, 33 of them), Lightning Sentry (74-75, 31 of them), Runed Giant (74-75, 30 of them), Injured Drakkari Refugee (74, 28 of them), Frosthorn Ram (70-71, 26 of them), Drakkari Defender (74, 22 of them), Diseased Drakkari (73, 22 of them), and 24 more.

**Places nothing is asked of you:** Venture Bay (57 enemies), The Forgotten Overlook (25 enemies), Granite Springs (16 enemies), Hollowstone Mine (13 enemies).

### Dalaran (74-80, Northrend)

Going from level 74 to 81 costs 9,774,900 experience.

*Not a place to level (city); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 5 | 0 | 75,600 | 1% | 1 | 74:3 80:2 | not a levelling zone |
| Alliance | 5 | 0 | 75,600 | 1% | 1 | 74:3 80:2 | not a levelling zone |

**Enemies no quest uses:** Violet Hold Guard (75, 8 of them), Skybreaker Marine (80, 4 of them), Kor'kron Reaver (80, 4 of them).

**Places nothing is asked of you:** The Underbelly (23 enemies), Dalaran City (22 enemies), Circle of Wills (17 enemies), Krasus' Landing (16 enemies), The Violet Hold (12 enemies).

### Zul'Drak (75-77, Northrend)

Going from level 75 to 78 costs 4,862,300 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 108 | 12 | 2,578,950 | 53% | 34 | 75:26 76:36 77:42 78:4 | 26 quests, about 824,660 experience, 9 with gear |
| Alliance | 108 | 12 | 2,578,950 | 53% | 34 | 75:26 76:36 77:42 78:4 | 26 quests, about 824,660 experience, 9 with gear |

**Enemies no quest uses:** Drakkari Skullcrusher (75-76, 40 of them), Dead Mam'toth Disciple (75-76, 40 of them), Quetz'lun Worshipper (76-77, 38 of them), Defeated Argent Footman (75-76, 38 of them), Defeated Argent Footman (75-76, 23 of them), Crusade Recruit (71, 23 of them), Argent Stand Defender (77, 19 of them), Argent Crusader (75-76, 18 of them), Argent Footman (75-76, 17 of them), Zim'Torga Defender (76-77, 15 of them), and 13 more.

**Places nothing is asked of you:** Light's Breach (27 enemies).

### Sholazar Basin (76-77, Northrend)

Going from level 76 to 78 costs 3,258,100 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 82 | 1 | 2,222,550 | 68% | 21 | 76:26 77:49 78:7 | 2 quests, about 58,120 experience, 1 with gear |
| Alliance | 82 | 1 | 2,222,550 | 68% | 21 | 76:26 77:49 78:7 | 2 quests, about 58,120 experience, 1 with gear |

**Enemies no quest uses:** Frenzyheart Ravager (77-78, 106 of them), Frenzyheart Hunter (77-78, 85 of them), Sparktouched Warrior (77-78, 62 of them), Servant of Freya (76-77, 61 of them), Sparktouched Oracle (77-78, 56 of them), Frosthowl Screecher (75-76, 44 of them), Mosswalker Victim (77-78, 37 of them), Storm Revenant (79-80, 23 of them), Aqueous Spirit (79-80, 20 of them), Frenzyheart Tracker (77-78, 14 of them), and 12 more.

**Places nothing is asked of you:** The Stormwright's Shelf (47 enemies), The Glimmering Pillar (22 enemies).

### The Storm Peaks (79-80, Northrend)

Going from level 79 to 81 costs 1,670,800 experience.

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 96 | 1 | 2,629,050 | 157% | 22 | 77:1 78:19 79:6 80:69 81:1 | nothing; it carries itself |
| Alliance | 95 | 2 | 2,555,550 | 153% | 23 | 77:1 78:25 79:5 80:63 81:1 | nothing; it carries itself |

**Enemies no quest uses:** Stormforged Soldier (78-79, 73 of them), Earthen Elite (80, 51 of them), Stormpeak Wyrm (80, 40 of them), Earthen Warder (80, 37 of them), Iron Dwarf Magus (78-79, 35 of them), Ice Steppe Rhino (78-79, 28 of them), Stormpeak Hatchling (80, 24 of them), Earthen Defender (79-80, 24 of them), Brunnhildar Challenger (79-80, 24 of them), Son of Hodir (79-80, 21 of them), and 46 more.

**Places nothing is asked of you:** Engine of the Makers (70 enemies), Dun Niffelem (63 enemies), Thunderfall (42 enemies), K3 (33 enemies), Dragonspine Peaks (24 enemies), The Frigid Tomb (23 enemies), Temple of Storms (23 enemies), Plain of Echoes (20 enemies).

### Icecrown (80-80, Northrend)

Going from level 80 to 81 costs 0 experience.

*Not a place to level (level cap); listed for completeness.*

| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |
|---|---|---|---|---|---|---|---|
| Horde | 166 | 25 | 3,849,600 | 0% | 53 | 78:14 79:7 80:144 81:1 | not a levelling zone |
| Alliance | 164 | 25 | 3,750,300 | 0% | 51 | 78:14 79:7 80:142 81:1 | not a levelling zone |

**Enemies no quest uses:** Boneguard Footman (80, 146 of them), Onslaught Harbor Guard (79-80, 90 of them), Lordaeron Footsoldier (80, 81 of them), Slain Alliance Soldier (80, 69 of them), Argent Peacekeeper (80, 63 of them), Skeletal Reaver (80, 51 of them), Risen Alliance Soldier (80, 47 of them), Onslaught Gryphon Rider (79-80, 47 of them), Carrion Hunter (80, 47 of them), Saronite Mine Slave (77-80, 45 of them), and 112 more.

**Places nothing is asked of you:** Argent Tournament Grounds (220 enemies), Deathspeaker's Watch (83 enemies), Crusaders' Pinnacle (28 enemies), The Hidden Hollow (21 enemies), Death's Rise (21 enemies), The Frozen Halls (20 enemies), Argent Pavilion (10 enemies).

## The quests we have added

| Number | Title | Zone | Level | Side | Kind |
|---|---|---|---|---|---|
| 9200002 | The Wandering Years | Mulgore | 8 | horde | deliver |
| 9200003 | Scouts of the Galak | Mulgore | 9 | horde | kill |
| 9200004 | A Debt Remembered | Mulgore | 9 | horde | deliver |
| 9200005 | The Swiftest Prey | Mulgore | 7 | horde | kill |
| 9200006 | Antlers Among the Stars | Mulgore | 8 | horde | collect |
| 9200007 | The Moon's Mercy | Mulgore | 10 | horde | slay |
| 9200008 | What Lives in the Fargodeep | Elwynn Forest | 6 | alliance | visit |
| 9200009 | The Darkness Took Them | Elwynn Forest | 7 | alliance | collect |
| 9200010 | Under the Whip | Elwynn Forest | 9 | alliance | slay |
| 9200011 | Arms for the Militia | Elwynn Forest | 9 | alliance | deliver |
| 9200012 | Word for Three Corners | Elwynn Forest | 10 | alliance | deliver |
| 9200013 | Older Than the Forge | Dun Morogh | 7 | alliance | visit |
| 9200014 | The Rememberer | Dun Morogh | 9 | alliance | kill |
| 9200015 | Troggs in the Diggings | Dun Morogh | 10 | alliance | deliver |
| 9200016 | Once of Our Blood | Teldrassil | 6 | alliance | visit |
| 9200017 | The Gardener of Starbreeze | Teldrassil | 8 | alliance | slay |
| 9200018 | Hands for Auberdine | Teldrassil | 10 | alliance | deliver |
| 9200019 | Sails from the East | Durotar | 7 | horde | visit |
| 9200020 | The Admiral's Last Order | Durotar | 8 | horde | collect |
| 9200021 | For the Darkspear | Durotar | 9 | horde | deliver |
| 9200022 | Bigger Game | Durotar | 11 | horde | deliver |
| 9200023 | The First Landing | Tirisfal Glades | 9 | horde | visit |
| 9200024 | What the Tide Kept | Tirisfal Glades | 9 | horde | collect |
| 9200025 | The Archbishop's Grave | Tirisfal Glades | 8 | horde | visit |
| 9200026 | Thin on the Ground | Tirisfal Glades | 10 | horde | deliver |
