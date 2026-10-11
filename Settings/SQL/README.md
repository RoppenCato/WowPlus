# SQL changes

Our own changes to the game data (items, spells, creatures, drops, quests, entry requirements) go here as `.sql` files, so they are in git and are applied again by themselves after the database is rebuilt or the server is updated.

## How it works

| Folder | Applied to database | Holds |
|--------|---------------------|-------|
| `world/` | `acore_world` | Game data: items, creatures, loot, quests, vendors, dungeon entry rules |
| `characters/` | `acore_characters` | Changes to characters. Rarely needed. |
| `auth/` | `acore_auth` | Changes to accounts and the realm. Rarely needed. |

`Start-Server.bat` applies **every file in these folders on every start**, in name order, before the servers start. That is why each file must be safe to run again and again.

## Adding a change

1. Create a file in the right folder. Start the name with the date so they stay in order, for example `world/2026-10-12_faster-mounts.sql`.
2. Write it so that running it twice gives the same result as running it once:
   - `UPDATE ... SET x = 5 WHERE ...` is fine.
   - `UPDATE ... SET x = x * 2` is **not**: it doubles again on every start.
   - For new rows, `DELETE` the row first and then `INSERT` it, or use `REPLACE INTO`.
3. Put a comment at the top saying what it does, why, and how to undo it.
4. Restart the server with `Start-Server.bat`. It prints `Applied SQL\world\<file>` for each file, or a warning if one failed.
5. Commit the file.

To see the error of a failing file, run it by hand from the main folder:

```
Tools\mysql\bin\mysql --defaults-file=Settings\mysql.ini acore_world < Settings\SQL\world\2026-10-12_faster-mounts.sql
```

## Undoing a change

Deleting the file does not undo it: the database keeps what the file did. Write a new file that puts the old values back (the originals are in `Server/data/sql/base/`), or restore a backup.

## Rules

- Do not edit the SQL files under `Server/data/sql/base/`, `archive/` or `updates/db_*/`. Those belong to AzerothCore and an update would overwrite or collide with the change.
- Take a backup first (`Backup-Database.bat`) before a change you are not sure about.

## What is here

| File | What it does |
|------|--------------|
| `world/2026-10-10_remove-attunements.sql` | Removes all 35 entry requirements for dungeons and raids (quests, heroic keys, achievements). Level requirements stay. |
| `world/2026-10-10_equipment-never-binds.sql` | Weapons and armour no longer bind, neither on pickup nor when equipped. |
| `world/2026-10-10_hearthstone-menu.sql` | Hands the hearthstone item to our travel menu. |
| `world/2026-10-10_hub.sql` | The hub on GM Island: its NPCs, the weapon master, every vendor list, the menu that leads to them, and the prices. |
| `world/2026-10-11_class-quest-abilities.sql` | The class trainers teach what the class quests used to: stances, Tame Beast, poisons, totems, demons, Bear Form and the rest. |
