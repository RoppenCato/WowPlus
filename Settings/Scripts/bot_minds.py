"""Bot minds: who the people behind the bots are, how each of them writes, and how to ask Claude to
speak as one of them. Nothing in this file is about one game, so any game's chat bridge can use it.

It works from the folder Settings/Bots (see Character-Sheet.md there):
  People.json            the people, one line each: the facts (Make-Bot-People.py)
  Backgrounds/<id>.json  the person in full: life, people around them, opinions, voice, sample lines
                         (Make-Bot-Backgrounds.py, or made by itself the first time a bot speaks)
  Topics.md, Styles/     things to bring up, and how chat sounds in each game
and keeps what belongs to this PC in a state folder (Database/BotChat):
  Assignments.json       which person plays which bot, per game
  Relations.json         which bots know each other outside the game

A bridge does:   minds = Minds(bots_dir, state_dir, "wow")
                 person = minds.person_for(bot_name, gender)
                 system = minds.who(person, familiarity) + minds.voice(person) + <its own game text>
                 lines  = minds.in_their_hand(person, lines, keep=[names])
"""
import datetime
import glob
import io
import json
import os
import random
import re
import subprocess
import threading
import zlib

CHAT_MODEL = "haiku"            # quick, for every line a bot says
BACKGROUND_MODEL = "sonnet"     # careful, once per person
REPLY_TIMEOUT = 75
BACKGROUND_TIMEOUT = 240

# How well a bot knows the player decides how much of its life it lets show
STRANGER, ACQUAINTANCE, FRIEND = 0, 1, 2

SCANDINAVIAN = ("Swedish", "Norwegian", "Danish")
NATIVE_ENGLISH = ("English",)

# How people type. Each person gets one of each, decided from their number, so that the mix is guaranteed
# and does not depend on what a model feels like writing.
CASES = ["normal"] * 6 + ["lower"] * 4                      # 4 in 10 never touch the shift key
FINAL_PERIOD = [False] * 6 + [True] * 4                     # most people leave the last full stop out
LAUGHS = ["haha", "haha", "lol", "lol", "hehe", "heh", "xD", ":D", "lmao", "none", "none"]
SMILEYS = ["", "", "", "", ":)", ":)", ":P", "^^", ";)", ":D"]
EXCLAIMS = ["never", "never", "rare", "rare", "rare", "some", "some"]
APOSTROPHES = [True] * 6 + [False] * 4                      # 4 in 10 write dont, im, thats
SHORTHAND = ["none"] * 5 + ["light"] * 3 + ["heavy"] * 2    # thx and tho; or u, ur and ppl as well
LENGTHS = ["terse"] * 3 + ["normal"] * 5 + ["talker"] * 2
THANKS = ["thx", "thx", "ty", "thanks"]
# The words said most often are where people differ most: everyone has their own hello and their own goodbye
HELLOS = ["hey", "hey", "hi", "hi", "hello", "yo", "heya", "hiya", "hey there", "sup", "o/", "hello", "hey", "hi"]
BYES = ["cya", "later", "bye", "see ya", "take care", "night", "bb", "laters", "see you", "ttyl", "bye bye"]
YESES = ["yeah", "yeah", "yep", "yes", "ya", "yup", "sure", "aye", "mhm", "ok", "kk"]
WELCOMES = ["np", "no problem", "anytime", "sure thing", "you're welcome", "any time", "happy to", "of course", "all good", "sure"]
FORMAL = {"hello": ["hello", "hi", "good evening"], "bye": ["bye", "take care", "see you", "goodbye"], "yes": ["yes", "sure", "certainly"],
          "welcome": ["you're welcome", "of course", "happy to", "not at all"]}
TOO_SHORT_FOR_SOME =("o/", "sup", "bb", "ttyl", "ya", "kk", "np")     # not from people who spell everything out

# How people react. Each person gets three of these, two of them from their temper, and leans on one at a time.
LEANS = {
    "tease": "tease them a little about it",
    "same": "you have been there yourself, and say so in your own words without telling the whole story",
    "shrug": "shrug it off: these things happen",
    "straight": "take it at face value and answer straight, nothing added",
    "them": "be more interested in them than in the subject",
    "busy": "answer, and let it show that you are in the middle of something",
    "funny": "see the funny side of it",
    "under": "understate it: say less than you mean",
    "pleased": "show plainly that you are pleased",
    "worry": "fret a little on their behalf",
    "less": "say less than they expect, and leave it there",
    "wander": "let it remind you of something else for a moment",
}
LEANS_BY_TEMPER = [("teas|sarcas|playful", ("tease", "funny")), ("worrier", ("worry", "same")), ("laid back|easy-going|patient", ("shrug", "under")),
                   ("shy|quiet|observant", ("less", "under")), ("curious", ("them", "same")), ("talkative|enthusias|cheerful", ("pleased", "wander")),
                   ("dreamy|distract", ("wander", "less")), ("blunt|grump|impatient|stubborn|competitive", ("busy", "straight")),
                   ("earnest|thoughtful|generous|polite|helpful", ("straight", "same"))]

LIGHT_SHORTHAND = [(r"i don'?t know", "idk"), (r"going to(?= [a-z])", "gonna"), (r"want to(?= [a-z])", "wanna"), (r"though", "tho"),
                   (r"no problem", "np"), (r"probably", "prob"), (r"okay", "ok")]
HEAVY_SHORTHAND = [(r"you'?re", "ur"), (r"your", "ur"), (r"you", "u"), (r"right now", "rn"), (r"to be honest", "tbh"),
                   (r"because", "cuz"), (r"people", "ppl"), (r"please", "pls"), (r"be right back", "brb")]

PRIVATE = re.compile(r"[\w.+-]+@[\w-]+\.\w+|[A-Za-z]:[\\/]|AppData|\\Users\\|/Users/|Windows 1[01]|PowerShell", re.I)
SECOND_TRY = re.compile(r"\b(?:no,? wait|scratch that|plainly:|rather:|i mean:)[^:]*?(?::|,|\.\.\.|$)", re.I)
META = re.compile(r"roleplay|in character|as an ai|here'?s (the|my) reply|chat message|language model|instructions|task for me", re.I)


def find_claude():
    """Newest Claude Code program installed by the Claude desktop app, or one on PATH."""
    tail = ("Claude", "claude-code", "*", "*", "claude.exe")
    found = glob.glob(os.path.join(os.environ.get("APPDATA", ""), *tail))
    found += glob.glob(os.path.join(os.environ.get("LOCALAPPDATA", ""), "Packages", "Claude_*", "LocalCache", "Roaming", *tail))
    if found:
        version = lambda p: [int(x) for x in re.findall(r"\d+", os.path.basename(os.path.dirname(os.path.dirname(p))))]
        return max(found, key=version)
    return "claude"


def read_lines(path, section=None):
    """The lines of a notes file without comments. With a section: only the lines under '## section'."""
    if not os.path.exists(path):
        return []
    found, inside = [], section is None
    with io.open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line.startswith("## "):
                inside = section is None or line[3:].strip().lower() == section
            elif line and not line.startswith("#") and inside:
                found.append(line)
    return found


class Minds:
    def __init__(self, bots_dir, state_dir, game, log=print):
        self.bots_dir, self.state_dir, self.game, self.log = bots_dir, state_dir, game, log
        self.backgrounds_dir = os.path.join(bots_dir, "Backgrounds")
        self.session_dir = os.path.join(state_dir, "_session")
        self.claude = find_claude()
        self.lock = threading.Lock()
        self.being_written = set()
        self.people, self.traits, self.assignments, self.relations = [], {}, {}, {}
        self.load()

    # ------------------------------------------------------------------------------------------
    # Files
    # ------------------------------------------------------------------------------------------

    def _json(self, path, default):
        try:
            with io.open(path, encoding="utf-8") as f:
                return json.load(f)
        except (OSError, ValueError):
            return default

    def _save(self, path, value):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with io.open(path, "w", encoding="utf-8") as f:
            json.dump(value, f, indent=1, sort_keys=True, ensure_ascii=False)

    def load(self):
        self.people = self._json(os.path.join(self.bots_dir, "People.json"), [])
        self.traits = self._json(os.path.join(self.bots_dir, "Traits.json"), {})
        self.assignments = self._json(os.path.join(self.state_dir, "Assignments.json"), {})
        self.relations = self._json(os.path.join(self.state_dir, "Relations.json"), {})

    def background_path(self, person):
        return os.path.join(self.backgrounds_dir, "%03d.json" % person["id"])

    def background(self, person):
        """The person in full, if it has been written. None otherwise."""
        return self._json(self.background_path(person), None) if person else None

    # ------------------------------------------------------------------------------------------
    # Who plays which bot, and who knows whom
    # ------------------------------------------------------------------------------------------

    def person_for(self, bot, gender=""):
        """The person who plays this bot. Chosen from the bot's name the first time, then kept for good."""
        if not self.people:
            return None
        by_id = {p["id"]: p for p in self.people}
        with self.lock:
            mine = self.assignments.setdefault(self.game, {})
            known = by_id.get(mine.get(bot.lower()))
            if known:
                return known
            fitting = [p for p in self.people if p["gender"] == gender] or self.people     # most play their own gender
            chosen = fitting[zlib.crc32(("botminds:" + bot.lower()).encode()) % len(fitting)]
            mine[bot.lower()] = chosen["id"]
            self._save(os.path.join(self.state_dir, "Assignments.json"), self.assignments)
            return chosen

    def how_they_know(self, bot, other):
        """How two bots know each other outside the game, if they do. Decided once per pair, then kept."""
        kinds = self.traits.get("relations") or []
        if not kinds or bot.lower() == other.lower():
            return ""
        pair = "|".join(sorted((bot.lower(), other.lower())))
        with self.lock:
            mine = self.relations.setdefault(self.game, {})
            if pair not in mine:
                roll = zlib.crc32(("botminds:knows:" + pair).encode())
                mine[pair] = kinds[(roll // 100) % len(kinds)] if roll % 100 < self.traits.get("known_percent", 10) else ""
                self._save(os.path.join(self.state_dir, "Relations.json"), self.relations)
            return mine[pair]

    # ------------------------------------------------------------------------------------------
    # The person, as text for a prompt
    # ------------------------------------------------------------------------------------------

    def hand(self, person):
        """How this person types: fixed habits, decided from their number."""
        r = random.Random("hand:%d" % person["id"])
        native = person.get("language") in NATIVE_ENGLISH
        hand = {"case": r.choice(CASES), "final_period": r.choice(FINAL_PERIOD), "laugh": r.choice(LAUGHS),
                "smiley": r.choice(SMILEYS), "exclaim": r.choice(EXCLAIMS), "native": native,
                "apostrophes": r.choice(APOSTROPHES), "shorthand": r.choice(SHORTHAND), "length": r.choice(LENGTHS),
                "thanks": r.choice(THANKS)}
        if hand["shorthand"] == "heavy" and not native:
            hand["shorthand"] = "light"                         # people writing a second language spell more out
        for key, words in (("hello", HELLOS), ("bye", BYES), ("yes", YESES), ("welcome", WELCOMES)):
            hand[key] = r.choice([w for w in words if hand["shorthand"] != "none" or w not in TOO_SHORT_FOR_SOME])
            if "formal" in person.get("style", ""):             # polite people keep to the plain words
                hand[key] = r.choice(FORMAL[key])
        return hand

    def leans(self, person):
        """The three ways this person tends to react: two from their temper, one of their own."""
        r = random.Random("leans:%d" % person["id"])
        mine = next((list(keys) for pattern, keys in LEANS_BY_TEMPER if re.search(pattern, person.get("temper", ""))), [])
        mine += r.sample([key for key in LEANS if key not in mine], 3 - len(mine))
        return [LEANS[key] for key in mine]

    def facts(self, person):
        woman = person["gender"] == "female"
        country = person.get("country", "")
        where = person["home"] if not country or country.replace("the ", "") in person["home"] else f"{person['home']} in {country}"
        return (f"{person['first_name']}, a {person['age']}-year-old {'woman' if woman else 'man'} from {where}. "
                f"Work: {person['occupation']}. Home life: {person['living']}. Temper: {person['temper']}. "
                f"Humour: {person['humour']}. Outside games: {' and '.join(person['hobbies'])}. "
                f"Games: {person['gaming']}; {person.get('experience', 'knows the game reasonably well')}.")

    def life_now(self, person, background):
        """What is going on in their life at the moment. With a written arc, it moves on every two weeks."""
        arc = (background or {}).get("arc") or []
        if not arc:
            return person.get("life_now", "")
        week = datetime.date.today().isocalendar()[1]
        return arc[(week // 2 + person["id"]) % len(arc)]

    def local_clock(self, person):
        now = datetime.datetime.now(datetime.timezone.utc)
        summer = person.get("summer")
        forward = (summer == "north" and 4 <= now.month <= 10) or (summer == "south" and (now.month >= 10 or now.month <= 3))
        return now + datetime.timedelta(hours=person.get("utc", 0) + (1 if forward else 0))

    def mood(self, person, bot):
        """Today's mood: partly the hour and the day where they live, partly chance."""
        there = self.local_clock(person)
        hour, weekend = there.hour, there.weekday() >= 5
        if hour < 5:
            clock = "up far too late, a bit wired and a bit tired"
        elif hour < 9:
            clock = "barely awake, first coffee"
        elif hour < 17:
            clock = "relaxed, a free day" if weekend else "a quiet hour in the day"
        elif hour < 22:
            clock = "winding down after the day" if not weekend else "easy weekend evening"
        else:
            clock = "should be in bed soon"
        r = random.Random(f"mood:{bot.lower()}:{datetime.date.today().isoformat()}")
        today = r.choice(["cheerful", "a bit flat", "content", "restless", "pleased with how today went", "slightly grumpy",
                          "in a silly mood", "focused", "distracted by something else on the desk", "calm"])
        return f"{today}; {clock}"

    def who(self, person, bot, familiarity=STRANGER):
        """Who you are: more of the life the better the other person is known."""
        if not person:
            return "About you, the person: an ordinary person who likes games.\n"
        background = self.background(person)
        there = self.local_clock(person)
        part = ("the middle of the night" if there.hour < 5 else "early morning" if there.hour < 8 else "morning" if there.hour < 12 else
                "afternoon" if there.hour < 17 else "evening" if there.hour < 22 else "late evening")
        text = f"About you, the person behind the character: {self.facts(person)}\n"
        if background:
            text += background.get("bio", "").strip() + "\n"
            text += "Games in your life: " + background.get("gaming", "").strip() + "\n"
            if familiarity >= ACQUAINTANCE:
                people = "; ".join(f"{p.get('name', '')} ({p.get('who', '')})" for p in background.get("people", [])[:6])
                if people:
                    text += f"People in your life: {people}.\n"
                if background.get("interruptions"):
                    text += "What pulls you away from the keyboard: " + "; ".join(background["interruptions"][:4]) + ".\n"
                details = background.get("details", [])
                if details:
                    text += "Small true things about you: " + " ".join(details[:8]) + "\n"
            if familiarity >= FRIEND and background.get("opinions"):
                text += "Things you think: " + " ".join(background["opinions"][:6]) + "\n"
            if background.get("open_up"):
                text += "How you are with people: " + background["open_up"].strip() + "\n"
        text += f"Going on in your life right now: {self.life_now(person, background)}\n"
        text += (f"Where you live it is {there.strftime('%H:%M')} on {there.strftime('%A')}, {part}. Mood: {self.mood(person, bot)}. "
                 "Players are spread over the world, so for others it may be another time of day.\n")
        text += {
            STRANGER: ("The other person is a stranger. Be the way you are with strangers: friendly enough, but your private life "
                       "stays private unless they ask, and even then you keep it light.\n"),
            ACQUAINTANCE: ("You have talked with the other person before. You are at ease, and something from your day may come up, "
                           "but you do not pour your heart out.\n"),
            FRIEND: ("The other person is someone you count as a friend here. You talk freely, mention the people in your life by "
                     "name, and can be in a bad mood in front of them.\n"),
        }[familiarity]
        return text

    def voice(self, person):
        """How you write. The strongest part of the prompt: without it every bot sounds the same."""
        if not person:
            return ""
        hand = self.hand(person)
        background = self.background(person) or {}
        written = background.get("voice", {})
        text = "How you write, which matters more than anything else here:\n"
        text += f"- {person['style'].capitalize()}. " + (written.get("summary", "").strip() + "\n" if written.get("summary") else "\n")
        text += ("- You type everything in lower case, 'i' included.\n" if hand["case"] == "lower"
                 else "- You use capitals the normal way.\n")
        text += ("- You end sentences with a full stop.\n" if hand["final_period"]
                 else "- You leave out the full stop at the end of a message.\n")
        text += {"never": "- You never use exclamation marks.\n", "rare": "- An exclamation mark from you is rare.\n",
                 "some": "- You use an exclamation mark when you mean it.\n"}[hand["exclaim"]]
        text += (f"- Your words for the everyday things: '{hand['hello']}' for hello, '{hand['bye']}' for goodbye, '{hand['yes']}' for yes, "
                 f"'{hand['welcome']}' when thanked. You use these, not other people's.\n")
        text += ("- The way you take things: " + "; ".join(self.leans(person)) + ".\n")
        text += (f"- If you laugh, the word is '{hand['laugh']}' and no other. You laugh only at what is funny, which most things are not.\n" if hand["laugh"] != "none"
                 else "- You do not write 'haha' or 'lol'; if something is funny you say so in words, or not at all.\n")
        text += (f"- Your smiley is {hand['smiley']} and you use it sparingly.\n" if hand["smiley"] else "- You do not use smileys.\n")
        text += {"terse": "- You are a person of few words: you say it in as few as will do, and often leave it at that.\n",
                 "normal": "",
                 "talker": "- You are a talker: when a subject interests you, you add a second thought where others would stop.\n"}[hand["length"]]
        if not hand["apostrophes"]:
            text += "- You do not bother with apostrophes: dont, im, thats.\n"
        text += {"none": "- You spell words out; no 'u' or 'thx' from you.\n",
                 "light": f"- You shorten a few common things: {hand['thanks']}, tho, idk, gonna, np.\n",
                 "heavy": f"- You type chat shorthand: u, ur, {hand['thanks']}, idk, rn, tbh, ppl, pls.\n"}[hand["shorthand"]]
        if written.get("fillers"):
            text += ("- Words you lean on: " + ", ".join(written["fillers"][:5]) + ". In one message out of four at most; "
                     "most of your messages have none of them.\n")
        if written.get("never"):
            text += "- You would never write: " + ", ".join(written["never"][:6]) + ".\n"
        if not hand["native"]:
            text += (f"- English is not your first language ({person.get('language')} is). "
                     + (written.get("english", "It is good, with a small slip now and then: a missing article, a word order from home.")).strip() + "\n")
        examples = [v for v in (background.get("examples") or {}).values() if v][:12]
        if examples:
            text += ("Lines you have typed at other times. They show the sound of you; do not reuse them:\n"
                     + "".join(f"  {line}\n" for line in examples))
        return text

    # ------------------------------------------------------------------------------------------
    # After Claude has written: put it in their hand
    # ------------------------------------------------------------------------------------------

    def in_their_hand(self, person, lines, keep=(), laugh=True):
        """Applies the fixed typing habits, whatever the model wrote. `keep`: names that stay capitalised.
        `laugh`: whether a laugh word may stay; a model given one puts it in every line."""
        if not person:
            return lines
        hand = self.hand(person)
        out = []
        for line in lines:
            if hand["exclaim"] == "never":
                line = re.sub(r"!+", ".", line)
            elif hand["exclaim"] == "rare":
                line = re.sub(r"!{2,}", "!", line)
                if line.count("!") > 1:
                    first = line.index("!")
                    line = line[:first + 1] + line[first + 1:].replace("!", ".")
            laughs = r"\b(?:a?haha+h?|hehe+|heh|lo+l|lmao+|xd+)\b"
            if hand["laugh"] == "none" or not laugh:
                line = re.sub(laughs + r"[,.!]?\s*", "", line, flags=re.I)
            else:
                line = re.sub(laughs, hand["laugh"], line, flags=re.I)
            smileys = r"(?<![\w:]):-?[)D]|;-?\)|:-?P\b|\^\^|xD\b"
            if not hand["smiley"]:
                line = re.sub(r"\s*(?:" + smileys + ")", "", line)
            else:
                line = re.sub(smileys, lambda m: hand["smiley"] if m.group(0).lower() != "xd" or hand["laugh"] != "xD" else m.group(0), line)
            if hand["shorthand"] != "none":
                swaps = [(r"thanks|thank you", hand["thanks"])] + LIGHT_SHORTHAND + (HEAVY_SHORTHAND if hand["shorthand"] == "heavy" else [])
                for long, short in swaps:
                    line = re.sub(r"(?<![\w'])(?:" + long + r")(?![\w'])",
                                  lambda m: short.capitalize() if m.group(0)[0].isupper() and len(short) > 1 else short,
                                  line, flags=re.I)
            if not hand["apostrophes"]:
                line = re.sub(r"(?<=[A-Za-z])'(?=(?:t|m|s|ll|ve|re|d)\b)", "", line)
            if hand["case"] == "lower":
                kept = {name.lower(): name for name in keep if name}
                line = re.sub(r"[A-Za-z']+", lambda m: kept.get(m.group(0).lower(), m.group(0).lower() if not m.group(0).isupper() or len(m.group(0)) < 2
                                                              else m.group(0)), line)
                line = re.sub(r":p\b", ":P", re.sub(r":d\b", ":D", re.sub(r"\bxd\b", "xD", line)))
            line = re.sub(r"\s{2,}", " ", line).strip()
            line = re.sub(r"\s+([,.!?])", r"\1", line)
            line = re.sub(r"^[,.\s]+|[,\s]+$", "", line)
            if hand["case"] == "normal" and line[:1].islower() and not re.match(r"(?:xD|o/)", line):
                line = line[0].upper() + line[1:]
            if not hand["final_period"] and line.endswith(".") and not line.endswith(".."):
                line = line[:-1]
            if line:
                out.append(line)
        return out

    # ------------------------------------------------------------------------------------------
    # Asking Claude
    # ------------------------------------------------------------------------------------------

    def ask(self, system, transcript, model=CHAT_MODEL, timeout=REPLY_TIMEOUT):
        """One answer from Claude Code in headless mode. Returns (text, error)."""
        args = [self.claude, "-p", "--model", model, "--tools", "", "--no-session-persistence", "--disable-slash-commands",
                "--strict-mcp-config", "--setting-sources", "", "--system-prompt", system]
        env = {k: v for k, v in os.environ.items() if not k.startswith(("CLAUDE", "ANTHROPIC"))}
        env["CLAUDE_CODE_DISABLE_AUTO_MEMORY"] = "1"       # the bots share nothing with any other use of Claude on this PC
        os.makedirs(self.session_dir, exist_ok=True)
        try:
            out = subprocess.run(args, input=transcript, capture_output=True, text=True, encoding="utf-8", errors="ignore",
                                 timeout=timeout, cwd=self.session_dir, env=env)
        except FileNotFoundError:
            return "", "Claude Code was not found. Is the Claude desktop app installed?"
        except subprocess.TimeoutExpired:
            return "", f"no reply within {timeout} seconds"
        text = (out.stdout or "").strip()
        if "Not logged in" in text or "/login" in text or "Not logged in" in (out.stderr or ""):
            return "", "Claude Code is not logged in. Run Login-Bot-Chat.bat once, then try again."
        if out.returncode != 0 or not text:
            return "", "Claude Code failed: " + ((out.stderr or text).strip()[:200] or f"exit code {out.returncode}")
        return text, ""

    @staticmethod
    def chat_lines(reply, max_lines, max_length):
        """Claude's reply as a few chat lines: only what is inside <msg>, nothing private, nothing about the task."""
        tagged = re.findall(r"<msg>(.*?)</msg>", reply, re.S)
        if tagged:
            reply = "\n".join(tagged)
        else:
            reply = "\n".join(line for line in re.sub(r"</?msg>", "", reply).splitlines() if not META.search(line))
        reply = reply.replace("’", "'").replace("‘", "'").replace("“", '"').replace("”", '"')
        reply = reply.replace("—", ", ").replace("–", "-").replace("…", "...")
        # Keep the letters of European languages, drop everything else (emoji, other scripts). "|" starts colour codes in some games.
        reply = "".join(c for c in reply if c == "\n" or " " <= c <= "~" or "À" <= c <= "ÿ").replace("|", "")
        lines = []
        for raw in reply.splitlines():
            raw = raw.strip().strip('"').strip()
            second_try = list(SECOND_TRY.finditer(raw))         # "lol no wait, rather: ..." keeps only what came last
            if second_try:
                raw = raw[second_try[-1].end():].strip(" ,.:")
            if PRIVATE.search(raw):
                continue
            while raw:
                if len(raw) <= max_length:
                    lines.append(raw)
                    break
                cut = raw.rfind(" ", 0, max_length + 1)
                cut = cut if cut > 20 else max_length
                lines.append(raw[:cut].strip())
                raw = raw[cut:].strip()
        return lines[:max_lines]

    # ------------------------------------------------------------------------------------------
    # Writing a person in full
    # ------------------------------------------------------------------------------------------

    def write_background(self, person, model=BACKGROUND_MODEL):
        """Has Claude write the person in full, from the facts, and saves it. Returns (background, error)."""
        hand = self.hand(person)
        system = (
            "You write the people behind the characters in an online game: ordinary, believable people, the kind you would "
            "actually meet in a guild. Not quirky, not tragic, not a novel's hero. Most lives are plain, with a few "
            "things that are entirely their own. Be concrete: real-sounding names for the people around them (fitting "
            "the country), the make of the car, what is for dinner on Thursdays, which shift they hate. Everything must "
            "fit the facts given and the country they live in. Nothing here is about any particular game: the same person "
            "could be playing any online game.\n"
            "Answer with one JSON object and nothing else, with exactly these keys:\n"
            '  "bio": 110 to 160 words in the second person ("You grew up..."): where they come from, how they ended up '
            "where they are, what a normal week looks like, what they care about, one thing they are a little proud of and "
            "one that nags at them.\n"
            '  "people": 3 to 6 objects {"name", "who"}: partner, children, a friend, a colleague or boss, a pet, as fits.\n'
            '  "details": 8 short sentences of small concrete facts in the second person.\n'
            '  "opinions": 6 short sentences in the second person: things they think, about everyday life and about how '
            "people behave in online games. Mild and human, some a little unfair.\n"
            '  "gaming": 40 to 70 words in the second person: the games they grew up with, how they play (rushing, exploring, '
            "collecting, helping), what they are like in a group, what annoys them in other players.\n"
            '  "arc": 6 sentences in the second person, in order: one thing going on in their life as it develops over about '
            "three months, starting from the situation given. Small and real.\n"
            '  "interruptions": 4 short phrases: the things that pull them away from the keyboard in the middle of a game '
            "(a child who will not sleep, the dog, a pager, the bus to catch).\n"
            '  "open_up": one sentence in the second person on how they are with strangers and how they are with friends.\n'
            '  "voice": an object with "summary" (two sentences on how they type in a game chat: length, rhythm, how they '
            'show humour or irritation), "fillers" (2 to 4 words or short phrases they lean on), "never" (3 to 5 words or '
            'phrases they would never type), and "english" (one sentence; for a native speaker their regional colour, for '
            "others how good their English is and the one or two slips they typically make).\n"
            '  "examples": an object with the keys greeting, agree, disagree, thanks, joke, annoyed, explain, smalltalk, '
            "tired, busy, pleased, goodbye: one chat line each, exactly as they would type it in a game chat with one hand "
            "on the mouse, about nothing game-specific. They must sound like one particular person and nobody else, and "
            "follow the typing habits given. Real chat is uneven: at least five of the twelve are four words or fewer, "
            "some are fragments without a subject, and none is over fifteen words. No two lines start with the same "
            "word. Their filler words and their pet habit show in at most three of the twelve; a habit that shows in "
            "every line is a caricature. No line may start with 'Ah', 'Oh', 'Well,' or 'Honestly', and none explains "
            "itself or rounds off neatly.\n"
        )
        shorthand = {"none": "none, words spelled out", "light": f"a little ({hand['thanks']}, tho, idk)",
                     "heavy": f"plenty (u, ur, {hand['thanks']}, rn, tbh)"}[hand["shorthand"]]
        length = {"terse": "a person of few words", "normal": "ordinary", "talker": "a talker"}[hand["length"]]
        habits = (f"Typing habits to follow in the examples: {'all lower case' if hand['case'] == 'lower' else 'normal capitals'}; "
                  f"{'full stops at the end' if hand['final_period'] else 'no full stop at the end of a message'}; "
                  f"exclamation marks: {hand['exclaim']}; laugh word: {hand['laugh']}; smiley: {hand['smiley'] or 'none'}; "
                  f"apostrophes: {'kept' if hand['apostrophes'] else 'left out (dont, im)'}; "
                  f"shorthand: {shorthand}; length: {length}; way of talking: {person['style']}; "
                  f"a habit seen now and then, not in every line: {person.get('typing_habit', '')}.")
        request = (f"The facts: {self.facts(person)} Going on in their life now: {person.get('life_now', '')}. "
                   f"They like to talk about: {', '.join(person.get('topics', []))}. First language: {person.get('language', 'English')}.\n"
                   f"{habits}\nWrite the JSON.")
        raw, error = self.ask(system, request, model=model, timeout=BACKGROUND_TIMEOUT)
        if error:
            return None, error
        found = re.search(r"\{.*\}", raw, re.S)
        try:
            background = json.loads(found.group(0)) if found else None
        except ValueError:
            background = None
        if not background or "bio" not in background or "examples" not in background:
            return None, "the answer was not the JSON asked for"
        if PRIVATE.search(json.dumps(background)):
            return None, "the answer held something that looks private"
        background["id"] = person["id"]
        background["written"] = datetime.date.today().isoformat()
        self._save(self.background_path(person), background)
        return background, ""

    def want_background(self, person):
        """Starts writing the person in full in the background, if that has not been done. Does not wait."""
        if not person or os.path.exists(self.background_path(person)):
            return
        with self.lock:
            if person["id"] in self.being_written:
                return
            self.being_written.add(person["id"])

        def work():
            background, error = self.write_background(person)
            self.log(f"   {person['first_name']} ({person['id']}) written in full." if background
                     else f"   ! could not write {person['first_name']} in full: {error}")
            with self.lock:
                self.being_written.discard(person["id"])

        threading.Thread(target=work, daemon=True).start()
