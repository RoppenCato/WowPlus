# Voices

Set up on 2026-10-27, from the plan in [Addons-Plan.md](Addons-Plan.md) (step 3). **Not heard in the game yet.**

Quest givers speak their quests aloud, and the story of a zone and of each place in it is read aloud the first time you come there. Three parts:

| What | Voiced by | Where it comes from |
|---|---|---|
| The game's own quests (about 3,000 lines of the old world's quests and gossip) | Spoken's voice packs, made with ElevenLabs | downloaded, installed in the game only |
| Our own 241 quests: offered, asked about and handed in (698 lines, 4.7 hours) | made on this PC with Kokoro, a free voice model; each giver keeps one voice that suits their race and sex | `Settings/Scripts/Make-Voices.py` |
| Zone and place lore (49 zones, 792 places) | Spoken's zone pack, made with ElevenLabs from warcraft.wiki.gg's text (CC BY-SA 4.0) | downloaded, installed in the game only; played by the Guide |

All three play through **Spoken's player**, which Spoken still makes for the 3.3.5 game: one queue so voices never talk over each other, subtitles, a stop button, and the game's music and sounds turned down while someone speaks. Its window and minimap button are Spoken's own (`/spoken` or the minimap button for its settings).

## In the game

- Talk to a quest giver: they say the quest as it opens (also in [WowPlus Dialogue](Dialogue.md)'s panel). Handing in, they say their thanks.
- Come to a zone, or a place in it, for the first time: its story is read. Once per character.
- In the Guide's Lore tab: **Listen** buttons for the zone and for the place you stand in.
- `/guide narrate off` stops the first-time readings (the Listen buttons still work); `/guide narrate on`.
- The 3.3.5 game cannot pause a sound. Stopping one makes Spoken restart the game's sound for a moment, so the music jumps.

## What is installed where

Not in the repository (large, and not ours to give away): in each game copy, `Interface\AddOns\`:

| Folder | What | From |
|---|---|---|
| `SpokenPlayer`, `SpokenQuests` | the player, 3.3.5 version 2.2.2 | `SpokenQuests-WoW_3.3.5-2.2.2.zip` |
| `SpokenQuestsAudioAlliance`, `...Horde`, `...Shared`, `...Gossip` | the game's quests, voiced | `SpokenQuestsAudioAll-2.4.0.zip` |
| `SpokenZonesAudio` | the zone lore, voiced | `SpokenZonesAudio-2.0.1.zip` |

All from [github.com/rusty-key/spoken-wow/releases](https://github.com/rusty-key/spoken-wow/releases), downloaded on 2026-10-27 and kept in `C:\dev\addon-refs\downloads`. The packs say they are for newer games; their `.toc` files were given `30300` in their `## Interface` line so that the 3.3.5 game loads them without "Load out of date AddOns". To set it up on another PC: unzip the three into the game's `Interface\AddOns` and make that same change.

In the repository: `Client/AddOns/SpokenQuestsAudioWowPlus`, our own pack, in the same shape as Spoken's (a `.toc`, `Module.lua`, and the lookups in `generated/`: quest title to quest number, by giver where two of our quests share a title, and each sound's length). Its sounds (`generated/sounds/`, 108 MB) are not committed; `Make-Voices.py` makes them again on any PC.

## Making our voices

```
python Settings/Scripts/Make-Voices.py              voice what is new or changed
python Settings/Scripts/Make-Voices.py --quest 9200237   one quest again
python Settings/Scripts/Make-Voices.py --list       who gets which voice
```

- Needs Python with `kokoro-onnx` and `soundfile` (`pip install kokoro-onnx soundfile`) and Kokoro's model files `kokoro-v1.0.onnx` and `voices-v1.0.bin` from [kokoro-onnx's releases](https://github.com/thewh1teagle/kokoro-onnx/releases) (model-files-v1.0) in `C:\dev\addon-refs\tts` (or the folder in the setting `WOWPLUS_TTS`).
- Only lines that are new or whose text changed are made again (`generated/made.json` remembers); a new quest takes seconds.
- Who sounds like what is in the script: a few of Kokoro's American and British voices per people and sex, and a slower pace for tauren and orcs. The player's name cannot be in a recording, so "$N" is left out where it can be and said as "friend" where it cannot.
- Kokoro is Apache 2.0: what it makes is ours to use.
- After making new voices: restart the server (Deploy copies the pack into the game) and restart the game; a running game does not see new sound files.

## Not voiced yet

- What our own people say when you hear them out (the "says" lines of talk objectives) and anything bots say.
- The newer zones (blood elf and draenei lands, Outland, Northrend): Spoken has no lore for them.
- Books and letters: Spoken has a pack for them, but no 3.3.5 player.

## To try

1. Log in: is there a Spoken minimap button? Does Spoken say in chat that it found its packs (4 of Spoken's and "Quests - WowPlus")?
2. Talk to an old quest giver (Gryan Stoutmantle, Marshal McBride): do they speak the quest?
3. One of our quests (your class trainer at level 10, or Edrin Halloway in Goldshire): do they speak, in a voice that fits?
4. Walk into a place you have not been to: is its story read?
5. Guide, Lore tab: Listen.
