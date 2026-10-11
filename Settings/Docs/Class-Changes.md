# Class changes

Goals and changes per class. **Status: discussion. Nothing here is changed in the game yet.**

Started 2026-10-11 from two ideas of Robin's:

- Look at class abilities that arrive very late, or only through quests, and decide whether to change that.
- Re-pace the key abilities so that **every ten levels feels big and changes how the class plays**.

The technical background (what the server can change alone and what needs a client patch) is in the [plan](Plan.md), section 9.

## The rule: keep the friction, remove the misery

Robin, 2026-10-11: *"A game like WoW is partly fun because of friction. Getting Tame Beast at 10 is fun, but not having a pet before 10 is miserable."*

This is the test every class change has to pass. Friction is what makes a reward feel earned: the quest for Tame Beast, saving for a mount, the walk to a trainer. Misery is being without something the class needs in order to work, while waiting for a level number. The two often sit in the same place, and the job is to separate them:

- **Keep the moment.** The quest, the milestone, the thing you go out and earn stays, and stays at its level.
- **Fix the gap before it.** If the class is broken or dull until that moment, it gets something to play with in the meantime, usually a weaker or simpler version.
- **Do not hand out the reward early.** Giving Tame Beast at level 1 would remove the misery and the moment together. Giving a hunter a plain starter pet at level 1, and leaving Tame Beast at 10 as the day you choose your own, removes only the misery.

Questions to ask of any ability before moving it:

1. Is the wait for it fun (something to look forward to, a quest worth doing), or just empty?
2. Is the class playable and enjoyable without it until then?
3. If we move it, what moment is lost, and is something left in its place?

So the aim is not "everything earlier". It is: no stretch of levels where the class is missing its core, no stretch of twenty levels with nothing new, and the big moments left where they are or made bigger.

## What the game does today

When each class first gets a new ability from its trainer, counted per ten levels. Taken from our server's trainer data on 2026-10-11.

| Class | 1-10 | 11-20 | 21-30 | 31-40 | 41-50 | 51-60 | 61-70 | 71-80 |
|-------|-----:|------:|------:|------:|------:|------:|------:|------:|
| Warrior | 9 | 11 | 6 | 5 | **2** | **1** | 3 | 3 |
| Paladin | 10 | 13 | 9 | 7 | 6 | 5 | 4 | 3 |
| Hunter | 10 | 15 | 9 | 8 | 5 | **3** | 4 | 5 |
| Rogue | 11 | 9 | 6 | **2** | **2** | **0** | 4 | 2 |
| Priest | 9 | 12 | 8 | **3** | **2** | 4 | 6 | 3 |
| Shaman | 10 | 13 | 17 | 7 | **1** | **2** | 7 | 2 |
| Mage | 10 | 19 | 10 | 10 | **3** | **3** | 9 | 5 |
| Warlock | 11 | 9 | 10 | 5 | **3** | **3** | 7 | 3 |
| Druid | 9 | 17 | 11 | 10 | **2** | 4 | 6 | 4 |
| Death Knight | | | | | | 19 | 17 | 5 |

Two things make the real picture worse than the table:

- **Many entries in the 41-60 columns are not new abilities at all.** They are the second rank of a talent (Mortal Strike at 48, Devastate at 60, Holy Shock, Mutilate, Vampiric Touch, Ice Barrier, Mangle and so on). A character without that talent gets nothing there.
- **The mage's numbers are mostly teleports and portals.** Eight of the nineteen at 11-20, six of the ten at 31-40.

**The pattern is the same for every class: almost everything arrives before level 40, levels 41 to 60 are a desert, and the next real batch comes at 61 to 70.** A rogue learns no new trainer ability at all between 51 and 60, a warrior one, a shaman one between 41 and 50. Those twenty levels are exactly where "every ten levels feels big" fails today.

## Key abilities that arrive late

Abilities that define how a class plays, and the level they come at. These are the candidates for moving earlier, into the desert.

| Class | Ability and level today |
|-------|-------------------------|
| Warrior | Spell Reflection 64, Commanding Shout 68, Intervene 70, Shattering Throw 71, Enraged Regeneration 75, Heroic Throw 80 |
| Paladin | Crusader Aura 62, Seal of Vengeance / Corruption 64 / 66, Avenging Wrath 70, Divine Plea 71, Shield of Righteousness 75, Sacred Shield 80 |
| Hunter | Steady Shot 50, Deterrence 60, Kill Command 66, Snake Trap 68, Misdirection 70, Kill Shot 71, Master's Call 75, Freezing Arrow 80 |
| Rogue | Envenom 62, Deadly Throw 64, Cloak of Shadows 66, Shiv 70, Tricks of the Trade 75, Fan of Knives 80 |
| Priest | Shadow Word: Death 62, Binding Heal 64, Shadowfiend 66, Prayer of Mending 68, Mass Dispel 70, Mind Sear 75, the Hymns 80 |
| Shaman | Wrath of Air Totem 64, Earth Elemental 66, Fire Elemental 68, Bloodlust / Heroism 70, Lava Burst 75, Hex 80 |
| Mage | Molten Armor 62, Arcane Blast 64, Ice Lance 66, Invisibility 68, Spellsteal 70, Frostfire Bolt 75, Mirror Image 80 |
| Warlock | Fel Armor 62, Incinerate 64, Soulshatter 66, Ritual of Souls 68, Seed of Corruption 70, Shadowflame 75, Demonic Circle 80 |
| Druid | Flight Form 60, Maim 62, Lifebloom 64, Lacerate 66, Cyclone 70, Swipe (Cat) 71, Savage Roar 75, Nourish 80 |
| Death Knight | Starts at 55 with a dense kit; Army of the Dead 80 |

## Abilities that come from quests

Our quest data has 129 class quests that teach something. Leaving out duplicates per race, and the old priest racial quests that no longer give anything, these are the ones that matter:

| Class | Level | Quest gives | Note |
|-------|------:|-------------|------|
| Warrior | 10 | Defensive Stance | A core stance behind a quest chain |
| Warrior | 30 | Berserker Stance | The same; a long chain ending on an island |
| Paladin | 12 | Redemption (resurrect) | |
| Paladin | 20 | Sense Undead | |
| Hunter | 10 | Tame Beast, and pet training | The class is not a hunter before this |
| Rogue | 20 | Poisons | A core part of the class |
| Shaman | 4, 10, 20, 30 | Earth, fire, water and air totems | The four totem quests |
| Warlock | 1, 10, 20, 30 | Imp, Voidwalker, Succubus, Felhunter | Each demon is a quest |
| Warlock | 50, 60 | Inferno, Ritual of Doom | |
| Druid | 10 | Bear Form | |
| Druid | 14, 16 | Cure Poison, Aquatic Form | |
| Druid | 70 | Swift Flight Form | A long chain with a dungeon boss |
| Paladin, Warlock | 40, 60 | Class mounts | The trainer sells them too in this version; the quests are optional |
| Death Knight | 55 | Runeforging, the mount, Death Gate | The starting zone; could be skipped by a module |

## How it could work

**A milestone every ten levels.** At 10, 20, 30 and so on, each class gets one ability that changes how it plays, chosen to fit the class's story of growing up: a stance, a form, a pet, a signature cooldown, a new kind of spell. Smaller abilities fill the levels between. The desert from 41 to 60 is filled first, mostly by pulling abilities down from 61 to 80.

**What can be moved cleanly.** The level at which a trainer teaches an ability is server data, and the trainer window shows what the server says, so this needs no client patch.

- **Abilities without numbers move freely:** Spell Reflection, Cloak of Shadows, Invisibility, Spellsteal, Misdirection, Bloodlust, Hex, Cyclone, Mass Dispel, Intervene and the like. They do the same thing at any level.
- **Abilities with numbers need care:** Lava Burst, Incinerate, Kill Shot, Arcane Blast. Their first rank is sized for level 62 to 80, so given at 45 they would hit far too hard. They need a weaker rank made for the earlier level, which is a number change, and the tooltip then shows the old numbers until we have a client patch.

**Quests.** The idea from the plan: the ability arrives at its level without a chore (from the trainer, or learned automatically), and the quest chain stays as an optional adventure with a better reward. Tame Beast, the stances, the totems, the demons and the forms are the obvious ones.

**Bots.** Bots learn from trainers by themselves, so they pick up the new levels. They already know how to use every one of these abilities, since they use them at the original levels today.

## To decide

1. Is the milestone idea right: one defining ability per ten levels, per class?
2. Fill the 41-60 desert by pulling abilities down from 61-80, and accept that 61-80 then has fewer new things? Or also spread out the crowded 11-30?
3. Quest abilities: given at their level without the quest, with the quest kept as an optional adventure?
4. Learned automatically on level-up, or still bought from a trainer?
5. Start with the abilities that have no numbers (clean, no wrong tooltips), and leave the damage abilities for when we have a client patch?
6. One class first as a trial, and which?

## Per class

*To be filled in, one class at a time, once the questions above are settled: the milestone at each ten levels, what moves where, and why.*
