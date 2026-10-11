# Bots: who they are

The files here describe the people behind the bots. Nothing in them is about one game, so the same files can serve WowPlus and MapleStoryPlus: a bridge only adds its own game's section of the topics and its own style file.

| File | What it is | Edit it? |
|---|---|---|
| `Traits.json` | The lists people are put together from: first names, ages, homes, occupations, home life, tempers, humour, hobbies, ways of talking, typing habits, gaming habits, what is going on in their life, favourite topics. | Yes. Then run `python Settings/Scripts/Make-Bot-People.py` to add people from the new lists. |
| `People.json` | The people: 300 of them, one per line. Each has a first name, gender, age, home with country, language and time zone, occupation, home life, temper, humour, two hobbies, a way of talking, a typing habit, a gaming habit, how experienced they are, something going on in their life, and three favourite topics. | Yes, by hand, one person at a time. The script keeps what is there and only adds missing people (`--fresh` starts over). |
| `Backgrounds/<id>.json` | The person in full: life story, the people around them, small true details, opinions, something going on in six steps, their voice and twelve sample lines. Written by Claude from the facts in `People.json`. | Yes, by hand. Or have it written again: `python Settings/Scripts/Make-Bot-Backgrounds.py --ids 17`. |
| `Character-Sheet.md` | Every part a bot is made of, how to write people in full, and how to test that they sound different. Start here. | |
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

Everything about who the bots are is in this folder and in one script, `Settings/Scripts/bot_minds.py`, and none of it is about one game. A game's bridge creates `Minds(bots_dir, state_dir, "maplestory")` and asks it for the person, the text about them, their voice, and for the typing habits to be applied to a reply. MapleStoryPlus has a copy of the folder and the scripts; `Update-From-WowPlus.md` in its `Settings/Bots` says what was copied and what its bridge needs. When people or backgrounds change here, copy `People.json`, `Traits.json` and `Backgrounds/` across so that the same person plays in both.

## Later

- Topics from outside: news, weather, sports results, written into `Topics.md` by a small script.
- A questionnaire to shape the lists in `Traits.json`.