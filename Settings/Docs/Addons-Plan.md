# Plan: five addons in the spirit of WoW Forever's

Decided 2026-10-27. Robin saw these addons for WoW Forever and wants the same on this server: "Spoken, Immersion, a new map addon called OpenMap, some sort of dungeon journal to see dungeon maps, bosses, loot etc., some sort of AtlasLoot or way to see loot."

None of them can be installed as they are: they are written for today's game client (Forever, Classic Era), and our 3.3.5 client has a much older addon interface. So each is **written again for 3.3.5**, as our own addon in `Client/AddOns`, reusing ideas, and code or text where the licence allows. Everything a book shows is made from **our own server's database**, so it is right for this server (our quests, our champions, our extra boss drops).

Claude cannot run the game: Claude writes and checks the code, Robin tries it in the game. Each step ends with a short list of things to try.

## The sources

Kept outside the project, in `C:\dev\addon-refs` (read only, never run).

| Addon | Where | Licence | What we take |
|---|---|---|---|
| Spoken | [rusty-key/spoken-wow](https://github.com/rusty-key/spoken-wow) | MIT (code). Game text is Blizzard's; zone lore is from warcraft.wiki.gg, CC BY-SA 4.0 | The way it plays lines (one queue, subtitles, game sound lowered). **Zone and subzone lore text**, `addons/Spoken_Places/Data/enUS` (CC BY-SA: credit, and share alike). Boss stories, `addons/Spoken_NPCs/Data/Stories.lua` (CC BY-SA). Not the recordings: their licence is not stated. |
| Immersion | [seblindfors/Immersion](https://github.com/seblindfors/Immersion) | Artistic-2.0 | The design only: the NPC talking up close, a line at a time, keys to answer. Written fresh. |
| OpenMap | [CurseForge](https://www.curseforge.com/wow/addons/openmap); GitHub repo offline (moving) | MIT | The idea: one seamless, zoomable continent made of the game's own zone maps. To fetch when the repo is back. |
| AtlasLoot / Atlas | old 3.3.5 versions exist | various | Nothing: they show the original game's loot, not ours. |

## The steps, in order

| # | Addon | What it does | Status |
|---|---|---|---|
| 1 | **WowPlus Guide** (grows out of the Loot Book) | One book for dungeons and loot. Per dungeon: where it is and its levels, the bosses with a 3D model and their story, what each drops (the game's own and ours), the dungeon's quests and where to get them. The champions of every zone. **Lore**: the story of every zone and place, the one you stand in first (Robin, 2026-10-27: "those new lore addons had like a lore journal"). | built, not tried ([Guide.md](Guide.md)) |
| 1b | Dungeon maps for the Guide | Most old dungeons have no map in 3.3.5. Drawn by us from the walkable ground the server already has for each dungeon (`Data/mmaps`), with the bosses where they stand. | built, not tried ([Guide.md](Guide.md)) |
| 2 | **WowPlus Dialogue** | Talking to an NPC: their face up close and talking, the text a page at a time, Space to go on, number keys to answer. Quest details, progress, rewards and NPC options. | built, then turned off at Robin's request, 2026-10-31 ([Dialogue.md](Dialogue.md)) |
| 3 | **Voices** | The game's own quests and the zone lore: Spoken's voice packs, played by Spoken's own 3.3.5 player (Robin, 2026-10-27: "Cant you use the spoken voices"). Our own 241 quests: our own pack in the same shape, made on this PC with Kokoro (Robin on the samples: "they were alright"). The Guide reads a zone's story the first time you come there. | built, then turned off at Robin's request, 2026-10-31 ([Voice.md](Voice.md)) |
| 4 | **WowPlus Map** | First: the unexplored shown faintly, coordinates, pins for dungeon entrances, champions, flight masters, inns and our quest givers. Then: the seamless continent to zoom and pan. | first part built, not tried ([Map.md](Map.md)); seamless map planned |

## Rules for all of them

- Interface `30300`, Lua 5.1, the 3.3.5 API (no `C_` namespaces, no `BackdropTemplate`, no atlases).
- Data files are made by a script in `Settings/Scripts` at every server start, like the Loot Book's, and the addons are copied into the game by `Deploy-Settings.ps1`.
- Large files (voices, map pictures) are not committed: they are made on this PC.
- Before Robin tries an addon, `python Settings/Scripts/Test-Addon.py <Addon> <steps>` runs it against a stand-in for the game, to catch Lua errors.
- Text taken from the wiki or Spoken's lore keeps its credit (in the data file's header and in this document).
