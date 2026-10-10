# Settings

Everything we tweak lives here. The server never reads these files directly: `Start-Server.bat` applies them every time it starts, so the routine is always **edit here, restart the server**.

## What is in this folder

| File | What it controls |
|------|------------------|
| `worldserver.conf` | The game world: rates, rules, performance, folders, database connections. |
| `authserver.conf` | The login server. Rarely needs changing. |
| `playerbots.conf` | The bots: how many, their levels, how they behave. |
| `mysql.ini` | The database program: port, memory use, tuning for bots. |
| `.env.example` | Template for `.env`, which holds the database password. `.env` itself is not in git. |
| `Deploy-Settings.ps1` | The script that applies the three `.conf` files to the built server. |
| `SQL/` | Our own changes to game data. See [SQL/README.md](SQL/README.md). |

## How the .conf files work

Each `.conf` file here contains **only the lines we changed**. The full list of settings, with explanations, is in the default files that come with the server:

| Our file | Default file with every setting |
|----------|---------------------------------|
| `worldserver.conf` | `Server/src/server/apps/worldserver/worldserver.conf.dist` |
| `authserver.conf` | `Server/src/server/apps/authserver/authserver.conf.dist` |
| `playerbots.conf` | `Server/modules/mod-playerbots/conf/playerbots.conf.dist` |

To change a setting:

1. Find it in the default file and read its explanation.
2. Copy the line (`Name = value`) into our file here and change the value. Do not edit the default file.
3. Restart the server.

`Deploy-Settings.ps1` then takes the default file, replaces the lines we list, and writes the complete result to `Build/bin/RelWithDebInfo/configs/`. That way new settings from a server update arrive automatically and our changes stay on top. If it warns that a setting "is not in" the default file, the name is misspelled or the setting was renamed in an update.

Do not edit the files in `Build/.../configs/`: they are overwritten on every start.

## Common changes

| I want to... | File | Setting |
|--------------|------|---------|
| Have more or fewer bots | `playerbots.conf` | `Playerbots.MinRandomBots` and `Playerbots.MaxRandomBots` (keep them equal) |
| Change XP, drop or gold rates | `worldserver.conf` | `Rate.XP.Kill`, `Rate.Drop.Item.*`, `Rate.Drop.Money` |
| Use more CPU cores for the world | `worldserver.conf` | `MapUpdate.Threads` (cores minus 2, at most 8) |
| Give the database more memory | `mysql.ini` | `innodb_buffer_pool_size` |

More bots need more memory and CPU. Raise the count in steps (100, 200, 500) and watch how the server copes. The first start after raising it takes longer, because new bot accounts and characters are created.

Changes to `mysql.ini` need the database restarted, not just the server: run `Stop-Server.bat`, then `Start-Server.bat`.

## The database password

The servers log in to MySQL as the user `acore`. Its password is in `Settings/.env` and is filled in wherever a `.conf` file says `{DB_PASSWORD}`.

The MySQL administrator (`root`) has no password. That is acceptable only because the database accepts connections from this PC alone (`bind-address=127.0.0.1` in `mysql.ini`). Set a root password before ever changing that.

## Creating the database from scratch

Only needed on a new PC or after deleting `Database/`. Run from the main `WowPlus` folder, in a Command Prompt:

```
Tools\mysql\bin\mysqld --defaults-file=Settings\mysql.ini --basedir=Tools\mysql --datadir=Database --initialize-insecure --console
```

Start the database (leave this window open):

```
Tools\mysql\bin\mysqld --defaults-file=Settings\mysql.ini --basedir=Tools\mysql --datadir=Database --console
```

In a second window, open the MySQL prompt:

```
Tools\mysql\bin\mysql --defaults-file=Settings\mysql.ini
```

and run this, with the password from `Settings/.env` in place of `YOUR_PASSWORD`:

```sql
CREATE USER 'acore'@'localhost' IDENTIFIED BY 'YOUR_PASSWORD';
CREATE DATABASE acore_world      DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE acore_characters DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE acore_auth       DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE acore_playerbots DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_unicode_ci;
GRANT ALL PRIVILEGES ON acore_world.*      TO 'acore'@'localhost' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON acore_characters.* TO 'acore'@'localhost' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON acore_auth.*       TO 'acore'@'localhost' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON acore_playerbots.* TO 'acore'@'localhost' WITH GRANT OPTION;
FLUSH PRIVILEGES;
```

The databases start empty. The world server fills them itself on its first start.
