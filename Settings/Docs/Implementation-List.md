# Implementation list

What we do and in what order, ticked off as we go. The reasoning behind each item is in the [Plan](Plan.md); this list is only the order and the state.

**How we work through it:** one phase at a time. Before a phase starts, Claude asks the open questions for it, we discuss, and the answers are written into the "Decided" line of that phase. Then it is built, tested in the game, and ticked off. Each phase ends with playing it for a while, because these changes affect how the others feel.

| Mark | Means |
|------|-------|
| `[ ]` | Not started |
| `[~]` | In progress, or built but not yet tested in the game |
| `[x]` | Done and tested in the game |

## Phase 0: Foundations

- [x] 0.1 Server built, database created, client set up, bots online
- [x] 0.2 The plan and this list written
- [~] 0.3 First modules added: all six compile and load; each still to be tried in the game (see the table below)
- [~] 0.4 Our own module `mod-wowplus` created (`Server/modules/mod-wowplus/`), holding the loot rule and the hearthstone change
- [ ] 0.5 Reference lists generated into `Settings/Reference/`: items by level and rarity, creatures by level and rank, chests, dungeons and their bosses
- [x] 0.6 Decided 2026-10-10: no Lua engine for now. Everything goes in our own module: one place and one way of doing things. To revisit if we want a community script that needs it.

### Modules added so far

| Module | For | Phase | State |
|--------|-----|-------|-------|
| [mod-solo-lfg](https://github.com/azerothcore/mod-solo-lfg) | Use the dungeon finder alone | 1 | `[~]` |
| [mod-dungeon-clear](https://github.com/jrad7/mod-dungeon-clear) | A tank bot leads a whole dungeon run | 1 | `[~]` |
| [mod-player-bot-level-brackets](https://github.com/DustinHendrickson/mod-player-bot-level-brackets) | Bots spread evenly over the levels, so there is always company at yours | 1 | `[~]` |
| [mod-autobalance](https://github.com/azerothcore/mod-autobalance) | Enemies in dungeons and raids get weaker when fewer players are inside | 1 | `[~]` |
| [mod-aoe-loot](https://github.com/azerothcore/mod-aoe-loot) | Loot all nearby corpses at once | 2 | `[~]` |
| [mod-junk-to-gold](https://github.com/noisiver/mod-junk-to-gold) | Grey items are sold automatically; keeps bags and bot bags clear | 2 | `[~]` |

Waiting for a decision before being added: random bonuses (`mod-random-enchants`), `mod-ale`.

Changes we made to a module's own code, to remember when it is updated:

| Module | File | Change | Why |
|--------|------|--------|-----|
| mod-player-bot-level-brackets | `src/mod-player-bot-level-brackets.cpp` | `equipAndSpecPersistence` renamed to `EquipAndSpecPersistence` | Our newer Playerbots renamed that setting; the module did not compile. |

## Phase 1: Solo access to dungeons and raids

Decided 2026-10-10:

- **Raids with a bot group, 5-player dungeons alone** (bots optional there).
- **Enemies get weaker** when the group is small: mod-autobalance, not mod-solocraft. Bots count as players, so a full bot group gets no help.
- **All attunements removed.** Level requirements stay.
- **Lockouts stay as in the game for now**; to revisit when the loot system exists.

- [~] 1.1 Enter raids without a raid group, raid quests alone (settings `Instance.IgnoreRaid`, `Quests.IgnoreRaid`)
- [~] 1.2 Attunements: all 35 entry requirements removed (`Settings/SQL/world/2026-10-10_remove-attunements.sql`)
- [ ] 1.3 Dungeon finder alone (mod-solo-lfg) tested
- [ ] 1.4 A dungeon with a bot group: invite, `addclass`, and mod-dungeon-clear tested
- [ ] 1.5 mod-autobalance tuned: one dungeon alone and one with bots; decide how easy it should be
- [x] 1.6 Lockouts: left as in the game for now
- [ ] 1.7 One raid tried with a bot group; list the bosses that do not work

## Phase 2: Loot around my level

Decided 2026-10-10:

- **Built as a rule in our own module** (`mod-wowplus`), not as generated database lists. Every number below is a setting.
- **Follows my level, but nothing from grey enemies.** Enemies that give no XP give no extra drops.
- **Everything is in the pool:** world and dungeon items, raid boss items, class tier sets, crafted items and legendaries (those at a tiny chance). Still out: quest items, test and unused items, PvP sets.
- **Leaning towards what my class can use**, not guaranteed. Anything can still drop.
- **Weapons and armour never bind**, so finds can be worn and later sent to an alt or given to a bot.
- **Generosity while levelling: a find every few minutes.** About one extra item per 15 to 20 normal kills, mostly green; a blue about once an hour; an epic a few times per session. Elites, rares and bosses much more.
- **Look-ahead: rising chance over the last 10 levels** before 60, 70 and 80. Tiny at 50, real by 58, first-raid epics possible near the end.
- **At max level the pool follows my own gear:** banded by item level, moving up as what I wear improves, with a small chance to jump ahead.

- **Rares always drop an extra item** (2026-10-11; first set to 60 percent). At least green like every extra drop, with better odds at blue and epic than an ordinary enemy: about 1 in 5 blue or better, 1 in 17 epic or better. Setting `WowPlus.Loot.Chance.Rare` in `Settings/mod_wowplus.conf`.
- **Rares are a real fight, and always possible alone** (2026-10-11). Plain rares: twice the health and a quarter more damage than before (they were barely stronger than ordinary enemies). Rare elites, built for groups, are brought down to about 2.5 times the health and 1.6 times the damage of an ordinary enemy. The frames stay: silver dragon for rares, silver with wings for rare elites. Settings `Rate.Creature.Elite.RARE.*` and `RAREELITE.*` in `Settings/worldserver.conf`.
- **Bots pass on the extra drops.** They are always mine; since nothing binds, I hand bots what I do not want.
- **Mounts get a loot table of their own, with its own chance.** Any kill that can give an extra drop also has a separate, small chance at a mount, so the mounts that are normally almost impossible to get are always possible.

- **All mounts are in the mount table**, not only the rare drops: vendor, reputation, profession and achievement mounts too.
- **The mount table follows my level**, like the gear: a mount drops only once I am high enough to ride it.
- **One flat chance** for the mount roll; no mount is rarer than another.
- **Companion pets get the same treatment:** a table of their own, with its own flat chance.
- **In a group the extra drops go straight into my bags**, with the normal "You receive loot" message, instead of onto the corpse. That is how bots pass: there is nothing for them to roll on. Alone, the extra drops are on the corpse or in the chest like any other loot. (Claude's design choice, 2026-10-10; say so if you would rather see them on the corpse in a group too.)

- [~] 2.1 The item lists: which items are in, which are out, banded by level and, above 50, by item level
- [~] 2.2 The extra drop roll in `mod-wowplus`, behind settings: chance by rarity and by enemy rank
- [~] 2.3 The 50 to 60, 60 to 70 and 70 to 80 look-ahead rules
- [~] 2.4 Weighting towards what the class can use
- [~] 2.5 Chests use the same lists, with their own chance
- [~] 2.5b The mount and pet tables: its own list and its own chance, rolled separately from gear
- [~] 2.6 Bots pass on the extra drops (in a group they go straight to my bags); bot gear settings so bots keep up
- [ ] 2.7 Bag pressure checked: AoE loot and junk selling tuned
- [~] 2.8 Weapons and armour never bind (`Settings/SQL/world/2026-10-10_equipment-never-binds.sql`, applied at the next server start)

## Phase 3: Items that can be special

Decided 2026-10-10:

- **Random bonuses on drops, crafted items and quest rewards.** About 1 item in 3 gets a bonus.
- **Rebalanced the same day, after a green ring came out with three strong bonuses.** mod-random-enchants picks the strength from the item's colour alone and lets any item roll three. Replaced by our own rule in `mod-wowplus` (the module stays installed for its list of bonuses, switched off): the strength follows the **item's level**, the colour shifts it by a tier, and the colour limits the number: green 1, blue 2, epic and legendary 3. Items that already rolled bonuses keep them.
- **Upgrading and reforging: decide after playing with the loot.**

- [~] 3.1 Random bonuses on dropped, crafted and quest items (`mod-wowplus`, settings `WowPlus.Bonus.*`)
- [ ] 3.2 Upgrading or reforging for gold
- [ ] 3.3 Experiment: one generated item copy in another rarity, to see what the client does with it

## Phase 4: Gold and vendors

Decided 2026-10-10:

- **Gold level: comfortable.** Roughly three times the game's normal income. Mounts, training and repairs are never a worry; the best things on the surprise vendors are saved up for over a few sessions.
- **The surprise vendors sell:** random blue gear for my level, recipes and patterns that normally drop in dungeons and raids or need reputation, a mount or pet or two at a high price, and crafting materials and consumables.

- [ ] 4.1 Write down the target: gold per hour by level, and the price of the best things
- [ ] 4.2 Gold sources tuned to the target
- [~] 4.3 Surprise stock on the game's own vendors, one of each, for the zone's level, changing weekly (`mod-wowplus`, settings `WowPlus.Vendors.*`). Armour and weapon vendors: usually a green, sometimes a blue. Other speciality vendors: now and then a potion, elixir or recipe. Very rarely a mount or pet. Not inns, food sellers, stables or starting areas.
- [x] 4.4 Rotating stock: done as part of 4.3, by the week instead of the weekday
- [ ] 4.5 Random daily stock from the item lists
- [~] 4.6 Wandering merchants: the game's own route-walking vendors get a theme from the world seed (beasts, mounts, potions, armour, weapons, recipes, oddities) and a stock of it. New wanderers on new roads: not yet.
- [ ] 4.7 More chests in the world, chest tiers, locked chests with keys

## Phase 5: Professions

Decided 2026-10-10:

- **Crafting is the reliable way, with a guaranteed bonus.** Drops are luck; crafting lets me choose the slot and the item, and every crafted piece of equipment always comes with at least one random bonus. Plus what only professions make: bags, enchants, gems, potions.
- **One character can learn every profession.**
- **Much faster:** a skill-up on almost every craft, and three times as much from gathering.

- [ ] 5.1 Decide what makes crafting worth it next to the new drops
- [~] 5.2 Number of professions and skill-up speed (settings `MaxPrimaryTradeSkill`, `SkillGain.*`, `SkillChance.*`)
- [~] 5.2b Three times the yield from gathering (`mod-wowplus`, setting `WowPlus.Professions.GatherMultiplier`)
- [~] 5.2c Crafted equipment always gets at least one random bonus (setting `WowPlus.Bonus.CraftedGuaranteed`)
- [ ] 5.3 Where materials come from when playing alone
- [ ] 5.4 Recipes that are locked behind raids or reputation
- [ ] 5.5 Crafted gear boosted

## Phase 6: Travel

Decided 2026-10-10:

- **No hearthstone cooldown.**
- **Teleports from the hearthstone.** Using the hearthstone opens a menu: my inn as now, plus the places I have unlocked.
- **Places:** the capital cities of the races, Shattrath and Dalaran, and the entrances of dungeons and raids. Maybe a few other places that make sense. Each unlocks the first time I get there.
- **In menus, not one long list:** grouped (for example Home, Cities, Dungeons, Raids, with dungeons and raids split by continent or expansion) so no page is bloated.
- **Everything is free.**
- Not chosen: the flight master whistle, riding earlier or faster, teleports to every flight point.

- [~] 6.1 Hearthstone without cooldown (`mod-wowplus`, setting `WowPlus.Travel.Hearthstone.NoCooldown`)
- [~] 6.2 The hearthstone menu: Home, Cities, Towns and landmarks, Dungeons and Raids per expansion (`mod-wowplus`, setting `WowPlus.Travel.Hearthstone.Menu`)
- [~] 6.3 Unlocking: a city or town when I first enter it, a dungeon or raid when I first go inside; remembered per character
- [~] 6.4 The place list: cities, Shattrath, Dalaran, every dungeon and raid entrance. Other places, decided 2026-10-10: the neutral hub towns (Booty Bay, Gadgetzan, Everlook, Ratchet, Area 52) and the Dark Portal.

## Phase 7: Classes

Decided 2026-10-10: **left for much later.** Loot, vendors, professions and travel are played with first.

Reopened 2026-10-11. The goals, the data and the decisions are in [Class-Changes.md](Class-Changes.md). The rule: **keep the friction, remove the misery.**

- **Abilities from class quests are taught by the trainer** at the level the quest was offered; the quests stay as optional adventures.
- **New milestone abilities come from the trainer too.**
- **A hunter has a starter pet from level 1**; Tame Beast stays at 10.
- The idea being worked on: a defining ability every ten levels, filling the empty levels 41 to 60.

- [ ] 7.1 `Class-Changes.md`: goals per class, before anything is changed
- [ ] 7.2 Decide whether we take on client patches
- [ ] 7.3 Spell levels and automatic learning
- [~] 7.4 Class quest abilities taught by the trainers (`Settings/SQL/world/2026-10-11_class-quest-abilities.sql`); hunter starter pet and shaman totem relic (`mod-wowplus`, settings `WowPlus.Classes.*`)
- [ ] 7.5 Class by class: spell changes, then the bot behaviour for that class

## Phase 8: The hub (removed 2026-10-11)

**Removed on 2026-10-11.** Robin: with an auction house that always has the crafting materials, gems, glyphs and enchants, and the gear vendors standing in the cities anyway, the island is not needed. What follows is how the hub was planned and built, kept for the record. Still in use from it: honor and arena points from kills, and no rating requirement (`src/WowPlusPvP.cpp`).

Decided 2026-10-10. The hub is **lean**: it covers what a solo player cannot reasonably get or do, and does not replace the cities. Class trainers, profession trainers, bank, mail, auction house, riding, reputation and emblem vendors all stay where they are in the world.

In the hub:

- **Cross-profession supplies:** the enchanting rods, vellums, vials, thread, flux, dyes and the other crafting reagents that are spread over many shops or made by another profession.
- **Materials that are unrealistic to get alone**, and only those:
  - made by another profession: smelted bars including Arcanite, Felsteel and Titansteel; bolts of cloth and the special cloths; cured leathers; engineering parts; alchemy transmutes
  - very rare world materials: Black Lotus, Arcane Crystal, Righteous Orb, Dark Iron Ore, Fel Lotus, Frost Lotus, Titanium
  - elemental materials: Essences, Primals and Eternals
  - **not** raid and heroic drops (Primal Nether, Frozen Orb and the like): those are earned by doing the content
- **Finished goods from other professions:** every glyph, cut gems, enchant scrolls, for gold.
- **Tier token vendors,** Tier 4 to Tier 10.
- **PvP vendors,** every season. **Paid in honor, as in the game, and kills give honor too:** enemies at my level in the world and in dungeons give a little honor, so PvP gear is a slow reward for simply playing.
- **Weapon masters:** one NPC teaching every weapon skill the class can learn.
- **Bots that hang around,** so it is not an empty room.
- **Everything is there from the start.** Nothing to buy or unlock.

- **Place: GM Island** (chosen 2026-10-10 after looking at Hyjal and the other candidates). A small grassy island with one manor house, far out at sea, on the Kalimdor map. Reached only through the hearthstone menu.

- **PvP gear keeps the game's own prices, in honor and arena points; kills give both, and no rating is required** (changed 2026-10-10: first decided as "everything costs honor", then simplified to this, because it needs no repricing of hundreds of items).

- [x] 8.1 Place chosen: GM Island
- [~] 8.1b The hub in the hearthstone menu, available from the start (`mod-wowplus`, setting `WowPlus.Hub.Enable`)
- [~] 8.2 The supplies vendor and the materials vendor, with first-guess prices (`Settings/SQL/world/2026-10-10_hub.sql`)
- [~] 8.3 Glyph, gem and enchant scroll vendors (same file)
- [~] 8.4 Tier token and PvP vendors: two more NPCs whose menu lines open the lists of the game's own vendors, filtered by faction and class
- [~] 8.5 Honor and arena points from kills at my level, no rating required (`mod-wowplus`, settings `WowPlus.Honor.*`, `WowPlus.ArenaPoints.*`, `WowPlus.Hub.NoRatingRequired`)
- [~] 8.6 The weapon master (same SQL file)
- [ ] 8.7 Bots in the hub
- [~] Sprint on Shift (2026-10-11): hold Shift for a burst of 30 percent more speed on foot or swimming. A full bar lasts 6 seconds and refills in 8, starting 1 second after stopping; twice the drain in combat. Run it empty and you are winded: 2 seconds before it refills, at three quarters speed, and no sprint until the bar is back to a quarter. Nothing while mounted, in cat, travel, aquatic or wolf form, in stealth, or with any other speed boost. A puff of dust at the feet on land. Server part in `mod-wowplus` (`WowPlus.Sprint.*`), stamina bar and Shift key in the addon `Client/AddOns/WowPlusSprint`.

## Any time: Bot chat

Its own build order is in [Bot-Chat.md](Bot-Chat.md).

- [ ] B.1 The bridge without the game
- [ ] B.2 The hook in the server
- [ ] B.3 Game knowledge from the database

## Experience (decided 2026-10-11)

Goal: about 70 to 100 hours to level 80, not slow alone, and new places pay better than grinding. A "rubber band" (a bonus while below the area's level) was proposed and turned down as too complicated.

- [~] Kills and quests give 1.5 times the experience (`Rate.XP.Kill`, `Rate.XP.Quest` in `Settings/worldserver.conf`)
- [~] Discovering an area gives 3 times the experience (`Rate.XP.Explore`)
- [~] Rested experience builds up 3 times as fast (`Rate.Rest.*`)
- [~] A party of bots does not eat the experience: your share is 0.75 with one bot, down to 0.45 in a full party of five, where the game gives 0.5 down to 0.28 (`WowPlus.XP.BotPartyShare`). Less than alone on purpose, because the party kills much faster.
- [~] A bonus the first time a character kills each rare (3 percent of the current level) and each dungeon or raid boss (4 percent). A quest is about 5. (`WowPlus.XP.FirstKill.*`)
- [~] Dungeons scaled down for a small party gave very little experience (Sunken Temple, alone, level 50). Doubled, so a dungeon enemy is worth about what an enemy outdoors is (`AutoBalance.RewardScaling.XP.Modifier` in `Settings/AutoBalance.conf`). A first guess, to be tuned by playing.

## Bots (decided 2026-10-11)

The whole picture is in [Bots.md](Bots.md) and [Bot-Chat.md](Bot-Chat.md).

- [~] Bots always accept party and guild invitations
- [~] About half the bots gather in your level range, the rest stay spread out
- [~] Bot gear varies per bot: most green, some with blues, a few really well geared
- [~] Passing bots buff you now and then
- [~] `.bots away` and `.bots alone on/off` to have the area to yourself
- [x] Bot health check (`Settings/Scripts/Test-Bots.py`): factions, healers, levels, log size, update time, our changes in the bot module; `--watch` and `--deaths` for bots that stand still or keep dying
- [x] Bots: all Horde for real (enough accounts), talked-with bots go on the friends list and keep their level
- [~] Menu addon (`Client/AddOns/WowPlusMenu`): one button with a menu of group orders, marks, companions and windows. Built, not tried in the game
- [~] In a group (`WowPlusGroupCare.cpp`): honest ready checks, a healer that asks for a drink, skull and moon buttons, bots back on their feet after a wipe; the loot rule becomes free for all so bots do not loot. Built, not tried in the game
- [~] Group board (`Settings/Docs/Group-Board.md`): people looking for a group to invite, posting your own group, arriving at the entrance, raid cap 10, leaving after the run. Built, not tried in the game. Battlegrounds not built (need Alliance bots on demand)
- [~] Bot chat through Claude: whisper, /say, party and guild chat; a person per bot with a full background, own voice and typing habits (Settings/Bots/Character-Sheet.md); memory notes per bot
- [~] Companions: two guild mates you choose, `.bots call` and `.bots dismiss`, a fire and small things when you rest, gestures and remarks on the road
- [~] Easy control: our own small button bar (addon `WowPlusBots`)
- [~] Bots speak first: a greeting at login, guild chat every 5 to 10 minutes with a short answer from another bot
- [~] Guild mates keep up with your level and gear
- [~] The people behind the bots as files that MapleStoryPlus can share (`Settings/Bots`)
- [ ] MapleStoryPlus reads the shared files
- [ ] Topics from outside (news, weather)
- [~] Bots notice things: moments (level-up, death, loot, rare and boss kills), meeting you in the world, rumours about vendors and rares, a gift by mail now and then

## Auction house (decided 2026-10-11)

- [~] The auction house is kept stocked by `mod-ah-bot`: 4000 to 5000 auctions of gear, trade goods and profession items (`Settings/mod_ahbot.conf`, `Settings/SQL/world/2026-10-11_auction-house.sql`)
- [~] One auction house for both factions (`AllowTwoSide.Interaction.Auction`)
- [~] The hub on GM Island is removed altogether, with its place in the hearthstone menu (`Settings/SQL/world/2026-10-11_remove-hub.sql`)
- [~] The auction house always has every crafting material, gem, glyph and enchant scroll: two auctions of each of about 2000 items, put up again when bought (`mod-wowplus`, `src/WowPlusAuction.cpp`, settings `WowPlus.Auction.Staples.*`)
- [ ] Decide: should the bot also buy what you put up for sale?
- [~] The auction house also buys what you put up, within vendor price times 3 (white), 5 (green), 12 (blue), 15 (epic)

## Talents (decided 2026-10-11)

- [~] The second talent set is free from level 10 (`WowPlus.Classes.DualSpecLevel`)
- [~] Resetting talents is free (`NoResetTalentsCost`), and a "Reset talents" button in the talent window does it (addon `WowPlusTalents`)

## Champions (decided 2026-10-11)

- [~] One or two enemies per zone are champions, chosen by the world seed: a name, a rare's silver dragon, more health, bigger, a level higher, back after 4 to 8 hours. As rares they always drop an extra item and give the first-kill bonus. (`Settings/SQL/world/2026-10-11_champions.sql`, table `wowplus_champions`)
- [~] A bounty on every champion: a real quest from the innkeepers of its zone (flight masters where there is no inn), paid in gold and experience. 86 of the 111 champions have someone to offer it; the rest stand in zones with neither.
- [~] Bots pass on where a champion has been seen
- [ ] Bounty boards as objects in towns, and bounties on other things than champions

## Ideas waiting (see [Ideas.md](Ideas.md))

- [ ] Bounties on more than champions
- [ ] Your own place
- [ ] Bots doing visible things

## The sky, the clock and the journal (decided 2026-10-11)

- [~] A day and night pass in an hour (`WowPlus.World.DayMinutes`); the minimap clock shows the world's time (addon `WowPlusClock`)
- [~] Weather more often and heavier, from the world seed, with settings (`WowPlus.Weather.*`): 40 percent of the time, 40 percent of that heavy, in spells of 20 minutes, and also in zones the game gives none
- [~] The journal: first kills, places discovered, levels, bounties, people met, letters and gifts, written per character and shown by the addon `WowPlusJournal` (`/journal`)
- [ ] Rumours in the journal, and whether they turned out true

## A room of your own (see [Housing.md](Housing.md))

- [~] A key per city from its innkeepers, claim any spot in the city, the key brings you back
- [~] 33 pieces of furniture from the innkeepers, placed freely, picked up again; bank chest, guild vault, mailbox, bed, fire, anvil, forge
- [ ] The room on the hearthstone menu
- [ ] Trophies, visitors and events
- [ ] Crafted and found furniture

## Gameplay feel (decided 2026-10-11)

Robin: avoid making the game feel too fast and action-based. So no dodge and no kill-streak momentum.

- [~] Loot that announces itself in its colour, big hits shown big, a record of your hardest hit with a fanfare (addon `WowPlusFlair`)
- [ ] Faster recovery between fights
- [ ] Enemies with a twist, telegraphed attacks on champions
- [ ] The runner, rifts, ambushes
- [ ] A difficulty you set yourself

## Travel (decided 2026-10-11)

Robin was not sure about teleporting everywhere with the hearthstone: it took the world between places away.

- [~] The hearthstone has no cooldown but only goes home; its menu of cities, towns and dungeons is switched off (`WowPlus.Travel.Hearthstone.Menu`)
- [~] The keys to your rooms are the other teleports. Price: 10 gold for the first, 20 for the second, 30 for the third..., Shattrath 1000, Dalaran 5000; bought in the innkeeper's conversation; on the keyring
- [~] Coaches: every innkeeper sends you to any other inn of your side on the same continent, for a fare like a flight's; fade to black (addon `WowPlusCoach`), no time passing, no events on the road (`WowPlus.Coach.*`, `src/WowPlusCoach.cpp`)
- [ ] Faster flights, boats that leave when you board, a faster ghost

## Classes: something of their own (decided 2026-10-11, see [Class-Changes.md](Class-Changes.md))

- [~] Hunter: many more beasts can be tamed, rares and champions too
- [~] Warlock: enslaved demons stay; Infernal and Doomguard stay in place of the ordinary demon
- [~] Mage: polymorph tomes drop
- [~] Rogue: junkboxes from pockets with gear in them; full speed in stealth
- [~] Shaman: four totems at once from 20; Ghost Wolf indoors
- [~] Druid: Travel Form at 14
- Not done on purpose: anything that is a talent (Charge in combat, stance rage, Shadowstep, exotic pets)
