# Bot chat: talking to bots through Claude

**Status: built 2026-10-11.** The bridge is tested on its own. The game side has not been tried in the game yet.

Talk to a bot and it answers like the person playing that character: someone with an age, a job, a temper and hobbies, who knows the game and remembers you.

## How to use it

1. Once: run `Login-Bot-Chat.bat` and sign in with your Claude account.
2. Start `Start-Bot-Chat.bat` and leave the window open while you play. It shows every exchange.
3. In the game, talk to a bot.

Close the window with Ctrl+C rather than the X: the bots then write down what they remember first. Without the window, bots simply stay quiet.

## Who answers

| Where you talk | Who answers |
|---|---|
| Whisper | The bot you whisper. |
| `/say` | A nearby bot you name. If you name nobody: the bot you have targeted. If you have no target: the closest bot, if it stands within 8 yards. |
| Party chat | A bot you name. If you name nobody: the bot that answered last keeps the talk going for two minutes; otherwise one of them answers half the time. |
| Guild chat | The same as party chat, with a one-in-three chance when nobody is named. |

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

## Who a bot is

Each bot is played by a person from `Settings/Bots/People.json`: a first name, age, gender, home, occupation, home life, temper, humour, hobbies, a way of talking, a typing habit, a gaming habit, something going on in their life, and favourite topics. The person is chosen from the bot's name the first time it speaks, and then written down, so a bot stays the same person. Their mood changes from day to day.

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

- At most three short lines per reply.
- They stay the person they are and never say they are an AI.
- They know nothing about you beyond what you told them, and nothing about this PC. Lines containing an email address, a file path or details of the computer never reach the game.
- They do not promise trades, gifts or meetings they cannot keep.

## How it is built

| Part | File | What it does |
|---|---|---|
| The game side | `Server/modules/mod-wowplus/src/WowPlusBotChat.cpp` | Decides which bot answers, sends the details to the bridge on a helper thread, and makes the bot say the lines that come back, with a typing pause between lines. Settings: `WowPlus.BotChat.*`. |
| The bridge | `Settings/Scripts/Bot-Chat-Bridge.py` | A small web server on this PC only (`127.0.0.1:8586`). Builds the person, keeps the conversation, loads and updates the notes, asks Claude, cleans the reply. |
| Claude | Claude Code running Haiku | Writes the reply. Uses your Claude login, no API key. Every reply is a fresh run with no tools and no access to anything on the PC. |

Test without the game:

```
python Settings/Scripts/Bot-Chat-Bridge.py --bot Mirelda --say "hey, how's it going?"
```

## Ideas for later

- Facts looked up from the database for exact answers (where an item drops, where a rare lives), as in MapleStoryPlus.
- Rumours: a bot passing on a hint about something the world is hiding.
- Companions with a richer memory and more to say.
