# Client

The game client we play with: World of Warcraft **3.3.5a, build 12340**. The game files are not in this repo (large, and not ours to redistribute). You supply the client yourself.

## What is in this folder

| Item | What it is |
|------|------------|
| `realmlist.wtf` | Tells the client to log in to our server on this PC (`127.0.0.1`). |
| `ChromieCraft_3.3.5a/` | The game. Ours is the ChromieCraft download of the 3.3.5a client (enUS, 16.6 GB). Not in git, like everything else in this folder except the two files above. |

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
2. Run `Extract-Client-Data.bat` in the main folder. It finds the game folder inside `Client/` by itself; to use a client somewhere else, drag that game folder onto the `.bat` file.
3. Wait. On our PC (8 cores) the first three steps took about 2 minutes and the last one (mmaps, the bots' pathfinding) about 11 minutes. On a slower PC the last step can take an hour or more.

The result lands in `Data/`: `dbc`, `maps`, `vmaps`, `mmaps` and `Cameras`, about 7 GB in total, most of it mmaps.

If a step fails, the manual steps are on the [AzerothCore wiki](https://www.azerothcore.org/wiki/server-setup).

## Creating an account

Accounts are made in the **world server window**, not on a website. Type:

```
account create <name> <password>
account set gmlevel <name> 3 -1
```

The second line makes the account a game master (3 is the highest level, `-1` means on all realms). Then log in to the game with that name and password.

## Useful commands in game

| Command | What it does |
|---------|--------------|
| `.server debug` | Shows the server version and which modules are loaded. `mod-playerbots` should be listed. |
| `.playerbots rndbot stats` | How many random bots are online, per level, class and race. |
| `.playerbots bot add <name>` | Log in one of your own characters as a bot. |
| `/w <botname> follow` | Whisper a bot in your party to control it (`follow`, `stay`, `attack`, ...). |

All bot commands: [Playerbot Commands](https://github.com/mod-playerbots/mod-playerbots/wiki/Playerbot-Commands). All GM commands: [GM commands](https://www.azerothcore.org/wiki/gm-commands).
