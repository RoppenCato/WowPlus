"""Writes the people behind the bots in full: a life, the people around them, opinions, a voice, and
sample lines. One file per person in Settings/Bots/Backgrounds, made by Claude from the facts in
People.json. See Settings/Bots/Character-Sheet.md.

The chat bridge does this by itself the first time a bot speaks, so this script is for doing many
at once, or for doing someone again.

    python Settings/Scripts/Make-Bot-Backgrounds.py --count 20      the next 20 people that have none
    python Settings/Scripts/Make-Bot-Backgrounds.py --ids 4 17 230  these people (again, if they exist)
    python Settings/Scripts/Make-Bot-Backgrounds.py --playing       everyone who already plays a bot
    python Settings/Scripts/Make-Bot-Backgrounds.py --show 17       print one person as the bots are told

Each person takes Claude about half a minute. It uses your Claude login, like the chat itself.
"""
import argparse
import os
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import bot_minds


def main():
    p = argparse.ArgumentParser(description="Write the people behind the bots in full")
    p.add_argument("--count", type=int, default=0)
    p.add_argument("--ids", type=int, nargs="*", default=[])
    p.add_argument("--playing", action="store_true")
    p.add_argument("--show", type=int)
    p.add_argument("--game", default="wow")
    p.add_argument("--model", default=bot_minds.BACKGROUND_MODEL)
    p.add_argument("--at-once", type=int, default=3)
    a = p.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    minds = bot_minds.Minds(os.path.join(ROOT, "Settings", "Bots"), os.path.join(ROOT, "Database", "BotChat"), a.game)
    by_id = {person["id"]: person for person in minds.people}

    if a.show:
        person = by_id[a.show]
        print(minds.who(person, "(bot)", bot_minds.FRIEND))
        print(minds.voice(person))
        return

    todo = [by_id[i] for i in a.ids if i in by_id]
    if a.playing:
        playing = {i for game in minds.assignments.values() for i in game.values()}
        todo += [by_id[i] for i in sorted(playing) if i in by_id and not os.path.exists(minds.background_path(by_id[i]))]
    if a.count:
        missing = [person for person in minds.people if not os.path.exists(minds.background_path(person)) and person not in todo]
        todo += missing[:a.count]
    if not todo:
        have = sum(os.path.exists(minds.background_path(person)) for person in minds.people)
        print(f"{have} of {len(minds.people)} people are written in full. Nothing asked for: see --count, --ids, --playing.")
        return

    def one(person):
        background, error = minds.write_background(person, model=a.model)
        print(f"{person['id']:>4} {person['first_name']:<10} {person['home']:<28} " + ("written" if background else "FAILED: " + error), flush=True)
        return bool(background)

    with ThreadPoolExecutor(max_workers=max(1, a.at_once)) as pool:
        done = sum(pool.map(one, todo))
    print(f"{done} of {len(todo)} written.")


if __name__ == "__main__":
    main()