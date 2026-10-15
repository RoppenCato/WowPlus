# Bot chat: talking to bots through Claude

**Status: built 2026-10-11.** The bridge is tested on its own. The game side has not been tried in the game yet.

Talk to a bot and it answers like the person playing that character: someone with an age, a job, a temper and hobbies, who knows the game and remembers you.

## How to use it

1. Once: run `Login-Bot-Chat.bat` and sign in with your Claude account.
2. `Start-Server.bat` opens the bot chat window for you (minimised, as "WowPlus Bot Chat"), unless one is already running. Leave it open while you play; it shows every exchange. `Start-Bot-Chat.bat` starts it by hand.
3. In the game, talk to a bot.

Close the window with Ctrl+C rather than the X: the bots then write down what they remember first. Without the window, bots simply stay quiet.

## Who answers

| Where you talk | Who answers |
|---|---|
| Whisper | The bot you whisper. |
| `/say` | A nearby bot you name. If you name nobody: the bot you have targeted. If you have no target: the closest bot, if it stands within 8 yards. |
| Party chat | A bot you name. If you name nobody: the bot that answered last keeps the talk going for two minutes; otherwise one of them answers half the time. |
| Guild chat | The same as party chat, with a one-in-three chance when nobody is named. |
| General, Trade and other zone channels | A bot of your zone that you name; otherwise the one that answered last, or one of them a quarter of the time. |

A line that is a bot command (`follow`, `stay`, `attack` ...) goes to the bot as a command and gets no chat answer, unless it is a question.

## Bots speaking first

- **A greeting.** When you come online, a guild mate usually says hello (70 percent of the time).
- **Guild chat.** Every 5 to 10 minutes a guild mate says something by itself: about its day, the game, one of its favourite topics. Half the time another guild mate answers, and sometimes the first says one more thing. It ends there unless you join in. While you say nothing in guild chat the pauses grow, up to three times as long; they go back to normal as soon as you say something.
- **Moments.** When you level up, die in a party, loot something blue or better, or kill a rare or a boss, one party member or guild mate may say something: about one moment in three, never two bots on the same one. A level that ends in 0 always gets a word.
- **Meeting in the world.** A guild mate, or a bot you have talked with before, that comes within 20 yards of you waves and says something. The same bot not again within three hours.
- **Rumours.** A guild mate who starts talking sometimes passes on something true: a vendor somewhere with an unusual item this week, or where a rare enemy of your level roams. They pass it on the way people do, roughly: the zone, not the spot. Ask a bot for news and it has one at hand.
- **A gift.** About every three hours of play a guild mate mails you a few potions for your level, with a short letter.
- **Companions** remark on things in party chat: see [Bots.md](Bots.md).

Settings: `WowPlus.BotChat.Guild.*`, `WowPlus.BotChat.Moments.Chance`, `WowPlus.BotChat.Meet.Chance` and `WowPlus.BotChat.Rumour.Chance` in `Settings/mod_wowplus.conf`.

## How talkative they are

Most of what people write in a game is plain and practical, so that is the default. Whether one message may end with a question, carry a joke, mention the bot's own day or have a smiley is decided by dice, with chances set at the top of `Settings/Scripts/Bot-Chat-Bridge.py` (`QUESTION_CHANCE` 0.2, `JOKE_CHANCE` 0.12, `PERSONAL_CHANCE` 0.12). A bot never asks a question two messages in a row. Raise the numbers for livelier bots.

The length of an answer is drawn too: often two to five words, now and then more, and people of few words draw from the short end. Hello, goodbye and thanks get the person's own word and nothing else. A laugh word stays only with a joke, or half the time when you laughed first. Bots are told they are players and not guides: no tips nobody asked for, no "glad I could help".

A bot that joins your party or guild says a short hello in that chat. It does not whisper you unasked, and it does not send the bot module's canned lines ("Hello, I follow you!", "Invite me to your group first").

## Who a bot is

Each bot is played by one of 300 people. The person is chosen from the bot's name the first time it speaks, and then written down, so a bot stays the same person. Every part of a person, where it is kept and how to change it is in the [character sheet](../Bots/Character-Sheet.md). In short:

- **A life.** The facts are in `Settings/Bots/People.json`. The rest (their story, the people around them by name, small true details, opinions, something going on that moves along every two weeks, what pulls them from the keyboard) is in `Settings/Bots/Backgrounds/<id>.json`, written by Claude Sonnet once per person. `Make-Bot-Backgrounds.py` writes many at once; otherwise the bridge has a person written the first time their bot speaks, and the bot has its whole life from the next talk on.
- **A voice.** Twelve sample lines per person, the words they lean on and the ones they would never use. On top of that, fixed typing habits that code applies to every line, so they hold whatever the model writes: lower case or capitals, full stops, exclamation marks, apostrophes, shorthand (thx, u, idk), few words or many, and their own word for hello, goodbye, yes, "you're welcome" and laughing.
- **How well they know you.** A stranger is friendly and keeps their life private. After you have talked, or as a guild mate, something from their day may come up. A friend (a guild mate you have talked with, or anyone after four talks) names the people in their life and can be in a bad mood in front of you.
- **Today.** The time and weekday where they live, and a mood.

To hear whether they sound like different people, without the game: `python Settings/Scripts/Test-Bot-Voices.py`.

The lists, the people, the topics and the style of each game are plain files you can edit: see [Settings/Bots](../Bots/README.md). They are written so that MapleStoryPlus can use the same files; `Styles/wow.md` is what keeps WoW chat sounding like WoW.

The bot also knows its own character (level, race, class, where it is), yours, and whether you are guild mates or in a party together.

**Against repeating themselves:** a bot is shown the last ten lines it said to you and told not to reuse their wording, jokes or openings.
## What a bot remembers

After a stretch of talk (eight lines, or four minutes of silence) the bot's notes about you are brought up to date: at most eight short lines such as "He plays a mage", "We joked about murlocs", "Wants to run Deadmines this week". Next time those notes are part of what the bot knows.

- Kept in `Database/BotChat/wow/<your character>/<bot>.json`, a few hundred bytes per bot. Plain text; you can read or edit them.
- **Guild mates keep their notes for good.**
- **Strangers** keep theirs too, since it costs almost nothing, but notes of a stranger you have not spoken to for 30 days are thrown away.
- Party and guild chat are one conversation that every bot in it follows. The bots that spoke in it remember it.

The notes are written by Claude from the conversation, so they are a summary, not a recording.

## What bots know about this server

`Settings/Reference/Bot-Knowledge.md` holds what every player here would know: the hearthstone menu, sprint, weekly vendor stock, rares. One fact per line. A change works at once, without restarting anything. This is also where hints and rumours for the adventure layer can go later.

For everything else about the game, Claude's own knowledge of patch 3.3.5 is used. Bots are told to say they are not sure rather than guess.

## Rules the bots follow

- At most three short lines per reply, and never more than what was said to them: a word or two gets a few words back.
- They stay the person they are and never say they are an AI.
- They know nothing about you beyond what you told them, and nothing about this PC. Lines containing an email address, a file path or details of the computer never reach the game.
- They do not promise trades, gifts or meetings they cannot keep.

## How it is built

| Part | File | What it does |
|---|---|---|
| The game side | `Server/modules/mod-wowplus/src/WowPlusBotChat.cpp` | Decides which bot answers, sends the details to the bridge on a helper thread, and makes the bot say the lines that come back, with a typing pause between lines. Settings: `WowPlus.BotChat.*`. |
| The bridge | `Settings/Scripts/Bot-Chat-Bridge.py` | A small web server on this PC only (`127.0.0.1:8586`). The part that is about this game: the world, the channels, the conversation, the notes about you, when bots speak. |
| The people | `Settings/Scripts/bot_minds.py` with `Settings/Bots` | Who plays which bot, their life and voice as text for Claude, their typing habits applied to every line, asking Claude, cleaning the reply. Not about one game; MapleStoryPlus uses the same file. |
| Claude | Claude Code running Haiku | Writes the reply. Uses your Claude login, no API key. Every reply is a fresh run with no tools and no access to anything on the PC. |

Test without the game:

```
python Settings/Scripts/Bot-Chat-Bridge.py --bot Mirelda --say "hey, how's it going?"
```

## Ideas for later

- Facts looked up from the database for exact answers (where an item drops, where a rare lives), as in MapleStoryPlus.
- Rumours: a bot passing on a hint about something the world is hiding.
- Companions with a richer memory and more to say.
