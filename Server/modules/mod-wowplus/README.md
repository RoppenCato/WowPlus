# mod-wowplus

Our own changes to the game, as one module. Each feature is behind a setting, so it can be switched off without removing code. Keeping our code here, instead of editing the core or `mod-playerbots`, is what lets `Update-Server.bat` keep merging upstream changes cleanly.

The reasons and decisions behind each feature are in [Settings/Docs/Plan.md](../../../Settings/Docs/Plan.md) and [Settings/Docs/Implementation-List.md](../../../Settings/Docs/Implementation-List.md).

## Features

| Feature | File | Settings |
|---------|------|----------|
| Extra loot: equipment for the player's level from any kill or chest, plus a mount table and a pet table | `src/WowPlusLoot.cpp` | `WowPlus.Loot.*` |
| Random bonuses on looted, crafted and quest-reward equipment; strength follows item level, colour limits how many; crafted items always get one | `src/WowPlusBonuses.cpp` | `WowPlus.Bonus.*` |
| Gathering gives a multiple of the normal amount | `src/WowPlusProfessions.cpp` | `WowPlus.Professions.*` |
| Hearthstone without cooldown, and a hearthstone menu to the cities, towns, dungeons and raids a character has visited | `src/WowPlusTravel.cpp` | `WowPlus.Travel.*` |
| Not built yet, files in place: surprise vendors, the hub, gold | `src/WowPlusVendors.cpp`, `WowPlusHub.cpp`, `WowPlusGold.cpp` | |

## When a rebuild is needed

| Change | What to do |
|--------|------------|
| A number or switch in a `.conf` file | Restart the server. No rebuild. |
| A file in `Settings/SQL/` | Restart the server. No rebuild. |
| Code in an existing `.cpp` file | `Build-Server.bat`, a few minutes: only that file is recompiled. |
| A new `.cpp` file, or a new module | `Build-Server.bat`, ten minutes or more: every module is recompiled. This is why the files for planned features already exist as empty stubs. |

So decisions are collected and built together, and tuning is done in the settings.

## Changing settings

The defaults, with an explanation of every setting, are in `conf/mod_wowplus.conf.dist`. To change one, put the line in `Settings/mod_wowplus.conf` (create the file if it is not there) and restart the server.

## How the extra loot works

1. At startup the module builds its item pools from the item data: weapons and armour by required level and rarity, all mounts, all companion pets. Left out are quest starters, PvP sets, temporary items, and items that nothing in the game hands out (never dropped, sold, crafted or given by a quest), which removes leftovers and test items. The count is printed in the server log.
2. Whenever the server has built the normal loot for a killed creature or an opened chest, the module rolls on top of it: once for equipment, once for a mount, once for a pet.
3. Who gets it: the human player. If a bot got the kill, the human in its group nearby. Bots on their own get nothing extra.
4. Alone, the item is added to the corpse or chest. In a group it is put straight into the player's bags with the usual loot message (or mailed if the bags are full).

## Adding a feature

1. Add a `.cpp` file in `src/` with a function `void AddSC_<Name>()` that creates its script objects.
2. Call it from `src/mod_wowplus_loader.cpp`.
3. Add its settings, with explanations, to `conf/mod_wowplus.conf.dist`, and read them once in a config-load hook, not on every use.
4. Run `Build-Server.bat`.

The coding rules of the core apply: `Server/.agents/docs/cpp-guidelines.md`.
