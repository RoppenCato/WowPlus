# Update from WowPlus: bots as whole people (2026-10-11)

This file travels with the copy of `Settings/Bots` that WowPlus hands to MapleStoryPlus. It says what was copied, what it does, and what the MapleStoryPlus bridge has to change to use it. In WowPlus itself the work is done; there it is only a record.

## What this update is

In WowPlus the bots went from "a line of facts and a general instruction to sound natural" to people with a life and a voice of their own:

- **A full background per person**, written by Claude Sonnet from the facts in `People.json`: life story, the people around them by name, small true details, opinions, something going on that moves along every two weeks, what pulls them away from the keyboard, how they type, and twelve sample lines.
- **Typing habits fixed in code** per person (lower case, full stops, apostrophes, shorthand, few words or many, their own hello, goodbye, yes, laugh and smiley), told to Claude and then applied to the reply once more in code.
- **Familiarity**: a stranger keeps their life private, a friend hears about it.
- **Rules found by listening** that keep bots from sounding like a helpdesk.

All of it is explained in [Character-Sheet.md](Character-Sheet.md). Read that first.

## What was copied

| File | New or changed | Note |
|---|---|---|
| `Settings/Bots/Character-Sheet.md` | new | The parts of a character, the workflow, the listening test. |
| `Settings/Bots/Backgrounds/*.json` | new | The people written in full so far. The rest are written on first use, or with the script below. |
| `Settings/Bots/People.json`, `Traits.json` | unchanged | Same 300 people in both projects. |
| `Settings/Bots/Topics.md` | new here | Has an `everyday` section and one per game (`## maplestory` is there). Only used when bots start talking by themselves. |
| `Settings/Scripts/bot_minds.py` | new | Everything about who the bots are. Not about one game. **Keep it identical in both projects.** |
| `Settings/Scripts/Make-Bot-Backgrounds.py` | new | Writes backgrounds: `--count 20`, `--playing`, `--ids 4 17`, `--show 17`. |
| `Settings/Scripts/Make-Bot-People.py` | unchanged | |
| `Settings/Bots/Reference/WowPlus-Bot-Chat-Bridge.py` | new, for reading | The WowPlus bridge as it is now: the worked example of a bridge built on `bot_minds.py`. Not run here. |
| `Settings/Bots/Reference/WowPlus-Test-Bot-Voices.py` | new, for reading | The WowPlus listening test, to adapt. |

Not copied: `Styles/maplestory.md` (this project's own is kept) and anything in `Database/BotChat`.

## What the MapleStoryPlus bridge needs to change

`Settings/Scripts/Bot-Chat-Bridge.py` here still builds the person itself (`load_people`, `person_for`, `describe`, `local_time`, `mood_today`, `clean`, `ask_claude`). Replace those with `bot_minds.py` and keep everything that is about MapleStory (item and monster facts, routes and fares, map names, the port, the 70-character lines).

1. **Create the minds once:** `minds = bot_minds.Minds(BOTS_DIR, STATE_DIR, "maplestory", log=log)`.
   It reads `Database/BotChat/Assignments.json` in the same shape as before (`{"maplestory": {"botname": id}}`), so bots already assigned stay the same people. Check that the file has that shape before the first run.
2. **The person:** `person = minds.person_for(bot)`. If the game knows the character's gender, pass it as the second argument.
3. **The system prompt** is: the game's own opening line (who the character is, where it stands), then `minds.who(person, bot, familiarity)`, then `minds.voice(person)`, then the MapleStory parts (style file, game facts, rules). Take the rules from `system_prompt` in the reference bridge; they are not about WoW except for the first sentence and the patch number.
4. **Familiarity:** with no memory here, use `bot_minds.STRANGER` for everyone, or `ACQUAINTANCE` for party and guild members. If memory is added (see below), use the reference bridge's `familiarity()`.
5. **Asking and cleaning:** `raw, error = minds.ask(system, transcript)`, then `lines = minds.chat_lines(raw, MAX_LINES, MAX_LINE_LENGTH)`, then `lines = minds.in_their_hand(person, lines, keep=[player, bot], laugh=laugh)`.
   `minds.ask` runs Claude in `Database/BotChat/_session`; the old `BOT_HOME` folder is no longer needed.
6. **What to do goes last in the transcript**, not in the system prompt: `Now: <task>` and `For this message: <manner>`, then "Write what <bot> types inside <msg></msg>". Copy from the reference bridge: `manner()`, `everyday()` (hello, goodbye and thanks get the person's own word), `reply_length()` (answer length is drawn), and the `joke` / `laugh` dice in `speak()`.
7. **Backgrounds on first use:** call `minds.want_background(person)` the first time a bot speaks. It returns at once and writes the file in the background.
8. **Game facts stay as they are.** When the item or route lookup has an answer, add it to the task ("Something you know: ...") the way the reference bridge adds a rumour, so the person says it in their own voice.

## Worth taking over as well

| From WowPlus | What it adds | Where to look in the reference bridge |
|---|---|---|
| Memory | Up to eight notes per bot about you, and what the bot said lately so it does not repeat itself. It is also what lets a bot become a friend. | `load_memory`, `note_said`, `remember`, `remember_what_is_due`, `forget_strangers` |
| Bots speaking first | A greeting, a remark in guild or party chat from their own life, another bot answering. | `greet`, `open_talk`, `react`, `joined`, `meet` (each needs the game server to ask for it) |
| The listening test | Six people answer the same lines side by side. | `WowPlus-Test-Bot-Voices.py`: change the made-up character in `ask()` to a MapleStory one and the standard lines to MapleStory ones |

## After the change, check

```
python Settings/Scripts/Make-Bot-Backgrounds.py --show 6
python Settings/Scripts/Bot-Chat-Bridge.py --say "hey" --say "where do i find steelies" --say "gtg"
```

The first prints a person as a bot is told about itself. In the second, the answers should be short, in one person's hand, and the middle one should carry the game fact in the person's own words.

## Keeping the two projects the same

Copy in both directions when they change: `People.json`, `Traits.json`, `Backgrounds/`, `Character-Sheet.md`, `bot_minds.py`, `Make-Bot-Backgrounds.py`, `Make-Bot-People.py`. A person then lives the same life in both games. If `bot_minds.py` needs something only one game wants, add it as an argument rather than a second version of the file.
