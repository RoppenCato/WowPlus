# The group board: finding people for a dungeon or a raid

A window that lists people who are looking for a group right now, to pick company from. Built on 2026-10-12. **Not yet tried in the game**: the server part compiles and starts, and the notes and lines the bots write were tested, but nobody has clicked through the window yet.

Open it with **Group** on the bot bar, or type `/board`. The addon is `WowPlus Group`; restart the game once after it is first installed.

## What you see

About fourteen people. For each: role (tank, healer, damage), name in the colour of their class, level, and a note in their own words ("tank, got an hour", "healer lf wc or sfk"). A green star marks someone you know (a guild mate or someone on your friends list). Hover for class and guild.

- They are the bots that are really in the world: your side, within three levels of you, alone, not in a dungeon or battleground.
- There are always at least two tanks and two healers. If nobody near your level has the role, one is taken from a little further away, and failing that someone from another level range becomes your level, the way guild mates keep up.
- The list changes. Everyone stays four to ten minutes and then makes room for someone else. The window looks again every twenty seconds.

## Going to

Choose the dungeon or raid at the top, or leave it at "Nowhere in particular". It decides two things: where the people you invite turn up, and how large the group may be.

## Inviting

Press **Invite**. The person joins and says hello in party chat. They do not appear next to you:

| Where you said you are going | What happens |
|---|---|
| A dungeon or raid | After 40 to 110 seconds they stand at its entrance and wait. The first to arrive says so. When you come within 40 yards, or go inside, they follow you. If you are already at the entrance or inside, they come straight to you. |
| Nowhere in particular | After 40 to 110 seconds they walk up to you. |

Someone who is close to you already simply walks over. Now and then (8 in 100) a stranger "has just found a group" when you ask; people you know never do.

## Posting your own group

Tick what you need (tank, healer, damage) and press **Post my group**. Over the next minutes people whisper you to ask for a place, and show at the top of the list in gold with **Accept** and **x**. The post lasts five minutes, or until the group is full; **Take post down** ends it sooner.

## How many

| Going to | Most in the group |
|---|---|
| Nowhere, or a dungeon | 5 |
| A raid | 10 (`WowPlus.Board.RaidCap`; up to 25 or 40 if you want to try) |

The sixth invitation turns the party into a raid by itself. Enemies are scaled to the number of players by the AutoBalance module, so a raid of ten works. The cap is about what goes wrong with many bots at once (pathing, standing in fire, slow pulls), not about the computer: 100 bots use about one processor core.

## Afterwards

- Going into a dungeon with people from the board writes a line in the journal: "Went into Shadowfang Keep with Grumdak, Velisha and Oski."
- When you have been out of the dungeon for five minutes, they say goodbye and leave the group, one after another.
- They remember you like any bot you talked with. Talk with someone more than once and they are on your friends list; invite them to the guild like anyone.
- Your companions (`.bots call`) are not from the board and stay.

## Bots you invite the ordinary way

A bot you invite by name or by clicking it still comes to you at once, as before. That used to be done by the bot module (`Playerbots.SummonWhenGroup`); it is switched off there and done by the board's code instead, so that people from the board can take their time.

## Settings (`WowPlus.Board.*` in `Settings/mod_wowplus.conf`)

| Setting | Default | What it is |
|---|---|---|
| `Enable` | 1 | The board, and bringing invited bots over. |
| `People` | 14 | How many are listed. |
| `LevelRange` | 3 | Levels above and below yours. |
| `Stay.MinMinutes`, `Stay.MaxMinutes` | 4, 10 | How long someone stays on the list. |
| `BusyChance` | 8 | Percent chance that a stranger has just found a group. |
| `RaidCap` | 10 | Most people in a raid made from the board. |
| `Travel.MinSeconds`, `Travel.MaxSeconds` | 40, 110 | How long someone takes to arrive. |
| `LeaveAfterMinutes` | 5 | Minutes outside after a run before they leave. 0: they stay until you remove them. |
| `Post.Minutes` | 5 | How long a post stays up. |
| `Post.MinSeconds`, `Post.MaxSeconds` | 12, 40 | Seconds between two people answering a post. |

## How it is built

| Part | File |
|---|---|
| The board, inviting, arriving, leaving | `Server/modules/mod-wowplus/src/WowPlusBoard.cpp` |
| The window | `Client/AddOns/WowPlusGroup` |
| Notes, applicants' whispers, "at the entrance", goodbyes | `Settings/Scripts/Bot-Chat-Bridge.py` (`board`, `apply_for_group`, `tell`). Without the bridge the notes are plain ("tank lfg") and those lines are not said. |

The window and the server talk by addon whispers with the prefix `WPG`.

## Battlegrounds: not built

A battleground needs an enemy team, and all bots are Horde. The plan was Alliance bots that exist only inside battlegrounds. Looking at the bot module showed what that takes:

- Bots queue for a battleground only if they are in the world, of the right ten-level bracket, and known to the module as "random bots", which is a private list. Bringing in Alliance bots on demand means a change inside the bot module.
- They would have to be given the level of your bracket, kept out of sight while they wait, and logged out afterwards.

It can be done, but not blind: it needs someone in the game to queue while it is being built. Until then, queueing for a battleground will most likely not start a match.

## To try first

1. `/board` opens the window and people are listed with notes.
2. Choose a dungeon, invite three people, go to the entrance: are they there, do they follow you in?
3. Post a group with "Healer" ticked: does someone whisper within a minute, and does Accept work?
4. Leave the dungeon and wait five minutes: do they say goodbye and leave?
5. Invite a bot the ordinary way and press Call for companions: do they still come at once?
