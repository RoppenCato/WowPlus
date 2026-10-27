# WowPlus

Our own private World of Warcraft 3.3.5a (Wrath of the Lich King) server, with AI bots that fill the world. Runs locally for now.

It is built on [AzerothCore](https://www.azerothcore.org/) with the [Playerbots](https://github.com/mod-playerbots/mod-playerbots) module.

## Folders

| Folder | What it is |
|--------|------------|
| [Server](Server/) | The server code: the AzerothCore Playerbots fork, with the `mod-playerbots` module inside `Server/modules/`. See [Server/README.md](Server/README.md). |
| [Client](Client/) | How to set up the game client and point it at this server. The game itself is not in the repo. See [Client/README.md](Client/README.md). |
| [Settings](Settings/) | Everything we tweak: server settings, bot settings, database settings, our own SQL changes. See [Settings/README.md](Settings/README.md). |
| `Tools/` | MySQL 8.4, CMake, Boost and OpenSSL, unpacked here instead of spread over Windows. |
| `Build/` | The compiled server. Made by `Build-Server.bat`; can be deleted and rebuilt. |
| `Data/` | Map data extracted from the game client (dbc, maps, vmaps, mmaps). |
| `Database/` | The MySQL data: accounts, characters, the world. This is the save file, so back it up. |
| `Backups/` | Database backups made by `Backup-Database.bat`. |

The last five are not in git (see [.gitignore](.gitignore)). `Tools/`, `Build/` and `Data/` can be recreated; `Database/` cannot.

## Where to change things

- **Server, bot and database settings:** edit the files in [Settings](Settings/), then restart the server. The guide is [Settings/README.md](Settings/README.md).
- **Number of bots:** `Playerbots.MinRandomBots` and `Playerbots.MaxRandomBots` in [Settings/playerbots.conf](Settings/playerbots.conf).
- **Server code (C++):** edit files under `Server/`, then run `Build-Server.bat`.
- **Game data (items, spells, creatures, drops):** SQL files, see [Settings/SQL/README.md](Settings/SQL/README.md).

## Documentation

| Document | What it is |
|----------|------------|
| [Server/README.md](Server/README.md) | Where the code comes from, the tools, building, changing code, updating. |
| [Client/README.md](Client/README.md) | Setting up the game client, extracting the map data, creating an account. |
| [Settings/README.md](Settings/README.md) | How the settings files work and the common changes. |
| [Plan](Settings/Docs/Plan.md) | Start here for where the project is going: a single-player WoW with bots. The ideas, what already exists for each, what needs new code, the suggested order and the open decisions. |
| [Implementation list](Settings/Docs/Implementation-List.md) | The plan as ordered steps, ticked off as we go, with what was decided per phase and which modules are added. |
| [Class changes](Settings/Docs/Class-Changes.md) | When each class gets its abilities today, which key abilities come late or only from quests, and the idea of a milestone every ten levels. Discussion, nothing changed yet. |
| [Ideas](Settings/Docs/Ideas.md) | Things we might do. First entry: an "adventure switch" that brings back mystery for the one who designed it all. |
| [Reference lists](Settings/Reference/README.md) | Lists of what is in the game. So far: every rare enemy with its level and zone. |
| [Modules](Settings/Docs/Modules.md) | What can be added: about 150 community modules by area (bots, solo play, classes, items, comfort, world), with a shortlist for us. None installed yet. |
| [The changes page](docs/README.md) | Everything this project changes in the game, on one web page with search and filters: `docs/index.html`. Can be put on the web with GitHub Pages. |
| [Bots](Settings/Docs/Bots.md) | How the bots work, what we changed (levels, gear, buffs, sending them away, your guild), and what is still open. |
| [Housing](Settings/Docs/Housing.md) | Design for a room of your own in the cities: what the game allows, what it would give, what would happen there. Not built. |
| [The Guide](Settings/Docs/Guide.md) | The in-game book of dungeons, raids, champions and zone lore (`/guide`). |
| [Bot chat](Settings/Docs/Bot-Chat.md) | Talking to bots through Claude: who answers, who each bot is, what they remember. Its window opens with `Start-Server.bat`. |

## Starting and stopping

| Script | What it does |
|--------|--------------|
| `Start-Server.bat` | Starts the database if it is not running, applies the files in `Settings/`, then starts the login server (minimised) and the world server. |
| `Stop-Server.bat` | Waits for the world server to be shut down, then stops the login server and the database cleanly. |
| `Build-Server.bat` | Compiles the server after code changes. The first build takes a long time; later builds only redo what changed. |
| `Update-Server.bat` | Fetches the newest core and playerbots module together, merges them into `Server/` and rebuilds. |
| `Backup-Database.bat` | Saves a copy of all four databases to a dated folder in `Backups/`. |
| `Extract-Client-Data.bat` | One-time: extracts the map data the server needs from the game client into `Data/`. |

The world server is ready when its window says `WORLD: World Initialized`. To stop it, type `server shutdown 5` in that window, which saves everything first. Do not just close the window.

Ports, all only reachable from this PC: 3724 (login), 8085 (world), 3307 (database).

## Quick start on a new PC

1. Clone this repo.
2. Install **Visual Studio Build Tools 2022** with the workload "Desktop development with C++".
3. Fill `Tools/` as described in [Server/README.md](Server/README.md#tools).
4. Copy `Settings/.env.example` to `Settings/.env` and choose a database password.
5. Create the database: [Settings/README.md](Settings/README.md#creating-the-database-from-scratch).
6. Run `Build-Server.bat`.
7. Put a 3.3.5a client in a folder inside `Client/` and run `Extract-Client-Data.bat`: [Client/README.md](Client/README.md).
8. Run `Start-Server.bat`. The first start fills the databases and creates the bots, which takes several minutes.
9. Create your account in the world server window: [Client/README.md](Client/README.md#creating-an-account).

## Troubleshooting

Problems we ran into, and ones the guides warn about.

| What you see | Cause | Fix |
|--------------|-------|-----|
| `Access denied for user 'acore'@'localhost' (using password: YES)` when a server starts | The password MySQL has for `acore` is not the one in `Settings/.env`. Happened during setup. | Open the MySQL prompt (`Tools\mysql\bin\mysql --defaults-file=Settings\mysql.ini`) and run `ALTER USER 'acore'@'localhost' IDENTIFIED BY 'the password from .env';` |
| The realm list shows the realm as **Offline** although the world server is running | The login server was started (or restarted) after the world server. It marks every realm offline when it starts, and the world server only reports itself online again at its next check-in. Happened on our first login. | Wait a minute and log in again (`UpdateUptimeInterval = 1` in `Settings/worldserver.conf`). `Start-Server.bat` starts the login server first so it does not happen there. At once, by hand: `UPDATE acore_auth.realmlist SET flag = 0;` |
| The login server window closes at once; `logs/Auth.log` says `No valid realms specified` | The world server was closed without a clean shutdown (or stopped halfway through starting), which leaves the realm marked as offline. Happened after our first trial start. | `Start-Server.bat` now clears the mark on every start. By hand: `UPDATE acore_auth.realmlist SET flag = 0;` in the MySQL prompt, then start `authserver.exe` again. |
| `'worldserver.exe' is not recognized as an internal or external command` although the file exists | The window it was started from does not run programs from the current folder. | Start through `Start-Server.bat`, which uses full paths. |
| The world server stops right after the database part, complaining about `dbc`, maps or `DataDir` | The map data in `Data/` is missing. | Run `Extract-Client-Data.bat`, see [Client/README.md](Client/README.md). |
| `Table 'playerbots_random_bots' doesn't exist` or `Unknown database 'acore_playerbots'` | The bots' database was not created or not filled. | Create it as in [Settings/README.md](Settings/README.md#creating-the-database-from-scratch). If the tables stay missing, import the files in `Server/modules/mod-playerbots/data/sql/playerbots/base/` into `acore_playerbots` by hand. |
| Hundreds of compile errors in `mod-playerbots` | The core is not the Playerbots fork, or the core and the module are from different points in time. | Use `Update-Server.bat`, which always updates both together. |
| Compile errors mentioning Boost | Boost 1.87 or newer. | Use Boost 1.78 to 1.83; we use 1.81. |
| The compiler runs out of memory | Too many files compiled at once. | Lower `--parallel 4` in `Build-Server.bat`. |
| CMake warnings about `CMP0167`, `BOOST_ROOT` or "deprecated" during the build | CMake 4 is newer than the project expects. | Harmless, ignore. |
| GitHub warns about a file larger than 50 MB when pushing | `playerbots_travelnode_path.sql` in the module is 66 MB. | Harmless. The hard limit is 100 MB. |
| A setting in `Settings/` seems to have no effect | It was changed in `Build/.../configs/` (overwritten on every start), or the name is misspelled. | Edit the file in `Settings/` and watch for a warning from `Start-Server.bat`. |
| The game shows old or wrong data after a server change | The client's cache. | Close the game and delete the `Cache/` folder inside the game folder. |

More: [AzerothCore common errors](https://www.azerothcore.org/wiki/common-errors) and [Playerbots troubleshooting](https://github.com/mod-playerbots/mod-playerbots/wiki/Troubleshooting).

## Backups

| What | How |
|------|-----|
| Code, settings and docs | Git. The whole `WowPlus` folder is one repo; our GitHub repo is the remote `origin`. |
| Database (accounts, characters) | Run `Backup-Database.bat`. The backups stay on this PC and are not sent to GitHub. |

## The original repos

The server code is merged in from two repos, with their history. Both are linked as remotes with pushing disabled, so nothing here can end up there.

| Remote | Repo | Branch | Where it lives here |
|--------|------|--------|---------------------|
| `upstream-core` | [mod-playerbots/azerothcore-wotlk](https://github.com/mod-playerbots/azerothcore-wotlk) | `Playerbot` | `Server/` |
| `upstream-playerbots` | [mod-playerbots/mod-playerbots](https://github.com/mod-playerbots/mod-playerbots) | `master` | `Server/modules/mod-playerbots/` |

`Update-Server.bat` pulls in their updates. Details are in [Server/README.md](Server/README.md#updating).
