# WowPlus Map

Built on 2026-10-27, from the plan in [Addons-Plan.md](Addons-Plan.md) (step 4, first part). **Not opened in the game yet.**

More on the game's own world map (M). A dark bar across the top of the map has a box for each of these, and your and the mouse's coordinates on the right.

| Box | Shows |
|---|---|
| Unexplored | the parts of the zone you have not found yet, drawn faintly, so the whole zone can be read and exploring still shows |
| Dungeons | dungeon and raid entrances with their dungeon finder icon; hover for the levels |
| Champions | where each champion lives (a skull); hover for its level and whether there is a bounty |
| Travel | flight masters (green) and inns (where coaches leave) |
| Our quests | the givers of this server's own quests that you can still take: your side, your class, not done yet; hover for the list with levels |

`/wpmap` says which are on; `/wpmap fog` (or dungeon, champion, travel, quest) turns one on or off, like the boxes. Remembered per character.

## Where it comes from

`Client/AddOns/WowPlusMap/Data.lua`, written by `Settings/Scripts/Make-Guide.py` at every server start:

- **The pieces of each zone map** come from the client's own table of them (WorldMapOverlay): the same pictures the game shows when you find a place, drawn the same way (in tiles of 256 pixels), only tinted.
- **Where a pin goes**: the spawn's position in the database, put on the zone map the server's own map files say it stands in (the same as the Guide's entrances).
- **Which quests are done**: the game asks the server when the map opens.

## Not yet

The seamless map from the plan: one continent made of all its zone maps, to zoom and pan like OpenMap. Next part of step 4.

## To try

1. Open the map in a zone you have only half explored: do the unexplored parts show, darker? Untick Unexplored: gone?
2. Westfall: the Deadmines' icon near Moonbrook, Thor the gryphon master at Sentinel Hill, three skulls for champions, and the quest givers of our Westfall quests (Alliance).
3. Hover a pin: a tooltip?
4. Do the coordinates follow you and the mouse?
5. A quest of ours handed in: does its giver's pin go (when it was their last)?
