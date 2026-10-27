"""Works out what every dungeon and raid boss drops on this server, and writes it down twice:
for the game (the table acore_world.wowplus_boss_loot) and for the guide addon
(Client/AddOns/WowPlusGuide/Loot.lua). Run by Settings/Deploy-Settings.ps1 at every server start;
running it by hand is harmless.

    python Settings/Scripts/Make-Boss-Loot.py            make both
    python Settings/Scripts/Make-Boss-Loot.py --boss 644 print one boss (by creature number or part of the name)

Every boss keeps the game's own loot. On top of that it has "new" drops, found nowhere on that boss
in the real game: about one item aimed at each class, taken from elsewhere in the game at the same
quality and item level as the boss's own loot. Which items is decided by WowPlus.World.Seed, so it is
the same at every start and different in another world. How often they drop is a setting of the game
(WowPlus.BossLoot.*). Explained in Settings/Docs/Boss-Loot.md.
"""
import argparse
import io
import os
import random
import re
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MYSQL = os.path.join(ROOT, "Tools", "mysql", "bin", "mysql.exe")
DBC = os.path.join(ROOT, "Data", "dbc")
BOOK = os.path.join(ROOT, "Client", "AddOns", "WowPlusGuide", "Loot.lua")

ITEM_WEAPON, ITEM_ARMOR, ITEM_RECIPE = 2, 4, 9
UNCOMMON, RARE, EPIC = 2, 3, 4
MAX_REFERENCE_DEPTH = 4
MIN_CHANCE_SHOWN = 0.05             # percent; anything rarer is left out of the book

CLASSES = {1: "Warrior", 2: "Paladin", 3: "Hunter", 4: "Rogue", 5: "Priest", 6: "Death Knight", 7: "Shaman", 8: "Mage",
           9: "Warlock", 11: "Druid"}
DEATH_KNIGHT, DEATH_KNIGHT_FIRST_LEVEL = 6, 55

# Armour a class wears from level 40 on, and before (item_template.subclass: 1 cloth, 2 leather, 3 mail, 4 plate)
ARMOUR = {1: (4, 3), 2: (4, 3), 6: (4, 4), 3: (3, 2), 7: (3, 2), 4: (2, 2), 11: (2, 2), 5: (1, 1), 8: (1, 1), 9: (1, 1)}
ARMOUR_SLOTS = {1, 3, 5, 6, 7, 8, 9, 10, 20}       # head, shoulders, chest, waist, legs, feet, wrists, hands, robe
# Weapons a class can use (item_template.subclass of class 2)
WEAPONS = {1: {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 13, 15, 16, 18}, 2: {0, 1, 4, 5, 6, 7, 8}, 3: {0, 1, 2, 3, 6, 7, 8, 10, 13, 15, 18},
           4: {0, 2, 3, 4, 7, 13, 15, 16, 18}, 5: {4, 10, 15, 19}, 6: {0, 1, 4, 5, 6, 7, 8}, 7: {0, 1, 4, 5, 10, 13, 15},
           8: {7, 10, 15, 19}, 9: {7, 10, 15, 19}, 11: {4, 5, 6, 10, 13, 15}}
SHIELD_USERS, SHIELD = {1, 2, 7}, 6
RELICS = {7: 2, 8: 11, 9: 7, 10: 6}                # libram, idol, totem, sigil -> the class it is for
# The stats a class is after (item_template.stat_type: 3 agility, 4 strength, 5 intellect, 6 spirit, 45 spell power, 38 attack power)
STRENGTH, AGILITY, CASTER = {4}, {3}, {5, 6, 45, 43}
WANTS = {1: STRENGTH, 6: STRENGTH, 2: STRENGTH | CASTER, 3: AGILITY, 4: AGILITY, 7: AGILITY | CASTER, 11: AGILITY | CASTER,
         5: CASTER, 8: CASTER, 9: CASTER}
TELLING = STRENGTH | AGILITY | CASTER


def sql(query, database="acore_world"):
    out = subprocess.run([MYSQL, "-uroot", "-P3307", "--protocol=TCP", "-hlocalhost", "-N", "-B", "--default-character-set=utf8mb4", database, "-e", query],
                         capture_output=True, text=True, encoding="utf-8", errors="replace")
    if out.returncode != 0:
        raise RuntimeError(out.stderr.strip()[:300])
    return [line.split("\t") for line in out.stdout.splitlines()]


def run_sql(script):
    out = subprocess.run([MYSQL, "-uroot", "-P3307", "--protocol=TCP", "-hlocalhost", "--default-character-set=utf8mb4", "acore_world"],
                         input=script, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if out.returncode != 0:
        raise RuntimeError(out.stderr.strip()[:300])


def dbc(name):
    """The rows of a client data file as lists of numbers, with a function to read a string field."""
    with open(os.path.join(DBC, name), "rb") as f:
        data = f.read()
    magic, records, fields, size, strings = struct.unpack_from("<4sIIII", data)
    if magic != b"WDBC":
        raise RuntimeError(name + " is not a DBC file")
    block = data[20 + records * size:]
    rows = [struct.unpack_from("<%dI" % fields, data, 20 + i * size) for i in range(records)]

    def text(offset):
        end = block.find(b"\0", offset)
        return block[offset:end].decode("utf-8", "replace")
    return rows, text


def world_seed():
    for path in (os.path.join(ROOT, "Settings", "mod_wowplus.conf"), os.path.join(ROOT, "Server", "modules", "mod-wowplus", "conf", "mod_wowplus.conf.dist")):
        try:
            with io.open(path, encoding="utf-8", errors="replace") as f:
                for line in f:
                    found = re.match(r"\s*WowPlus\.World\.Seed\s*=\s*(\d+)", line)
                    if found:
                        return int(found.group(1))
        except OSError:
            pass
    return 1


# ---------------------------------------------------------------------------------------------
# The game's own loot
# ---------------------------------------------------------------------------------------------

def load_loot(table):
    """entry -> rows (item, reference, chance, group, max_count) of one loot table."""
    loot = {}
    for entry, item, reference, chance, group, most in sql(f"SELECT Entry, Item, Reference, Chance, GroupId, MaxCount FROM {table} WHERE QuestRequired = 0"):
        loot.setdefault(int(entry), []).append((int(item), int(reference), float(chance), int(group), int(most)))
    return loot


def flatten(rows, references, depth=0):
    """item -> the chance in percent, roughly, that it is in the loot. Groups and references are followed."""
    chances = {}
    groups = {}
    for row in rows:
        groups.setdefault(row[3], []).append(row)
    for group, members in groups.items():
        named = sum(chance for _, _, chance, _, _ in members if chance > 0)
        unnamed = [m for m in members if m[2] <= 0]
        share = max(0.0, 100.0 - named) / len(unnamed) if unnamed and group else 0.0
        for item, reference, chance, _, most in members:
            chance = chance if chance > 0 else (share if group else 100.0)
            if reference:
                if depth >= MAX_REFERENCE_DEPTH:
                    continue
                for inner, inner_chance in flatten(references.get(reference, []), references, depth + 1).items():
                    together = min(100.0, inner_chance * chance / 100.0 * max(1, most))
                    chances[inner] = min(100.0, chances.get(inner, 0.0) + together)
            else:
                chances[item] = min(100.0, chances.get(item, 0.0) + chance)
    return chances


# ---------------------------------------------------------------------------------------------
# Who an item is for
# ---------------------------------------------------------------------------------------------

def suits(item, cls):
    """Would this class be glad of this item? A rough reading of armour type, weapon type and stats."""
    if item["allowed"] not in (-1, 0) and not item["allowed"] & (1 << (cls - 1)):
        return False
    if item["allowed"] not in (-1, 0) and bin(item["allowed"] & 0x5FF).count("1") == 1:
        return True                                     # made for this class alone
    if cls == DEATH_KNIGHT and item["need"] < DEATH_KNIGHT_FIRST_LEVEL:
        return False

    if item["class"] == ITEM_ARMOR:
        if item["subclass"] in RELICS:
            return RELICS[item["subclass"]] == cls
        if item["subclass"] == SHIELD:
            if cls not in SHIELD_USERS:
                return False
        elif item["slot"] in ARMOUR_SLOTS:
            if item["subclass"] != ARMOUR[cls][0 if item["need"] >= 40 else 1]:
                return False
    elif item["subclass"] not in WEAPONS[cls]:
        return False

    telling = item["stats"] & TELLING
    return not telling or bool(telling & WANTS[cls])


# ---------------------------------------------------------------------------------------------

def main():
    p = argparse.ArgumentParser(description="What bosses drop on this server")
    p.add_argument("--boss", help="print one boss instead of writing anything: creature number or part of the name")
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    started = time.time()
    seed = world_seed()

    # Items
    stat_columns = ", ".join(f"stat_type{i}, stat_value{i}" for i in range(1, 11))
    items = {}
    for row in sql(f"SELECT entry, class, subclass, name, Quality, ItemLevel, RequiredLevel, InventoryType, AllowableClass, {stat_columns} "
                   f"FROM item_template WHERE Quality >= {UNCOMMON} OR class = {ITEM_RECIPE}"):
        stats = {int(row[9 + 2 * i]) for i in range(10) if int(row[10 + 2 * i]) != 0}
        items[int(row[0])] = {"id": int(row[0]), "class": int(row[1]), "subclass": int(row[2]), "name": row[3], "quality": int(row[4]),
                              "level": int(row[5]), "need": int(row[6]), "slot": int(row[7]), "allowed": int(row[8]), "stats": stats}

    creature_loot = load_loot("creature_loot_template")
    references = load_loot("reference_loot_template")
    chest_loot = load_loot("gameobject_loot_template")

    # Everything that drops somewhere: where the new drops are taken from
    dropping = set()
    for table in (creature_loot, references, chest_loot):
        for rows in table.values():
            dropping.update(item for item, reference, _, _, _ in rows if not reference)
    pool = [item for item in items.values() if item["id"] in dropping and item["class"] in (ITEM_WEAPON, ITEM_ARMOR)
            and RARE <= item["quality"] <= EPIC and item["level"] > 0 and item["slot"] and not re.search(r"test|deprecated|monster -|\bold\b|gladiator", item["name"], re.I)]
    pool.sort(key=lambda item: item["id"])
    shelves = {}                                        # (quality, item level) -> items, to find candidates quickly
    for item in pool:
        shelves.setdefault((item["quality"], item["level"]), []).append(item)

    # Bosses: the creatures the game counts as dungeon and raid encounters
    templates = {int(r[0]): {"entry": int(r[0]), "name": r[1], "level": int(r[2]), "loot": int(r[3]), "modes": [int(x) for x in r[4:7]]}
                 for r in sql("SELECT entry, name, maxlevel, lootid, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3 FROM creature_template")}
    encounters, encounter_name = dbc("DungeonEncounter.dbc")
    encounter = {row[0]: {"map": row[1], "order": row[3], "name": encounter_name(row[5])} for row in encounters}
    maps, map_name = dbc("Map.dbc")
    map_info = {row[0]: {"name": map_name(row[5]), "raid": row[2] == 2} for row in maps}

    bosses = {}                                         # creature entry -> boss
    for number, credit in sql("SELECT entry, creditEntry FROM instance_encounters WHERE creditType = 0 ORDER BY entry"):
        number, credit = int(number), int(credit)
        if credit in bosses or credit not in templates or number not in encounter:
            continue
        bosses[credit] = dict(templates[credit], map=encounter[number]["map"], order=encounter[number]["order"])

    written, book = [], {}
    for boss in bosses.values():
        info = map_info.get(boss["map"], {"name": f"Map {boss['map']}", "raid": False})
        labels = (["10 players", "25 players", "10 heroic", "25 heroic"] if info["raid"] else ["Normal", "Heroic", "Heroic", "Heroic"])
        modes = []
        for index, entry in enumerate([boss["entry"]] + boss["modes"]):
            template = templates.get(entry)
            if not template or (index and not entry):
                continue
            chances = flatten(creature_loot.get(template["loot"], []), references) if template["loot"] else {}
            own = sorted(((items[i], c) for i, c in chances.items() if i in items and c >= MIN_CHANCE_SHOWN),
                         key=lambda pair: (-pair[0]["quality"], -pair[1], pair[0]["name"]))
            gear = [(item, chance) for item, chance in own if item["class"] in (ITEM_WEAPON, ITEM_ARMOR) and item["quality"] >= UNCOMMON and item["slot"]]

            # New drops: one for each class, at the quality and item level of what the boss has itself
            new = []
            if gear:
                # The quality most of the boss's own gear has: a dungeon boss with one epic among blues gives new blues
                qualities = [item["quality"] for item, _ in gear]
                best = max(RARE, min(EPIC, max(set(qualities), key=lambda q: (qualities.count(q), -q))))
                levels = sorted(item["level"] for item, _ in gear if item["quality"] >= best - 1)
                low, high = levels[len(levels) // 2] - 3, levels[-1] + 2
                mine = {item["id"] for item, _ in own}
                for cls in CLASSES:
                    for quality in (best, best - 1):
                        if quality < RARE:
                            continue
                        taken = {n[0]["id"] for n in new}
                        fitting = [item for level in range(low, high + 1) for item in shelves.get((quality, level), ())
                                   if item["id"] not in mine and item["id"] not in taken and suits(item, cls)]
                        if fitting:
                            new.append((random.Random(f"{seed}:{entry}:{cls}").choice(fitting), cls))
                            break

            if not own and not new:
                continue
            modes.append({"label": labels[index] if (boss["modes"][0] or info["raid"] and any(boss["modes"])) else "", "entry": entry, "own": own, "new": new})
            for item, chance in gear:
                written.append((entry, item["id"], 0, 0, round(chance, 3)))
            for item, cls in new:
                written.append((entry, item["id"], 1, cls, 1))

        if modes:
            place = book.setdefault(boss["map"], {"map": boss["map"], "name": info["name"], "raid": info["raid"], "bosses": [], "level": 0})
            place["bosses"].append({"name": boss["name"], "entry": boss["entry"], "order": boss["order"], "level": boss["level"], "modes": modes})
            place["level"] = max(place["level"], boss["level"])

    if a.boss:
        for place in book.values():
            for boss in place["bosses"]:
                if a.boss == str(boss["entry"]) or (not a.boss.isdigit() and a.boss.lower() in boss["name"].lower()):
                    print(f"{boss['name']} ({boss['entry']}), {place['name']}")
                    for mode in boss["modes"]:
                        print(f"  {mode['label'] or 'Drops'} (creature {mode['entry']})")
                        for item, chance in mode["own"]:
                            print(f"    {chance:5.1f}%  {item['name']} (item level {item['level']})")
                        for item, cls in mode["new"]:
                            print(f"    new     {item['name']} (item level {item['level']}), for a {CLASSES[cls]}")
        return

    # For the game
    script = ["DROP TABLE IF EXISTS wowplus_boss_loot;",
              "CREATE TABLE wowplus_boss_loot (creature INT UNSIGNED NOT NULL, item INT UNSIGNED NOT NULL, fresh TINYINT UNSIGNED NOT NULL, "
              "class TINYINT UNSIGNED NOT NULL, weight FLOAT NOT NULL, PRIMARY KEY (creature, item, fresh)) "
              "COMMENT='WowPlus: what bosses drop besides the game''s own loot. Made by Settings/Scripts/Make-Boss-Loot.py at every start';"]
    unique = {(c, i, f): (c, i, f, cls, w) for c, i, f, cls, w in written}
    rows = sorted(unique.values())
    for start in range(0, len(rows), 1000):
        script.append("INSERT INTO wowplus_boss_loot VALUES " + ",".join("(%d,%d,%d,%d,%s)" % row for row in rows[start:start + 1000]) + ";")
    run_sql("\n".join(script))

    # For the guide
    def lua(text):
        return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'

    names = {}
    out = ["-- Made by Settings/Scripts/Make-Boss-Loot.py at every server start. Do not edit: it is written over.",
           f"-- World seed {seed}. Places, their bosses, and for each way of playing (normal, heroic ...) what drops:",
           "-- o = the game's own loot as {item, chance in percent}, n = new on this server as {item, class}.",
           "WowPlusGuideLoot = {"]
    for place in sorted(book.values(), key=lambda place: (place["raid"], place["level"], place["name"])):
        out.append(f"  {{ map = {place['map']}, name = {lua(place['name'])}, raid = {'true' if place['raid'] else 'false'}, level = {place['level']}, bosses = {{")
        for boss in sorted(place["bosses"], key=lambda boss: (boss["order"], boss["name"])):
            out.append(f"    {{ name = {lua(boss['name'])}, entry = {boss['entry']}, modes = {{")
            for mode in boss["modes"]:
                own = ",".join("{%d,%s}" % (item["id"], ("%.1f" % chance).rstrip("0").rstrip(".")) for item, chance in mode["own"])
                new = ",".join("{%d,%d}" % (item["id"], cls) for item, cls in mode["new"])
                out.append(f"      {{ label = {lua(mode['label'])}, o = {{{own}}}, n = {{{new}}} }},")
                for item, _ in mode["own"] + mode["new"]:
                    names[item["id"]] = item
            out.append("    } },")
        out.append("  } },")
    out.append("}")
    out.append("-- Names and qualities, so that the book can show and search items the game has not loaded yet")
    out.append("WowPlusGuideItems = {")
    for item in sorted(names.values(), key=lambda item: item["id"]):
        out.append(f"[{item['id']}]={{{lua(item['name'])},{item['quality']},{item['level']}}},")
    out.append("}")
    os.makedirs(os.path.dirname(BOOK), exist_ok=True)
    text = "\n".join(out) + "\n"
    try:
        with io.open(BOOK, encoding="utf-8", newline="") as f:
            unchanged = f.read() == text
    except OSError:
        unchanged = False
    if not unchanged:
        with io.open(BOOK, "w", encoding="utf-8", newline="") as f:
            f.write(text)

    new_count = sum(1 for row in rows if row[2])
    print(f"Boss loot: {sum(len(place['bosses']) for place in book.values())} bosses in {len(book)} dungeons and raids, "
          f"{len(rows) - new_count} of the game's own drops and {new_count} new ones (seed {seed}), in {time.time() - started:.1f} s."
          + ("" if unchanged else " Guide's loot written."))


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as error:
        print(f"Boss loot not made: {error}")
        sys.exit(1)
