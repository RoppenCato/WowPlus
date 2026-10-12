"""Puts the quests in Settings/Quests into the game.

Every quest is one small file there (see Settings/Quests/README.md). This script checks them, turns
them into the game's database rows, and applies them. Settings/Deploy-Settings.ps1 runs it at every
server start; the quest tool (Quest-Tool.bat) runs it when you press "Put into the game".

    python Settings/Scripts/Make-Quests.py            check all quests and put them into the game
    python Settings/Scripts/Make-Quests.py --check    only check, change nothing
    python Settings/Scripts/Make-Quests.py --reload   also tell the running server to read them again
    python Settings/Scripts/Make-Quests.py --sql      print the database rows instead of applying them

Our quests have numbers from 9200000 on. Everything this script wrote before is taken out again first,
so a quest file that is deleted is gone from the game at the next start.
"""
import argparse
import glob
import io
import json
import os
import math
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MYSQL = os.path.join(ROOT, "Tools", "mysql", "bin", "mysql.exe")
QUESTS_DIR = os.path.join(ROOT, "Settings", "Quests")
FIRST_ID, LAST_ID = 9200000, 9299999
RACES = {"horde": 690, "alliance": 1101, "both": 0}
TYPES = {
    "kill": "Kill a number of one or more kinds of enemy.",
    "collect": "Bring a number of items that enemies drop (the tool can make them drop while you are on the quest).",
    "fetch": "Bring items you buy, make, gather or already find in the world.",
    "deliver": "Go and talk to someone else; no task besides the journey.",
    "slay": "Kill one named enemy.",
}
LOOT_MARK = "WowPlus quest"
FIRST_SPAWN, LAST_SPAWN = 9200000, 9299999     # the numbers of the enemies our quests add to the world


def sql(query, database="acore_world"):
    out = subprocess.run([MYSQL, "-uroot", "-P3307", "--protocol=TCP", "-hlocalhost", "-N", "-B", "--default-character-set=utf8mb4", database, "-e", query],
                         capture_output=True, text=True, encoding="utf-8", errors="replace")
    if out.returncode != 0:
        raise RuntimeError(out.stderr.strip()[:400])
    return [line.split("\t") for line in out.stdout.splitlines()]


def run_sql(script):
    out = subprocess.run([MYSQL, "-uroot", "-P3307", "--protocol=TCP", "-hlocalhost", "--default-character-set=utf8mb4", "acore_world"],
                         input=script, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if out.returncode != 0:
        raise RuntimeError(out.stderr.strip()[:400])


def load_quests():
    """The quest files, as (path, quest). A file that cannot be read comes back as (path, error text)."""
    found = []
    for path in sorted(glob.glob(os.path.join(QUESTS_DIR, "*.json"))):
        try:
            with io.open(path, encoding="utf-8-sig") as f:
                found.append((path, json.load(f)))
        except (OSError, ValueError) as error:
            found.append((path, f"cannot be read: {error}"))
    return found


def next_id(quests=None):
    used = [q["id"] for _, q in (quests if quests is not None else load_quests()) if isinstance(q, dict) and isinstance(q.get("id"), int)]
    return max(used + [FIRST_ID]) + 1


def file_name(quest):
    slug = re.sub(r"[^a-z0-9]+", "-", quest.get("title", "quest").lower()).strip("-")[:40] or "quest"
    return f"{quest['id']}-{slug}.json"


def numbers(values):
    return ",".join(str(int(v)) for v in values) or "0"


def known(quests):
    """Names of the creatures and items the quests mention, to check them and to write texts with."""
    creatures, items = set(), set()
    for quest in quests:
        creatures.update(int(quest[k]) for k in ("giver", "turn_in") if str(quest.get(k, "")).isdigit())
        creatures.update(int(s["creature"]) for s in quest.get("spawns") or [] if str(s.get("creature", "")).isdigit())
        for objective in quest.get("objectives") or []:
            if "creature" in objective:
                creatures.add(int(objective["creature"]))
            if "item" in objective:
                items.add(int(objective["item"]))
            creatures.update(int(d["creature"]) for d in objective.get("drops_from") or [] if "creature" in d)
        reward = quest.get("reward") or {}
        items.update(int(r["item"]) for r in (reward.get("items") or []) + (reward.get("choices") or []) if "item" in r)
    creature_rows = sql(f"SELECT entry, name, lootid FROM creature_template WHERE entry IN ({numbers(creatures)})")
    item_rows = sql(f"SELECT entry, name FROM item_template WHERE entry IN ({numbers(items)})")
    return ({int(r[0]): {"name": r[1], "loot": int(r[2])} for r in creature_rows}, {int(r[0]): r[1] for r in item_rows})


def validate(quest, creatures, items, ids_seen=None):
    """What is wrong with one quest. An empty list means it can go into the game."""
    wrong = []
    need = lambda ok, text: wrong.append(text) if not ok else None
    need(isinstance(quest.get("id"), int) and FIRST_ID < quest["id"] <= LAST_ID, f"id must be a number from {FIRST_ID + 1} to {LAST_ID}")
    if ids_seen is not None and isinstance(quest.get("id"), int):
        need(quest["id"] not in ids_seen, f"id {quest.get('id')} is used by another quest file")
        ids_seen.add(quest["id"])
    need(isinstance(quest.get("title"), str) and 2 <= len(quest["title"]) <= 60, "title: 2 to 60 letters")
    level = quest.get("level")
    need(isinstance(level, int) and 1 <= level <= 80, "level: 1 to 80")
    if isinstance(level, int):
        need(isinstance(quest.get("min_level"), int) and 1 <= quest["min_level"] <= level, "min_level: from 1 up to the quest's level")
        need(quest.get("max_level", 0) == 0 or (isinstance(quest.get("max_level"), int) and quest["max_level"] >= level), "max_level: 0 for none, or at least the quest's level")
    need(quest.get("side") in RACES, "side: horde, alliance or both")
    need(quest.get("type") in TYPES, "type: " + ", ".join(TYPES))
    need(isinstance(quest.get("zone"), int) and quest["zone"] > 0, "zone: the number of the zone it belongs to")
    for who in ("giver", "turn_in"):
        need(isinstance(quest.get(who), int) and quest[who] in creatures, f"{who}: the number of a creature that exists")

    objectives = quest.get("objectives") or []
    kills = [o for o in objectives if "creature" in o]
    brings = [o for o in objectives if "item" in o]
    need(len(kills) <= 4, "at most four kinds of enemy to kill")
    need(len(brings) <= 6, "at most six kinds of item to bring")
    for objective in objectives:
        need(isinstance(objective.get("count"), int) and 1 <= objective["count"] <= 200, "every objective needs a count from 1 to 200")
        if "creature" in objective:
            need(objective["creature"] in creatures, f"enemy {objective['creature']} does not exist")
        if "item" in objective:
            need(objective["item"] in items, f"item {objective['item']} does not exist")
        for drop in objective.get("drops_from") or []:
            creature = creatures.get(drop.get("creature"))
            need(creature is not None, f"drops_from: creature {drop.get('creature')} does not exist")
            need(creature is None or creature["loot"] > 0, f"drops_from: {creature['name'] if creature else '?'} has no loot of its own, so nothing can be added to it")
            need(isinstance(drop.get("chance"), (int, float)) and 0 < drop["chance"] <= 100, "drops_from: chance from 1 to 100")
    kind = quest.get("type")
    if kind in ("kill", "slay"):
        need(bool(kills), "a kill quest needs at least one enemy")
        need(kind != "slay" or (len(kills) == 1 and kills[0].get("count") == 1 and not brings), "slay: exactly one enemy, once")
    if kind in ("collect", "fetch"):
        need(bool(brings), "this kind of quest needs at least one item to bring")
    if kind == "deliver":
        need(not objectives, "deliver: no objectives; the task is to reach the other person")
        need(quest.get("giver") != quest.get("turn_in"), "deliver: giver and turn_in must be two different people")

    texts = quest.get("texts") or {}
    for key, least in (("details", 20), ("objectives", 5), ("completion", 5)):
        need(isinstance(texts.get(key), str) and len(texts[key].strip()) >= least, f"texts.{key}: missing or too short")
    reward = quest.get("reward") or {}
    need(isinstance(reward.get("money", 0), int) and 0 <= reward.get("money", 0) <= 5000000, "reward.money: copper, 0 to 5000000")
    need(isinstance(reward.get("xp_difficulty", 5), int) and 0 <= reward.get("xp_difficulty", 5) <= 8, "reward.xp_difficulty: 0 (none) to 8")
    need(len(reward.get("items") or []) <= 4, "at most four reward items that are always given")
    need(len(reward.get("choices") or []) <= 6, "at most six reward items to choose from")
    for given in (reward.get("items") or []) + (reward.get("choices") or []):
        need(given.get("item") in items, f"reward item {given.get('item')} does not exist")
        need(isinstance(given.get("count", 1), int) and 1 <= given.get("count", 1) <= 200, "reward item count: 1 to 200")
    spawns = quest.get("spawns") or []
    need(len(spawns) <= 40, "at most forty added enemies per quest")
    for spawn in spawns:
        need(spawn.get("creature") in creatures, f"spawns: creature {spawn.get('creature')} does not exist")
        need(all(isinstance(spawn.get(k), (int, float)) for k in ("map", "x", "y", "z")), "spawns: each needs map, x, y and z")
    after = quest.get("after", 0)
    need(isinstance(after, int) and after >= 0, "after: 0, or the number of the quest that must be done first")
    return wrong


# ---------------------------------------------------------------------------------------------
# Where a quest shows on the map: the blue areas for its tasks and the mark for handing it in
# ---------------------------------------------------------------------------------------------

AREA_REACH = 120.0          # enemies closer together than this are shown as one area
AREA_MARGIN = 30.0          # how far an area reaches past the outermost enemy
MOST_AREAS = 3              # areas per task; the game's own quests rarely have more


def map_areas():
    """zone -> the number the client knows that zone's map by (WorldMapArea.dbc)."""
    found = {}
    try:
        with open(os.path.join(ROOT, "Data", "dbc", "WorldMapArea.dbc"), "rb") as f:
            data = f.read()
        magic, records, fields, size, strings = struct.unpack_from("<4sIIII", data)
        for i in range(records):
            number, map_id, zone = struct.unpack_from("<3I", data, 20 + i * size)
            if zone:
                found[zone] = number
    except (OSError, struct.error):
        pass
    return found


def hull(points):
    """The outline around some points, counter-clockwise."""
    points = sorted(set(points))
    if len(points) < 3:
        return points

    def turn(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for point in points:
        while len(lower) >= 2 and turn(lower[-2], lower[-1], point) <= 0:
            lower.pop()
        lower.append(point)
    for point in reversed(points):
        while len(upper) >= 2 and turn(upper[-2], upper[-1], point) <= 0:
            upper.pop()
        upper.append(point)
    return lower[:-1] + upper[:-1]


def areas(points):
    """Groups places that lie together and draws an outline around each group, a little wider than the group."""
    groups = []
    for point in points:
        near = [g for g in groups if any(math.hypot(point[0] - q[0], point[1] - q[1]) <= AREA_REACH for q in g)]
        merged = [point]
        for g in near:
            merged += g
            groups.remove(g)
        groups.append(merged)
    groups.sort(key=len, reverse=True)
    outlines = []
    for group in groups[:MOST_AREAS]:
        cx, cy = sum(q[0] for q in group) / len(group), sum(q[1] for q in group) / len(group)
        ring = []
        for q in group:                                 # eight points around each place, so that even one enemy gives an area
            for step in range(8):
                ring.append((q[0] + AREA_MARGIN * math.cos(step * math.pi / 4), q[1] + AREA_MARGIN * math.sin(step * math.pi / 4)))
        outline = hull([(round(x), round(y)) for x, y in ring])
        outlines.append(sorted(outline, key=lambda q: math.atan2(q[1] - cy, q[0] - cx))[:12])
    return outlines


def build_map_marks(quests):
    """The rows that make the quests show on the map."""
    wanted = set()
    for quest in quests:
        wanted.update((quest["turn_in"],))
        for objective in quest.get("objectives") or []:
            if "creature" in objective:
                wanted.add(objective["creature"])
            wanted.update(d["creature"] for d in objective.get("drops_from") or [])
    places = {}                                         # creature -> [(map, x, y)]
    for entry, map_id, x, y in sql(f"SELECT id, map, position_x, position_y FROM creature WHERE id IN ({numbers(wanted)}) AND guid < {FIRST_SPAWN}"):
        places.setdefault(int(entry), []).append((int(map_id), float(x), float(y)))
    for quest in quests:
        for spawn in quest.get("spawns") or []:
            places.setdefault(int(spawn["creature"]), []).append((int(spawn["map"]), float(spawn["x"]), float(spawn["y"])))

    zones = map_areas()
    out = [f"DELETE FROM quest_poi WHERE QuestID BETWEEN {FIRST_ID} AND {LAST_ID};",
           f"DELETE FROM quest_poi_points WHERE QuestID BETWEEN {FIRST_ID} AND {LAST_ID};"]
    for quest in quests:
        number, zone = quest["id"], zones.get(quest["zone"], 0)
        blob = 0

        def mark(objective_index, points, map_id):
            nonlocal blob
            out.append(f"INSERT INTO quest_poi (QuestID, id, ObjectiveIndex, MapID, WorldMapAreaId, Floor, Priority, Flags, VerifiedBuild) "
                       f"VALUES ({number}, {blob}, {objective_index}, {map_id}, {zone}, 0, 0, 1, 0);")
            for index, (x, y) in enumerate(points):
                out.append(f"INSERT INTO quest_poi_points (QuestID, Idx1, Idx2, X, Y, VerifiedBuild) VALUES ({number}, {blob}, {index}, {int(round(x))}, {int(round(y))}, 0);")
            blob += 1

        kill_index, item_index = 0, 4                   # the game counts enemies from 0 and items from 4
        for objective in quest.get("objectives") or []:
            if "creature" in objective:
                sources, index = [objective["creature"]], kill_index
                kill_index += 1
            else:
                sources, index = [d["creature"] for d in objective.get("drops_from") or []], item_index
                item_index += 1
            found = [place for source in sources for place in places.get(source, [])]
            if not found:
                continue
            map_id = found[0][0]
            for outline in areas([(x, y) for m, x, y in found if m == map_id]):
                mark(index, outline, map_id)
        ender = places.get(quest["turn_in"])
        if ender:
            mark(-1, [(ender[0][1], ender[0][2])], ender[0][0])
    return "\n".join(out) + "\n"


def text(value):
    """A text for the database: the game writes $B for a new line and $N for the player's name."""
    value = (value or "").replace("\r", "").strip()
    value = value.replace("\n", "$B")
    return "'" + value.replace("\\", "\\\\").replace("'", "''") + "'"


def padded(values, size):
    return (list(values) + [(0, 0)] * size)[:size]


def build_sql(quests, creatures, items):
    out = [f"-- Made by Settings/Scripts/Make-Quests.py from the files in Settings/Quests. Do not edit: it is written over.",
           f"DELETE FROM creature_queststarter WHERE quest BETWEEN {FIRST_ID} AND {LAST_ID};",
           f"DELETE FROM creature_questender WHERE quest BETWEEN {FIRST_ID} AND {LAST_ID};",
           f"DELETE FROM quest_offer_reward WHERE ID BETWEEN {FIRST_ID} AND {LAST_ID};",
           f"DELETE FROM quest_request_items WHERE ID BETWEEN {FIRST_ID} AND {LAST_ID};",
           f"DELETE FROM quest_template_addon WHERE ID BETWEEN {FIRST_ID} AND {LAST_ID};",
           f"DELETE FROM quest_template WHERE ID BETWEEN {FIRST_ID} AND {LAST_ID};",
           f"DELETE FROM creature_loot_template WHERE Comment LIKE '{LOOT_MARK} %';",
           f"DELETE FROM creature WHERE guid BETWEEN {FIRST_SPAWN} AND {LAST_SPAWN};"]
    people = set()
    spawn_number = FIRST_SPAWN
    for quest in quests:
        number = quest["id"]
        objectives = quest.get("objectives") or []
        kills = padded([(o["creature"], o["count"]) for o in objectives if "creature" in o], 4)
        brings = padded([(o["item"], o["count"]) for o in objectives if "item" in o], 6)
        reward = quest.get("reward") or {}
        given = padded([(r["item"], r.get("count", 1)) for r in reward.get("items") or []], 4)
        choices = padded([(r["item"], r.get("count", 1)) for r in reward.get("choices") or []], 6)
        texts = quest["texts"]
        ender = creatures[quest["turn_in"]]["name"]
        columns = ["ID", "QuestType", "QuestLevel", "MinLevel", "QuestSortID", "RewardXPDifficulty", "RewardMoney", "Flags", "AllowableRaces"]
        values = [number, 2, quest["level"], quest["min_level"], quest["zone"], reward.get("xp_difficulty", 5), reward.get("money", 0), 0, RACES[quest["side"]]]
        for index, (creature, count) in enumerate(kills, 1):
            columns += [f"RequiredNpcOrGo{index}", f"RequiredNpcOrGoCount{index}"]
            values += [creature, count]
        for index, (item, count) in enumerate(brings, 1):
            columns += [f"RequiredItemId{index}", f"RequiredItemCount{index}"]
            values += [item, count]
        for index, (item, count) in enumerate(given, 1):
            columns += [f"RewardItem{index}", f"RewardAmount{index}"]
            values += [item, count]
        for index, (item, count) in enumerate(choices, 1):
            columns += [f"RewardChoiceItemID{index}", f"RewardChoiceItemQuantity{index}"]
            values += [item, count]
        columns += ["LogTitle", "LogDescription", "QuestDescription", "QuestCompletionLog"]
        values += [text(quest["title"]), text(texts["objectives"]), text(texts["details"]), text(texts.get("return") or f"Return to {ender}.")]
        out.append(f"INSERT INTO quest_template ({', '.join(columns)}) VALUES ({', '.join(str(v) for v in values)});")
        out.append(f"INSERT INTO quest_template_addon (ID, MaxLevel, PrevQuestID) VALUES ({number}, {quest.get('max_level', 0)}, {quest.get('after', 0)});")
        out.append(f"INSERT INTO quest_offer_reward (ID, RewardText, VerifiedBuild) VALUES ({number}, {text(texts['completion'])}, 0);")
        if texts.get("progress"):
            out.append(f"INSERT INTO quest_request_items (ID, EmoteOnComplete, EmoteOnIncomplete, CompletionText, VerifiedBuild) VALUES ({number}, 0, 0, {text(texts['progress'])}, 0);")
        out.append(f"INSERT IGNORE INTO creature_queststarter (id, quest) VALUES ({quest['giver']}, {number});")
        out.append(f"INSERT IGNORE INTO creature_questender (id, quest) VALUES ({quest['turn_in']}, {number});")
        people.update((quest["giver"], quest["turn_in"]))
        # More of an enemy than the game has: each is a copy of one that is there already, at a new place
        for spawn in quest.get("spawns") or []:
            spawn_number += 1
            out.append("INSERT INTO creature (guid, id, map, zoneId, areaId, spawnMask, phaseMask, equipment_id, position_x, position_y, position_z, orientation, "
                       "spawntimesecs, wander_distance, currentwaypoint, curhealth, curmana, MovementType, npcflag, unit_flags, dynamicflags, ScriptName, VerifiedBuild, CreateObject, Comment) "
                       f"SELECT {spawn_number}, id, {int(spawn['map'])}, zoneId, areaId, spawnMask, phaseMask, equipment_id, {float(spawn['x'])}, {float(spawn['y'])}, {float(spawn['z'])}, "
                       f"{float(spawn.get('o', 0))}, spawntimesecs, GREATEST(wander_distance, 5), 0, curhealth, curmana, 1, npcflag, unit_flags, dynamicflags, ScriptName, VerifiedBuild, CreateObject, "
                       f"'{LOOT_MARK} {number}' FROM creature WHERE id = {int(spawn['creature'])} AND guid < {FIRST_SPAWN} ORDER BY guid LIMIT 1;")
        # Items that only drop while someone is on the quest
        for objective in objectives:
            for drop in objective.get("drops_from") or []:
                loot = creatures[drop["creature"]]["loot"]
                out.append("INSERT IGNORE INTO creature_loot_template (Entry, Item, Reference, Chance, QuestRequired, LootMode, GroupId, MinCount, MaxCount, Comment) "
                           f"VALUES ({loot}, {objective['item']}, 0, {float(drop['chance'])}, 1, 1, 0, 1, 1, '{LOOT_MARK} {number}');")
    if people:
        out.append(f"UPDATE creature_template SET npcflag = npcflag | 2 WHERE entry IN ({numbers(people)});     -- they must be able to give quests")
    return "\n".join(out) + "\n" + build_map_marks(quests)


def check_all():
    """Returns (good quests, problems as text lines, creature names, item names)."""
    loaded = load_quests()
    quests = [q for _, q in loaded if isinstance(q, dict)]
    creatures, items = known(quests) if quests else ({}, {})
    good, problems, seen = [], [], set()
    for path, quest in loaded:
        name = os.path.basename(path)
        if not isinstance(quest, dict):
            problems.append(f"{name}: {quest}")
            continue
        wrong = validate(quest, creatures, items, seen)
        if wrong:
            problems += [f"{name}: {line}" for line in wrong]
        else:
            good.append(quest)
    return good, problems, creatures, items


def reload_server():
    """Tells the running world server to read quests again. Nothing happens if it is not running."""
    sender = os.path.join(HERE, "Send-Server-Command.ps1")
    for command in ("reload quest_template", "reload creature_queststarter", "reload creature_questender", "reload creature_loot_template"):
        subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", sender, command],
                       capture_output=True, creationflags=getattr(subprocess, "CREATE_NEW_CONSOLE", 0))


def apply(reload=False):
    """Checks and applies. Returns (number of quests in the game, problems)."""
    good, problems, creatures, items = check_all()
    run_sql(build_sql(good, creatures, items))
    if reload:
        reload_server()
    return len(good), problems


def main():
    p = argparse.ArgumentParser(description="Put the quests in Settings/Quests into the game")
    p.add_argument("--check", action="store_true")
    p.add_argument("--reload", action="store_true")
    p.add_argument("--sql", action="store_true")
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    try:
        good, problems, creatures, items = check_all()
        for line in problems:
            print("  ! " + line)
        if a.sql:
            print(build_sql(good, creatures, items))
        elif a.check:
            print(f"Quests: {len(good)} are fine, {len(problems)} problems. Nothing was changed.")
        else:
            run_sql(build_sql(good, creatures, items))
            if a.reload:
                reload_server()
            print(f"Quests: {len(good)} put into the game" + (f", {len(problems)} problems (those quests were left out)" if problems else "")
                  + (". The server was told to read them again." if a.reload else "."))
    except RuntimeError as error:
        print(f"Quests not made: {error}")
        sys.exit(1)
    sys.exit(1 if problems and a.check else 0)


if __name__ == "__main__":
    main()
