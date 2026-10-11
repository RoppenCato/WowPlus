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

## Decided so far

**2026-10-11: abilities from class quests are taught by the trainer.** Robin: *"Playing solo makes those class quests quite miserable, so I think those abilities need to be gained from the trainer."* The class quests were built for a time when a guild helped you through them. Alone they are chores, and several hold back something the class needs in order to work. So:

- The trainer teaches, at the level the quest was offered, what the quest taught. Built: `Settings/SQL/world/2026-10-11_class-quest-abilities.sql`.

  | Class | Level | From the trainer now |
  |-------|------:|----------------------|
  | Warrior | 10 | Defensive Stance, Sunder Armor, Taunt |
  | Warrior | 30 | Berserker Stance, Intercept |
  | Paladin | 12 | Redemption |
  | Hunter | 10 | Tame Beast and the pet commands; Feed Pet, Revive Pet |
  | Rogue | 20 | Poisons |
  | Shaman | 4, 10, 20 | Stoneskin Totem, Searing Totem, Healing Stream Totem |
  | Warlock | 10, 20, 30 | Voidwalker, Succubus, Felhunter |
  | Warlock | 50, 60 | Inferno, Ritual of Doom |
  | Druid | 10, 14 | Bear Form with Growl and Maul; Cure Poison |

- The quests themselves stay in the game, untouched, for whoever wants the adventure.
- **New milestone abilities also come from the trainer**, not from new quests.
- **A hunter has a pet from level 1** (Robin's example of misery: "not having a pet before 10"). A plain animal from the starting zone, given once, with the spells to call, dismiss, feed and revive it. Tame Beast is still level 10, from the trainer: the day you choose your own. Built in `mod-wowplus`, setting `WowPlus.Classes.HunterStarterPet`.
- **A shaman gets the Totem of the Earthen Ring at level 4.** The totem quests also handed out the four totem items, without which totems cannot be cast, and a trainer cannot give items. This one relic replaces all four. Setting `WowPlus.Classes.ShamanTotemRelic`.

Still to decide: the questions under "To decide" below, apart from number 3, which this answers.

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

## Early levels: what each class lacks most

Asked 2026-10-11: the most lacking aspect at 1-10 and at 10-20 for each class, like the hunter's missing pet, as candidates for easy fixes. **Status: assessment, nothing changed.**

The levels of the abilities are from our server's trainer data. Which gap hurts most is Claude's judgement from that data and from how these classes are generally known to play in this version; it has not been play-tested by us. Death knights start at 55 and are left out.

| Class | Most lacking at 1-10 | Most lacking at 10-20 | Easy fix to consider | How clean |
|-------|----------------------|-----------------------|----------------------|-----------|
| Warrior | **Staying alive between fights.** No self-heal of any kind, so it eats after every second fight. Rage starts empty, so each fight opens with auto-attacks. | **No way to fight more than one enemy** until Cleave at 20. Still no healing. | Bloodrage (10) from level 1 or 4, so fights start with rage. Cleave or Thunder Clap is already at 6, so the real gap is sustain: see "everyone" below. | Clean: Bloodrage has no damage numbers |
| Paladin | **Nothing to press.** A seal, Judgement at 4, then waiting for auto-attacks. It is safe but it is the dullest start in the game. | **Still nothing to press** until Exorcism and Consecration, both at 20. | Exorcism (20) to about 8 and Consecration (20) to about 14, so there is a rotation before 20. | Needs their first rank weakened; tooltip then overstates |
| Hunter | **No pet** (fixed: starter pet from level 1). Also cannot shoot at close range, so anything that reaches it is a melee fight with a weak melee attack. | **Mana.** Aspect of the Viper comes at 20. Otherwise in good shape once it has a pet. | Aspect of the Viper (20) to 10 or 12. | Clean: it works in percentages |
| Rogue | **Staying alive.** No self-heal, Evasion at 8, Sprint at 10. Fine against one enemy, in trouble against two. | **No poisons until 20, and no way at all to hit several enemies.** Fan of Knives, the only one, comes at 80. | Poisons (20) to 10. Fan of Knives (80) to somewhere between 20 and 40: its damage is a percentage of weapon damage, so it scales by itself. | Both clean |
| Priest | **Killing speed and mana.** Smite and one damage-over-time spell, then out of mana and wanding. Without a wand it is slower still. | **The same, until Mind Flay or Holy Fire at 20.** The slowest leveller of all until Shadowform at 40. | A wand in the starting bags, so the first levels are not spent without one. Cheaper damage spells at low level. Shadowfiend (66) much earlier: it is a pet whose strength follows the priest's level, and it gives mana back. | Wand: clean. Costs: the modifier route, to be tried. Shadowfiend: clean. |
| Shaman | **Totems needed the quests** (fixed: trainer and relic). Otherwise a decent start. | **Mana.** Water Shield, the class's mana engine, comes at 20. | Water Shield (20) to 8 or 10. | Nearly clean: its first rank returns a fixed, small amount |
| Mage | **Drinking.** Strong, but out of mana after two or three fights and sitting down. | **Drinking still.** Evocation at 20 helps once per eight minutes. | Shorter Evocation cooldown; or see "everyone" below. | Clean: one row in the cooldown table |
| Warlock | Little. It has a pet from level 1 and a self-heal and mana loop early (Life Tap 6, Drain Life 14). | **Soul shards.** Summoning a demon costs a shard, shards fill the bags, and running out means farming them. | Summoning without a shard, or a shard returned when the demon is dismissed. | Needs code or a spell data change; to look at |
| Druid | **No form at all.** For ten levels it is a weak caster with two damage spells. Bear Form at 10 is the moment (now from the trainer). | **The only form is the bear,** which survives everything and kills slowly. Cat Form, the damage form, comes at 20. Travel Form at 16. | Cat Form (20) to 10, alongside the bear, so that level 10 is "choose your animal". Or leave it: ten levels as a bear is a real part of the class. | Clean: forms have no damage numbers of their own |

### Everyone

Three things are the same for every class at these levels and are probably the largest single source of low-level tedium:

- **Sitting down to eat and drink.** Health and mana come back slowly at low level, so a large share of the first twenty levels is spent seated. `worldserver.conf` has rates for health and mana regeneration; our module could also apply a boost only below a chosen level, so that the higher levels keep their balance.
- **No mount until 20.** This is friction of the good kind for most people, and the hearthstone menu already softens travel. Listed for completeness.
- **Bag space.** With the extra loot dropping, four small bags fill fast. Junk selling helps; bigger starting bags would be an easy fix.

### Which are the cheapest wins

Roughly in order of value for effort:

1. Faster regeneration out of combat at low levels: helps all nine classes at once.
2. Rogue: Poisons at 10, Fan of Knives much earlier.
3. Hunter and shaman: their mana ability at 10 instead of 20.
4. Priest: a starting wand, and Shadowfiend early.
5. Warrior: Bloodrage from the start.
6. Druid: Cat Form at 10.
7. Paladin: Exorcism and Consecration earlier. The most needed of all, and the least clean, because their damage has to be scaled down.

## Per class

### Mage

**Status: proposal on paper, 2026-10-11. Nothing changed in the game.**

#### Today

| Levels | New abilities from the trainer | How it feels |
|--------|--------------------------------|--------------|
| 1-10 | Fireball, Frostbolt, Fire Blast, Arcane Missiles, Polymorph, Frost Nova, conjured food and water | Good. A new button every two levels. |
| 11-20 | Arcane Explosion, Flamestrike, Blizzard, Blink, Evocation, Mana Shield, the first teleports | The best stretch. Blink at 20 changes the class. |
| 21-30 | Scorch, Counterspell, Cone of Cold, mana gems, Ice Armor, Ice Block | Good. Ice Block at 30 is a real milestone. |
| 31-40 | Mage Armor at 34, then only portals | Thin. Level 40 gives a portal. |
| 41-50 | Nothing but two more portals | **Empty.** (Ice Barrier at 46 is only for those with the talent.) |
| 51-60 | Arcane Brilliance at 56, a teleport at 60 | **Empty.** (Dragon's Breath at 56 is a talent again.) |
| 61-70 | Molten Armor 62, Arcane Blast 64, Ice Lance 66, Invisibility 68, Spellsteal 70, Ritual of Refreshment 70 | Crowded: six abilities in nine levels. |
| 71-80 | Frostfire Bolt 75, Mirror Image 80 | Fine. |

From 35 to 61 a mage learns one buff and a handful of portals. That is 26 levels, about a third of the whole journey.

#### The ladder

One defining ability at every tenth level. **Bold** = moved.

| Level | Milestone | What it adds to how you play | Today |
|------:|-----------|------------------------------|-------|
| 10 | Frost Nova | Control: freeze and step back | 10 |
| 20 | Blink (with Evocation) | Movement, and your mana back | 20 |
| 30 | Ice Block | Survival: undo a mistake | 30 |
| 40 | **Invisibility** | Escape, and slipping past what you do not want to fight | 68 |
| 50 | **Spellsteal** | Taking the enemy's magic for yourself; dungeons at this level are full of buffed casters | 70 |
| 60 | **Molten Armor** and **Ice Lance** | A third armour to choose between, and an instant spell that punishes frozen targets | 62, 66 |
| 70 | **Frostfire Bolt** (with Ritual of Refreshment) | A new main nuke | 75 |
| 80 | Mirror Image | The capstone stays the capstone | 80 |

And two smaller moves to break up the empty levels in between:

| Level | Ability | Today |
|------:|---------|-------|
| 46 | **Arcane Brilliance** | 56 |
| 64 | Arcane Blast | 64 (stays) |

After this, the longest stretch without something new is about six levels instead of twenty-six.

#### How clean each move is

| Move | Numbers involved? | Verdict |
|------|-------------------|---------|
| Invisibility 68 to 40 | None | Clean |
| Spellsteal 70 to 50 | None | Clean |
| Arcane Brilliance 56 to 46 | It is the group version of Arcane Intellect; it gives the same amount as the single-target rank you already have | Clean |
| Molten Armor 62 to 60 | A percentage, not a fixed number | Clean |
| Ice Lance 66 to 60 | Fixed damage sized for level 66. Six levels early it is a little strong, and it is a weak spell unless the target is frozen. | Acceptable; watch it |
| Frostfire Bolt 75 to 70 | Fixed damage sized for level 75. Five levels early it would out-damage Fireball and Frostbolt until 75. | Acceptable as a reward, or weaken its first rank (then the tooltip is wrong until we have a client patch) |
| Arcane Blast | Fixed damage sized for 64, and it stacks. At 50 or 60 it would be far stronger than anything else. | **Not moved** |

#### What the ladder cannot fix

- There are only eight late abilities to draw on, and three of them are damage spells that do not move well. So 60 is the weakest rung: a third armour and an instant spell, useful but not class-changing in the way Blink or Invisibility are.
- A mage's talents are where much of the excitement at 40 to 60 really comes from (Ice Barrier, Combustion, Presence of Mind, the Water Elemental at 60 for frost). The ladder does not touch talents.

#### Misery to check, in Robin's sense

Not friction worth keeping, if they bother you:

- **Drinking.** A low-level mage spends a large share of play sitting down. Evocation comes at 20 and has an eight-minute cooldown.
- **Conjuring.** Food and water come in ranks you must go back to a trainer for.
- **Portals as "rewards".** Level 40 and 50 hand you a portal, which a solo player has little use for, and our hearthstone menu now does more.

#### To decide for the mage

1. Is the ladder right? In particular Invisibility at 40 and Spellsteal at 50.
2. Level 60: accept the weaker rung, or put Mirror Image there and find something else for 80?
3. Frostfire Bolt at 70: as it is (strong for five levels), weakened, or left at 75?
4. Is the drinking misery? If so: a shorter Evocation cooldown, or faster mana regeneration out of combat, are both settings-level changes.
