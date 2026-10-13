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
CLASSES = {"warrior": 1, "paladin": 2, "hunter": 4, "rogue": 8, "priest": 16, "death knight": 32, "shaman": 64, "mage": 128, "warlock": 256, "druid": 1024}
PERKS = {
    "pet_fed": "A hunter's pet loses happiness half as fast.",
    "stable_slot": "One stable slot, free, when the quest is handed in.",
    "skinning": "Skinning a beast sometimes gives one more of what it gave.",
}
TYPES = {
    "visit": "Go to a place, or hear someone out (may be mixed with the other tasks).",
    "kill": "Kill a number of one or more kinds of enemy.",
    "collect": "Bring a number of items that enemies drop (the tool can make them drop while you are on the quest).",
    "fetch": "Bring items you buy, make, gather or already find in the world.",
    "deliver": "Go and talk to someone else; no task besides the journey.",
    "slay": "Kill one named enemy.",
}
LOOT_MARK = "WowPlus quest"
FIRST_CREATURE, LAST_CREATURE = 9210000, 9219999   # people and enemies our quests add to the game, with names of their own
FIRST_CREDIT, LAST_CREDIT = 9220000, 9239999       # what the game counts when a place is reached or someone is heard out
PLACE, TALK = 0, 1

# Adding enemies must not crowd a place. Around every added enemy:
ROOM = 18.0                 # yards to the nearest other creature: far enough that pulling one does not pull the other
IN_SIGHT = 40.0             # the distance within which creatures are counted
MOST_IN_SIGHT = 6           # at most this many creatures within that distance, the added one not counted
CLEAR_OF_PEOPLE = 60.0      # yards from anyone a player can talk to (quest givers, vendors, guards)
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
        for objective in quest.get("objectives") or []:
            if "talk" in objective:
                creatures.add(int(objective["talk"]))
        creatures.update(int(c["like"]) for c in quest.get("creatures") or [] if str(c.get("like", "")).isdigit())
    creature_rows = sql(f"SELECT entry, name, lootid FROM creature_template WHERE entry IN ({numbers(creatures)}) AND entry < {FIRST_CREATURE}")
    item_rows = sql(f"SELECT entry, name FROM item_template WHERE entry IN ({numbers(items)})")
    found = {int(r[0]): {"name": r[1], "loot": int(r[2])} for r in creature_rows}
    for quest in quests:                                # the people and enemies the files themselves make
        for new in quest.get("creatures") or []:
            if isinstance(new.get("id"), int) and new.get("like") in found:
                found[new["id"]] = {"name": new.get("name", ""), "loot": found[new["like"]]["loot"], "new": True}
    return (found, {int(r[0]): r[1] for r in item_rows})


def counted(quest):
    """The tasks the game counts in its four "kill" places, in order: enemies to kill, places to reach, people to hear.
    Each as (what the game counts, how many, the objective)."""
    slots = []
    for objective in quest.get("objectives") or []:
        if "creature" in objective:
            slots.append((objective["creature"], objective.get("count", 1), objective))
    for objective in quest.get("objectives") or []:
        if "place" in objective or "talk" in objective:
            slots.append((FIRST_CREDIT + (quest["id"] - FIRST_ID) * 4 + len(slots), 1, objective))
    return slots


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
    need(quest.get("class") is None or quest.get("class") in CLASSES, "class: leave out for everyone, or one of " + ", ".join(CLASSES))
    need(quest.get("perk") is None or quest.get("perk") in PERKS, "perk: leave out, or one of " + ", ".join(PERKS))
    need(quest.get("type") in TYPES, "type: " + ", ".join(TYPES))
    need(isinstance(quest.get("zone"), int) and quest["zone"] > 0, "zone: the number of the zone it belongs to")
    for who in ("giver", "turn_in"):
        need(isinstance(quest.get(who), int) and quest[who] in creatures, f"{who}: the number of a creature that exists")

    objectives = quest.get("objectives") or []
    kills = [o for o in objectives if "creature" in o]
    brings = [o for o in objectives if "item" in o]
    visits = [o for o in objectives if "place" in o or "talk" in o]
    need(len(kills) + len(visits) <= 4, "at most four of: kinds of enemy to kill, places to reach, people to hear")
    need(len(brings) <= 6, "at most six kinds of item to bring")
    for objective in visits:
        need(isinstance(objective.get("text"), str) and 3 <= len(objective["text"]) <= 60, "a place or a talk needs \"text\": the line in the quest log, 3 to 60 letters")
        if "place" in objective:
            place = objective["place"]
            need(isinstance(place, dict) and all(isinstance(place.get(k), (int, float)) for k in ("map", "x", "y")), "place: needs map, x and y")
            need(isinstance(place, dict) and 5 <= place.get("radius", 25) <= 150, "place: radius from 5 to 150 yards")
        else:
            need(objective["talk"] in creatures, f"talk: creature {objective['talk']} does not exist")
            says = objective.get("says") or []
            need(isinstance(says, list) and 1 <= len(says) <= 6 and all(isinstance(line, str) and 1 <= len(line) <= 250 for line in says),
                 "talk: \"says\" is one to six lines of at most 250 letters, which the person says to the player")
    for new in quest.get("creatures") or []:
        need(isinstance(new.get("id"), int) and FIRST_CREATURE < new["id"] <= LAST_CREATURE, f"creatures: id from {FIRST_CREATURE + 1} to {LAST_CREATURE}")
        need(new.get("like") in creatures and not creatures.get(new.get("like"), {}).get("new"), f"creatures: \"like\" must be an existing creature of the game ({new.get('like')})")
        need(isinstance(new.get("name"), str) and 2 <= len(new["name"]) <= 60, "creatures: a name of 2 to 60 letters")
        need(all(isinstance(new.get(k), (int, float)) for k in ("map", "x", "y", "z")), "creatures: each needs map, x, y and z")
        need(new.get("kind") in ("person", "enemy"), "creatures: kind is person or enemy")
    for objective in objectives:
        if "place" in objective or "talk" in objective:
            continue
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
    if kind == "visit":
        need(bool(visits), "a visit quest needs a place to reach or someone to hear")
    if kind in ("kill", "slay"):
        need(bool(kills), "a kill quest needs at least one enemy")
        need(kind != "slay" or (len(kills) == 1 and kills[0].get("count") == 1 and not brings and not visits), "slay: exactly one enemy, once")
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
    lives = {}                                          # creature -> the zone it stands in, for a hand-in outside the quest's own zone
    for entry, map_id, x, y, zone_id in sql(f"SELECT id, map, position_x, position_y, zoneId FROM creature WHERE id IN ({numbers(wanted)}) AND guid < {FIRST_SPAWN}"):
        places.setdefault(int(entry), []).append((int(map_id), float(x), float(y)))
        lives.setdefault(int(entry), int(zone_id))
    for quest in quests:
        for spawn in quest.get("spawns") or []:
            places.setdefault(int(spawn["creature"]), []).append((int(spawn["map"]), float(spawn["x"]), float(spawn["y"])))
        for new in quest.get("creatures") or []:
            places.setdefault(int(new["id"]), []).append((int(new["map"]), float(new["x"]), float(new["y"])))
    talked = {o["talk"] for quest in quests for o in quest.get("objectives") or [] if "talk" in o}
    missing = [c for c in talked if c not in places]
    if missing:
        for entry, map_id, x, y in sql(f"SELECT id, map, position_x, position_y FROM creature WHERE id IN ({numbers(missing)}) AND guid < {FIRST_SPAWN}"):
            places.setdefault(int(entry), []).append((int(map_id), float(x), float(y)))

    zones = map_areas()
    out = [f"DELETE FROM quest_poi WHERE QuestID BETWEEN {FIRST_ID} AND {LAST_ID};",
           f"DELETE FROM quest_poi_points WHERE QuestID BETWEEN {FIRST_ID} AND {LAST_ID};"]
    for quest in quests:
        number, zone = quest["id"], zones.get(quest["zone"], 0)
        blob = 0

        def mark(objective_index, points, map_id, on_map=None):
            nonlocal blob
            out.append(f"INSERT INTO quest_poi (QuestID, id, ObjectiveIndex, MapID, WorldMapAreaId, Floor, Priority, Flags, VerifiedBuild) "
                       f"VALUES ({number}, {blob}, {objective_index}, {map_id}, {on_map or zone}, 0, 0, 1, 0);")
            for index, (x, y) in enumerate(points):
                out.append(f"INSERT INTO quest_poi_points (QuestID, Idx1, Idx2, X, Y, VerifiedBuild) VALUES ({number}, {blob}, {index}, {int(round(x))}, {int(round(y))}, 0);")
            blob += 1

        item_index = 4                                  # the game counts enemies, places and talks from 0 and items from 4
        slot_of = {id(objective): index for index, (_, _, objective) in enumerate(counted(quest))}
        for objective in quest.get("objectives") or []:
            if "place" in objective:
                place = objective["place"]
                ring = [(place["x"] + place.get("radius", 25) * math.cos(step * math.pi / 4), place["y"] + place.get("radius", 25) * math.sin(step * math.pi / 4)) for step in range(8)]
                mark(slot_of[id(objective)], ring, int(place["map"]))
                continue
            if "talk" in objective:
                at = places.get(objective["talk"])
                if at:
                    mark(slot_of[id(objective)], [(at[0][1], at[0][2])], at[0][0])
                continue
            if "creature" in objective:
                sources, index = [objective["creature"]], slot_of[id(objective)]
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
            mark(-1, [(ender[0][1], ender[0][2])], ender[0][0], zones.get(lives.get(quest["turn_in"], 0)))
    return "\n".join(out) + "\n"


def neighbours(map_id):
    """Every creature the game itself has on a map, as (x, y, is someone to talk to)."""
    rows = sql(f"SELECT c.position_x, c.position_y, (t.npcflag <> 0) FROM creature c JOIN creature_template t ON t.entry = c.id "
               f"WHERE c.map = {int(map_id)} AND c.guid < {FIRST_SPAWN} AND t.type NOT IN (8, 10)")
    return [(float(x), float(y), flag == "1") for x, y, flag in rows]


def crowding(x, y, others):
    """Why an enemy should not be added at this place, or '' if there is room. `others`: (x, y, is a person)."""
    in_sight = 0
    for ox, oy, person in others:
        distance = math.hypot(x - ox, y - oy)
        if distance < 0.5:
            continue                                    # itself
        if person and distance < CLEAR_OF_PEOPLE:
            return f"only {distance:.0f} yards from someone players talk to (at least {CLEAR_OF_PEOPLE:.0f})"
        if distance < ROOM:
            return f"only {distance:.0f} yards from another creature (at least {ROOM:.0f})"
        if distance <= IN_SIGHT:
            in_sight += 1
    if in_sight > MOST_IN_SIGHT:
        return f"{in_sight} creatures within {IN_SIGHT:.0f} yards already (at most {MOST_IN_SIGHT})"
    return ""


def crowded(quests):
    """Problems with the enemies the quests add, as (quest id, text)."""
    found, by_map = [], {}
    for quest in quests:
        for spawn in quest.get("spawns") or []:
            by_map.setdefault(int(spawn["map"]), []).append((quest["id"], spawn))
    for map_id, spawns in by_map.items():
        others = neighbours(map_id) + [(float(s["x"]), float(s["y"]), False) for _, s in spawns]
        for number, spawn in spawns:
            why = crowding(float(spawn["x"]), float(spawn["y"]), others)
            if why:
                found.append((number, f"the added enemy at {spawn['x']}, {spawn['y']} is {why}"))
    return found


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
    for table, column in (("creature_template", "entry"), ("creature_template_model", "CreatureID"), ("creature_equip_template", "CreatureID"),
                          ("creature_template_addon", "entry")):
        out.append(f"DELETE FROM {table} WHERE {column} BETWEEN {FIRST_CREATURE} AND {LAST_CREDIT};")
    out += ["DROP TABLE IF EXISTS wowplus_quest_perks;",
            "CREATE TABLE wowplus_quest_perks (quest INT UNSIGNED NOT NULL PRIMARY KEY, perk VARCHAR(32) NOT NULL) "
            "COMMENT='WowPlus: lasting perks our quests give when handed in. Made by Settings/Scripts/Make-Quests.py';",
            "DROP TABLE IF EXISTS wowplus_quest_goals;",
            "CREATE TABLE wowplus_quest_goals (quest INT UNSIGNED NOT NULL, credit INT UNSIGNED NOT NULL, kind TINYINT UNSIGNED NOT NULL, map INT UNSIGNED NOT NULL, "
            "x FLOAT NOT NULL, y FLOAT NOT NULL, radius FLOAT NOT NULL, creature INT UNSIGNED NOT NULL, says TEXT, PRIMARY KEY (quest, credit)) "
            "COMMENT='WowPlus: places to reach and people to hear for our quests. Made by Settings/Scripts/Make-Quests.py';"]
    people, listeners = set(), set()
    spawn_number = FIRST_SPAWN
    # People and enemies with names of their own: each a copy of a creature the game has, standing at one place
    for quest in quests:
        for new in quest.get("creatures") or []:
            number, like, person = new["id"], new["like"], new["kind"] == "person"
            out += ["DROP TEMPORARY TABLE IF EXISTS wp_new;",
                    f"CREATE TEMPORARY TABLE wp_new AS SELECT * FROM creature_template WHERE entry = {like};",
                    f"UPDATE wp_new SET entry = {number}, name = {text(new['name'])}, subname = {text(new.get('title', ''))}, AIName = '', ScriptName = '', "
                    f"gossip_menu_id = 0, KillCredit1 = 0, KillCredit2 = 0, npcflag = {3 if person else 0}, "
                    f"`rank` = {int(new.get('rank', 0)) if not person else '`rank`'}, HealthModifier = HealthModifier * {float(new.get('health', 1))}, DamageModifier = DamageModifier * {float(new.get('damage', 1))}"
                    + (f", minlevel = {int(new['level'])}, maxlevel = {int(new['level'])}" if isinstance(new.get("level"), int) else "") + ";",
                    "INSERT INTO creature_template SELECT * FROM wp_new;", "DROP TEMPORARY TABLE wp_new;",
                    f"INSERT INTO creature_template_model (CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild) "
                    f"SELECT {number}, Idx, CreatureDisplayID, DisplayScale * {float(new.get('size', 1))}, Probability, VerifiedBuild FROM creature_template_model "
                    f"WHERE CreatureID = {int(new.get('looks_like', like))};",
                    f"INSERT INTO creature_equip_template (CreatureID, ID, ItemID1, ItemID2, ItemID3, VerifiedBuild) "
                    f"SELECT {number}, ID, ItemID1, ItemID2, ItemID3, VerifiedBuild FROM creature_equip_template WHERE CreatureID = {like};"]
            spawn_number += 1
            out.append("INSERT INTO creature (guid, id, map, zoneId, areaId, spawnMask, phaseMask, equipment_id, position_x, position_y, position_z, orientation, "
                       "spawntimesecs, wander_distance, currentwaypoint, curhealth, curmana, MovementType, npcflag, unit_flags, dynamicflags, ScriptName, VerifiedBuild, CreateObject, Comment) "
                       f"SELECT {spawn_number}, {number}, {int(new['map'])}, {int(quest['zone'])}, 0, 1, 1, (SELECT IF(COUNT(*) > 0, 1, 0) FROM creature_equip_template WHERE CreatureID = {like}), "
                       f"{float(new['x'])}, {float(new['y'])}, {float(new['z'])}, {float(new.get('o', 0))}, {int(new.get('respawn', 300))}, {0 if person else 4}, 0, 1, 0, {0 if person else 1}, "
                       f"0, 0, 0, '', 0, 0, '{LOOT_MARK} {quest['id']}';")
    for quest in quests:
        number = quest["id"]
        objectives = quest.get("objectives") or []
        slots = counted(quest)
        kills = padded([(credit, count) for credit, count, _ in slots], 4)
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
        for index, (credit, _, objective) in enumerate(slots, 1):
            if "place" not in objective and "talk" not in objective:
                continue
            columns.append(f"ObjectiveText{index}")
            values.append(text(objective["text"]))
            place = objective.get("place") or {}
            talk = "talk" in objective
            says = "\n".join(objective.get("says") or [])
            out.append(f"INSERT INTO wowplus_quest_goals VALUES ({number}, {credit}, {TALK if talk else PLACE}, {int(place.get('map', 0))}, {float(place.get('x', 0))}, "
                       f"{float(place.get('y', 0))}, {float(place.get('radius', 25))}, {int(objective.get('talk', 0))}, {text(says).replace('$B', chr(92) + 'n')});")
            # What the game counts must exist as a creature, though it is never seen
            out.append(f"INSERT IGNORE INTO creature_template (entry, name, minlevel, maxlevel, faction, unit_class, type, flags_extra) "
                       f"VALUES ({credit}, {text(objective['text'])}, 1, 1, 35, 1, 10, 130);")
            out.append(f"INSERT IGNORE INTO creature_template_model (CreatureID, Idx, CreatureDisplayID, DisplayScale, Probability, VerifiedBuild) VALUES ({credit}, 0, 11686, 1, 1, 0);")
            if talk:
                listeners.add(objective["talk"])
        out.append(f"INSERT INTO quest_template ({', '.join(columns)}) VALUES ({', '.join(str(v) for v in values)});")
        out.append(f"INSERT INTO quest_template_addon (ID, MaxLevel, AllowableClasses, PrevQuestID) "
                   f"VALUES ({number}, {quest.get('max_level', 0)}, {CLASSES.get(quest.get('class'), 0)}, {quest.get('after', 0)});")
        if quest.get("perk"):
            out.append(f"INSERT INTO wowplus_quest_perks (quest, perk) VALUES ({number}, '{quest['perk']}');")
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
    if listeners:
        out.append(f"UPDATE creature_template SET npcflag = npcflag | 1 WHERE entry IN ({numbers(listeners)});  -- and these to be spoken to")
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
    # Enemies that would crowd a place are named, and their quest is left out until they are moved
    tight = crowded(good)
    for number, why in tight:
        problems.append(f"quest {number}: {why}")
    good = [quest for quest in good if quest["id"] not in {number for number, _ in tight}]
    return good, problems, creatures, items


def reload_server():
    """Tells the running world server to read quests again. Nothing happens if it is not running."""
    sender = os.path.join(HERE, "Send-Server-Command.ps1")
    for command in ("reload quest_template", "reload creature_queststarter", "reload creature_questender", "reload creature_loot_template", "wowplus quests"):
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
