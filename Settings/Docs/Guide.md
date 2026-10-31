# The Guide

Built on 2026-10-27, from the plan in [Addons-Plan.md](Addons-Plan.md) (step 1). It replaces the Loot Book. **Not opened in the game yet.**

`/guide`, or **Guide** in the menu. `/guide lore` opens the story of the zone you are in, `/guide map` a dungeon's map. `/lootbook` still opens it too.

## What is in it

Four tabs along the bottom.

| Tab | The list on the left | The page |
|---|---|---|
| Dungeons, Raids | every dungeon or raid in level order, with its icon from the dungeon finder | The dungeon's own picture with its name, levels and where the entrance is. **Bosses**: the list, the boss as a model you can turn with the mouse, its story, and what it drops (the game's own with the chance, then "New on this server" with the class each is for; buttons for heroic and the raid sizes). **Quests**: every quest that belongs there, with its level, side, who gives it and where; hover for what it asks. **Map**: the place from above, with the bosses numbered and the way in marked; click a boss for its page. |
| Champions | the 111 champions in level order | The champion as a model, where it lives (zone and map coordinates), whether there is a bounty, and what it drops. |
| Lore | the zone you are in first, then every zone | The story of the zone, then of each place in it. The place you stand in is shown in green and scrolled to. |

**Find an item** (bottom): type three letters or more to see every boss and champion that drops something by that name. On any item: hover for the tooltip, Shift-click to link it, Ctrl-click to try it on.

The first time you enter a zone that has a story you have not read, the chat says so once.

## Where it comes from

Everything is made from this server's own database and the client's data files at every server start, so our own quests, champions and extra boss drops are in it.

| File | Made by | What |
|---|---|---|
| `Client/AddOns/WowPlusGuide/Loot.lua` | `Settings/Scripts/Make-Boss-Loot.py` | bosses and what they drop ([Boss-Loot.md](Boss-Loot.md)) |
| `Client/AddOns/WowPlusGuide/Places.lua` | `Settings/Scripts/Make-Guide.py` | per dungeon: name, levels and picture (the client's dungeon finder table, holiday bosses left out), the entrance (the doorway's position on the zone map), the quests, the boss stories; the champions and their drops |
| `Client/AddOns/WowPlusGuide/Lore.lua` | `Settings/Scripts/Make-Guide.py` | zone and place lore |
| `Client/AddOns/WowPlusGuide/Maps/<map>.tga`, `Maps.lua` | `Settings/Scripts/Make-Dungeon-Maps.py` | the map pictures (made on this PC, not committed) and where the bosses and doors are on them |

## The dungeon maps

Most old dungeons have no map in the 3.3.5 game, so we draw our own from what the server knows: `Data/mmaps`, the walkable ground the bots find their way on. `Make-Dungeon-Maps.py` keeps what can be reached (joined to a door or to something living there; the mesh also covers roofs and the outside of walls), draws it from above on parchment, with north up, and puts the bosses where they stand in the database.

- Lower ground is darker, a drop of about three yards is an ink line, water is blue, lava and slime red.
- **A floor above another hides it.** Places with many storeys (Blackrock Spire, for one) are harder to read than flat ones.
- Parts that are not joined and lie far apart are moved together into a grid (the Scarlet Monastery's four wings, Dire Maul's), so **those maps do not show how the parts lie in the world**.
- Places on open ground have the whole landscape in the mesh, so they are cropped to the ground around their bosses and doors (Shadowfang Keep, Zul'Farrak, the Black Morass, Durnholde, Mount Hyjal). Those outside the keep are shown smooth, with only the outline inked: they read as a rough lie of the land, not as a plan.
- A boss that only appears during a fight, or is summoned, has no place in the database: it is listed with a dash and no pin.

Deploy runs it at every server start and it draws only what is missing. `--map 36` draws one again and leaves a preview in `Build/dungeon-map-previews`; `--all` draws all (about four minutes). It needs Python with numpy and Pillow. A picture that is new needs a **restart of the game**, not just `/reload`.

**A dungeon's quests** are the ones the game files under it, and also those that ask for an enemy that lives in it or for an item such an enemy drops (the game files *The Defias Brotherhood* under Westfall, not the Deadmines).

**Boss stories and zone lore** are taken from the addon Spoken ([rusty-key/spoken-wow](https://github.com/rusty-key/spoken-wow)), which wrote them from [warcraft.wiki.gg](https://warcraft.wiki.gg) and Blizzard's Adventure Guide. That text is under **CC BY-SA 4.0**: it may be used and changed, with credit, and anything made from it stays under the same licence. Copies are kept in `Settings/Reference/Boss-Stories.lua` and `Settings/Reference/Zone-Lore/`, so the server does not need the download. The lore covers the 49 zones of the old world (Kalimdor and the Eastern Kingdoms) and 792 places in them; Outland, Northrend and the blood elf and draenei lands have none yet.

## Not covered yet

- The lore of the newer zones.

## To try

1. `/guide`: does it open at a dungeon for your level, with the dungeon's picture at the top and icons in the list?
2. Click a boss: does the model show (drag to turn it), with its story beside it and its loot to the right?
3. Quests: are the Deadmines' quests there (Red Silk Bandanas, The Defias Brotherhood, and our own The Ship Under the Hill for the Horde)? Does hovering show what each asks?
4. Champions: pick one near your level. Model, place and drops?
5. Lore: does it open at the zone you are in, with the place you stand in shown in green?
6. Type "sword" in Find an item.
7. Go to a zone you have not been to since: does the chat mention the guide once?
8. Map, in the Deadmines: the picture, the numbered bosses, E for the way in? Hover and click a pin. Is north right, and do the pins sit where the bosses are when you go there?
