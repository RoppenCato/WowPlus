"""Health check for the bots. Reads the database, the logs and the bot module's source, and prints
pass or fail for each thing that has gone wrong before. Run it after a restart, a settings change
or an update of the bot module. It changes nothing.

    python Settings/Scripts/Test-Bots.py              the quick checks (a few seconds)
    python Settings/Scripts/Test-Bots.py --watch 10   also watch the bots for 10 minutes: who stands still, who stays dead
    python Settings/Scripts/Test-Bots.py --deaths 20  also sample every 2 minutes for 20 minutes: who keeps dying

What each check means, and what to do when it fails, is in Settings/Docs/Bots.md under "Health check".
"""
import argparse
import io
import os
import re
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MYSQL = os.path.join(ROOT, "Tools", "mysql", "bin", "mysql.exe")
LOGS = os.path.join(ROOT, "Build", "bin", "RelWithDebInfo", "logs")
BOT_CONF = os.path.join(ROOT, "Build", "bin", "RelWithDebInfo", "configs", "modules", "playerbots.conf")
BOT_SOURCE = os.path.join(ROOT, "Server", "modules", "mod-playerbots", "src")
ALLIANCE = (1, 3, 4, 7, 11)
BOTS = "FROM acore_characters.characters c JOIN acore_auth.account a ON a.id = c.account WHERE a.username LIKE 'RNDBOT%' AND c.online = 1"

# Our own changes inside the bot module. An update of the module can undo them without anyone noticing,
# so each is looked for by a line that only our change contains.
PATCHES = [
    ("Bot/Factory/PlayerbotFactory.cpp", "wowPlusLoweringChance", "bots dress differently (a wardrobe class per bot)"),
    ("Bot/Factory/PlayerbotFactory.h", "wowPlusLoweringChance", "bots dress differently (declaration)"),
    ("Bot/PlayerbotAI.cpp", "wowPlusLastAsked", "no whispers from a bot that was not spoken to"),
    ("Bot/PlayerbotAI.h", "wowPlusLastAsked", "no whispers from a bot that was not spoken to (declaration)"),
    ("Mgr/Security/PlayerbotSecurity.cpp", "WowPlus", "no canned refusal lines ('Invite me to your group first')"),
    ("Bot/RandomPlayerbotMgr.cpp", "WowPlusFollowFaction", "the bots follow the faction you play"),
    ("Bot/RandomPlayerbotMgr.h", "WowPlusFollowFaction", "the bots follow the faction you play (declaration)"),
]

# The bot module's own talent tabs that heal, per class: priest discipline and holy, paladin holy, shaman and druid restoration.
# playerbots_random_bots keeps each bot's spec as "specNo" (value = spec index + 1).
HEALING_SPECS = {5: (1, 2), 2: (1,), 7: (3,), 11: (3,)}

results = []


def sql(query):
    out = subprocess.run([MYSQL, "-uroot", "-P3307", "--protocol=TCP", "-hlocalhost", "-N", "-e", query],
                         capture_output=True, text=True, encoding="utf-8", errors="replace")
    if out.returncode != 0:
        raise RuntimeError(out.stderr.strip()[:200])
    return [line.split("\t") for line in out.stdout.splitlines()]


def report(ok, name, detail):
    """ok: True pass, False fail, None just information."""
    results.append(ok)
    print(f"  {'PASS' if ok else 'INFO' if ok is None else 'FAIL'}  {name:<34} {detail}")


def setting(name, default):
    try:
        with io.open(BOT_CONF, encoding="utf-8", errors="replace") as f:
            for line in f:
                found = re.match(r"\s*" + re.escape(name) + r"\s*=\s*(\S+)", line)
                if found:
                    return found.group(1)
    except OSError:
        pass
    return default


def tail(path, seconds):
    """The lines of a log written in the last so many seconds (logs start each line with a time)."""
    try:
        with io.open(path, encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
    except OSError:
        return []
    limit = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(time.time() - seconds))
    stamped = [line for line in lines if re.match(r"\d{4}-\d\d-\d\d \d\d:\d\d:\d\d", line)]
    return [line for line in stamped if line[:19] >= limit] if stamped else lines


def save_all():
    """Has the world server write every character to the database, so that positions and health are current."""
    subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(HERE, "Send-Server-Command.ps1"), "saveall"],
                   capture_output=True, creationflags=getattr(subprocess, "CREATE_NEW_CONSOLE", 0))
    time.sleep(8)


def snapshot():
    rows = sql(f"SELECT c.guid, c.name, c.level, c.map, c.zone, ROUND(c.position_x), ROUND(c.position_y), c.health, c.playerFlags, c.xp, c.money {BOTS}")
    return {int(r[0]): {"name": r[1], "level": int(r[2]), "map": int(r[3]), "zone": int(r[4]), "x": float(r[5]), "y": float(r[6]),
                        "dead": int(r[7]) == 0 or int(r[8]) & 0x10 != 0, "xp": int(r[9]), "money": int(r[10])} for r in rows}


def quick():
    # The servers
    def listening(port):
        with socket.socket() as s:
            s.settimeout(1)
            return s.connect_ex(("127.0.0.1", port)) == 0
    report(listening(8085), "World server running", "port 8085 answers" if listening(8085) else "nothing listens on port 8085: start it with Start-Server.bat")
    report(None, "Bot chat window", "running" if listening(8586) else "not running (Start-Bot-Chat.bat); bots will not talk")

    # How many
    wanted_low, wanted_high = int(setting("Playerbots.MinRandomBots", 500)), int(setting("Playerbots.MaxRandomBots", 500))
    online = int(sql(f"SELECT COUNT(*) {BOTS}")[0][0])
    report(wanted_low * 0.9 <= online <= wanted_high, "Bots in the world", f"{online} (settings ask for {wanted_low} to {wanted_high})")

    # Factions. The bots follow the side that is being played, or was played last (WowPlus.Bots.FollowFaction);
    # with that switched off, the two ratios in playerbots.conf decide.
    alliance = int(sql(f"SELECT COUNT(*) {BOTS} AND c.race IN ({','.join(map(str, ALLIANCE))})")[0][0])
    count = {"Alliance": alliance, "Horde": online - alliance}
    real = ("FROM acore_characters.characters c JOIN acore_auth.account a ON a.id = c.account "
            "WHERE a.username NOT LIKE 'RNDBOT%' AND a.username NOT LIKE 'AHBOT%'")
    playing = sql(f"SELECT c.name, c.race {real} AND c.online = 1 ORDER BY c.guid LIMIT 1")
    last = playing or sql(f"SELECT c.name, c.race {real} AND c.logout_time > 0 ORDER BY c.logout_time DESC LIMIT 1")
    if last:
        side = "Alliance" if int(last[0][1]) in ALLIANCE else "Horde"
        other = "Horde" if side == "Alliance" else "Alliance"
        why = f"{last[0][0]} {'is playing' if playing else 'played last'}"
        report(count[other] == 0, "Factions", f"{count['Alliance']} Alliance, {count['Horde']} Horde; {why}, so {side} is expected"
               + (". The switch takes a few minutes after logging in." if count[other] else ""))
    else:
        report(None, "Factions", f"{count['Alliance']} Alliance, {count['Horde']} Horde (nobody has played yet)")

    # Roles. The module's own log counts a healer only while it is healing in a group, so count by talent spec instead.
    specs = sql(f"SELECT c.class, r.value, COUNT(*) FROM acore_playerbots.playerbots_random_bots r JOIN acore_characters.characters c ON c.guid = r.bot "
                "JOIN acore_auth.account a ON a.id = c.account WHERE r.event = 'specNo' AND c.online = 1 AND a.username LIKE 'RNDBOT%' GROUP BY 1, 2")
    healers = sum(int(n) for cls, spec, n in specs if int(spec) in HEALING_SPECS.get(int(cls), ()))
    share = 100 * healers / max(1, online)
    report(10 <= share <= 30, "Healers (by talent spec)", f"{healers} of {online} ({share:.0f}%; 10 to 30% is healthy)")

    # Levels
    brackets = {int(b): int(n) for b, n in sql(f"SELECT FLOOR((c.level - 1) / 10), COUNT(*) {BOTS} GROUP BY 1")}
    spread = ", ".join(f"{b * 10 + 1}-{b * 10 + 10}: {n}" for b, n in sorted(brackets.items()))
    players = sql("SELECT c.name, c.level FROM acore_characters.characters c JOIN acore_auth.account a ON a.id = c.account "
                  "WHERE c.online = 1 AND a.username NOT LIKE 'RNDBOT%' AND a.username NOT LIKE 'AHBOT%'")
    players = [(name, int(level)) for name, level in players if not int(sql(f"SELECT COUNT(*) FROM acore_playerbots.playerbots_random_bots r JOIN acore_characters.characters c ON c.guid = r.bot WHERE c.name = '{name}'")[0][0])]
    if players:
        name, level = players[0]
        mine = brackets.get((level - 1) // 10, 0)
        report(mine >= 0.3 * online, "Bots around your level", f"{mine} of {online} in {name}'s ten levels ({spread}). Takes about 15 minutes after logging in.")
    else:
        report(None, "Levels", f"{spread} (nobody is playing, so they are spread evenly)")

    # The board brings invited bots over itself, so the bot module must not do it as well
    started = [line for line in tail(os.path.join(LOGS, "Server.log"), 7 * 86400) if "WowPlus board:" in line]
    report(bool(started) or None, "Group board", started[-1].split("WowPlus board:")[1].strip() if started else "no start line in Server.log yet")

    # The people you got to know keep their level: they are on a friends list or in your guild
    protected = int(sql("SELECT COUNT(DISTINCT s.friend) FROM acore_characters.character_social s JOIN acore_characters.characters c ON c.guid = s.friend "
                        "JOIN acore_auth.account a ON a.id = c.account WHERE s.flags & 1 AND a.username LIKE 'RNDBOT%'")[0][0])
    report(None, "Bots kept at their level", f"{protected} on a friends list" + ("" if setting("Playerbots.LevelBrackets.IgnoreFriendListed", "1") == "1"
                                                                                else " but Playerbots.LevelBrackets.IgnoreFriendListed is off"))

    # Logs
    hour = tail(os.path.join(LOGS, "Playerbots.log"), 3600)
    report(len(hour) < 3000, "Bot log size", f"{len(hour)} lines in the last hour (over 3000 means the debug level is on)")
    server = tail(os.path.join(LOGS, "Server.log"), 7 * 86400)
    missing = [line for line in server if "Missing property" in line]
    report(not missing, "Settings the server asked for", f"{len(missing)} 'Missing property' lines in Server.log" + (": " + missing[0].split("Missing property")[1].split(" in ")[0].strip() if missing else ""))
    errors = [line for line in tail(os.path.join(LOGS, "Errors.log"), 7 * 86400) if re.search(r"playerbot|\bbot\b", line, re.I)]
    report(not errors, "Errors about bots", f"{len(errors)} in Errors.log" + (": " + errors[-1].strip()[:90] if errors else ""))
    diffs = [int(m.group(1)) for line in server for m in [re.search(r"Update time diff: (\d+)ms", line)] if m]
    if diffs:
        recent = diffs[-6:]
        report(max(recent) < 100, "World update time", f"last {len(recent)} readings: {', '.join(map(str, recent))} ms (under 50 is smooth, over 100 is felt)")
    else:
        report(None, "World update time", "no reading yet (one is written every five minutes)")
    quests = [m.groups() for line in tail(os.path.join(LOGS, "Playerbots.log"), 3 * 3600) for m in [re.search(r"Accepted: (\d+), Rewarded: (\d+), Dropped: (\d+)", line)] if m]
    if quests:
        a, r, d = map(int, quests[-1])
        report(None, "Bot questing since the start", f"{a} accepted, {r} handed in, {d} dropped")

    # Our changes inside the bot module
    gone = []
    for path, marker, what in PATCHES:
        try:
            with io.open(os.path.join(BOT_SOURCE, path), encoding="utf-8", errors="replace") as f:
                if marker not in f.read():
                    gone.append(what)
        except OSError:
            gone.append(f"{what} (file {path} is gone)")
    report(not gone, "Our changes in the bot module", f"all {len(PATCHES)} in place" if not gone else "MISSING: " + "; ".join(gone))


def watch(minutes):
    print(f"\nWatching the bots for {minutes} minutes...")
    save_all()
    before = snapshot()
    time.sleep(minutes * 60)
    save_all()
    after = snapshot()
    both = [g for g in before if g in after]
    still, dead = [], []
    for g in both:
        a, b = before[g], after[g]
        moved = a["map"] != b["map"] or abs(a["x"] - b["x"]) + abs(a["y"] - b["y"]) > 10
        if a["dead"] and b["dead"]:
            dead.append(b)
        elif not moved and a["xp"] == b["xp"] and a["money"] == b["money"] and a["level"] == b["level"]:
            still.append(b)
    def names(bots):
        return ", ".join(f"{b['name']} ({b['level']}, zone {b['zone']})" for b in bots[:8]) + (" ..." if len(bots) > 8 else "")
    report(len(still) <= 0.25 * len(both), f"Standing still for {minutes} minutes", f"{len(still)} of {len(both)}" + (": " + names(still) if still else ""))
    report(len(dead) <= 0.05 * len(both), f"Dead at both ends", f"{len(dead)} of {len(both)}" + (": " + names(dead) if dead else ""))


def deaths(minutes):
    samples = max(2, minutes // 2 + 1)
    print(f"\nSampling who is dead every 2 minutes, {samples} times...")
    seen, names = {}, {}
    for i in range(samples):
        save_all()
        for g, bot in snapshot().items():
            names[g] = bot
            seen.setdefault(g, []).append(bot["dead"])
        if i < samples - 1:
            time.sleep(112)
    often = sorted(((sum(flags), g) for g, flags in seen.items() if sum(flags) > 3), reverse=True)
    worst = ", ".join(f"{names[g]['name']} ({names[g]['level']}, zone {names[g]['zone']}): dead in {n} of {len(seen[g])}" for n, g in often[:8])
    total = sum(sum(flags) for flags in seen.values())
    report(len(often) <= 2, "Bots that keep being dead", f"{len(often)} were dead in more than 3 of {samples} samples; on average {total / samples:.1f} bots dead at a time"
           + (". " + worst if worst else ""))


def main():
    p = argparse.ArgumentParser(description="Health check for the bots")
    p.add_argument("--watch", type=int, default=0, help="minutes to watch for bots that stand still or stay dead")
    p.add_argument("--deaths", type=int, default=0, help="minutes to sample for bots that keep dying")
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    print("Bot health check, " + time.strftime("%Y-%m-%d %H:%M"))
    try:
        quick()
        if a.watch:
            watch(a.watch)
        if a.deaths:
            deaths(a.deaths)
    except RuntimeError as error:
        print(f"  The database did not answer: {error}")
        sys.exit(2)
    failed = sum(r is False for r in results)
    print(f"\n{'All checks passed.' if not failed else str(failed) + ' check' + ('s' if failed > 1 else '') + ' failed.'}")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
