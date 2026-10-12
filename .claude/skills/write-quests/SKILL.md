---
name: write-quests
description: Write new lore-based quests for a zone of the WowPlus server (World of Warcraft 3.3.5a). Use when Robin asks for quests, a quest chain, or "something to do" in a named zone or for a level range. Researches the zone's lore, checks what its existing quests already tell, picks an untold story, and writes 2 to 3 quests as files in Settings/Quests that go into the game.
---

# Writing quests for a zone

The aim: quests that feel like they were always in the game, and that leave the player knowing something true about the world (a people, a place, an old event) that the zone's own quests never told them.

Everything is done from the WowPlus folder (`C:\dev\WowPlus`). A quest is one JSON file in `Settings/Quests`; the format is in `Settings/Quests/README.md`. Read that file first if the format is not fresh in mind.

## Before starting

Settle these from the request, and ask only if they cannot be guessed:

- **Zone.** If only a level was given: `python Settings/Scripts/Quest-Tool.py --zones <level>`. If no zone was given at all, read the findings at the top of `Settings/Quests/Zone-Audit.md` and propose the zone that needs quests most for the side being played; its zone entry also lists enemies and places no quest uses, which are the first things to build a story on.
- **Side.** Horde, Alliance or both. Default: the side of the character Robin plays (the health check `Test-Bots.py` prints who played last), and "both" only for neutral towns.
- **Shape.** A chain of 2 to 3 quests, or 2 to 3 separate ones. Default: a chain when the story has a beginning and an end; separate quests when it is a place with several small truths.

## What the quests are for (Robin, 2026-10-12)

- **Something on the way.** Quests along the roads between zones, so that travelling has something in it.
- **The peoples and places the game says little about.** The kobolds of Elwynn, the quilboar of the Barrens: the wikis know far more about them than the game's own quests tell.
- **Fresh things to do and see.** A quest the player has not done before is the point.
- **Variation.** Not only "kill ten of these". Mix the kinds of task within a chain and across a zone.
- Old world only for now (no Outland, no Northrend), from the lowest levels upward.

## The steps

### 1. Look up the zone in the game

```
python Settings/Scripts/Quest-Tool.py --zone "<zone>" --side <side>
python Settings/Scripts/Quest-Tool.py --quests "<zone>" --side <side>
```

The first lists the people who could give a quest (number, map position, title) and the enemies (number, where they are on the map, level, how many). The second lists every quest the zone already has, with its giver and its task. Keep both outputs at hand; every later choice is made from them.

### 2. Find the zone's lore

Search the web for the zone on the Warcraft wikis (wowwiki-archive.fandom.com, warcraft.wiki.gg, wowpedia) and read the zone's own page plus the pages of two or three things in it: its peoples, its ruins, its named figures, its history before the game begins.

- **The game is patch 3.3.5.** Use the world as it was before the Cataclysm. Wiki pages mix eras; ignore anything marked Cataclysm or later (the Barrens are not split, Thrall leads the Horde, Magni rules Ironforge).
- Take facts, not sentences. Notes in your own words; never copy wiki text into a quest.

### 3. Note story options

Write down five to eight candidate stories, one or two lines each, each with: what the player would learn, who could plausibly tell it, and which creatures or items in the zone it could use. A candidate that needs something the zone does not have (step 1) is not a candidate.

### 4. Remove what is already told

Go through the existing quests (step 1) and strike every candidate whose story the zone already tells, or whose task is the same enemies for the same reason. Also read `Settings/Quests/Stories.md`, the record of stories our own quests have used, and strike those.

A different angle on a known subject is fine (the quilboar from the quilboar's side); the same angle is not.

**Overlap is welcome when it adds lore** (Robin, 2026-10-12). A quest may send the player to the same enemies or the same place as an existing quest if it tells something the existing one does not: the two can then be done together, which is pleasant. What is struck is retelling, not sharing ground. Even so, places and enemies that no quest uses (the zone's entry in `Settings/Quests/Zone-Audit.md` lists them) come first when the story allows.

### 5. Choose one

Pick the story that is most interesting and teaches the most, in this order of weight:

1. It teaches something true and specific about a race, a place or an event.
2. It can be told through what the player does, not only through what they are told.
3. Its giver has a reason to care, and stands near where it happens.
4. It gives the player a reason to go somewhere in the zone they might not have gone.

Say which story was chosen and, in a line each, which were close and why they lost. Then go on without waiting, unless Robin asked to choose. Stories that lost but were good go into the "Considered and not used" table of `Settings/Quests/Stories.md`, for the next time the zone comes up.

### 6. Choose the people

From the list in step 1, pick who starts and who ends each quest.

- Someone whose race, title or place fits the story. A tauren elder for tauren history, not the nearest vendor.
- Prefer people who give few or no quests already; they are the ones the game left silent.
- Check the map positions: the giver should be within reach of the task. Positions are in the same 0 to 100 numbers players use.
- Guards, trainers and innkeepers are a last resort. Never a creature that only appears during a holiday.

### 7. Write the quests

Two or three files. For a chain, each later quest has `"after": <id of the one before>`.

**Tasks.** Only what the tool can make: kill, collect, fetch, deliver, slay (see the README). Shape the chain so the tasks differ: for example a delivery that sets the scene, a collection that shows the problem, a named enemy that ends it. Use `drops_from` to make an existing item a quest drop when the story needs an object.

**Numbers.**
- Level: within the zone's range and near the level of the enemies used. Later quests in a chain may be one level higher.
- `min_level`: three below the quest's level.
- Counts: 6 to 12 kills; 4 to 8 collected items with a drop chance of 35 to 60. Never a number that means more than about fifteen minutes.
- Look at how many of the enemy the zone has (the lookup says "N of them"). Ask for at most a third of them. **With fewer than about fifteen, add more**: `python Settings/Scripts/Quest-Tool.py --spots <creature> <count>` prints places near where they live, to paste into the quest file as `"spawns"`. Robin found six centaur for a quest that asked for four "too bothersome"; a quest's enemies should be there when the player arrives. Added enemies must not crowd the place: the picker and the check keep each one 18 yards from any other creature, allow at most 6 creatures within 40 yards, and keep 60 yards from anyone players talk to (README, "Adding enemies without crowding"). Add the fewest that make the quest comfortable, and prefer an enemy the zone already has plenty of over adding to a rare one.
- Money and gear: `python Settings/Scripts/Quest-Tool.py --rewards <level>`. Usual money, experience difficulty 5, and 6 for the last quest of a chain. A choice of three or four items of different armour types (and a weapon) on the last quest; a few healing potions or nothing on the others. Nothing that only some classes can use is "always given".

**Words.** In the voice of the speaker's people, as the game writes them.
- `details`: 50 to 90 words. The giver speaks to the player; the reason comes before the request. One true piece of lore, said the way that person would say it, not explained like a book.
- `objectives`: one sentence, exact: what, how many, and whom to return to, with the place.
- `progress`: one sentence. `completion`: 25 to 50 words, and on the last quest it should land what the player has learned.
- `$N` for the player's name at most once per text. An empty line between paragraphs.
- No modern words, no winks at the player, no "as you know". Names of places and peoples exactly as the game spells them.

### 8. Check and put into the game

```
python Settings/Scripts/Make-Quests.py --check
python Settings/Scripts/Make-Quests.py --reload
```

Fix whatever the check names. Then:

- Add a line per story to `Settings/Quests/Stories.md` (zone, what it tells, the quest numbers).
- Add the quests to the table at the end of `Settings/Quests/README.md`.
- Add them to the "New quests" area of `docs/changes.js` and bump `updated`.
- Run `python Settings/Scripts/Make-Zone-Audit.py` so that the survey counts the new quests.
- Commit and push.

### 9. Report

Tell Robin, briefly: the story and what it teaches, each quest in a line (giver, task, where), where to pick it up on the map, and that it has not been played. Mention anything in it that is your own invention rather than established lore.

## Rules that hold throughout

- **Nothing invented that the client cannot show.** Only creatures, people and items that exist. No new names for things that must be clicked or looted.
- **Do not contradict the game.** If an existing quest says a thing happened one way, it happened that way.
- **Small inventions are allowed and must be marked.** A private motive or a family tie for a minor person is fine; a new fact about a major figure is not.
- **One story, told properly,** beats three thin ones. If the zone has no untold story worth telling, say so and offer the nearest zone that has.
