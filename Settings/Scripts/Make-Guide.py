"""Writes what the WowPlus Guide addon knows about places, besides their loot
(Client/AddOns/WowPlusGuide/Places.lua). Run by Settings/Deploy-Settings.ps1 at every server start, after
Make-Boss-Loot.py (which writes the loot, Loot.lua); running it by hand is harmless.

    python Settings/Scripts/Make-Guide.py

For every dungeon and raid: its levels, where its entrance is, the quests that belong to it and who
gives them, and the stories of its bosses. And the champions of the world, with where they live and
what they drop. Everything is read from this server's own database and the client's data files, so
it shows our own quests and champions too. Explained in Settings/Docs/Guide.md.

The boss stories are taken from Spoken (github.com/rusty-key/spoken-wow, addons/Spoken_NPCs/Data/
Stories.lua), written there from warcraft.wiki.gg and Blizzard's Adventure Guide: CC BY-SA 4.0. A copy
of that file is kept in Settings/Reference/Boss-Stories.lua so that the server does not need the
download.
"""
import importlib.util
import io
import os
import re
import struct
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DBC = os.path.join(ROOT, "Data", "dbc")
OUT = os.path.join(ROOT, "Client", "AddOns", "WowPlusGuide", "Places.lua")
STORIES = os.path.join(ROOT, "Settings", "Reference", "Boss-Stories.lua")
LORE_IN = os.path.join(ROOT, "Settings", "Reference", "Zone-Lore")
LORE_OUT = os.path.join(ROOT, "Client", "AddOns", "WowPlusGuide", "Lore.lua")

spec = importlib.util.spec_from_file_location("boss_loot", os.path.join(HERE, "Make-Boss-Loot.py"))
loot = importlib.util.module_from_spec(spec)
spec.loader.exec_module(loot)


def sql(query):
    """Rows of a query. Unlike Make-Boss-Loot's, text may hold odd characters: rows are split on line
    feeds alone, and mysql's escapes (\\n, \\t, \\\\) are undone."""
    out = loot.subprocess.run([loot.MYSQL, "-uroot", "-P3307", "--protocol=TCP", "-hlocalhost", "-N", "-B", "--default-character-set=utf8mb4", "acore_world", "-e", query],
                              capture_output=True)       # bytes: text mode would turn a stray \r inside a quest into a new row
    if out.returncode != 0:
        raise RuntimeError(out.stderr.decode("utf-8", "replace").strip()[:300])
    out.stdout = out.stdout.decode("utf-8", "replace").replace("\r", "")
    unescape = {"n": "\n", "t": "\t", "0": "", "\\": "\\"}
    return [[re.sub(r"\\(.)", lambda m: unescape.get(m.group(1), m.group(1)), cell) for cell in line.split("\t")]
            for line in out.stdout.split("\n") if line]


CONTINENTS = (0, 1, 530, 571)
ALLIANCE_RACES, HORDE_RACES = 0x44D, 0x2B2          # human, dwarf, night elf, gnome, draenei / orc, undead, tauren, troll, blood elf


def table(name):
    """Rows of a client data file as (numbers, floats, string reader)."""
    with open(os.path.join(DBC, name), "rb") as f:
        data = f.read()
    _, records, fields, size, _ = struct.unpack_from("<4sIIII", data)
    block = data[20 + records * size:]
    rows = []
    for i in range(records):
        raw = data[20 + i * size: 20 + (i + 1) * size]
        rows.append((struct.unpack("<%dI" % fields, raw), struct.unpack("<%df" % fields, raw)))

    def text(offset):
        return block[offset:block.find(b"\0", offset)].decode("utf-8", "replace")
    return rows, text


def lua(text):
    return '"' + str(text).replace("\\", "\\\\").replace('"', '\\"').replace("\r", "").replace("\n", "\\n") + '"'


def key(name):
    """How Spoken files a boss: lower case, no punctuation."""
    return re.sub(r"\s+", " ", re.sub(r"[^a-z0-9 ]", "", name.lower().replace("-", " "))).strip()


def clean(text):
    """Quest text as the game shows it, with its codes made plain."""
    text = (text or "").replace("$B", "\n").replace("$b", "\n").replace("$N", "you").replace("$n", "you")
    text = re.sub(r"\$[Gg]\s*([^:;]*):([^;]*);", r"\1", text)
    text = re.sub(r"\$[CcRr]", "adventurer", text)
    return re.sub(r"\n{3,}", "\n\n", text).strip()


def write(path, text):
    """Writes a file unless it already says this. Returns True when nothing changed."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    try:
        with io.open(path, encoding="utf-8", newline="") as f:
            if f.read() == text:
                return True
    except OSError:
        pass
    with io.open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    return False


def lua_string(raw):
    """A Lua string literal's contents as text."""
    return raw.encode("utf-8").decode("unicode_escape").encode("latin-1").decode("utf-8")


def write_lore():
    """The lore of zones and the places in them, from Spoken's Spoken_Places (Settings/Reference/Zone-Lore),
    keyed by the zone's English name as the game says it. Returns how many places have lore."""
    try:
        with io.open(os.path.join(LORE_IN, "Zones.lua"), encoding="utf-8") as f:
            zones_text = f.read()
        with io.open(os.path.join(LORE_IN, "Subzones.lua"), encoding="utf-8") as f:
            subzones_text = f.read()
    except OSError:
        return 0
    field = lambda block, name: lua_string((re.search(name + r'\s*=\s*"((?:[^"\\]|\\.)*)"', block) or [None, ""])[1])
    zones = {}
    for number, block in re.findall(r"\n\t\[(\d+)\] = \{(.*?)\n\t\},", zones_text, re.S):
        if field(block, "full"):
            zones[int(number)] = {"name": field(block, "name"), "full": field(block, "full"), "source": field(block, "source"), "places": []}
    for number, block in re.findall(r"\n\t\[(\d+)\] = \{(.*?)\n\t\},", subzones_text, re.S):
        zone = zones.get(int(number))
        if not zone:
            continue
        for place in re.findall(r'\n\t\t\["[^"]+"\] = \{(.*?)\n\t\t\},', block, re.S):
            if field(place, "full"):
                zone["places"].append((field(place, "name"), field(place, "full"), field(place, "source")))
    out = ["-- Made by Settings/Scripts/Make-Guide.py from Spoken's lore (github.com/rusty-key/spoken-wow, addons/Spoken_Places).",
           "-- The text is from warcraft.wiki.gg, CC BY-SA 4.0 (each entry names its page), except where an entry has no",
           "-- page: that prose was written for Spoken. Keyed by the zone's English name; places = {name, text, page}.",
           "WowPlusGuideLore = {"]
    for zone in sorted(zones.values(), key=lambda zone: zone["name"]):
        places = ",".join("{%s,%s,%s}" % (lua(n), lua(t), lua(src)) for n, t, src in sorted(zone["places"]))
        out.append("  [%s] = { full = %s, source = %s, places = {%s} }," % (lua(zone["name"]), lua(zone["full"]), lua(zone["source"]), places))
    out.append("}")
    write(LORE_OUT, "\n".join(out) + "\n")
    return len(zones) + sum(len(zone["places"]) for zone in zones.values())


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    started = time.time()

    # Zones and where things are on their maps
    areas, area_text = table("AreaTable.dbc")
    area_name = {n[0]: area_text(n[11]) for n, _ in areas}
    zone_to_map = {n[0]: n[1] for n, _ in areas if n[2] == 0 and n[1] not in CONTINENTS}   # zone -> the map it is on
    maps_info, map_text = table("Map.dbc")
    instances = {n[0]: (map_text(n[5]), n[2] == 2) for n, _ in maps_info if n[2] in (1, 2)}   # dungeons and raids: name, raid?
    world_maps, _ = table("WorldMapArea.dbc")
    zone_maps = [(n[1], n[2], f[4], f[5], f[6], f[7]) for n, f in world_maps if n[2] and n[1] in CONTINENTS]

    def on_map(map_id, x, y):
        """(zone name, x %, y %) of a point, on the smallest zone map that holds it."""
        best = None
        for m, area, left, right, top, bottom in zone_maps:
            if m == map_id and right <= y <= left and bottom <= x <= top:
                size = (left - right) * (top - bottom)
                if not best or size < best[0]:
                    best = (size, area, (left - y) / (left - right) * 100, (top - x) / (top - bottom) * 100)
        if not best:
            return None
        return area_name.get(best[1], "?"), round(best[2], 1), round(best[3], 1)

    # Levels and pictures, from the dungeon finder's own table (holiday bosses left out: the Headless
    # Horseman would make the Scarlet Monastery a level 82 dungeon)
    levels, pictures = {}, {}
    dungeons, dungeon_text = table("LFGDungeons.dbc")
    for n, _ in dungeons:
        map_id, low, high, enter = n[23], n[21], n[22], n[18]
        if n[24] == 0 and low and not n[25] & 4:
            have = levels.get(map_id)
            levels[map_id] = (min(have[0], low), max(have[1], high), min(have[2], enter)) if have else (low, high, enter)
            pictures.setdefault(map_id, dungeon_text(n[28]))

    # Entrances: the doorways that lead into each place
    triggers, _ = table("AreaTrigger.dbc")
    trigger_at = {n[0]: (n[1], f[2], f[3]) for n, f in triggers}
    entrances = {}
    for number, target in sql("SELECT ID, target_map FROM areatrigger_teleport"):
        at = trigger_at.get(int(number))
        if at and at[0] in CONTINENTS and int(target) not in entrances:
            spot = on_map(*at)
            if spot:
                entrances[int(target)] = spot

    # Who gives a quest, and where they stand
    zone_of_spawn = {}
    for entry, zone in sql("SELECT id, MIN(zoneId) FROM creature GROUP BY id"):
        zone_of_spawn[int(entry)] = int(zone)
    givers = {}
    for quest, name, entry in sql("SELECT s.quest, t.name, t.entry FROM creature_queststarter s JOIN creature_template t ON t.entry = s.id"):
        givers.setdefault(int(quest), (name, area_name.get(zone_of_spawn.get(int(entry), 0), "")))
    for quest, name in sql("SELECT s.quest, t.name FROM gameobject_queststarter s JOIN gameobject_template t ON t.entry = s.id"):
        givers.setdefault(int(quest), (name, ""))
    for quest, name in sql("SELECT startquest, name FROM item_template WHERE startquest > 0"):
        givers.setdefault(int(quest), (name + " (an item)", ""))

    # A dungeon's quests: those the game files under it, and those that ask for an enemy that lives in
    # it or for something such an enemy drops (The Defias Brotherhood is filed under Westfall)
    lives_in = {}
    for entry, map_id in sql("SELECT DISTINCT id, map FROM creature"):
        if int(map_id) in instances:
            lives_in.setdefault(int(entry), set()).add(int(map_id))
    dropped_in = {}
    for item, entry in sql("SELECT l.Item, t.entry FROM creature_loot_template l JOIN creature_template t ON t.lootid = l.Entry WHERE l.QuestRequired = 1"):
        for map_id in lives_in.get(int(entry), ()):
            dropped_in.setdefault(int(item), set()).add(map_id)
    wanted = ", ".join(f"RequiredNpcOrGo{i}" for i in range(1, 5)) + ", " + ", ".join(f"RequiredItemId{i}" for i in range(1, 7))
    quests = {}
    for row in sql(f"SELECT ID, LogTitle, QuestLevel, MinLevel, AllowableRaces, QuestSortID, LogDescription, {wanted} FROM quest_template"):
        number, title, level, need, races, sort, summary = row[:7]
        places = set()
        if int(sort) > 0 and zone_to_map.get(int(sort)) in instances:
            places.add(zone_to_map[int(sort)])
        for creature in row[7:11]:
            places |= lives_in.get(int(creature), set()) if int(creature) > 0 else set()
        for item in row[11:17]:
            places |= dropped_in.get(int(item), set())
        if not places or int(number) not in givers or re.search(r"deprecated|unused|\[ph\]|test", title, re.I):
            continue
        races = int(races)
        side = "" if not races or (races & ALLIANCE_RACES and races & HORDE_RACES) else ("A" if races & ALLIANCE_RACES else "H")
        for map_id in places:
            quests.setdefault(map_id, []).append((int(level), title, side, givers[int(number)], clean(summary), int(number)))

    # Boss stories
    stories = {}
    try:
        with io.open(STORIES, encoding="utf-8") as f:
            text = f.read()
        for block in re.finditer(r"\[(\d+)\]\s*=\s*\{(.*?)\n\t\},", text, re.S):
            for name, story in re.findall(r'\["([^"]+)"\]\s*=\s*"((?:[^"\\]|\\.)*)"', block.group(2)):
                stories.setdefault(int(block.group(1)), {})[name] = story.encode("utf-8").decode("unicode_escape").encode("latin-1").decode("utf-8")
    except OSError:
        pass

    out = ["-- Made by Settings/Scripts/Make-Guide.py at every server start. Do not edit: it is written over.",
           "-- Per dungeon and raid (by map): levels {from, to, can enter at}, where the entrance is {zone, x, y},",
           "-- quests {level, title, side, giver, where the giver is, what it asks, quest number}, and boss stories.",
           "-- The boss stories are from Spoken (github.com/rusty-key/spoken-wow), written from warcraft.wiki.gg and",
           "-- Blizzard's Adventure Guide, CC BY-SA 4.0. Settings/Docs/Guide.md.",
           "WowPlusGuidePlaces = {"]
    maps = (set(levels) | set(entrances) | set(quests) | set(stories)) & set(instances)
    for map_id in sorted(maps):
        parts = ["name = %s" % lua(instances[map_id][0]), "raid = %s" % ("true" if instances[map_id][1] else "false")]
        if map_id in levels:
            parts.append("levels = {%d,%d,%d}" % levels[map_id])
        if pictures.get(map_id):
            parts.append("picture = %s" % lua(pictures[map_id]))
        if map_id in entrances:
            zone, x, y = entrances[map_id]
            parts.append("where = {%s,%s,%s}" % (lua(zone), x, y))
        if map_id in quests:
            rows = sorted(quests[map_id])
            parts.append("quests = {" + ",".join("{%d,%s,%s,%s,%s,%s,%d}" % (level, lua(title), lua(side), lua(giver[0]), lua(giver[1]), lua(summary), number)
                                                 for level, title, side, giver, summary, number in rows) + "}")
        if map_id in stories:
            parts.append("stories = {" + ",".join("[%s]=%s" % (lua(name), lua(story)) for name, story in sorted(stories[map_id].items())) + "}")
        out.append("  [%d] = { %s }," % (map_id, ", ".join(parts)))
    out.append("}")

    # Champions: where they live and what they drop
    references = loot.load_loot("reference_loot_template")
    creature_loot = loot.load_loot("creature_loot_template")
    item_info = {int(r[0]): (r[1], int(r[2]), int(r[3])) for r in sql("SELECT entry, name, Quality, ItemLevel FROM item_template")}
    champions, named = [], {}
    for entry, name, kind, level, map_id, x, y, lootid, bounty in sql(
            "SELECT w.entry, w.name, w.kind, w.level, c.map, c.position_x, c.position_y, t.lootid, "
            "(SELECT q.ID FROM quest_template q WHERE q.RequiredNpcOrGo1 = w.entry LIMIT 1) "
            "FROM wowplus_champions w JOIN creature c ON c.guid = w.guid JOIN creature_template t ON t.entry = w.entry"):
        spot = on_map(int(map_id), float(x), float(y))
        if not spot:
            continue
        chances = loot.flatten(creature_loot.get(int(lootid), []), references) if int(lootid) else {}
        drops = sorted(((i, c) for i, c in chances.items() if i in item_info and item_info[i][1] >= 2 and c >= loot.MIN_CHANCE_SHOWN),
                       key=lambda pair: (-item_info[pair[0]][1], -pair[1]))[:24]
        for i, _ in drops:
            named[i] = item_info[i]
        champions.append((int(level), name, kind, spot, int(entry), drops, 0 if bounty in ("", "NULL") else int(bounty)))
    out.append("-- Champions: {level, name, what it is one of, zone, x, y, creature, drops {item, chance}, bounty quest}")
    out.append("WowPlusGuideChampions = {")
    for level, name, kind, (zone, x, y), entry, drops, bounty in sorted(champions):
        out.append("  {%d,%s,%s,%s,%s,%s,%d,{%s},%d}," % (level, lua(name), lua(kind), lua(zone), x, y, entry,
                                                      ",".join("{%d,%s}" % (i, ("%.1f" % c).rstrip("0").rstrip(".")) for i, c in drops), bounty))
    out.append("}")
    out.append("-- Names, qualities and item levels of the champions' drops")
    out.append("WowPlusGuideMoreItems = {")
    for i, (name, quality, item_level) in sorted(named.items()):
        out.append("[%d]={%s,%d,%d}," % (i, lua(name), quality, item_level))
    out.append("}")

    unchanged = write(OUT, "\n".join(out) + "\n")
    lore = write_lore()
    told = sum(len(s) for s in stories.values())
    print(f"Guide: {len(maps)} places ({len(entrances)} with an entrance, {sum(len(q) for q in quests.values())} quests, {told} boss stories), "
          f"{len(champions)} champions, lore of {lore} places, in {time.time() - started:.1f} s." + ("" if unchanged else " Written."))


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as error:
        print(f"Guide not made: {error}")
        sys.exit(1)
