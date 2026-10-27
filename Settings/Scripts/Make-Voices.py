"""Voices our own quests (Settings/Quests), as a voice pack for Spoken Quests, the addon that reads the game's
own quests aloud. Spoken's player plays our pack exactly like its own: when a quest of ours is offered, asked
about or handed in, the person who gives it says it.

    python Settings/Scripts/Make-Voices.py              voice what is new or changed (the rest is kept)
    python Settings/Scripts/Make-Voices.py --quest 9200237   just that quest, again
    python Settings/Scripts/Make-Voices.py --list       who gets which voice, nothing made

Writes Client/AddOns/SpokenQuestsAudioWowPlus: the pack's lookups (committed) and its sound files (made on
this PC, not committed; Settings/Docs/Voice.md). Deploy-Settings.ps1 copies it into the game like our other
addons.

The voices are made on this PC with Kokoro, a free voice model (kokoro-onnx; the model files are in
C:\\dev\\addon-refs\\tts, see Voice.md). Each person keeps one voice, picked by their race and sex from a
few that suit, so two dwarves need not sound alike. About eight seconds of speech per second of work.
"""
import argparse
import glob
import hashlib
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
DBC = os.path.join(ROOT, "Data", "dbc")
QUESTS = os.path.join(ROOT, "Settings", "Quests")
PACK = os.path.join(ROOT, "Client", "AddOns", "SpokenQuestsAudioWowPlus")
SOUNDS = os.path.join(PACK, "generated", "sounds", "quests")
MADE = os.path.join(PACK, "generated", "made.json")      # what each sound file was made from, so only changes are made again
MODEL = os.environ.get("WOWPLUS_TTS", r"C:\dev\addon-refs\tts")

# Voices that suit each people (Kokoro's names: a = American, b = British, f = female, m = male)
MALE = {"human": ["bm_george", "am_michael", "bm_lewis", "am_adam"], "orc": ["am_onyx", "am_fenrir"], "dwarf": ["bm_george", "bm_daniel"],
        "nightelf": ["bm_fable", "am_liam"], "undead": ["bm_lewis", "am_echo"], "tauren": ["am_onyx", "am_fenrir"], "gnome": ["am_puck", "am_eric"],
        "troll": ["am_eric", "am_fenrir"], "bloodelf": ["bm_daniel", "bm_fable"], "draenei": ["bm_fable", "bm_george"], "goblin": ["am_puck", "am_eric"],
        "": ["am_michael", "bm_george", "am_adam"]}
FEMALE = {"human": ["bf_emma", "af_heart", "af_sarah"], "orc": ["af_nicole", "af_kore"], "dwarf": ["bf_alice", "bf_emma"],
          "nightelf": ["bf_isabella", "af_sky"], "undead": ["bf_lily", "af_river"], "tauren": ["af_kore", "af_aoede"], "gnome": ["af_bella", "af_jessica"],
          "troll": ["af_nova", "af_kore"], "bloodelf": ["bf_lily", "bf_isabella"], "draenei": ["bf_alice", "af_aoede"], "goblin": ["af_bella", "af_jessica"],
          "": ["bf_emma", "af_heart"]}
SPEED = {"tauren": 0.85, "orc": 0.9, "dwarf": 0.95, "gnome": 1.05, "goblin": 1.05, "undead": 0.92}
RACES = {1: "human", 2: "orc", 3: "dwarf", 4: "nightelf", 5: "undead", 6: "tauren", 7: "gnome", 8: "troll", 9: "goblin", 10: "bloodelf", 11: "draenei"}
PAUSE = 0.45                                                # seconds between paragraphs


def sql(query):
    out = subprocess.run([MYSQL, "-uroot", "-P3307", "--protocol=TCP", "-hlocalhost", "-N", "-B", "--default-character-set=utf8mb4", "acore_world", "-e", query],
                         capture_output=True)
    if out.returncode != 0:
        raise RuntimeError(out.stderr.decode("utf-8", "replace").strip()[:300])
    return [line.split("\t") for line in out.stdout.decode("utf-8", "replace").replace("\r", "").split("\n") if line]


def dbc(name):
    with open(os.path.join(DBC, name), "rb") as f:
        data = f.read()
    _, records, fields, size, _ = struct.unpack_from("<4sIIII", data)
    block = data[20 + records * size:]
    rows = [struct.unpack_from("<%dI" % fields, data, 20 + i * size) for i in range(records)]
    return rows, lambda offset: block[offset:block.find(b"\0", offset)].decode("utf-8", "replace")


def people(ids):
    """creature -> (name, people, female?), from its first model."""
    displays, _ = dbc("CreatureDisplayInfo.dbc")
    display = {r[0]: (r[1], r[3]) for r in displays}
    extras, _ = dbc("CreatureDisplayInfoExtra.dbc")
    extra = {r[0]: (r[1], r[2]) for r in extras}
    models, model_name = dbc("CreatureModelData.dbc")
    model = {r[0]: model_name(r[2]).lower() for r in models}
    found = {}
    for entry, name, shown in sql(f"SELECT t.entry, t.name, (SELECT m.CreatureDisplayID FROM creature_template_model m WHERE m.CreatureID = t.entry ORDER BY m.Idx LIMIT 1) "
                                  f"FROM creature_template t WHERE t.entry IN ({','.join(str(i) for i in ids)})"):
        race, female = "", False
        model_id, extra_id = display.get(int(shown) if shown.isdigit() else 0, (0, 0))
        if extra_id in extra:
            race, female = RACES.get(extra[extra_id][0], ""), extra[extra_id][1] == 1
        else:
            path = model.get(model_id, "")
            found_race = re.search(r"character\\(\w+)\\(male|female)", path)
            if found_race:
                race, female = found_race.group(1).replace("scourge", "undead"), found_race.group(2) == "female"
            elif "banshee" in path:
                race, female = "undead", True
        found[int(entry)] = (name, race if race in MALE else "", female)
    return found


def voice_of(entry, race, female):
    pool = (FEMALE if female else MALE)[race]
    pick = pool[int(hashlib.md5(str(entry).encode()).hexdigest(), 16) % len(pool)]
    return pick, SPEED.get(race, 0.95), "en-gb" if pick.startswith("b") else "en-us"


def spoken(text):
    """Quest text as it can be said by a recording, which cannot know who listens: the name goes."""
    text = re.sub(r",\s*\$[NnCcRr]\b", "", text)
    text = re.sub(r"\$[NnCcRr]\s*,\s*", "", text)
    text = re.sub(r"\$[NnCcRr]\b", "friend", text)
    text = re.sub(r"\$[Gg]\s*([^:;]*):([^;]*);", r"\1", text)
    return text.replace("$B", "\n").replace("$b", "\n").strip()


def lua(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def main():
    p = argparse.ArgumentParser(description="Voice our quests")
    p.add_argument("--quest", type=int, help="make this quest's voices again")
    p.add_argument("--list", action="store_true", help="only show who gets which voice")
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    quests = []
    for path in sorted(glob.glob(os.path.join(QUESTS, "92*.json"))):
        with io.open(path, encoding="utf-8") as f:
            quests.append(json.load(f))
    new_people = {c["id"]: c for q in quests for c in q.get("creatures") or []}
    who = people({q["giver"] for q in quests} | {q.get("turn_in", q["giver"]) for q in quests})
    for entry, creature in new_people.items():               # our own people are not in the game's tables by that number until applied
        if entry not in who and creature.get("like") in who:
            _, race, female = who[creature["like"]]
            who[entry] = (creature["name"], race, female)

    lines = []                                              # (file name, quest, event, speaker, text)
    for q in quests:
        texts = q.get("texts") or {}
        for event, key, speaker in (("accept", "details", q["giver"]), ("progress", "progress", q.get("turn_in", q["giver"])),
                                    ("complete", "completion", q.get("turn_in", q["giver"]))):
            if texts.get(key):
                lines.append((f"{q['id']}-{event}", q, event, speaker, spoken(texts[key])))

    if a.list:
        for speaker in sorted({line[3] for line in lines}):
            name, race, female = who.get(speaker, ("?", "", False))
            print(f"{speaker:>8}  {name:<34} {race or '?':<9} {'female' if female else 'male':<7} {voice_of(speaker, race, female)[0]}")
        return 0

    os.makedirs(SOUNDS, exist_ok=True)
    try:
        with io.open(MADE, encoding="utf-8") as f:
            made = json.load(f)
    except (OSError, ValueError):
        made = {}

    todo = []
    for name, q, event, speaker, text in lines:
        _, race, female = who.get(speaker, ("", "", False))
        voice, speed, lang = voice_of(speaker, race, female)
        stamp = hashlib.md5(f"{voice}|{speed}|{text}".encode()).hexdigest()
        if a.quest and q["id"] != a.quest:
            continue
        if a.quest or made.get(name, {}).get("stamp") != stamp or not os.path.exists(os.path.join(SOUNDS, name + ".ogg")):
            todo.append((name, text, voice, speed, lang, stamp))

    if todo:
        import numpy as np
        import soundfile as sf
        from kokoro_onnx import Kokoro
        kokoro = Kokoro(os.path.join(MODEL, "kokoro-v1.0.onnx"), os.path.join(MODEL, "voices-v1.0.bin"))
        started, spoken_seconds = time.time(), 0.0
        for number, (name, text, voice, speed, lang, stamp) in enumerate(todo, 1):
            parts, rate = [], 24000
            for paragraph in [part.strip() for part in re.split(r"\n\s*\n", text) if part.strip()]:
                # A long paragraph in one go can be too much for the model: by sentences, a few at a time
                sentences = re.split(r"(?<=[.!?])\s+", paragraph.replace("\n", " "))
                chunk = ""
                for sentence in sentences + [None]:
                    if sentence is not None and len(chunk) + len(sentence) < 400:
                        chunk = (chunk + " " + sentence).strip()
                        continue
                    if chunk:
                        samples, rate = kokoro.create(chunk, voice=voice, speed=speed, lang=lang)
                        parts.append(samples)
                    chunk = sentence or ""
                parts.append(np.zeros(int(rate * PAUSE), dtype=np.float32))
            audio = np.concatenate(parts[:-1]) if len(parts) > 1 else parts[0]
            with sf.SoundFile(os.path.join(SOUNDS, name + ".ogg"), "w", rate, 1, format="OGG", subtype="VORBIS") as f:
                for at in range(0, len(audio), rate):       # in pieces: one long write can bring the library down
                    f.write(audio[at:at + rate])
            seconds = round(len(audio) / rate, 2)
            spoken_seconds += seconds
            made[name] = {"stamp": stamp, "seconds": seconds, "voice": voice}
            if number % 10 == 0 or number == len(todo):
                with io.open(MADE, "w", encoding="utf-8") as f:
                    json.dump(made, f, indent=1, sort_keys=True)
                print(f"  {number} of {len(todo)} made ({spoken_seconds / 60:.0f} minutes of speech in {(time.time() - started) / 60:.0f} minutes)", flush=True)

    # The lookups Spoken Quests reads: the quest's number from its title (and from the person, where two
    # quests share a title), and each sound's length
    lookup = {"accept": {}, "progress": {}, "complete": {}}
    for name, q, event, speaker, text in lines:
        if name not in made:
            continue
        title, person = q["title"], who.get(speaker, ("",))[0]
        have = lookup[event].get(title)
        if have is None:
            lookup[event][title] = {person: q["id"]}
        else:
            have[person] = q["id"]
    out = ["if not VoiceOver or not VoiceOver.DataModules then return end",
           "-- Made by Settings/Scripts/Make-Voices.py from Settings/Quests. Do not edit: it is written over.",
           "SpokenQuestsAudioWowPlus.QuestIDLookup = {"]
    for event in ("accept", "progress", "complete"):
        out.append(f"\t[{lua(event)}] = {{")
        for title, by_person in sorted(lookup[event].items()):
            if len(set(by_person.values())) == 1:
                out.append(f"\t\t[{lua(title)}] = {next(iter(by_person.values()))},")
            else:
                out.append(f"\t\t[{lua(title)}] = {{ " + " ".join(f"[{lua(person)}] = {number}," for person, number in sorted(by_person.items())) + " },")
        out.append("\t},")
    out.append("}")
    with io.open(os.path.join(PACK, "generated", "quest_id_lookups.lua"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    out = ["if not VoiceOver or not VoiceOver.DataModules then return end",
           "SpokenQuestsAudioWowPlus.SoundLengthLookupByFileName = {"]
    for name in sorted(made):
        out.append(f"\t[{lua(name)}] = {made[name]['seconds']},")
    out.append("}")
    with io.open(os.path.join(PACK, "generated", "sound_length_table.lua"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    with io.open(MADE, "w", encoding="utf-8") as f:
        json.dump(made, f, indent=1, sort_keys=True)
    print(f"Voices: {len(made)} lines of our quests in the pack, {len(todo)} made this time.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
