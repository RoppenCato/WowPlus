"""Writes Settings/Bots/People.json: the people who play the bots.

Each person is put together from the lists in Settings/Bots/Traits.json. Run this again after
changing the lists. People already in the file are kept as they are (so a bot you know stays the
same person, and anything you edited by hand survives); only missing ones are added.

    python Settings/Scripts/Make-Bot-People.py            add people until there are 300
    python Settings/Scripts/Make-Bot-People.py --count 500
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

# Words in a list entry that only fit some ages: (word, youngest, oldest)
AGE_LIMITS = [("pupil", 0, 18), ("student", 18, 30), ("PhD", 24, 33), ("apprentice", 16, 25), ("retired", 50, 99),
              ("parents", 0, 24), ("student housing", 18, 28), ("kids have moved out", 48, 99), ("kids", 27, 58),
              ("baby", 24, 42), ("single parent", 24, 55), ("married", 25, 99), ("divorced", 30, 99),
              ("parental leave", 24, 42), ("exam", 0, 30), ("driving test", 16, 24), ("promotion", 22, 60),
              ("wedding", 23, 45), ("kid starting school", 28, 45), ("dentist", 27, 99), ("engineer", 24, 99)]


def weighted(r, rows, weight):
    return r.choices(rows, weights=[weight(row) for row in rows])[0]


def make_person(number, traits):
    r = random.Random(f"person:{number}")
    gender = "female" if r.random() * 100 < traits["female_percent"] else "male"
    low, high, _ = weighted(r, traits["ages"], lambda row: row[2])
    age = r.randint(low, high)
    fits = lambda text: all(young <= age <= old for word, young, old in AGE_LIMITS if word in text)
    home = weighted(r, traits["homes"], lambda row: row.get("weight", 1))
    if age <= 17:
        occupation = "upper secondary school pupil"
    elif age >= 63:
        occupation = r.choice(["retired", "retired early", "works part time, nearly retired"])
    else:
        occupation = r.choice([o for o in traits["occupations"] if fits(o)])
    edgy = r.random() * 100 < traits["edgy_percent"]
    return {
        "id": number,
        "first_name": r.choice(traits["names"][home["names"]][gender]),
        "gender": gender,
        "age": age,
        "home": home["place"],
        "country": home["country"],
        "language": home["language"],
        "utc": home["utc"],
        "summer": home["summer"],
        "occupation": occupation,
        "living": r.choice([text for text in traits["living"] if fits(text)]),
        "temper": r.choice(traits["tempers_edgy" if edgy else "tempers"]),
        "humour": r.choice(traits["humours"]),
        "hobbies": r.sample(traits["hobbies"], 2),
        "style": r.choice(traits["styles"]),
        "typing_habit": r.choice(traits["typing_habits"]),
        "gaming": r.choice(traits["gaming_habits"]),
        "experience": weighted(r, traits["experience"], lambda row: row[1])[0],
        "life_now": r.choice([text for text in traits["life_now"] if fits(text)]),
        "topics": r.sample(traits["favourite_topics"], 3),
    }


def main():
    p = argparse.ArgumentParser(description="Make the people who play the bots")
    p.add_argument("--count", type=int, default=300)
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