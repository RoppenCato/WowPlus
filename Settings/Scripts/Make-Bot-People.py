"""Writes Settings/Bots/People.json: the people who play the bots.

Each person is put together from the lists in Settings/Bots/Traits.json. Run this again after
changing the lists. People already in the file are kept as they are (so a bot you know stays the
same person, and anything you edited by hand survives); only missing ones are added.

    python Settings/Scripts/Make-Bot-People.py            add people until there are 240
    python Settings/Scripts/Make-Bot-People.py --count 400
    python Settings/Scripts/Make-Bot-People.py --fresh    throw everyone away and start over
"""
import argparse
import io
import json
import os
import random

HERE = os.path.dirname(os.path.abspath(__file__))
BOTS = os.path.join(os.path.dirname(HERE), "Bots")
PEOPLE = os.path.join(BOTS, "People.json")


def make_person(number, traits):
    r = random.Random(f"person:{number}")
    gender = "female" if r.random() < 0.4 else "male"
    age = r.choice(traits["ages"])
    # Keep a life believable for the age
    def fits_age(text):
        limits = [("pupil", 0, 18), ("student", 18, 30), ("PhD", 24, 33), ("apprentice", 16, 25), ("retired", 50, 99),
                  ("parents", 0, 24), ("student housing", 18, 28), ("kids have moved out", 48, 99),
                  ("two kids", 27, 55), ("single parent", 24, 55), ("married", 25, 99), ("parental leave", 24, 42),
                  ("exam", 0, 30), ("driving test", 16, 24), ("promotion", 22, 60), ("wedding", 23, 45)]
        return all(low <= age <= high for word, low, high in limits if word in text)

    if age <= 17:
        occupation = "upper secondary school pupil"
    elif age >= 62:
        occupation = r.choice(["retired", "retired early", "works part time, nearly retired"])
    else:
        occupation = r.choice([o for o in traits["occupations"] if fits_age(o)])
    living = r.choice([text for text in traits["living"] if fits_age(text)])
    life_now = r.choice([text for text in traits["life_now"] if fits_age(text)])
    return {
        "id": number,
        "first_name": r.choice(traits["first_names_" + gender]),
        "gender": gender,
        "age": age,
        "home": r.choice(traits["homes"]),
        "occupation": occupation,
        "living": living,
        "temper": r.choice(traits["tempers"]),
        "humour": r.choice(traits["humours"]),
        "hobbies": r.sample(traits["hobbies"], 2),
        "style": r.choice(traits["styles"]),
        "typing_habit": r.choice(traits["typing_habits"]),
        "gaming": r.choice(traits["gaming_habits"]),
        "life_now": life_now,
        "topics": r.sample(traits["favourite_topics"], 3),
    }


def main():
    p = argparse.ArgumentParser(description="Make the people who play the bots")
    p.add_argument("--count", type=int, default=240)
    p.add_argument("--fresh", action="store_true")
    a = p.parse_args()
    with io.open(os.path.join(BOTS, "Traits.json"), encoding="utf-8") as f:
        traits = json.load(f)
    people = []
    if os.path.exists(PEOPLE) and not a.fresh:
        with io.open(PEOPLE, encoding="utf-8") as f:
            people = json.load(f)
    known = {person["id"] for person in people}
    added = 0
    for number in range(1, a.count + 1):
        if number not in known:
            people.append(make_person(number, traits))
            added += 1
    people.sort(key=lambda person: person["id"])
    with io.open(PEOPLE, "w", encoding="utf-8", newline="\n") as f:
        f.write("[\n" + ",\n".join(" " + json.dumps(person, ensure_ascii=False) for person in people) + "\n]\n")
    print(f"{len(people)} people in {PEOPLE} ({added} new)")


if __name__ == "__main__":
    main()