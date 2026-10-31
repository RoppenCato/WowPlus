"""Draws a map of every dungeon and raid for the Guide, from the ground the server knows can be walked on
(Data/mmaps, the same files the bots find their way with), with the bosses and doors where they really are.
Most old dungeons have no map at all in the 3.3.5 game. See Settings/Docs/Guide.md.

    python Settings/Scripts/Make-Dungeon-Maps.py            make the pictures that are missing
    python Settings/Scripts/Make-Dungeon-Maps.py --map 36   this dungeon again, with a preview in Build/dungeon-map-previews
    python Settings/Scripts/Make-Dungeon-Maps.py --all      all of them again

Writes Client/AddOns/WowPlusGuide/Maps/<map>.tga (made on this PC, not committed: drawn from the extracted map
data) and Client/AddOns/WowPlusGuide/Maps.lua (where the bosses and doors are on each picture, in percent).
Deploy-Settings.ps1 runs it at every server start; with nothing missing it takes no time. Needs numpy and
Pillow; without them the pictures from last time stay.

A picture is seen from above: lower ground is darker, a drop is drawn as an ink line, and a floor above
another hides it. Only ground that can be reached counts: what is joined to a door or to something living
there. The navigation mesh also covers roofs and the outside of the walls, which nobody can get to.
"""
import argparse
import glob
import io
import json
import os
import re
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MYSQL = os.path.join(ROOT, "Tools", "mysql", "bin", "mysql.exe")
MESHES = os.path.join(ROOT, "Data", "mmaps")
GUIDE = os.path.join(ROOT, "Client", "AddOns", "WowPlusGuide")
PICTURES = os.path.join(GUIDE, "Maps")
INDEX = os.path.join(ROOT, "Build", "dungeon-maps.json")      # what is on each picture, so that only missing ones are drawn
PREVIEWS = os.path.join(ROOT, "Build", "dungeon-map-previews")

SIZE = 512                                                     # pixels; the game wants a power of two
MARGIN = 26
SUPER = 2                                                      # drawn this much larger, then shrunk, for smooth edges
MESH_HEADER = 100
POLY_SIZE = 32
EXTERNAL = 0x8000
# Places built on open ground: their mesh holds the whole landscape, so the picture keeps only this many yards
# around the bosses and doors (Shadowfang Keep, Zul'Farrak, the Black Morass, Durnholde, Mount Hyjal)
OPEN_GROUND = {33: 45, 209: 70, 269: 90, 560: 90, 534: 90}
SHADOWFANG_KEEP = 33
MIN_HEIGHT_RANGE = 15.0                                        # yards: a nearly flat place is not shaded as if it were hilly
MIN_ISLAND = 400                                               # square yards: smaller ground with only a stray creature on it is left out
LAVA, WATER = 0x02 | 0x04, 0x08                                # what the ground's flags mean: magma or slime, water
TRIGGER = 128                                                  # creature_template.flags_extra: an invisible helper, which may float anywhere


def sql(query):
    out = subprocess.run([MYSQL, "-uroot", "-P3307", "--protocol=TCP", "-hlocalhost", "-N", "-B", "--default-character-set=utf8mb4", "acore_world", "-e", query],
                         capture_output=True)
    if out.returncode != 0:
        raise RuntimeError(out.stderr.decode("utf-8", "replace").strip()[:300])
    return [line.split("\t") for line in out.stdout.decode("utf-8", "replace").replace("\r", "").split("\n") if line]


def load_mesh(map_id):
    """Every walkable polygon of a map: lists of world (x, y, z) corners, and which polygons touch."""
    polys, edges, borders = [], [], {}
    for path in sorted(glob.glob(os.path.join(MESHES, "%03d[0-9][0-9][0-9][0-9].mmtile" % map_id))):
        with open(path, "rb") as f:
            data = f.read()
        at = len(data) - struct.unpack_from("<I", data, 12)[0]   # the header before the mesh has grown over versions: it says the mesh's size
        if at < 0 or len(data) < at + MESH_HEADER:
            continue
        poly_count, vert_count = struct.unpack_from("<2i", data, at + 24)
        verts_at = at + MESH_HEADER
        raw = struct.unpack_from("<%df" % (vert_count * 3), data, verts_at)
        # The mesh keeps a point as (world y, world z, world x)
        verts = [(raw[i * 3 + 2], raw[i * 3], raw[i * 3 + 1]) for i in range(vert_count)]
        polys_at = verts_at + vert_count * 12
        first = len(polys)
        for index in range(poly_count):
            where = polys_at + index * POLY_SIZE
            corners = struct.unpack_from("<6H", data, where + 4)
            neighbours = struct.unpack_from("<6H", data, where + 16)
            count, area_type = struct.unpack_from("<BB", data, where + 30)
            if area_type >> 6 == 1 or count < 3:                # a connection between points, not ground
                polys.append(None)
                continue
            points = [verts[corners[i]] for i in range(count)]
            polys.append({"points": points, "area": area_type & 0x3f, "height": sum(p[2] for p in points) / count})
            for side in range(count):
                neighbour = neighbours[side]
                if neighbour & EXTERNAL:
                    # An edge along the tile's border: joined below to whichever edge lies against it
                    a, b = points[side], points[(side + 1) % count]
                    along_x = abs(a[0] - b[0]) > abs(a[1] - b[1])
                    line = (along_x, round(a[1] if along_x else a[0], 1))
                    lo, hi = sorted((a[0], b[0]) if along_x else (a[1], b[1]))
                    borders.setdefault(line, []).append((first + index, lo, hi, (a[2] + b[2]) / 2))
                elif neighbour:
                    edges.append((first + index, first + neighbour - 1))
    for rows in borders.values():
        for i, (poly, lo, hi, height) in enumerate(rows):
            for other, other_lo, other_hi, other_height in rows[i + 1:]:
                if lo < other_hi and other_lo < hi and abs(height - other_height) < 3:
                    edges.append((poly, other))
    return polys, edges


def area_of(points):
    total = 0.0
    for i, (x, y, _) in enumerate(points):
        nx, ny, _ = points[(i + 1) % len(points)]
        total += x * ny - nx * y
    return abs(total) / 2


class Finder:
    """Which polygon a point stands on."""
    CELL = 16.0

    def __init__(self, polys):
        self.polys, self.cells = polys, {}
        for index, poly in enumerate(polys):
            if not poly:
                continue
            xs, ys = [p[0] for p in poly["points"]], [p[1] for p in poly["points"]]
            for cx in range(int(min(xs) // self.CELL), int(max(xs) // self.CELL) + 1):
                for cy in range(int(min(ys) // self.CELL), int(max(ys) // self.CELL) + 1):
                    self.cells.setdefault((cx, cy), []).append(index)

    def find(self, x, y, z):
        best, best_gap = None, 12.0
        for reach in (0, 1):
            for dx in range(-reach, reach + 1):
                for dy in range(-reach, reach + 1):
                    for index in self.cells.get((int(x // self.CELL) + dx, int(y // self.CELL) + dy), ()):
                        poly = self.polys[index]
                        points = poly["points"]
                        px = sum(p[0] for p in points) / len(points)
                        py = sum(p[1] for p in points) / len(points)
                        inside = self.contains(points, x, y)
                        gap = abs(poly["height"] - z) + (0 if inside else 4 + ((px - x) ** 2 + (py - y) ** 2) ** 0.5)
                        if gap < best_gap:
                            best, best_gap = index, gap
            if best is not None:
                break
        return best

    @staticmethod
    def contains(points, x, y):
        inside = False
        for i, (ax, ay, _) in enumerate(points):
            bx, by, _ = points[(i + 1) % len(points)]
            if (ay > y) != (by > y) and x < (bx - ax) * (y - ay) / (by - ay) + ax:
                inside = not inside
        return inside


def reachable(polys, edges, seeds):
    """The polygons joined to a door or a boss, or to something living there on ground of some size."""
    parent = list(range(len(polys)))

    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for a, b in edges:
        if polys[a] and polys[b]:
            parent[root(a)] = root(b)
    areas = {}
    for i, poly in enumerate(polys):
        if poly:
            areas[root(i)] = areas.get(root(i), 0.0) + area_of(poly["points"])
    keep = set()
    for index, must in seeds:
        group = root(index)
        if must or areas.get(group, 0) >= MIN_ISLAND:
            keep.add(group)
    musts = {root(index) for index, must in seeds if must}
    total = sum(areas[group] for group in keep) or 1.0
    groups = {}
    for i, poly in enumerate(polys):
        if poly and root(i) in keep:
            groups.setdefault(root(i), []).append(i)
    # A scrap of ground with nothing important on it counts only if it lies among the rest
    big = {g: v for g, v in groups.items() if g in musts or areas[g] >= total * 0.05}
    return list(big.values()) or list(groups.values()), [v for g, v in groups.items() if g not in big]


def bounds(polys, indices):
    points = [p for i in indices for p in polys[i]["points"]]
    return min(p[0] for p in points), max(p[0] for p in points), min(p[1] for p in points), max(p[1] for p in points)


def layout(polys, groups, scraps):
    """How the world is laid on the picture: normally one view with north up. Parts that are not joined and lie
    far apart (the Scarlet Monastery's four wings) are moved together into a grid of their own cells, at one scale."""
    box = bounds(polys, [i for g in groups for i in g])
    whole = max(box[1] - box[0], 1.0) * max(box[3] - box[2], 1.0)
    boxes = [bounds(polys, g) for g in groups]
    filled = sum(max(b[1] - b[0], 1.0) * max(b[3] - b[2], 1.0) for b in boxes)
    if len(groups) < 2 or filled / whole > 0.3:
        # One view; a scrap counts if it is within it
        kept = [i for g in groups for i in g]
        for scrap in scraps:
            b = bounds(polys, scrap)
            if b[0] >= box[0] and b[1] <= box[1] and b[2] >= box[2] and b[3] <= box[3]:
                kept += scrap
        span = max(box[1] - box[0], box[3] - box[2], 1.0)
        scale = (SIZE - 2 * MARGIN) / span
        centre_x, centre_y = (box[0] + box[1]) / 2, (box[2] + box[3]) / 2

        def place(x, y):
            # North is up: world x runs north, world y runs west
            return SIZE / 2 + (centre_y - y) * scale, SIZE / 2 + (centre_x - x) * scale
        return kept, place

    # Northern parts first, in rows, west to east within a row
    order = sorted(range(len(groups)), key=lambda n: -(boxes[n][0] + boxes[n][1]))
    columns = int(len(groups) ** 0.5 + 0.999)
    rows = (len(groups) + columns - 1) // columns
    order = [n for start in range(0, len(order), columns)
             for n in sorted(order[start:start + columns], key=lambda n: -(boxes[n][2] + boxes[n][3]))]
    cell_w, cell_h = SIZE / columns, SIZE / rows
    gap = MARGIN * 0.7
    scale = min(min((cell_w - 2 * gap) / max(b[3] - b[2], 1.0), (cell_h - 2 * gap) / max(b[1] - b[0], 1.0)) for b in boxes)
    views = []
    for position, n in enumerate(order):
        b = boxes[n]
        views.append((b, cell_w * (position % columns + 0.5), cell_h * (position // columns + 0.5)))

    def place(x, y):
        def away(view):
            b = view[0]
            return max(b[0] - x, x - b[1], 0) + max(b[2] - y, y - b[3], 0)
        b, at_x, at_y = min(views, key=away)
        return at_x + ((b[2] + b[3]) / 2 - y) * scale, at_y + ((b[0] + b[1]) / 2 - x) * scale
    return [i for g in groups for i in g], place


def draw(polys, kept, place, open_ground=False):
    """The picture of these polygons. On open ground the shading is smoothed and only cliffs are inked: its
    polygons follow the slopes of the land, and each would otherwise look like a step."""
    import numpy as np
    from PIL import Image, ImageDraw, ImageFilter

    heights = sorted(polys[i]["height"] for i in kept)
    low, high = heights[len(heights) // 100], heights[-1 - len(heights) // 100]     # a stray ledge does not set the range
    if high - low < MIN_HEIGHT_RANGE:                               # nearly flat: a calm middle tone
        low = (low + high) / 2 - MIN_HEIGHT_RANGE * 0.65
    span_z = max(high - low, MIN_HEIGHT_RANGE)
    big = SIZE * SUPER
    shade = Image.new("L", (big, big), 0)
    mask = Image.new("L", (big, big), 0)
    water = Image.new("L", (big, big), 0)
    lava = Image.new("L", (big, big), 0)
    pen, pen_mask, pen_water, pen_lava = ImageDraw.Draw(shade), ImageDraw.Draw(mask), ImageDraw.Draw(water), ImageDraw.Draw(lava)
    for index in sorted(kept, key=lambda i: polys[i]["height"]):       # lower first, so that upper floors lie on top
        poly = polys[index]
        outline = [tuple(v * SUPER for v in place(p[0], p[1])) for p in poly["points"]]
        level = int(40 + 215 * min(1.0, max(0.0, (poly["height"] - low) / span_z)))
        pen.polygon(outline, fill=level)
        pen_mask.polygon(outline, fill=255)
        pen_water.polygon(outline, fill=255 if poly["area"] & WATER else 0)
        pen_lava.polygon(outline, fill=255 if poly["area"] & LAVA else 0)

    rng = np.random.default_rng(7)
    # Parchment: warm, blotched, darker toward the edges
    blotches = Image.fromarray((rng.random((SIZE // 8, SIZE // 8)) * 255).astype("uint8")).resize((big, big), Image.BICUBIC).filter(ImageFilter.GaussianBlur(big / 48))
    grain = rng.random((big, big)) * 0.05
    yy, xx = np.mgrid[0:big, 0:big] / big - 0.5
    vignette = np.clip(1.08 - (xx ** 2 + yy ** 2) * 1.15, 0.55, 1.0)
    paper = ((np.asarray(blotches, dtype="f") / 255 - 0.5) * 0.28 + 0.84 + grain - 0.03) * vignette
    picture = np.stack([paper * 0.86, paper * 0.72, paper * 0.50], axis=2)

    inside = np.asarray(mask, dtype="f") / 255
    # A soft shadow under the ground, as if it were cut out and laid on the page
    shadow = np.asarray(mask.filter(ImageFilter.GaussianBlur(big / 90)).transform((big, big), Image.AFFINE, (1, 0, -big / 300, 0, 1, -big / 300)), dtype="f") / 255
    picture *= (1 - shadow * 0.45)[..., None]

    # The ground: deep sepia low down, pale cream high up
    if open_ground:
        shade = shade.filter(ImageFilter.GaussianBlur(big / 110))
    level = np.asarray(shade, dtype="f") / 255
    low_colour, high_colour = np.array([0.40, 0.27, 0.16]), np.array([0.97, 0.91, 0.74])
    ground = low_colour + (high_colour - low_colour) * (level[..., None] ** 0.85)
    wet = np.asarray(water.filter(ImageFilter.GaussianBlur(2)), dtype="f")[..., None] / 255
    ground = ground * (1 - wet * 0.65) + np.array([0.25, 0.45, 0.62]) * wet * 0.65
    hot = np.asarray(lava.filter(ImageFilter.GaussianBlur(2)), dtype="f")[..., None] / 255
    ground = ground * (1 - hot * 0.75) + np.array([0.78, 0.26, 0.08]) * hot * 0.75
    picture = picture * (1 - inside[..., None]) + ground * inside[..., None]

    # Ink: around the ground, and wherever one floor drops away from another
    thick = np.asarray(mask.filter(ImageFilter.MaxFilter(5)), dtype="f") / 255
    rim = np.clip(thick - inside, 0, 1)
    jump = 3.0 / span_z if not open_ground else 2.0                    # a drop of three yards; on open ground nothing is inked but the rim
    dx = np.abs(np.diff(level, axis=1, prepend=level[:, :1]))
    dy = np.abs(np.diff(level, axis=0, prepend=level[:1, :]))
    edge = ((dx > jump * 0.84) | (dy > jump * 0.84)) * inside
    edge = np.asarray(Image.fromarray((edge * 255).astype("uint8")).filter(ImageFilter.MaxFilter(3)), dtype="f") / 255
    ink = np.clip(rim + edge * 0.75, 0, 1)[..., None]
    picture = picture * (1 - ink * 0.88) + np.array([0.13, 0.08, 0.04]) * ink * 0.88

    result = Image.fromarray((np.clip(picture, 0, 1) * 255).astype("uint8"), "RGB").resize((SIZE, SIZE), Image.LANCZOS)
    return result


def main():
    p = argparse.ArgumentParser(description="Draw the dungeon maps for the Guide")
    p.add_argument("--map", type=int, help="draw this map again, with a preview")
    p.add_argument("--all", action="store_true", help="draw all of them again")
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    started = time.time()

    with io.open(os.path.join(GUIDE, "Places.lua"), encoding="utf-8") as f:
        names = {int(m): n for m, n in re.findall(r'^  \[(\d+)\] = \{ name = "([^"]*)"', f.read(), re.M)}
    bosses = {}
    with io.open(os.path.join(GUIDE, "Loot.lua"), encoding="utf-8") as f:
        current = None
        for line in f:
            found = re.match(r"\s*\{ map = (\d+),", line)
            if found:
                current = int(found.group(1))
            found = re.match(r"\s*\{ name = .*, entry = (\d+), modes", line)
            if found:
                bosses.setdefault(current, []).append(int(found.group(1)))

    try:
        with io.open(INDEX, encoding="utf-8") as f:
            index = {int(k): v for k, v in json.load(f).items()}
    except (OSError, ValueError):
        index = {}
    os.makedirs(PICTURES, exist_ok=True)
    wanted = [a.map] if a.map else [m for m in sorted(names) if a.all or m not in index or not os.path.exists(os.path.join(PICTURES, "%d.tga" % m))]

    drawn = 0
    if wanted:
        try:
            import numpy, PIL                                   # noqa: E401
        except ImportError:
            print("Dungeon maps not made: they need numpy and Pillow (pip install numpy pillow).")
            wanted = []
    doors = {}
    if wanted:
        for m, x, y, z in sql("SELECT target_map, target_position_x, target_position_y, target_position_z FROM areatrigger_teleport"):
            doors.setdefault(int(m), []).append((float(x), float(y), float(z)))
    for map_id in wanted:
        polys, edges = load_mesh(map_id)
        if not any(polys):
            index.pop(map_id, None)
            continue
        finder = Finder(polys)
        seeds = []
        for x, y, z in doors.get(map_id, []):
            found = finder.find(x, y, z)
            if found is not None:
                seeds.append((found, True))
        spawned = {}
        for entry, x, y, z, flags in sql(f"SELECT c.id, c.position_x, c.position_y, c.position_z, t.flags_extra FROM creature c JOIN creature_template t ON t.entry = c.id WHERE c.map = {map_id}"):
            entry, x, y, z = int(entry), float(x), float(y), float(z)
            spawned.setdefault(entry, (x, y, z))
            if int(flags) & TRIGGER:
                continue
            found = finder.find(x, y, z)
            if found is not None:
                seeds.append((found, entry in bosses.get(map_id, [])))
        groups, scraps = reachable(polys, edges, seeds)
        if not groups:
            print(f"  {names.get(map_id, map_id)}: nothing reachable found, no map.")
            index.pop(map_id, None)
            continue
        if map_id in OPEN_GROUND:
            marks = [spawned[e] for e in bosses.get(map_id, []) if e in spawned] + doors.get(map_id, [])
            margin = OPEN_GROUND[map_id]
            low_x, high_x = min(m[0] for m in marks) - margin, max(m[0] for m in marks) + margin
            low_y, high_y = min(m[1] for m in marks) - margin, max(m[1] for m in marks) + margin

            def near(i):
                return all(low_x <= q[0] <= high_x and low_y <= q[1] <= high_y for q in polys[i]["points"])
            groups = [kept_part for kept_part in ([i for i in g if near(i)] for g in groups) if kept_part]
            scraps = []
        kept, place = layout(polys, groups, scraps)
        picture = draw(polys, kept, place, map_id in OPEN_GROUND and map_id != SHADOWFANG_KEEP)     # the keep is a building: its floors are wanted
        picture.save(os.path.join(PICTURES, "%d.tga" % map_id), rle=False)
        if a.map:
            os.makedirs(PREVIEWS, exist_ok=True)
            picture.save(os.path.join(PREVIEWS, "%d.png" % map_id))

        def percent(x, y):
            px, py = place(x, y)
            return round(px / SIZE * 100, 1), round(py / SIZE * 100, 1)

        index[map_id] = {"bosses": {str(e): percent(*spawned[e][:2]) for e in bosses.get(map_id, []) if e in spawned},
                         "doors": [percent(x, y) for x, y, _ in doors.get(map_id, [])]}
        drawn += 1
        print(f"  {names.get(map_id, map_id)}: {len(kept)} of {sum(1 for x in polys if x)} polygons, {len(index[map_id]['bosses'])} bosses placed "
              f"({time.time() - started:.0f} s)", flush=True)

    if drawn or a.map:
        os.makedirs(os.path.dirname(INDEX), exist_ok=True)
        with io.open(INDEX, "w", encoding="utf-8") as f:
            json.dump(index, f)
    out = ["-- Made by Settings/Scripts/Make-Dungeon-Maps.py. Do not edit: it is written over.",
           "-- Per dungeon and raid (by map): where each boss (by creature) and door is on Maps/<map>.tga, in percent.",
           "WowPlusGuideMaps = {"]
    for map_id in sorted(index):
        bits = ",".join("[%s]={%s,%s}" % (e, x, y) for e, (x, y) in sorted(index[map_id]["bosses"].items(), key=lambda kv: int(kv[0])))
        door_bits = ",".join("{%s,%s}" % tuple(d) for d in index[map_id]["doors"])
        out.append("  [%d] = { bosses = {%s}, doors = {%s} }," % (map_id, bits, door_bits))
    out.append("}")
    with io.open(os.path.join(GUIDE, "Maps.lua"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    print(f"Dungeon maps: {len(index)} in the Guide, {drawn} drawn now, in {time.time() - started:.0f} s.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as error:
        print(f"Dungeon maps not made: {error}")
        sys.exit(1)
