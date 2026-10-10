# SQL changes

Our own changes to the game data (items, spells, creatures, drops, quests) go here as `.sql` files, so they are in git and can be applied again after the world database is rebuilt.

## How to add one

1. Create a file in this folder. Start the name with the date so they stay in order, for example `2026-10-12_faster-mounts.sql`.
2. Write it so it can be run more than once without breaking (`UPDATE`, or `DELETE` followed by `INSERT`).
3. Stop the world server, then apply it:

   ```
   Tools\mysql\bin\mysql --defaults-file=Settings\mysql.ini acore_world < Settings\SQL\2026-10-12_faster-mounts.sql
   ```

4. Start the server again and commit the file.

The databases are `acore_world` (game data), `acore_characters` (characters), `acore_auth` (accounts) and `acore_playerbots` (bot data).

## Rules

- Do not edit the SQL files under `Server/data/sql/base/`, `archive/` or `updates/db_*/`. Those belong to AzerothCore and an update would overwrite or collide with the change.
- Take a backup first (`Backup-Database.bat`) before a change you are not sure about.
