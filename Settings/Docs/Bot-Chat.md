# Bot chat: talking to bots through Claude

**Status: first version built 2026-10-11.** The bridge is tested on its own. The game side has not been tried in the game yet.

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

## Who a bot is

Each bot's person is made up from its name: age, where they live, their work, temper, humour, two hobbies, way of talking, why they play, and something going on in their life right now. The same name always gives the same person, so nothing has to be stored for it. Their mood changes from day to day.

The lists these are picked from are at the top of `Settings/Scripts/Bot-Chat-Bridge.py`. Add to them freely; note that changing a list changes who existing bots are.

The bot also knows its own character (level, race, class, where it is), yours, and whether you are guild mates or in a party together.

## What a bot remembers

After a stretch of talk (eight lines, or four minutes of silence) the bot's notes about you are brought up to date: at most eight short lines such as "He plays a mage", "We joked about murlocs", "Wants to run Deadmines this week". Next time those notes are part of what the bot knows.

- Kept in `Database/BotChat/<your character>/<bot>.json`, a few hundred bytes per bot. Plain text; you can read or edit them.
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

- Bots that start a conversation themselves (a guild mate greeting you when you log in, a party member commenting on a boss kill).
- Facts looked up from the database for exact answers (where an item drops, where a rare lives), as in MapleStoryPlus.
- Rumours: a bot passing on a hint about something the world is hiding.
- Companions with a richer memory and more to say.
