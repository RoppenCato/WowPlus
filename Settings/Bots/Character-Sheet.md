# Character sheet: what a bot is made of

A bot feels like a person when three things hold at once: there is **a life** behind it, it has **a voice** nobody else has, and it **behaves** like someone typing in a game rather than someone answering questions. This page lists every part, where it is kept, and how to make more or change it. Nothing here is about one game; the same people play in WowPlus and MapleStoryPlus.

## The parts

| # | Part | What it gives | Where it is | Made by |
|---|---|---|---|---|
| 1 | **Facts** | Name, age, gender, home and time zone, language, work, home life, temper, humour, hobbies, way of talking, gaming habit, experience, favourite topics. | `People.json`, one line each | `Make-Bot-People.py` from the lists in `Traits.json` |
| 2 | **Life story** | Where they come from, a normal week, what they care about, one thing they are proud of and one that nags. | `Backgrounds/<id>.json` → `bio` | Claude Sonnet, once |
| 3 | **People around them** | Partner, children, a friend, a boss, a pet, all with names. A friend hears "Mats took my bed again"; a stranger does not. | → `people` | same |
| 4 | **Small true things** | Eight concrete details: the car, Thursday's dinner, the shift they hate. These are what make a life sound lived. | → `details` | same |
| 5 | **Opinions** | Six mild, slightly unfair views on everyday life and on how people behave in games. Only friends hear them. | → `opinions` | same |
| 6 | **Games in their life** | What they grew up with, how they play, what they are like in a group, what annoys them. | → `gaming` | same |
| 7 | **Something going on** | One thing in their life in six steps over about three months (a late parcel, a bad back, relatives staying). It moves on every two weeks, so a bot has news. | → `arc` | same |
| 8 | **Interruptions** | What pulls them from the keyboard: a child who will not sleep, the dog, a tram to catch. | → `interruptions` | same |
| 9 | **Reserve** | How they are with strangers and with friends. | → `open_up` | same |
| 10 | **Voice** | How they type in two sentences, the words they lean on, the words they would never use, and what their English is like (regional colour, or the slips of a second language). | → `voice` | same |
| 11 | **Sample lines** | Twelve lines they have typed: greeting, agreeing, disagreeing, thanks, a joke, annoyed, explaining, small talk, tired, busy, pleased, goodbye. The strongest single thing for making a bot sound like itself. | → `examples` | same |
| 12 | **Hand** | Fixed typing habits: lower case or capitals, full stop or none, exclamation marks, apostrophes, shorthand (thx, u, idk), few words or many, their own laugh word and smiley, and their own words for hello, goodbye, yes and "you're welcome". | not stored: worked out from the person's number in `bot_minds.py` | code |
| 13 | **Ways of reacting** | Three out of twelve (teases, shrugs it off, frets, sees the funny side, says less than expected ...), two of them from their temper. | not stored: from temper and number | code |
| 14 | **Clock and mood** | The time and weekday where they live, and a mood for today. | worked out when they speak | code |
| 15 | **Familiarity** | Stranger, acquaintance or friend, from how often you have talked and whether you share a guild. Decides how much of parts 3 to 5 the bot may use. | the bridge, from its memory of you | code |
| 16 | **Memory of you** | Up to eight notes about you, including what the bot told you about itself, so the story stays the same. | `Database/BotChat/<game>/<you>/<bot>.json` | Claude Haiku, after a talk |
| 17 | **Who they know** | About one pair of bots in ten know each other outside the game. | `Database/BotChat/Relations.json` | code |
| 18 | **The game's own sound** | The words and tone of one game's chat. | `Styles/<game>.md` | by hand |
| 19 | **Things to bring up** | Everyday topics, the game's topics, and from the background: what is going on, small true things, interruptions. | `Topics.md` and the background | by hand, and Claude |

Parts 12 to 14 are decided by code and not by Claude on purpose. Left to itself a model gives everyone the same tidy sentences; when the code says "this one never uses the shift key and writes thx", it stays true in every line, and the mix across 300 people is guaranteed (4 in 10 lower case, 3 in 10 of few words, and so on). After Claude has written a line, the same habits are applied to it once more in code (`in_their_hand`), so a slip by the model never reaches the game.

## Writing the people in full

`People.json` holds the facts for 300 people. The background (parts 2 to 11) is written by Claude Sonnet from those facts, one file per person, about a minute each:

```
python Settings/Scripts/Make-Bot-Backgrounds.py --count 20      the next 20 people that have none
python Settings/Scripts/Make-Bot-Backgrounds.py --playing       everyone who already plays a bot
python Settings/Scripts/Make-Bot-Backgrounds.py --ids 4 17      these people, again
python Settings/Scripts/Make-Bot-Backgrounds.py --show 17       print one person as a bot is told about itself
```

You do not have to run it. The first time a bot speaks whose person has no background, the bridge has it written while you play; that first talk uses the facts alone, and from the next one on the bot has its whole life.

A background is a plain file. **Edit it by hand** when you want someone to be different: change the sample lines and the bot's voice changes with them.

### What a good background looks like

- Plain lives. Not quirky, not tragic. A few things entirely their own.
- Concrete: names that fit the country, a make of car, a day of the week.
- Sample lines that are uneven like real chat: several of four words or fewer, fragments, no two starting the same way.
- A habit that shows in three lines out of twelve. In every line it is a caricature (the first version of Sofie started all eight of hers with "so").
- Nothing about one game.

To change what all backgrounds are asked for, edit the request in `write_background` in `bot_minds.py`, then write a few again with `--ids` and read them.

## What makes the talk sound real

These are in the bridge (`Bot-Chat-Bridge.py`) and were found by listening:

| Rule | Why |
|---|---|
| Answer length is drawn, not fixed: often two to five words, now and then more. People of few words draw from the short end. | A model writes every answer the same length. Real chat is uneven. |
| Hello, goodbye and thanks get the person's own word and nothing more. | They are most of what is said, and where everyone sounded alike ("Glad I could help", "Enjoy your dinner"). |
| A laugh word stays only with a joke, or half the time when you laughed first. | Given a laugh word, a model puts it in every other line. |
| "You are a player, not a guide": no tips nobody asked for, no good luck, no comforting, and a list of phrases never to type. | The assistant's helpfulness is the surest sign of a bot. |
| Questions back, jokes and talk of their own day are each decided by dice. | Otherwise every message ends with "You?". |
| Asked how they are, they answer from today's mood and what is going on. | Otherwise all say "Pretty good so far". |
| A stranger keeps their life to themselves; a friend names the people in it. | It makes getting to know a bot worth something. |
| The message is written once: no correcting itself, and code cuts off a second attempt. | Haiku sometimes argued with itself inside the message. |

## Listening test

After changing a prompt, a rule or the backgrounds, listen before playing:

```
python Settings/Scripts/Test-Bot-Voices.py                six people answer the same seven lines
python Settings/Scripts/Test-Bot-Voices.py --ids 2 6 12   these people
python Settings/Scripts/Test-Bot-Voices.py --friend       the same, as guild mates who know you
python Settings/Scripts/Test-Bot-Voices.py --talk 6       a short conversation with one person
python Settings/Scripts/Test-Bot-Voices.py --say "..."    your own line
```

Read the answers to one line down the column and ask:

1. **Could I tell who said what with the names covered?** If not, the voices are too alike.
2. **Do several start with the same word?** The numbers at the end show the most common first words.
3. **Is anyone being a helpdesk?** Advice nobody asked for, wishing well, apologising.
4. **Are the lengths uneven?** An average near six words with a few long ones is right. All the same length is wrong.
5. **Does anything sound written rather than typed?** Three-part sentences, a neat ending, "that said".
6. **With `--friend`: does their life show a little more**, without being poured out?

It uses a throwaway folder, so the bots' real memories of you are untouched.

## Changing the mix

| To change | Edit |
|---|---|
| Who exists: names, homes, work, tempers | `Traits.json`, then `Make-Bot-People.py` |
| One person | their line in `People.json`, their file in `Backgrounds/` |
| How many type in lower case, use shorthand, write little | the lists at the top of `bot_minds.py` (`CASES`, `SHORTHAND`, `LENGTHS` ...). This changes existing people's habits too. |
| The words for hello, goodbye, yes | `HELLOS`, `BYES`, `YESES`, `WELCOMES` in `bot_minds.py` |
| The ways of reacting | `LEANS` and `LEANS_BY_TEMPER` in `bot_minds.py` |
| How often they ask back, joke, talk of themselves | the chances at the top of the bridge |
| Phrases bots must never type | the "player, not a guide" rule in the bridge's `system_prompt` |
