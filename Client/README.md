# Client

The game client we play with: World of Warcraft **3.3.5a, build 12340**. The game files are not in this repo (large, and not ours to redistribute). You supply the client yourself.

## What is in this folder

| Item | What it is |
|------|------------|
| `realmlist.wtf` | Tells the client to log in to our server on this PC (`127.0.0.1`). |
| `Copy-Settings.bat` | Copies game settings between the two game copies, to a new account or to a new character. See [Game settings](#game-settings-where-they-live-and-how-to-reuse-them). |
| `ChromieCraft_3.3.5a/` | The game, **kept unmodified**. Ours is the ChromieCraft download of the 3.3.5a client (enUS, 16.6 GB). This is the copy the server's map data is extracted from, so nothing gets installed into it. |
| `Reforged_3.3.5a/` | A second copy of the game, made 2026-10-10, for the [Project Reforged](#project-reforged-hd-graphics) HD graphics. Until Reforged is installed it is identical to the first. |

Both game folders are not in git, like everything else in this folder except our own files at the top.

## Setting up the client

1. Put the 3.3.5a client in a folder inside `Client/`, so that `Client/<folder>/Wow.exe` exists. Ours is `Client/ChromieCraft_3.3.5a/`.
2. Check the version: right-click `Wow.exe` > Properties > Details. It must say 3.3.5.12340.
3. Copy `Client/realmlist.wtf` over the client's own one. It is in `Data/enUS/realmlist.wtf` inside the game folder (the `enUS` folder is named after the client's language, for example `enGB` or `deDE`). A downloaded client usually points at someone else's server; ours pointed at `logon.chromiecraft.com` and was changed to `127.0.0.1` on 2026-10-10.
4. Delete the `Cache/` folder inside the game folder if there is one. Do this again whenever the game shows odd data after a server change.
5. Always start the game with `Wow.exe`, never `Launcher.exe` (the launcher tries to patch the game).

## Language

The **server** needs data extracted from an **enUS** (English US) client, because the bots look spells up by their English names. The client you *play* with can be any language.

## Extracting the map data (once)

The server needs maps and game tables taken out of the client. This is done once, and again only if `Data/` is lost.

1. Build the server first (`Build-Server.bat`), which also builds the extractors.
2. Run `Extract-Client-Data.bat` in the main folder. It uses `Client/ChromieCraft_3.3.5a/`; to use a client somewhere else, drag that game folder onto the `.bat` file. Always use an unmodified client, never the Reforged copy.
3. Wait. On our PC (8 cores) the first three steps took about 2 minutes and the last one (mmaps, the bots' pathfinding) about 11 minutes. On a slower PC the last step can take an hour or more.

The result lands in `Data/`: `dbc`, `maps`, `vmaps`, `mmaps` and `Cameras`, about 7 GB in total, most of it mmaps.

If a step fails, the manual steps are on the [AzerothCore wiki](https://www.azerothcore.org/wiki/server-setup).

## Project Reforged (HD graphics)

**Status (2026-10-10): the patch files are installed in `Reforged_3.3.5a/`. `Wow.exe` there is still the original: the EXE patcher, which Reforged calls mandatory, has not been run yet. DXVK and Lexara are not installed. Not tested in game yet.**

Installed: patches A (HD), B, C, D, E, G, I, M, N, P and S (standard), version 1.4.2, 19.3 GB together. The game copy is 36 GB with them. Left out on purpose: U (upright orcs).

[Project Reforged](https://projectreforged.github.io/wotlk/) is an HD overhaul of the 3.3.5a client: better character models, creatures, gear, buildings, environment, interface, spell effects and sound. It only changes the game client. The server needs no change, and the bots and the extracted map data are not affected. Version 1.4.2 (2026-09-18) at the time of writing.

### Why a second copy

Reforged replaces game files and requires a modified `Wow.exe`. The server's map data must come from an unmodified client, and the download we unpacked the game from is gone. So the original stays clean in `ChromieCraft_3.3.5a/` and Reforged goes into `Reforged_3.3.5a/`. If something breaks, delete `Reforged_3.3.5a/` and copy the original again.

Both copies log in to the same server with the same account. Game settings, key bindings and addons are per copy (the `WTF/` and `Interface/AddOns/` folders), so an addon has to be put in the copy you play with.

### The parts

Downloads: <https://projectreforged.github.io/wotlk/downloads/>

| Patch | What it changes | Size | Notes |
|-------|-----------------|-----:|-------|
| A | Character models, as an HD or an SD variant | 2.6 GB (HD) | Core. Pick one variant; we use HD. |
| C | Creatures | 2.3 GB | Core. Needs A. |
| G | Gear and weapons | 3.2 GB | Core |
| I | Interface | 0.4 GB | Core |
| B, D, E | Buildings, world objects, environment | 2.1 + 3.7 + 2.2 GB | One set: all three or none |
| M | Maps and loading screens | 2.8 GB | Optional |
| N | Darker nights | 3 MB | Optional. Remove `patch-N.mpq` if nights are too dark. |
| P | Spell effects | 80 MB | Optional |
| S | Sounds and music | 0.4 GB | Optional. The standard version needs M; there is a standalone version that does not. |
| U | Upright orcs | 7 MB | Optional. Needs A and C. Not installed: we do not want it. |

To remove a part again, close the game and delete its `patch-X.mpq` from `Client/Reforged_3.3.5a/Data/` (B, D and E only together; C and U need A).

Also on that page:

| Tool | Needed? | What it is |
|------|---------|------------|
| St0ny's AIO WoW EXE Patcher | Required by Reforged | A script that modifies `Wow.exe` (memory limits, graphics and more). There is a "Project Reforged Preset" for it. |
| DXVK | Strongly recommended by Reforged | A replacement `d3d9.dll` for the game folder, for better rendering performance. |
| Lexara HD Font Renderer | Optional | Sharper fonts. |

### Installing

Everything goes into `Client/Reforged_3.3.5a/`, never into `ChromieCraft_3.3.5a/`.

1. Download the patches you want. Start with the core ones (A, C, G, I).
2. Put the patch files in `Client/Reforged_3.3.5a/Data/`. That is where 3.3.5a patch files normally go; the Reforged site itself does not say, so check their Discord (linked on their site) if the game does not pick them up.
3. Patch `Wow.exe` with the EXE patcher, see [The EXE patcher](#the-exe-patcher) below.
4. Optionally add DXVK's `d3d9.dll` to `Client/Reforged_3.3.5a/`.
5. Delete `Client/Reforged_3.3.5a/Cache/` if it exists.
6. Start `Client/Reforged_3.3.5a/Wow.exe`.

### The EXE patcher

[St0ny's AIO WoW EXE Patcher](https://github.com/Raz0r1337/St0nys-AIO-WoW-EXE-Patcher) is a PowerShell script (`apply_patches.ps1`, started by `patcher-win.bat`) that changes bytes in `Wow.exe`.

How to run it:

1. Download the newest zip from the repo's [Releases](https://github.com/Raz0r1337/St0nys-AIO-WoW-EXE-Patcher/releases) page.
2. Unpack it and put `apply_patches.ps1` and `patcher-win.bat` in `Client/Reforged_3.3.5a/`, next to `Wow.exe`.
3. Close the game, then double-click `patcher-win.bat`.
4. Choose English, open the patch menu (`1`), press `R` for the **Reforged** preset, and confirm.

What we checked on 2026-10-10 by reading the script (version 2026.10.10.7), without running it:

- It makes no network connections, starts no other programs and does not touch the registry. It only reads and writes `Wow.exe` and two settings files of its own (`patcher_selection.ini`, `patcher_state.ini`) in the same folder.
- It refuses to patch anything but the original 3.3.5a `Wow.exe`, which it recognises by its SHA256 fingerprint. Ours matches.
- It keeps the original as `Wow.exe.ORI` and the previous version as `Wow.exe.BAK`. Its start menu can restore either (`3` = original).
- The Reforged preset is described as safe. It includes the loader for the Lexara font renderer, which does nothing unless Lexara's DLL is in the folder.
- The other preset, "St0nys_Wow.exe" (`S`), switches off the game's anti-cheat and adds movement changes. Harmless on our own server, but that `Wow.exe` must never be used on a public server.

What we did not check: the individual byte changes of each patch. The repo was two weeks old with 17 stars at the time, so treat it as a small hobby project, and re-read the script if you download a newer version.

### Things to know

- **The patcher and the DLL are programs from individual GitHub accounts, and the patch files come from an anonymous storage host.** We have not verified them. Windows Defender may flag a modified `Wow.exe` or the DLL.
- Some server modules need client patches of their own (new races, new battlegrounds, see [Modules.md](../Settings/Docs/Modules.md)). Those can clash with Reforged's files and have to be checked one by one.
- To go back to the plain game, just play with `ChromieCraft_3.3.5a/Wow.exe`.

## Game settings: where they live and how to reuse them

The game keeps its settings in the `WTF/` folder inside each game copy, in three layers:

| Layer | File or folder | What is in it | Shared by |
|-------|----------------|---------------|-----------|
| Game copy | `WTF/Config.wtf` | Graphics, resolution, sound, the realmlist | Every account that uses this game copy. Nothing to do for a new account. |
| Account | `WTF/Account/<ACCOUNT>/` | Key bindings, interface options, macros, addon settings | Every character on that account |
| Character | `WTF/Account/<ACCOUNT>/<Realm>/<Character>/` | Chat windows, per-character options, per-character key bindings if you ticked that box | That character only |

So graphics follow the game copy, and key bindings and options follow the account. A new **account** starts from defaults, and each of our two game copies has its own `WTF/`.

Not in these files: **action bars** (which spell is on which button). They are stored on the server per character and cannot be copied this way.

### Copy-Settings.bat

Close the game, then double-click `Client/Copy-Settings.bat`. It asks what to do:

| Choice | Use it when | What it copies |
|--------|-------------|----------------|
| 1. All settings from one game copy to the other | You set the game up in one copy and want the other to match | The whole `WTF/` folder: graphics, sound, every account and character |
| 2. A new account gets the settings of an existing account | Right after `account create`, **before** the first login with the new account | The account layer, into both game copies |
| 3. A new character gets the settings of an existing character | After creating the character, before logging in with it | The character layer, into both game copies |

For choice 2, create the account first, run the script, then log in. If you log in first, the game writes default settings for the new account; run the script again afterwards to overwrite them.

The game also keeps a copy of account and character settings on the server. For a brand-new account or character the server has none, so the copied files are the ones used.

Graphics note: `Config.wtf` from the Reforged copy also works in the plain copy, but the two look different, so you may want different graphics settings in each. Choice 1 makes them identical.

## Creating an account

Accounts are made in the **world server window**, not on a website. Type:

```
account create <name> <password>
account set gmlevel <name> 3 -1
```

The second line makes the account a game master (3 is the highest level, `-1` means on all realms). Then log in to the game with that name and password.

## Test characters

A ready-made high-level character for trying things (dungeon finder, loot, class changes) cannot be created from the server console, so this is how the first one was made on 2026-10-10. The server has to be **stopped** for the first part.

1. Take a spare level 1 bot character of the wanted race and class (the bots are ordinary characters on accounts named `RNDBOT...`) and move it to our account in the database: set `account`, `level`, `money` and a position in a city in `acore_characters.characters`, and set `at_login = 1` so the game asks for a new name at the first login.
2. Give it the class trainer's spells up to its level by copying them from `acore_world.trainer_spell` into `acore_characters.character_spell`. Ranks depend on the rank before, so this is repeated until nothing more is added. Spells that continue a talent are left out, because the talent is not chosen yet.
3. Raise its weapon and defense skills in `acore_characters.character_skills`.
4. Remove it from `acore_playerbots.playerbots_random_bots`, so the bot system forgets it.
5. Start the server and mail it gear and supplies from the world server window: `send items <name> <subject> <text> <item id> <item id>:<count> ...` (at most 12 stacks per mail).

The first one is an Undead mage, level 50, with 1000 gold. In its mailbox: a full set of blue cloth gear with staff and wand, four 18-slot bags, food, water and mage reagents. Its 41 talent points are unspent.

## Useful commands in game

| Command | What it does |
|---------|--------------|
| `.server debug` | Shows the server version and which modules are loaded. `mod-playerbots` should be listed. |
| `.playerbots rndbot stats` | How many random bots are online, per level, class and race. |
| `.playerbots bot add <name>` | Log in one of your own characters as a bot. |
| `/w <botname> follow` | Whisper a bot in your party to control it (`follow`, `stay`, `attack`, ...). |

All bot commands: [Playerbot Commands](https://github.com/mod-playerbots/mod-playerbots/wiki/Playerbot-Commands). All GM commands: [GM commands](https://www.azerothcore.org/wiki/gm-commands).

## Our own addons

The folder `AddOns` here holds addons that belong to this server. `Start-Server.bat` copies them into every game copy (`Interface\AddOns`) each time it starts, so you never install them by hand. If the game was open at that moment, type `/reload` in chat.

| Addon | What it does |
|---|---|
| `WowPlusSprint` | Hold Shift while running on foot or swimming to sprint. Shows a thin stamina bar below your character while stamina is not full; it turns red when you have run it empty. Hold Alt and drag the bar to move it, `/sprintbar reset` puts it back. The numbers are in `Settings/mod_wowplus.conf` (`WowPlus.Sprint.*`). |
| `WowPlusBots` | A small bar of buttons for your bots: Follow, Stay, Attack, Flee, Call and Dismiss (your companions), Away and Alone (have the area to yourself). Drag it by the word "Bots"; `/botbar` hides or shows it, `/botbar reset` puts it back. |
