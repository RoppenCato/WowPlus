"""The quest tool: a page in your browser for making new quests.

Start it with Quest-Tool.bat. It opens http://127.0.0.1:8590 and stays running in its window until
you close it. The page reads the game's own data through this script (zones, the people and enemies
in them, items), draws the zone from the server's map files, and saves each quest as a file in
Settings/Quests. "Put into the game" runs Make-Quests.py.

Without the page, for looking things up while writing a quest file by hand:

    python Settings/Scripts/Quest-Tool.py --zones 10          zones that suit level 10
    python Settings/Scripts/Quest-Tool.py --zone Durotar      who and what is in a zone
    python Settings/Scripts/Quest-Tool.py --zone 14 --side alliance
    python Settings/Scripts/Quest-Tool.py --rewards 10        reward ideas for a level 10 quest

How a quest file looks is in Settings/Quests/README.md.
"""
import argparse
import array
import importlib.util
import io
import json
import os
import random
import re
import struct
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DATA = os.path.join(ROOT, "Data")
PAGE = os.path.join(ROOT, "Settings", "Quest-Tool", "index.html")
CACHE = os.path.join(ROOT, "Database", "QuestTool")
PORT = 8590
TILE = 533.3333333                  # yards across one map tile
MAP_WIDTH = 150                     # how many points across the zone the terrain is sampled at
HORDE, ALLIANCE = 4, 2              # the groups a faction can be friendly or hostile to

spec = importlib.util.spec_from_file_location("make_quests", os.path.join(HERE, "Make-Quests.py"))
quests = importlib.util.module_from_spec(spec)
spec.loader.exec_module(quests)
sql = quests.sql


# ---------------------------------------------------------------------------------------------
# The client's data files
# ---------------------------------------------------------------------------------------------

def dbc(name):
    with open(os.path.join(DATA, "dbc", name), "rb") as f:
        data = f.read()
    magic, records, fields, size, strings = struct.unpack_from("<4sIIII", data)
    block = data[20 + records * size:]
    rows = [data[20 + i * size: 20 + (i + 1) * size] for i in range(records)]

    def text(offset):
        return block[offset:block.find(b"\0", offset)].decode("utf-8", "replace")
    return rows, fields, text


class World:
    """Everything the page asks for, read once and kept."""

    def __init__(self):
        rows, fields, text = dbc("AreaTable.dbc")
        self.areas = {}
        for row in rows:
            v = struct.unpack_from("<%dI" % fields, row)
            self.areas[v[0]] = {"name": text(v[11]), "map": v[1], "parent": v[2]}

        rows, fields, text = dbc("WorldMapArea.dbc")
        self.bounds = {}                                # zone -> where its map sits in the world
        for row in rows:
            v = struct.unpack_from("<4I4f", row)
            if v[2]:
                self.bounds[v[2]] = {"map": v[1], "left": v[4], "right": v[5], "top": v[6], "bottom": v[7]}

        rows, fields, text = dbc("FactionTemplate.dbc")
        self.factions = {}
        for row in rows:
            v = struct.unpack_from("<%dI" % fields, row)
            self.factions[v[0]] = {"own": v[3], "friend": v[4], "enemy": v[5]}

        rows, fields, text = dbc("QuestXP.dbc")
        self.quest_xp = {struct.unpack_from("<I", row)[0]: struct.unpack_from("<%dI" % fields, row)[1:] for row in rows}

        self.templates = {}
        for r in sql("SELECT entry, name, IFNULL(subname, ''), minlevel, maxlevel, faction, npcflag, `rank`, type, lootid FROM creature_template"):
            self.templates[int(r[0])] = {"entry": int(r[0]), "name": r[1], "title": r[2], "min": int(r[3]), "max": int(r[4]), "faction": int(r[5]),
                                         "flags": int(r[6]), "rank": int(r[7]), "type": int(r[8]), "loot": int(r[9])}
        self.givers = {int(r[0]) for r in sql("SELECT DISTINCT id FROM creature_queststarter")}

        # How high a zone's quests are: the middle half of them, so that a few odd ones do not stretch it
        levels = {}
        for zone, level in sql("SELECT QuestSortID, QuestLevel FROM quest_template WHERE QuestSortID > 0 AND QuestLevel > 0 AND MinLevel > 0"):
            levels.setdefault(int(zone), []).append(int(level))
        self.zones = []
        for zone, found in levels.items():
            if len(found) < 8 or zone not in self.bounds or zone not in self.areas:
                continue
            found.sort()
            self.zones.append({"id": zone, "name": self.areas[zone]["name"], "low": found[len(found) // 4], "high": found[len(found) * 3 // 4],
                               "quests": len(found)})
        self.zones.sort(key=lambda z: (z["low"], z["name"]))
        self.tiles = {}
        self.lock = threading.Lock()

    # -----------------------------------------------------------------------------------------

    def stance(self, faction, side):
        """friendly, hostile or neutral towards a player of that side ("both": towards either)."""
        template = self.factions.get(faction)
        if not template:
            return "neutral"
        groups = {"horde": HORDE, "alliance": ALLIANCE}.get(side, HORDE | ALLIANCE)
        if template["enemy"] & (groups | 1) and not template["friend"] & groups:
            return "hostile"
        if (template["friend"] | template["own"]) & groups:
            return "friendly"
        return "neutral"

    def to_map(self, zone, x, y):
        """A place in the world as a point on the zone's map, 0 to 1 from the top left."""
        b = self.bounds[zone]
        return (b["left"] - y) / (b["left"] - b["right"]), (b["top"] - x) / (b["top"] - b["bottom"])

    def zone(self, zone, side):
        """Who could give a quest here, and what there is to fight."""
        spawns = {}
        for entry, x, y in sql(f"SELECT id, position_x, position_y FROM creature WHERE zoneId = {int(zone)} AND map = {self.bounds[zone]['map']} "
                               "AND guid NOT IN (SELECT guid FROM game_event_creature)"):
            spawns.setdefault(int(entry), []).append(self.to_map(zone, float(x), float(y)))
        people, enemies = [], []
        for entry, places in spawns.items():
            t = self.templates.get(entry)
            if not t or not t["name"] or t["type"] in (8, 10, 11, 12) or re.search(r"\[|trigger|bunny|invisible|\(\d\)", t["name"], re.I):
                continue                                # critters, pets, totems and the game's own helpers
            where = [[round(px, 4), round(py, 4)] for px, py in places if -0.05 <= px <= 1.05 and -0.05 <= py <= 1.05]
            if not where:
                continue
            row = {"entry": entry, "name": t["name"], "title": t["title"], "level": t["max"], "low": t["min"], "count": len(places),
                   "rank": t["rank"], "at": where[:60]}
            stance = self.stance(t["faction"], side)
            if stance == "friendly" and len(places) <= 4:
                row["gives"] = entry in self.givers
                row["kind"] = ("innkeeper" if t["flags"] & 65536 else "flight master" if t["flags"] & 8192 else "trainer" if t["flags"] & 16
                               else "vendor" if t["flags"] & 128 else "guard" if t["flags"] & 268435456 else "")
                people.append(row)
            elif stance != "friendly":
                row["hostile"] = stance == "hostile"
                row["loot"] = t["loot"] > 0
                enemies.append(row)
        people.sort(key=lambda p: (not p["gives"], p["name"]))
        enemies.sort(key=lambda e: (e["level"], e["name"]))
        return {"people": people, "enemies": enemies}

    def loot(self, creature):
        """What a creature drops that could be asked for: its plain drops, likeliest first."""
        t = self.templates.get(int(creature))
        if not t or not t["loot"]:
            return []
        rows = sql("SELECT l.Item, i.name, l.Chance, i.Quality, i.class FROM creature_loot_template l JOIN item_template i ON i.entry = l.Item "
                   f"WHERE l.Entry = {t['loot']} AND l.Reference = 0 AND l.QuestRequired = 0 AND i.class NOT IN (2, 4) ORDER BY l.Chance DESC LIMIT 25")
        return [{"item": int(r[0]), "name": r[1], "chance": round(float(r[2]), 1), "quality": int(r[3])} for r in rows]

    def search(self, kind, words):
        words = re.sub(r"[%_'\\]", "", words)[:40]
        if len(words) < 3:
            return []
        if kind == "item":
            rows = sql(f"SELECT entry, name, Quality, RequiredLevel FROM item_template WHERE name LIKE '%{words}%' ORDER BY (name LIKE '{words}%') DESC, name LIMIT 30")
            return [{"item": int(r[0]), "name": r[1], "quality": int(r[2]), "level": int(r[3])} for r in rows]
        rows = sql(f"SELECT entry, name, maxlevel FROM creature_template WHERE name LIKE '%{words}%' ORDER BY (name LIKE '{words}%') DESC, name LIMIT 30")
        return [{"entry": int(r[0]), "name": r[1], "level": int(r[2])} for r in rows]

    def rewards(self, level, seed=0):
        """What a quest of this level usually pays, and gear it could offer."""
        level = max(1, min(80, int(level)))
        money = sql(f"SELECT ROUND(AVG(RewardMoney)) FROM quest_template WHERE QuestLevel = {level} AND RewardMoney > 0 AND QuestType = 2")
        xp = self.quest_xp.get(level, [0] * 10)
        rows = sql("SELECT entry, name, Quality, class, subclass, InventoryType, RequiredLevel FROM item_template "
                   f"WHERE class IN (2, 4) AND Quality IN (2, 3) AND RequiredLevel <= {level} AND ItemLevel BETWEEN {max(1, level - 1)} AND {level + 7} "
                   "AND InventoryType > 0 AND RandomProperty = 0 AND RandomSuffix = 0 AND name NOT LIKE '%Test%' AND name NOT LIKE '%Deprecated%' AND name NOT LIKE '%(%' "
                   "AND entry IN (SELECT RewardChoiceItemID1 FROM quest_template UNION SELECT RewardChoiceItemID2 FROM quest_template "
                   "UNION SELECT RewardChoiceItemID3 FROM quest_template UNION SELECT RewardItem1 FROM quest_template)")
        gear = [{"item": int(r[0]), "name": r[1], "quality": int(r[2]), "kind": self.kind_of(int(r[3]), int(r[4]), int(r[5])), "level": int(r[6])} for r in rows]
        random.Random(f"{level}:{seed}").shuffle(gear)
        gear = sorted(gear[:36], key=lambda g: (g["kind"], g["name"]))
        potions = sql("SELECT entry, name FROM item_template WHERE class = 0 AND subclass IN (1, 2, 3) AND Quality = 1 "
                      f"AND RequiredLevel BETWEEN {max(0, level - 8)} AND {level} AND name NOT LIKE '%Test%' AND name NOT LIKE '%(%' AND BuyPrice > 0 ORDER BY RequiredLevel DESC LIMIT 6")
        return {"money": int(float(money[0][0])) if money and money[0][0] not in ("NULL", "") else level * level * 5,
                "xp": list(xp), "gear": gear, "other": [{"item": int(r[0]), "name": r[1]} for r in potions]}

    @staticmethod
    def kind_of(cls, subclass, slot):
        if cls == 2:
            return "Weapon"
        if slot in (2, 11, 12):
            return {2: "Neck", 11: "Ring", 12: "Trinket"}[slot]
        if slot == 16:
            return "Cloak"
        return {1: "Cloth", 2: "Leather", 3: "Mail", 4: "Plate", 6: "Shield"}.get(subclass, "Other")

    # -----------------------------------------------------------------------------------------
    # The lie of the land, from the server's own map files
    # -----------------------------------------------------------------------------------------

    def tile(self, map_id, gx, gy):
        """One map tile: its heights (129 x 129) and where its water stands, or None if there is no land there."""
        key = (map_id, gx, gy)
        if key in self.tiles:
            return self.tiles[key]
        found = None
        try:
            with open(os.path.join(DATA, "maps", "%03u%02u%02u.map" % (map_id, gx, gy)), "rb") as f:
                data = f.read()
            area_at, area_size, height_at, height_size, liquid_at, liquid_size = struct.unpack_from("<6I", data, 12)
            flags, low, high = struct.unpack_from("<Iff", data, height_at + 4)
            if flags & 1:                               # flat
                heights = None
            elif flags & 2:
                raw = array.array("H", data[height_at + 16: height_at + 16 + 129 * 129 * 2])
                step = (high - low) / 65535.0
                heights = [low + v * step for v in raw]
            elif flags & 4:
                raw = data[height_at + 16: height_at + 16 + 129 * 129]
                step = (high - low) / 255.0
                heights = [low + v * step for v in raw]
            else:
                heights = list(array.array("f", data[height_at + 16: height_at + 16 + 129 * 129 * 4]))
            water = None
            if liquid_at:
                liquid_flags, _, _, off_x, off_y, width, height, level = struct.unpack_from("<BBHBBBBf", data, liquid_at + 4)
                water = (off_x, off_y, width, height, level)
            found = (heights, low, water)
        except (OSError, struct.error):
            pass
        self.tiles[key] = found
        return found

    def height(self, map_id, x, y):
        """(height, is water) at a place in the world. None where there is no land."""
        fx, fy = 32 - x / TILE, 32 - y / TILE
        gx, gy = int(fx), int(fy)
        tile = self.tile(map_id, gx, gy)
        if not tile:
            return None
        heights, low, water = tile
        ix, iy = min(128, int((fx - gx) * 128)), min(128, int((fy - gy) * 128))
        h = heights[ix * 129 + iy] if heights else low
        wet = False
        if water:
            off_x, off_y, width, depth, level = water
            wet = off_x <= ix < off_x + width and off_y <= iy < off_y + depth and h < level
        return h, wet

    def terrain(self, zone):
        """The zone as rows of heights for the page to shade, with -9999 for no land and water marked apart."""
        path = os.path.join(CACHE, f"terrain-{int(zone)}.json")
        try:
            with io.open(path, encoding="utf-8") as f:
                return json.load(f)
        except (OSError, ValueError):
            pass
        b = self.bounds[zone]
        wide = MAP_WIDTH
        tall = max(10, int(round(wide * abs(b["top"] - b["bottom"]) / abs(b["left"] - b["right"]))))
        heights, water = [], []
        with self.lock:
            for j in range(tall):
                x = b["top"] - (j + 0.5) / tall * (b["top"] - b["bottom"])
                for i in range(wide):
                    y = b["left"] - (i + 0.5) / wide * (b["left"] - b["right"])
                    found = self.height(b["map"], x, y)
                    heights.append(-9999 if found is None else int(round(found[0])))
                    water.append(1 if found and found[1] else 0)
            self.tiles.clear()
        result = {"wide": wide, "tall": tall, "heights": heights, "water": water}
        os.makedirs(CACHE, exist_ok=True)
        with io.open(path, "w", encoding="utf-8") as f:
            json.dump(result, f)
        return result


# ---------------------------------------------------------------------------------------------
# Writing the texts with Claude (the same way the bots talk)
# ---------------------------------------------------------------------------------------------

def write_texts(world, quest):
    sys.path.insert(0, HERE)
    import bot_minds
    creatures, items = quests.known([quest])
    name = lambda entry: creatures.get(entry, {}).get("name", "someone")
    tasks = []
    for objective in quest.get("objectives") or []:
        if "creature" in objective:
            tasks.append(f"kill {objective['count']} of the creature called {name(objective['creature'])}")
        if "item" in objective:
            source = ", ".join(name(d["creature"]) for d in objective.get("drops_from") or [])
            tasks.append(f"bring {objective['count']} of the item {items.get(objective['item'], 'something')}" + (f" (taken from {source})" if source else ""))
    zone = world.areas.get(quest.get("zone"), {}).get("name", "the land")
    system = ("You write quests for World of Warcraft (the original game through Wrath of the Lich King) in the voice of the game itself: "
              "the quest giver speaks to the player, plainly and in character, with a reason for the task that belongs to the place. "
              "No modern words, no jokes that break the world, no lists. Use $N for the player's name at most once, $C for their class "
              "if it helps. Separate paragraphs with a blank line. Answer with one JSON object and nothing else, with the keys: "
              '"title" (2 to 5 words), "details" (what the giver says when offering it, 50 to 90 words), "objectives" (one sentence saying '
              'exactly what to do and whom to return to), "progress" (what the person says while it is not done, one sentence), '
              '"completion" (what the person says when it is done, 25 to 50 words).')
    request = (f"A level {quest.get('level')} quest in {zone} for {quest.get('side', 'both')} players. Given by {name(quest.get('giver'))}, "
               f"handed in to {name(quest.get('turn_in'))}. The task: {'; '.join(tasks) or 'go and speak to the other person'}."
               + (f" What I have in mind: {quest['idea']}" if quest.get("idea") else "") + "\nWrite the JSON.")
    minds = bot_minds.Minds(os.path.join(ROOT, "Settings", "Bots"), os.path.join(ROOT, "Database", "BotChat"), "wow", log=lambda text: None)
    raw, error = minds.ask(system, request, model="sonnet", timeout=120)
    if error:
        return {"error": error}
    found = re.search(r"\{.*\}", raw, re.S)
    try:
        return json.loads(found.group(0)) if found else {"error": "no texts came back"}
    except ValueError:
        return {"error": "the answer could not be read"}


# ---------------------------------------------------------------------------------------------

world = None


class Handler(BaseHTTPRequestHandler):
    def send(self, body, kind="application/json; charset=utf-8", status=200):
        data = body if isinstance(body, bytes) else json.dumps(body, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", kind)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        url = urlparse(self.path)
        q = {k: v[0] for k, v in parse_qs(url.query).items()}
        try:
            if url.path == "/":
                with open(PAGE, "rb") as f:
                    self.send(f.read(), "text/html; charset=utf-8")
            elif url.path == "/api/zones":
                self.send({"zones": world.zones, "types": quests.TYPES})
            elif url.path == "/api/zone":
                self.send(world.zone(int(q["id"]), q.get("side", "both")))
            elif url.path == "/api/terrain":
                self.send(world.terrain(int(q["id"])))
            elif url.path == "/api/loot":
                self.send(world.loot(q["creature"]))
            elif url.path == "/api/search":
                self.send(world.search(q.get("kind", "item"), q.get("q", "")))
            elif url.path == "/api/rewards":
                self.send(world.rewards(q.get("level", 1), q.get("seed", 0)))
            elif url.path == "/api/quests":
                listed = []
                for path, quest in quests.load_quests():
                    listed.append({"file": os.path.basename(path), "quest": quest if isinstance(quest, dict) else None,
                                   "error": None if isinstance(quest, dict) else quest})
                self.send({"quests": listed, "next_id": quests.next_id()})
            else:
                self.send({"error": "not found"}, status=404)
        except Exception as error:                      # the window must keep running
            self.send({"error": str(error)}, status=500)

    def do_POST(self):
        url = urlparse(self.path)
        try:
            body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", 0))).decode("utf-8") or "{}")
            if url.path == "/api/save":
                quest = body
                if not isinstance(quest.get("id"), int) or not quest["id"]:
                    quest["id"] = quests.next_id()
                creatures, items = quests.known([quest])
                others = {q["id"] for _, q in quests.load_quests() if isinstance(q, dict) and q.get("id") != quest["id"] and isinstance(q.get("id"), int)}
                wrong = quests.validate(quest, creatures, items, set(others))
                if wrong:
                    self.send({"saved": False, "problems": wrong, "id": quest["id"]})
                    return
                os.makedirs(quests.QUESTS_DIR, exist_ok=True)
                for old in os.listdir(quests.QUESTS_DIR):       # the title may have changed, and with it the file's name
                    if old.startswith(f"{quest['id']}-") and old.endswith(".json"):
                        os.remove(os.path.join(quests.QUESTS_DIR, old))
                name = quests.file_name(quest)
                with io.open(os.path.join(quests.QUESTS_DIR, name), "w", encoding="utf-8") as f:
                    json.dump(quest, f, indent=2, ensure_ascii=False)
                self.send({"saved": True, "file": name, "id": quest["id"]})
            elif url.path == "/api/apply":
                count, problems = quests.apply(reload=True)
                self.send({"count": count, "problems": problems})
            elif url.path == "/api/write":
                self.send(write_texts(world, body))
            else:
                self.send({"error": "not found"}, status=404)
        except Exception as error:
            self.send({"error": str(error)}, status=500)

    def log_message(self, *args):
        pass


def main():
    global world
    p = argparse.ArgumentParser(description="The quest tool")
    p.add_argument("--zones", type=int, help="list the zones that suit this level")
    p.add_argument("--zone", help="list who and what is in a zone (name or number)")
    p.add_argument("--side", default="horde", help="horde, alliance or both")
    p.add_argument("--rewards", type=int, help="reward ideas for a quest of this level")
    p.add_argument("--no-browser", action="store_true")
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    world = World()

    if a.zones:
        for zone in world.zones:
            if zone["low"] - 2 <= a.zones <= zone["high"] + 2:
                print(f"{zone['id']:>5}  {zone['name']:<28} levels {zone['low']} to {zone['high']}")
        return
    if a.zone:
        zone = next((z for z in world.zones if str(z["id"]) == a.zone or z["name"].lower() == a.zone.lower()), None)
        if not zone:
            print("No such zone. Try --zones <level>.")
            return
        found = world.zone(zone["id"], a.side)
        print(f"{zone['name']} ({zone['id']}), levels {zone['low']} to {zone['high']}, for {a.side}\n\nPeople who could give a quest:")
        for person in found["people"]:
            print(f"  {person['entry']:>6}  {person['name']}" + (f" <{person['title']}>" if person["title"] else "")
                  + (f" ({person['kind']})" if person["kind"] else "") + ("  gives quests already" if person["gives"] else ""))
        print("\nEnemies:")
        for enemy in found["enemies"]:
            print(f"  {enemy['entry']:>6}  {enemy['name']:<32} level {enemy['low']}-{enemy['level']}, {enemy['count']} of them"
                  + ("" if enemy["loot"] else ", drops nothing"))
        return
    if a.rewards:
        found = world.rewards(a.rewards)
        print(f"Level {a.rewards}: usual money {found['money']} copper; experience by difficulty 1 to 8: {found['xp'][1:9]}")
        for gear in found["gear"]:
            print(f"  {gear['item']:>6}  {gear['kind']:<8} {gear['name']} (needs level {gear['level']})")
        return

    server = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print(f"The quest tool is at http://127.0.0.1:{PORT}  ({len(world.zones)} zones). Close this window to stop it.", flush=True)
    if not a.no_browser:
        threading.Timer(0.5, lambda: webbrowser.open(f"http://127.0.0.1:{PORT}")).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
