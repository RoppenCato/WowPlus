# Server

The server code. This folder is a copy of two projects, merged in with their history:

| What | From | Branch | Where |
|------|------|--------|-------|
| The core (the game server itself) | [mod-playerbots/azerothcore-wotlk](https://github.com/mod-playerbots/azerothcore-wotlk) | `Playerbot` | this folder |
| The bots | [mod-playerbots/mod-playerbots](https://github.com/mod-playerbots/mod-playerbots) | `master` | `modules/mod-playerbots/` |

The core is **not** standard AzerothCore. The Playerbots module only works with this fork; building it against the normal AzerothCore repo gives hundreds of compile errors.

## Why the source is merged in, not linked

We chose to merge both repos into this one (a "subtree merge"), instead of git submodules or clones that git ignores, because we plan to change a lot of the code:

- Our changes to the core, the bots, the settings and the docs are all committed and pushed in one place, with one history.
- A fresh clone of WowPlus has everything needed to build. No second step, no forks to keep in sync.
- Upstream updates still come in with a normal merge, and git shows exactly where they collide with our changes.

The price is a large repo (the core has about 1.3 GB of history) and that updates can produce merge conflicts in files we have edited.

## Where things are

| Folder | What it is |
|--------|------------|
| `src/server/game/` | The core gameplay code. |
| `src/server/scripts/` | Bosses, spells, commands. Class spells are in `scripts/Spells/spell_<class>.cpp`. |
| `modules/mod-playerbots/src/` | The bot code. Class behaviour is in `Ai/Class/`. |
| `modules/mod-playerbots/conf/` | The bots' default settings. Ours go in `Settings/playerbots.conf`. |
| `data/sql/` | The database content. Do not edit `base/`, `archive/` or `updates/db_*/`; our own SQL goes in `Settings/SQL/`. |
| `src/server/apps/*/` | The default settings files (`*.conf.dist`). Ours go in `Settings/`. |

`AGENTS.md` and `.agents/docs/` hold the upstream coding rules. Read them before changing C++ or SQL.

## Tools

The build needs these. Only the first is installed on Windows; the rest are unpacked into `Tools/` in the main folder, which is not in git.

| Tool | Version we use | Where | How to get it |
|------|----------------|-------|---------------|
| Visual Studio Build Tools 2022 | 17.x, MSVC 14.44 | Installed on Windows | [vs_buildtools.exe](https://aka.ms/vs/17/release/vs_buildtools.exe), workload "Desktop development with C++" |
| MySQL | 8.4.11 LTS | `Tools/mysql` | The "Windows (x86, 64-bit), ZIP Archive" from [dev.mysql.com](https://dev.mysql.com/downloads/mysql/8.4.html), unpacked |
| CMake | 4.4.4 | `Tools/cmake` | The `windows-x86_64.zip` from [cmake.org](https://cmake.org/download/), unpacked |
| Boost | 1.81.0 | `Tools/boost` | `boost_1_81_0-msvc-14.3-64.exe` from [SourceForge](https://sourceforge.net/projects/boost/files/boost-binaries/1.81.0/), installed into that folder |
| OpenSSL | 3.5.9 | `Tools/openssl` | "Win64 OpenSSL v3.5.x" (the full one, not Light) from [slproweb.com](https://slproweb.com/products/Win32OpenSSL.html), installed into that folder |

Version limits to respect:

- **MySQL:** 8.4 LTS. MariaDB, MySQL 5.7, 8.1 and the newer yearly releases are not supported.
- **Boost:** 1.78 to 1.83. Version 1.87 and later break the build.
- **OpenSSL:** 3.x.

Each folder must directly contain the tool, for example `Tools/cmake/bin/cmake.exe`, `Tools/boost/boost/version.hpp`, `Tools/openssl/include/`, `Tools/mysql/bin/mysqld.exe`.

## Building

Run `Build-Server.bat` in the main folder. It:

1. Configures the build with CMake, pointing it at the tools in `Tools/`. Output goes to `Build/`.
2. Compiles everything in `RelWithDebInfo` mode: the two servers, the bots, and the extractors for the client data.
3. Copies the MySQL and OpenSSL `.dll` files next to the programs.

The result is in `Build/bin/RelWithDebInfo/`.

The first build compiles several thousand files and takes a long time. After that, only changed files are recompiled. The world server must be stopped while building, because Windows locks a running `.exe`.

To start completely fresh, delete the `Build/` folder and build again.

## Changing code

1. Stop the server (`Stop-Server.bat`).
2. Edit files under `Server/`.
3. Run `Build-Server.bat`.
4. Start the server and test.
5. Commit.

New files under `modules/mod-playerbots/` are picked up automatically. After adding or removing source files, the configure step at the start of the build notices it.

## Updating

Run `Update-Server.bat` in the main folder. It needs all our changes to be committed first. It then:

1. Fetches the newest `Playerbot` branch of the core and `master` of the module.
2. Merges both into `Server/`. They are always updated **together**: the core and the module are developed in step, and a new module on an old core (or the reverse) usually does not compile.
3. Rebuilds.

On the next start the world server applies any new database updates itself.

By hand, the same thing is:

```
git fetch upstream-core Playerbot
git fetch upstream-playerbots master
git merge -X subtree=Server upstream-core/Playerbot
git merge -X subtree=Server/modules/mod-playerbots upstream-playerbots/master
```

If a merge reports conflicts, upstream changed the same lines we did. Fix the listed files and commit, or run `git merge --abort` to go back to how it was.

Take a backup first (`Backup-Database.bat`): database updates cannot be undone by going back to older code.

## Adding other modules

Not done yet. In short: put the module in `modules/<name>/`, add `!/modules/<name>` to `Server/.gitignore` (it ignores unknown modules), rebuild, and add its settings file to `Settings/` and to the list in `Settings/Deploy-Settings.ps1`. The fork can lag behind standard AzerothCore, so a module written for the newest AzerothCore may need an older version of itself. Always read the module's own README.
