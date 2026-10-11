"""Puts the same lines to several of the people behind the bots and prints the answers side by side,
so that you can hear whether they sound like different people. Use it after changing the prompts in
Bot-Chat-Bridge.py or bot_minds.py, or after writing new backgrounds. Nothing is kept: it uses its
own throwaway folder (Database/BotChat/_test), not the bots' real memories.

    python Settings/Scripts/Test-Bot-Voices.py                    6 people that are written in full
    python Settings/Scripts/Test-Bot-Voices.py --ids 1 3 17       these people
    python Settings/Scripts/Test-Bot-Voices.py --say "any tips?"  your own line instead of the standard ones
    python Settings/Scripts/Test-Bot-Voices.py --friend           as guild mates who have talked with you before
    python Settings/Scripts/Test-Bot-Voices.py --talk 17          a short conversation with one person

What to look for is in Settings/Bots/Character-Sheet.md, under "Listening test".
"""
import argparse
import importlib.util
import os
import shutil
import sys
from collections import Counter
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

LINES = ["hey", "how's your day going?", "any idea where I should level at 22?", "lol I just died to a murloc",
         "thanks for the help earlier", "what do you do when you're not playing?", "gtg, dinner"]
TALK = ["hi, you busy?", "what are you up to", "nice. been playing long?", "haha same", "what's it like where you live",
        "ok I should get back to questing", "cya"]


def load_bridge():
    spec = importlib.util.spec_from_file_location("bridge", os.path.join(HERE, "Bot-Chat-Bridge.py"))
    bridge = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bridge)
    return bridge


def main():
    p = argparse.ArgumentParser(description="Hear several bots answer the same lines")
    p.add_argument("--ids", type=int, nargs="*", default=[])
    p.add_argument("--count", type=int, default=6)
    p.add_argument("--say", action="append")
    p.add_argument("--friend", action="store_true")
    p.add_argument("--talk", type=int)
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    bridge = load_bridge()
    test_dir = os.path.join(bridge.STATE_DIR, "_test")
    shutil.rmtree(test_dir, ignore_errors=True)
    bridge.STATE_DIR, bridge.MEMORY_DIR = test_dir, os.path.join(test_dir, bridge.GAME)
    bridge.WRITE_PEOPLE_IN_FULL = False
    bridge.log = lambda text: None
    minds = bridge.start_minds(quiet=True)
    by_id = {person["id"]: person for person in minds.people}

    ids = [a.talk] if a.talk else a.ids
    people = [by_id[i] for i in ids if i in by_id] or [q for q in minds.people if minds.background(q)][:a.count]
    if not people:
        print("Nobody is written in full yet. Run Make-Bot-Backgrounds.py --count 6 first.")
        return
    bots = {}
    for person in people:
        bot = "Voice" + "abcdefghij"[person["id"] // 100] + "abcdefghij"[person["id"] // 10 % 10] + "abcdefghij"[person["id"] % 10]
        minds.assignments.setdefault(bridge.GAME, {})[bot.lower()] = person["id"]
        bots[bot] = person
        if a.friend:
            bridge.save_memory("Tester", bot, {"notes": ["Plays an undead mage.", "Easy to talk to."], "talks": 5, "said": [], "guildmate": True})

    def ask(bot, line, channel):
        person = bots[bot]
        q = {"player": "Tester", "pclass": "Mage", "prace": "Undead", "plevel": "22", "pzone": "Silverpine Forest",
             "bot": bot, "bclass": "Shaman", "brace": "Tauren", "blevel": "24", "bzone": "The Barrens",
             "bgender": person["gender"], "channel": channel, "guildmate": a.friend, "party": False,
             "guild": "Night Shift", "others_list": [], "text": line}
        return " / ".join(text for _, text in bridge.reply(q)) or "(nothing)"

    for bot, person in bots.items():
        hand = minds.hand(person)
        print(f"{person['first_name']:<10} {person['id']:>3}  {person['age']}, {person['home']}, {person['occupation']}; {person['temper']}; "
              f"{hand['case']} case, {hand['length']}, laugh {hand['laugh']}, shorthand {hand['shorthand']}")
    print()

    if a.talk:
        bot = next(iter(bots))
        for line in a.say or TALK:
            print(f"{'You':<10} {line}")
            print(f"{bots[bot]['first_name']:<10} {ask(bot, line, 'whisper')}")
        return

    answers = []
    for line in a.say or LINES:
        print(f"You: {line}")
        # Each bot gets its own whisper, so that nobody hears the others
        with ThreadPoolExecutor(max_workers=3) as pool:
            said = list(pool.map(lambda bot: ask(bot, line, "whisper"), bots))
        for (bot, person), text in zip(bots.items(), said):
            print(f"   {person['first_name']:<10} {text}")
            answers.append(text)
        print()

    # A few numbers that show sameness at a glance
    words = [len(text.split()) for text in answers]
    openers = Counter(text.split()[0].lower().strip(",.!") for text in answers if text.split())
    print(f"{len(answers)} answers. Words per answer: average {sum(words) / len(words):.1f}, longest {max(words)}. "
          f"With a question: {sum('?' in t for t in answers)}. With an exclamation mark: {sum('!' in t for t in answers)}.")
    print("Most common first words: " + ", ".join(f"{word} ({n})" for word, n in openers.most_common(6)))


if __name__ == "__main__":
    main()
