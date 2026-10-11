# Bots: who they are

The files here describe the people behind the bots. Nothing in them is about one game, so the same files can serve WowPlus and MapleStoryPlus: a bridge only adds its own game's section of the topics and its own style file.

| File | What it is | Edit it? |
|---|---|---|
| `Traits.json` | The lists people are put together from: first names, ages, homes, occupations, home life, tempers, humour, hobbies, ways of talking, typing habits, gaming habits, what is going on in their life, favourite topics. | Yes. Then run `python Settings/Scripts/Make-Bot-People.py` to add people from the new lists. |
| `People.json` | The people: 300 of them, one per line. Each has a first name, gender, age, home with country, language and time zone, occupation, home life, temper, humour, two hobbies, a way of talking, a typing habit, a gaming habit, how experienced they are, something going on in their life, and three favourite topics. | Yes, by hand, one person at a time. The script keeps what is there and only adds missing people (`--fresh` starts over). |
| `Topics.md` | Things bots bring up by themselves: an everyday section and one per game. | Yes. Works at once. |
| `Styles/wow.md`, `Styles/maplestory.md` | How chat sounds in each game: its words, its abbreviations, its tone. This is what keeps a WoW bot sounding like WoW and a Maple bot like Maple, even when the same person plays both. | Yes. Works at once. |

What is personal to this PC is kept outside the repository, in `Database/BotChat`:

| File | What it is |
|---|---|
| `Relations.json` | Which bots know each other outside the game, and how. |
| `Assignments.json` | Which person plays which bot, per game. Fixed the first time a bot speaks, so a bot stays the same person even when `People.json` grows. To give a bot a particular person, put that person's `id` here. |
| `wow/<your character>/<bot>.json` | The history: what that bot remembers about you (up to eight notes) and what it said lately (so that it does not repeat itself). |

## How the people were shaped (Robin's answers, 2026-10-11)

- **Where they live:** Europe, Japan, China, the United States, Canada, Australia and New Zealand. Each home has a time zone, and a bot knows what time it is at home: a Japanese player may say it is the middle of the night there.
- **Ages:** a mix, but six in ten are between 30 and 45.
- **Men and women:** about 60 to 40.
- **Temper:** 95 in 100 are simply nice. The rest are sharper (sarcastic, blunt, grumpy, competitive) but nobody is unpleasant.
- **Language:** English. Only when you write Swedish to a Swede, Norwegian or Dane do they answer in their own language.
- **Experience:** some are new, most are casual or veterans. They do not ask you how to do things in the game.
- **They can know each other:** about one pair in ten of the bots in your guild or party know each other outside the game (siblings, cousins, old school friends, former colleagues or flatmates, friends from another game). It is decided once per pair and kept in `Database/BotChat/Relations.json`.

## Using the same files from MapleStoryPlus

Not connected yet. The MapleStoryPlus bridge still has its own small lists. To share, it would read `People.json`, `Topics.md` (sections `everyday` and `maplestory`) and `Styles/maplestory.md` from this folder, and keep its own `maplestory` section in `Assignments.json` and its own history folder. The same person could then play a bot in both games.

## Later

- Topics from outside: news, weather, sports results, written into `Topics.md` by a small script.
- A questionnaire to shape the lists in `Traits.json`.