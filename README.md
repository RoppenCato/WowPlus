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
| [Bot chat](Settings/Docs/Bot-Chat.md) | Design: talking to bots through Claude, like in MapleStoryPlus. Not built yet. |

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
7. Put a 3.3.5a client in `Client/WoW/` and run `Extract-Client-Data.bat`: [Client/README.md](Client/README.md).
8. Run `Start-Server.bat`. The first start fills the databases and creates the bots, which takes several minutes.
9. Create your account in the world server window: [Client/README.md](Client/README.md#creating-an-account).

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
