"""Surveys every zone: how many quests it has for each side, how they are spread over the levels, how
much experience they give against what the levels cost, and which enemies and places no quest uses.
Writes Settings/Quests/Zone-Audit.md. The point is to see where quests should be added.

    python Settings/Scripts/Make-Zone-Audit.py

Run it again after adding quests; the part above the line "Made by the script" in the document is
written by hand and kept.
"""
import glob
import importlib.util
import io
import json
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
OUT = os.path.join(ROOT, "Settings", "Quests", "Zone-Audit.md")
MARK = "<!-- Made by the script from here on -->"

spec = importlib.util.spec_from_file_location("quest_tool", os.path.join(HERE, "Quest-Tool.py"))
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
sql = tool.sql

HORDE, ALLIANCE = 690, 1101
CONTINENTS = {0: "Eastern Kingdoms", 1: "Kalimdor", 530: "Outland", 571: "Northrend"}
# The blood elf and draenei lands are on the Outland map in the game's files, but lie elsewhere
ELSEWHERE = {3430: "Eastern Kingdoms", 3433: "Eastern Kingdoms", 3487: "Eastern Kingdoms", 4080: "Eastern Kingdoms",
             3524: "Kalimdor", 3525: "Kalimdor", 3557: "Kalimdor"}
# Cities and other places with quests but no levelling: listed, but nothing is asked of them
NOT_FOR_LEVELLING = {1519: "city", 1537: "city", 1657: "city", 3557: "city", 1637: "city", 1638: "city", 1497: "city", 3487: "city",
                     3703: "city", 4395: "city", 493: "druid sanctuary", 4080: "daily quests at the level cap"}

TARGET_SHARE = 70           # a zone should offer this share, in percent, of the experience its levels cost, from quests alone
QUESTS_PER_ITEM = 3         # roughly one quest in three gives gear to choose
FEW_SPAWNS = 4              # enemies with fewer than this in a zone are not worth a remark
BIG_PLACE = 10              # a place with this many enemies and no quest touching it is "unused"
DUNGEON_KINDS = {81: "dungeon", 85: "dungeon", 62: "raid", 88: "raid", 89: "raid"}      # quest_template.QuestInfoID
FEW_DUNGEON_QUESTS = 3      # a dungeon with fewer quests than this for a side gives little reason to go


def setting(name, default):
    try:
        with io.open(os.path.join(ROOT, "Build", "bin", "RelWithDebInfo", "configs", "worldserver.conf"), encoding="utf-8", errors="replace") as f:
            for line in f:
                found = re.match(r"\s*" + re.escape(name) + r"\s*=\s*([\d.]+)", line)
                if found:
                    return float(found.group(1))
    except OSError:
        pass
    return default


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    started = time.time()
    world = tool.World()
    quest_rate = setting("Rate.XP.Quest", 1.0)
    level_cost = {int(l): int(x) for l, x in sql("SELECT Level, Experience FROM player_xp_for_level")}

    # Quests a player can actually get: with someone or something that starts them, not seasonal, not repeatable
    started_by = {int(r[0]) for r in sql("SELECT quest FROM creature_queststarter UNION SELECT quest FROM gameobject_queststarter "
                                         "UNION SELECT startquest FROM item_template WHERE startquest > 0")}
    seasonal = {int(r[0]) for r in sql("SELECT questId FROM game_event_seasonal_questrelation")}
    givers = {}                                         # quest -> the creatures that give it
    for quest, creature in sql("SELECT quest, id FROM creature_queststarter"):
        givers.setdefault(int(quest), []).append(int(creature))

    def open_to(number, races, side):
        if races:
            return bool(races & (HORDE if side == "horde" else ALLIANCE))
        people = [world.templates[c] for c in givers.get(number, []) if c in world.templates]
        if not people:
            return True                                 # started by an object or an item: anyone
        return any(world.stance(t["faction"], side) != "hostile" for t in people)
    columns = ("ID, QuestSortID, QuestLevel, MinLevel, AllowableRaces, RewardXPDifficulty, Flags, QuestInfoID, LogTitle, "
               "RequiredNpcOrGo1, RequiredNpcOrGo2, RequiredNpcOrGo3, RequiredNpcOrGo4, "
               "RequiredItemId1, RequiredItemId2, RequiredItemId3, RequiredItemId4, RequiredItemId5, RequiredItemId6, "
               "RewardItem1, RewardChoiceItemID1, IFNULL((SELECT SpecialFlags FROM quest_template_addon a WHERE a.ID = quest_template.ID), 0)")
    quests = {}
    for r in sql(f"SELECT {columns} FROM quest_template WHERE QuestSortID > 0"):
        number, zone, level, low, races, difficulty, flags, info = (int(v) for v in r[:8])
        title = r[8]
        filed = zone
        zone = world.areas.get(zone, {}).get("parent") or zone       # a quest filed under a part of a zone (Red Cloud Mesa) belongs to the zone (Mulgore)
        if number not in started_by or number in seasonal or flags & 0x1000 or flags & 0x8000 or int(r[21]) & 1:
            continue                                    # nobody gives it, a holiday, daily, weekly or repeatable
        if level <= 0 or re.search(r"UNUSED|NYI|DEPRECATED|<TXT>|\[PH\]|^TEST", title, re.I) or not title:
            continue
        quests.setdefault(zone, []).append({
            "id": number, "level": level, "min": low, "difficulty": difficulty, "group": info in (1, 41, 62, 81, 85, 88, 89), "title": title,
            "filed": filed, "info": info,
            "horde": open_to(number, races, "horde"), "alliance": open_to(number, races, "alliance"),
            "kills": [int(v) for v in r[9:13] if int(v) > 0], "items": [int(v) for v in r[13:19] if int(v) > 0],
            "gear": int(r[19]) > 0 or int(r[20]) > 0, "ours": number >= 9200000})

    # Which creature drops which item, to see whom the "bring me" quests use
    drops = {}                                          # item -> creature templates' loot ids
    references = {}
    for entry, item, reference in sql("SELECT Entry, Item, Reference FROM reference_loot_template"):
        references.setdefault(int(entry), []).append((int(item), int(reference)))
    for entry, item, reference in sql("SELECT Entry, Item, Reference FROM creature_loot_template"):
        if int(reference):
            for inner, deeper in references.get(int(reference), []):
                if not deeper:
                    drops.setdefault(inner, set()).add(int(entry))
        else:
            drops.setdefault(int(item), set()).add(int(entry))
    kill_credit = {int(r[0]): [int(r[1]), int(r[2])] for r in sql("SELECT entry, KillCredit1, KillCredit2 FROM creature_template WHERE KillCredit1 > 0 OR KillCredit2 > 0")}

    spawns = {}                                         # zone -> creature -> [area]
    for entry, zone, area in sql("SELECT id, zoneId, areaId FROM creature WHERE zoneId > 0 AND map IN (0, 1, 530, 571) AND guid NOT IN (SELECT guid FROM game_event_creature)"):
        spawns.setdefault(int(zone), {}).setdefault(int(entry), []).append(int(area))

    def xp_of(quest):
        row = world.quest_xp.get(quest["level"])
        return int(row[quest["difficulty"]] * quest_rate) if row and quest["difficulty"] < len(row) else 0

    zones = []
    for zone, found in quests.items():
        if zone not in world.bounds or world.bounds[zone]["map"] not in CONTINENTS or zone not in world.areas or len(found) < 3:
            continue
        levels = sorted(q["level"] for q in found if not q["group"]) or sorted(q["level"] for q in found)     # dungeons under a zone do not stretch it
        low, high = levels[len(levels) // 4], levels[len(levels) * 3 // 4]
        cost = sum(level_cost.get(level, 0) for level in range(low, high + 1))
        info = {"id": zone, "name": world.areas[zone]["name"], "continent": ELSEWHERE.get(zone, CONTINENTS[world.bounds[zone]["map"]]),
                "low": low, "high": high, "cost": cost, "sides": {}, "special": NOT_FOR_LEVELLING.get(zone, "level cap" if low >= 80 else "")}
        for side in ("horde", "alliance"):
            mine = [q for q in found if q[side]]
            solo = [q for q in mine if not q["group"]]
            spread = {}
            for q in mine:
                spread[q["level"]] = spread.get(q["level"], 0) + 1
            xp = sum(xp_of(q) for q in solo)
            share = round(100 * xp / cost) if cost else 0
            typical = max(1, int(world.quest_xp.get((low + high) // 2, [0] * 9)[5] * quest_rate))
            short = max(0, cost * TARGET_SHARE // 100 - xp)
            wanted = (short + typical - 1) // typical if len(mine) >= 3 or short else 0
            most = max(sum(1 for q in found if q["horde"]), sum(1 for q in found if q["alliance"]))
            belongs = len(mine) >= 3 and len(mine) * 4 >= most      # a quarter of what the other side has, or it is only passing through
            if info["special"] or not belongs:
                wanted, short = 0, 0
            info["sides"][side] = {"belongs": belongs and not info["special"], "quests": len(mine), "group": len(mine) - len(solo), "xp": xp, "share": share, "spread": spread,
                                   "gear": sum(q["gear"] for q in mine), "add": wanted, "add_xp": short, "ours": sum(q["ours"] for q in mine)}

        # Enemies no quest touches, and places where nothing is asked of the player
        used = set()
        for q in found:
            used.update(q["kills"])
            for item in q["items"]:
                for loot in drops.get(item, ()):
                    used.add(loot)                      # loot ids are the creature's own number for nearly all creatures
        for entry, credits in kill_credit.items():
            if any(c in used for c in credits):
                used.add(entry)
        unused, places = [], {}
        for entry, where in spawns.get(zone, {}).items():
            t = world.templates.get(entry)
            if not t or t["type"] in (8, 10, 11, 12) or not t["name"] or re.search(r"\[|trigger|bunny|invisible|\(\d\)|dummy|credit", t["name"], re.I):
                continue
            if world.stance(t["faction"], "horde") == "friendly" or world.stance(t["faction"], "alliance") == "friendly":
                continue                                # somebody's guards and townsfolk are not enemies to write quests about
            touched = entry in used or t["loot"] in used
            for area in where:
                place = places.setdefault(area, {"enemies": 0, "touched": 0})
                place["enemies"] += 1
                place["touched"] += touched
            if not touched and len(where) >= FEW_SPAWNS and abs(t["max"] - (low + high) / 2) <= 8:
                unused.append((len(where), t["name"], t["min"], t["max"]))
        info["unused"] = sorted(unused, reverse=True)
        info["quiet"] = sorted(((p["enemies"], world.areas.get(area, {}).get("name", str(area))) for area, p in places.items()
                                if p["enemies"] >= BIG_PLACE and p["touched"] == 0 and area != zone and area in world.areas), reverse=True)
        zones.append(info)

    zones.sort(key=lambda z: (z["low"], z["high"], z["name"]))

    # ---- Dungeons and raids: their quests are filed under the dungeon itself, or under a zone with a mark ----
    map_rows, map_fields, map_text = tool.dbc("Map.dbc")
    instances = {}                                      # map -> (name, "dungeon" or "raid")
    for row in map_rows:
        v = tool.struct.unpack_from("<%dI" % map_fields, row)
        if v[2] in (1, 2):
            instances[v[0]] = (map_text(v[5]), "dungeon" if v[2] == 1 else "raid")
    by_name = {name.lower(): number for number, (name, kind) in instances.items()}
    lives_in = {}                                       # creature -> the instance it stands in
    for entry, map_id in sql("SELECT DISTINCT id, map FROM creature WHERE map NOT IN (0, 1, 530, 571)"):
        if int(map_id) in instances:
            lives_in.setdefault(int(entry), int(map_id))

    dungeons = {}
    for found in quests.values():
        for q in found:
            area = world.areas.get(q["filed"], {})
            where = area.get("map") if area.get("map") in instances else None
            if where is None and q["info"] in DUNGEON_KINDS:
                targets = list(q["kills"]) + [loot for item in q["items"] for loot in drops.get(item, ())]
                where = next((lives_in[t] for t in targets if t in lives_in), None)
            if where is None and q["info"] not in DUNGEON_KINDS:
                continue
            if where is None:                           # filed under the place outside a dungeon that has the same name
                where = by_name.get(area.get("name", "").lower())
            name, kind = instances.get(where, (f"{area.get('name', q['filed'])} (which dungeon or wing is not told)", DUNGEON_KINDS.get(q["info"], "dungeon")))
            dungeons.setdefault((name, kind), []).append(q)
    dungeon_list = []
    for (name, kind), found in dungeons.items():
        levels = sorted(q["level"] for q in found)
        dungeon_list.append({"name": name, "kind": kind, "low": levels[len(levels) // 4], "high": levels[len(levels) * 3 // 4], "quests": sorted(found, key=lambda q: (q["level"], q["title"]))})
    dungeon_list.sort(key=lambda d: (d["low"], d["high"], d["name"]))

    # ---- Writing it down -----------------------------------------------------------------------
    def side_cell(z, side):
        s = z["sides"][side]
        if s["quests"] < 3:
            return "–"
        if z["special"]:
            return str(s["quests"])
        return f"{s['quests']} ({s['share']}%)" + ("" if s["belongs"] else " passing through")

    def add_cell(z):
        if z["special"]:
            return z["special"]
        parts = [f"{side[0].upper()} +{z['sides'][side]['add']}" for side in ("horde", "alliance") if z["sides"][side]["belongs"] and z["sides"][side]["add"]]
        return ", ".join(parts) or "enough"

    out = [MARK, "",
           f"Made by `Settings/Scripts/Make-Zone-Audit.py` on {time.strftime('%Y-%m-%d')}. Run it again after adding quests.", "",
           "## How to read it", "",
           "- **Levels**: the middle half of the zone's quests, which is where you usually are when you level there.",
           f"- **Quests (share)**: quests a player of that side can get, and in brackets how much of the experience those levels cost that the zone's solo quests give, at this server's quest rate ({quest_rate:g}). "
           f"100% means the quests alone carry you through the zone's levels; kills and discoveries come on top.",
           f"- **Add**: how many ordinary quests would bring that side to {TARGET_SHARE}%. That is the mark where a zone is a full choice by itself. \"–\" means the side has no business there.",
           "- Not counted: holiday quests, daily and repeatable ones, quests nobody gives, and (for the share) dungeon, raid and group quests.", "",
           "## All zones by level", "",
           "| Levels | Zone | Continent | Horde quests (share) | Alliance quests (share) | Ours | Enemies no quest uses | Add |",
           "|---|---|---|---|---|---|---|---|"]
    for z in zones:
        ours = max(z["sides"]["horde"]["ours"], z["sides"]["alliance"]["ours"])
        out.append(f"| {z['low']}-{z['high']} | {z['name']} | {z['continent']} | {side_cell(z, 'horde')} | {side_cell(z, 'alliance')} | {ours or ''} | {len(z['unused'])} | {add_cell(z)} |")

    # How many ways there are to level at each stretch
    out += ["", "## Choices per level", "",
            f"For every five levels: the zones a side can level in (at least 3 quests there), and how many of those carry {TARGET_SHARE}% or more by themselves. Few full choices means the stretch is narrow.", "",
            "| Levels | Horde zones | of which full | Alliance zones | of which full |", "|---|---|---|---|---|"]
    for start in range(1, 80, 5):
        end = start + 4
        cells = []
        for side in ("horde", "alliance"):
            here = [z for z in zones if z["low"] <= end and z["high"] >= start and z["sides"][side]["belongs"]]
            full = [z for z in here if z["sides"][side]["share"] >= TARGET_SHARE]
            cells += [f"{len(here)}: " + ", ".join(z["name"] for z in here), str(len(full))]
        out.append(f"| {start}-{end} | {cells[0]} | {cells[1]} | {cells[2]} | {cells[3]} |")

    out += ["", "## Dungeons and raids", "",
            "Quests that belong to a dungeon or raid, by the level of the place. They are not counted in the zones' shares above. "
            f"A place with fewer than {FEW_DUNGEON_QUESTS} quests for a side gives that side little reason to go beyond the loot.", "",
            "| Levels | Place | Kind | Horde quests | Alliance quests | Experience, Horde | Experience, Alliance | Give gear | Note |",
            "|---|---|---|---|---|---|---|---|---|"]
    for d in dungeon_list:
        horde, alliance = [q for q in d["quests"] if q["horde"]], [q for q in d["quests"] if q["alliance"]]
        few = [side for side, mine in (("Horde", horde), ("Alliance", alliance)) if len(mine) < FEW_DUNGEON_QUESTS]
        out.append(f"| {d['low']}-{d['high']} | {d['name']} | {d['kind']} | {len(horde)} | {len(alliance)} | {sum(xp_of(q) for q in horde):,} | {sum(xp_of(q) for q in alliance):,} | "
                   f"{sum(q['gear'] for q in d['quests'])} | {('few for ' + ' and '.join(few)) if few else ''} |")
    out += ["", "### The quests of each dungeon and raid", ""]
    for d in dungeon_list:
        out.append(f"**{d['name']}** ({d['kind']}, {d['low']}-{d['high']}): " + "; ".join(
            f"{q['title']} ({q['level']}{'' if q['horde'] and q['alliance'] else ', Horde' if q['horde'] else ', Alliance'})" for q in d["quests"]) + ".")
        out.append("")

    out += ["## Zone by zone", ""]
    for z in zones:
        out += [f"### {z['name']} ({z['low']}-{z['high']}, {z['continent']})", ""]
        out.append(f"Going from level {z['low']} to {z['high'] + 1} costs {z['cost']:,} experience.")
        out.append("")
        out.append("| Side | Quests | Group or dungeon | Experience from solo quests | Share | Give gear | Spread by quest level | Would be good to add |")
        out.append("|---|---|---|---|---|---|---|---|")
        if z["special"]:
            out.insert(len(out) - 2, f"*Not a place to level ({z['special']}); listed for completeness.*")
            out.insert(len(out) - 2, "")
        for side in ("horde", "alliance"):
            s = z["sides"][side]
            if s["quests"] < 3:
                continue
            spread = " ".join(f"{level}:{count}" for level, count in sorted(s["spread"].items()))
            gear_add = (s["add"] + QUESTS_PER_ITEM - 1) // QUESTS_PER_ITEM
            add = (f"{s['add']} quests, about {s['add_xp']:,} experience, {gear_add} with gear" if s["add"] else
                   "not a levelling zone" if z["special"] else "only passing through" if not s["belongs"] else "nothing; it carries itself")
            out.append(f"| {side.capitalize()} | {s['quests']} | {s['group']} | {s['xp']:,} | {s['share']}% | {s['gear']} | {spread} | {add} |")
        out.append("")
        if z["unused"]:
            listed = ", ".join(f"{name} ({low}{'-' + str(high) if high != low else ''}, {count} of them)" for count, name, low, high in z["unused"][:10])
            out.append(f"**Enemies no quest uses:** {listed}" + (f", and {len(z['unused']) - 10} more." if len(z["unused"]) > 10 else "."))
        else:
            out.append("**Enemies no quest uses:** none of note.")
        if z["quiet"]:
            out.append("")
            out.append("**Places nothing is asked of you:** " + ", ".join(f"{name} ({count} enemies)" for count, name in z["quiet"][:8]) + ".")
        out.append("")

    # Our own quests
    out += ["## The quests we have added", "", "| Number | Title | Zone | Level | Side | Kind |", "|---|---|---|---|---|---|"]
    for path in sorted(glob.glob(os.path.join(ROOT, "Settings", "Quests", "*.json"))):
        try:
            with io.open(path, encoding="utf-8-sig") as f:
                q = json.load(f)
            out.append(f"| {q['id']} | {q['title']} | {world.areas.get(q['zone'], {}).get('name', q['zone'])} | {q['level']} | {q['side']} | {q['type']} |")
        except (OSError, ValueError, KeyError):
            pass

    head = ""
    try:
        with io.open(OUT, encoding="utf-8") as f:
            old = f.read()
        head = old.split(MARK)[0] if MARK in old else ""
    except OSError:
        pass
    with io.open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write((head or "# Where quests are needed\n\n") + "\n".join(out) + "\n")
    thin = [z for z in zones if any(z["sides"][s]["belongs"] and z["sides"][s]["share"] < TARGET_SHARE for s in ("horde", "alliance"))]
    print(f"Zone survey: {len(zones)} zones, {len(thin)} of them below {TARGET_SHARE}% for a side, in {time.time() - started:.0f} s. Written to Settings/Quests/Zone-Audit.md")


if __name__ == "__main__":
    main()
