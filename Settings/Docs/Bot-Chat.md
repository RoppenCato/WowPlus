# Bot chat: talking to bots through Claude

**Status: design only. Nothing here is built yet.**

The goal is the same as in MapleStoryPlus: whisper a bot and get an answer that reads like a real player, who has a personality and knows the game. This document describes how the MapleStoryPlus version works, what carries over, what is different in WoW, and the order to build it in.

## How it works in MapleStoryPlus

Three parts, all on this PC:

| Part | File in MapleStoryPlus | What it does |
|------|------------------------|--------------|
| The hook in the game server | `Server/.../soloMapling/BotChat/BotChatBridge.java`, called from `WhisperHandler.java` | When a player whispers a bot, sends the bot's name, level, job and map, the player's name and map, and the text to the bridge. Whispers the reply lines back with a short "typing" pause. If the bridge is not running, the bot stays silent as before. |
| The bridge | `Settings/Scripts/Bot-Chat-Bridge.py`, started with `Start-Bot-Chat.bat` | A small web server on `127.0.0.1:8585` in its own window. Builds the bot's personality, keeps the conversation, looks up game facts, asks Claude for the reply, cleans it, and sends it back. The window shows every exchange. |
| Claude | Claude Code running Haiku, signed in once with `Login-Bot-Chat.bat` | Writes the reply. Uses the Claude subscription login, so there is no API key. Each reply is a fresh run with no tools, no memory and no access to anything on the PC. |

What made it work well, and should be kept:

- **A personality from the name.** Age, job, mood, humour, hobby and way of talking are picked using the bot's name as the seed, so the same bot is the same person every time without anything being saved.
- **Facts are looked up, not guessed.** The bridge finds the items and monsters named in the message in the reference lists and hands only those facts to Claude as a `[Game knowledge]` note. Without a note, the bot is told to say it is not sure.
- **Small talk gets no lookup.** "Do you like cats?" is not a question about the monster Cat.
- **Follow-ups work.** "Where is that?" reuses what the conversation was last about.
- **Directions start from where the player stands**, using a route search over the map links.
- **Strict output rules.** The reply must come inside `<msg>` tags, at most three short lines. Anything that looks like a comment about the task is dropped.
- **Privacy filter.** Lines containing an email address, a file path or details of the PC never reach the game.
- **Stays in character.** Never says it is an AI; reacts to odd requests like a confused player would.
- **Limits.** Three replies at a time, a conversation is forgotten after 10 minutes of silence, the last 12 lines are kept.
- **A test mode without the game:** `--say "..."`.

## What is different in WoW

| Topic | MapleStoryPlus | WowPlus |
|-------|----------------|---------|
| Server language | Java, one small class | C++, inside `mod-playerbots`. Needs a rebuild to change. |
| What a whisper to a bot means | Nothing, until the bot chat used it | **Already a command.** `follow`, `stay`, `stats`, `attack` and many more are whispered to bots. Chat must not break that. |
| Calling the bridge | A virtual thread per whisper | Bot code runs on the map threads, where waiting is not allowed: one slow call would freeze every bot on that map. The call must happen on a separate worker thread, and the reply must be delivered on a later tick. |
| Game knowledge | Reference lists (`.tsv` files) we generated | The `acore_world` database already holds it all: which creatures drop an item, where creatures spawn, vendors, quests. The bridge can ask MySQL directly. |
| Where the bot is | A map id, turned into a name by a list | The server knows the zone and area name itself and can send the name. |
| Directions | Route search over portals | WoW is an open world. "Which zone, near which town, on which continent" is usually enough, plus the nearest flight master. No route search needed at first. |
| Who the bot is | Level and job | Race, class, talent spec, level, faction, guild, and what it is doing (questing, in a dungeon group, in a battleground). Much more to build a character from. |
| Chat line length | 70 characters per line | Up to 255, so replies can be one or two normal sentences. |
| Other chat | Whispers only | Say, party, guild and the general channels all exist and bots are in them. Party and guild chat are natural later steps. |
| Bridge port | 8585 | **8586**, so both games can run at the same time. |

## Where to hook in

The place already exists. In `Server/modules/mod-playerbots/src/Bot/PlayerbotAI.cpp`:

- `PlayerbotAI::HandleCommand(...)` receives every chat line directed at a bot and queues it as a command.
- `PlayerbotAI::HandleCommands()` runs the queue. When a whisper is **not a known command**, nothing happens today: the "Unknown command" reply there is commented out.

That gap is the hook: **a whisper that is not a command is chat**, and goes to the bridge. Commands keep working exactly as before.

- The reply is sent with the existing `PlayerbotAI::Whisper(text, playerName)`.
- A random bot does not accept everything from a stranger: there is a security check at the top of `HandleCommand`. Chat needs to be let through that check without also letting strangers command the bot. This has to be checked in the code before building.

Rules from the module's own guidelines (`Server/modules/mod-playerbots/AGENTS.md`) that apply:

- **Off by default.** A setting in `playerbots.conf.dist`, for example `Playerbots.BotChat.Enabled = 0` and `Playerbots.BotChat.Url`, which we turn on in `Settings/playerbots.conf`.
- **No waiting on map threads.** See the table above.
- **Cheap when unused.** If the setting is off or the bridge is not running, the cost must be nothing and the bot stays silent.

## What the server sends, and what comes back

Same shape as in MapleStoryPlus: a small form post to `http://127.0.0.1:8586/chat`, plain text lines back, one chat line per row.

| Field | Example |
|-------|---------|
| `player`, `player_level`, `player_class`, `player_zone` | `Robin`, `23`, `Paladin`, `Redridge Mountains` |
| `bot`, `level`, `race`, `class`, `spec` | `Thalwen`, `24`, `Night Elf`, `Hunter`, `Marksmanship` |
| `zone`, `area` | `Duskwood`, `Darkshire` |
| `guild` | `Knights of Elwynn` or empty |
| `doing` | `questing`, `in a party with the player`, `in a dungeon`, `in a battleground`, `in a city` |
| `channel` | `whisper` now; `party`, `guild`, `say` later |
| `text` | What the player wrote, cut to 255 characters |

## The bridge

Start from a copy of `Bot-Chat-Bridge.py` and change the game-specific parts. It would live in `Settings/Scripts/Bot-Chat-Bridge.py`, with `Start-Bot-Chat.bat` and `Login-Bot-Chat.bat` in the main folder, as in MapleStoryPlus.

Carries over unchanged: finding and calling Claude Code, the conversation memory and its limits, the `<msg>` tags and the cleaning, the privacy filter, the log window, the `--say` test mode.

Changes:

- **Persona.** Keep the real-life side (age, job, mood, humour, hobby, way of talking) and add a game side from what the server sends: how long they have played, whether they prefer questing, dungeons or battlegrounds, what they think of the other faction. Race and class should colour it, not turn it into roleplay: these are *players*, not their characters.
- **The prompt.** WoW words instead of MapleStory words: gold, mats, mobs, grinding, quest hub, instance, tank, healer, dps, aggro, pull, wipe, LFG, AH, hearthstone, flight path, rep, talents, spec. The game is Wrath of the Lich King, around 2009 to 2010; the bot knows nothing later.
- **Knowledge.** Replace the `.tsv` lookups with database queries, run by the bridge against `acore_world` on port 3307 with a read-only MySQL user made for it:

  | Question | Tables |
  |----------|--------|
  | Which creature drops this item? | `item_template`, `creature_loot_template`, `reference_loot_template`, `creature_template` |
  | Where does this creature live? | `creature` (spawn positions and map), turned into a zone name |
  | Who sells this? | `npc_vendor`, `creature_template` |
  | Which quest gives this, where does it start? | `quest_template`, `creature_queststarter` |
  | Which dungeon is for my level? | A small hand-written list, like `Places.tsv` in MapleStoryPlus |

  The names are loaded once when the bridge starts, so finding what a message is about stays as quick as it is now. Only the details are fetched per question.
- **Directions.** Zone, nearest town, continent. Later perhaps the nearest flight master from the player's zone.

## Build order

Small steps, each one usable on its own:

1. **Bridge first, without the game.** Copy the script, change port, persona and prompt, leave knowledge out. Test with `--say`. No server change needed.
2. **The hook.** The setting, the worker thread, the call in `HandleCommands()` for whispers that are not commands, the reply as whispers with a typing pause. Rebuild. Now any bot can be whispered.
3. **Knowledge from the database.** Drops and where creatures live first, then vendors and quests.
4. **Party chat.** A bot in your party answers when you say its name. Needs a rule for which bot answers when several could.
5. **Saved bots and history.** The original design from MapleStoryPlus: a small file per bot with backstory and a summary of earlier talks. Easier here than in MapleStory, because WoW's random bots are real saved characters that keep their name between restarts.
6. **Bots that do what they say.** In WoW a bot *can* join your party, follow you or trade, because those commands exist. Claude could answer "sure, invite me" and the bridge could return a command for the bot to run. This is the big step beyond the MapleStory version and needs care: only a short list of harmless commands, and only for the player it is talking to.

## Things to settle before building

- **Which bots answer.** All of them, or only bots in your party, guild or nearby? All is simplest and is what the MapleStory prototype does.
- **Greetings and command-like words.** "hi" or "help" may already be commands. Decide whether a known command always wins (simple, predictable) or whether a short list of words goes to chat instead.
- **Usage.** Every reply is a Claude Code run on the subscription. With one player this is small. If friends join, a cap per player per hour is worth having.
- **Reply time.** 2 to 4 seconds in MapleStory. Fine for whispers, slow for party chat in a fight, so bots should not chat in combat.
- **Language.** The MapleStory prompt asks for English. Decide whether bots should answer in the language they are written to in.

## An existing alternative

There is a community module, [mod-ollama-chat](https://github.com/DustinHendrickson/mod-ollama-chat), that makes playerbots talk through a language model running locally with Ollama. It covers more chat types out of the box, including bots talking among themselves. Reasons to still build our own: it uses Claude through the subscription we already have, the personality and knowledge design from MapleStoryPlus is known to work, and one bridge design serves both games. It is worth reading its code before step 2, to see how it hooks into chat and stays off the map threads.
