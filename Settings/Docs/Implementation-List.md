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
- [~] 0.3 First modules added (see the table below)
- [ ] 0.4 Our own module `mod-wowplus` created, empty, with one setting that proves it loads
- [ ] 0.5 Reference lists generated into `Settings/Reference/`: items by level and rarity, creatures by level and rank, chests, dungeons and their bosses
- [ ] 0.6 Decide: Lua engine (`mod-ale`) for quick scripts, or everything in our own module

### Modules added so far

| Module | For | Phase | State |
|--------|-----|-------|-------|
| [mod-solo-lfg](https://github.com/azerothcore/mod-solo-lfg) | Use the dungeon finder alone | 1 | `[~]` |
| [mod-dungeon-clear](https://github.com/jrad7/mod-dungeon-clear) | A tank bot leads a whole dungeon run | 1 | `[~]` |
| [mod-player-bot-level-brackets](https://github.com/DustinHendrickson/mod-player-bot-level-brackets) | Bots spread evenly over the levels, so there is always company at yours | 1 | `[~]` |
| [mod-aoe-loot](https://github.com/azerothcore/mod-aoe-loot) | Loot all nearby corpses at once | 2 | `[~]` |
| [mod-junk-to-gold](https://github.com/noisiver/mod-junk-to-gold) | Grey items are sold automatically; keeps bags and bot bags clear | 2 | `[~]` |

Waiting for a decision before being added: a scaling module (`mod-autobalance` or `mod-solocraft`), random bonuses (`mod-random-enchants`), `mod-ale`.

## Phase 1: Solo access to dungeons and raids

Decided: *nothing yet*

- [ ] 1.1 Enter raids without a raid group, raid quests alone (settings)
- [ ] 1.2 Attunements: go through the 35 entry requirements, keep or drop each (SQL)
- [ ] 1.3 Dungeon finder alone (mod-solo-lfg) tested
- [ ] 1.4 A dungeon with a bot group: invite, `addclass`, and mod-dungeon-clear tested
- [ ] 1.5 Scaling module chosen, installed, and tuned in one dungeon alone and one with bots
- [ ] 1.6 Lockouts: decide how often dungeons and raids reset
- [ ] 1.7 One raid tried with a bot group; list the bosses that do not work

## Phase 2: Loot around my level

Decided: *nothing yet*

- [ ] 2.1 The item lists: which items are in, which are out, banded by level and, above 50, by item level
- [ ] 2.2 The extra drop roll in `mod-wowplus`, behind settings: chance by rarity and by enemy rank
- [ ] 2.3 The 50 to 60, 60 to 70 and 70 to 80 look-ahead rules
- [ ] 2.4 Optional weighting towards what the class can use
- [ ] 2.5 Chests use the same lists, with their own chance
- [ ] 2.6 Bots and the extra drops: may they roll, and bot gear settings so bots keep up
- [ ] 2.7 Bag pressure checked: AoE loot and junk selling tuned

## Phase 3: Items that can be special

Decided: *nothing yet*

- [ ] 3.1 Random bonuses on dropped, crafted and quest items
- [ ] 3.2 Upgrading or reforging for gold
- [ ] 3.3 Experiment: one generated item copy in another rarity, to see what the client does with it

## Phase 4: Gold and vendors

Decided: *nothing yet*

- [ ] 4.1 Write down the target: gold per hour by level, and the price of the best things
- [ ] 4.2 Gold sources tuned to the target
- [ ] 4.3 Vendors with limited stock of rare things (SQL)
- [ ] 4.4 Rotating stock by weekday (SQL)
- [ ] 4.5 Random daily stock from the item lists
- [ ] 4.6 A travelling merchant
- [ ] 4.7 More chests in the world, chest tiers, locked chests with keys

## Phase 5: Professions

Decided: *nothing yet*

- [ ] 5.1 Decide what makes crafting worth it next to the new drops
- [ ] 5.2 Number of professions and skill-up speed (settings)
- [ ] 5.3 Where materials come from when playing alone
- [ ] 5.4 Recipes that are locked behind raids or reputation
- [ ] 5.5 Crafted gear boosted

## Phase 6: Travel

Decided: *nothing yet*

- [ ] 6.1 Choose from the travel ideas in the plan
- [ ] 6.2 Build and try the chosen ones

## Phase 7: Classes

Decided: *nothing yet*

- [ ] 7.1 `Class-Changes.md`: goals per class, before anything is changed
- [ ] 7.2 Decide whether we take on client patches
- [ ] 7.3 Spell levels and automatic learning
- [ ] 7.4 Class quests: skills at their level, quests as optional adventures with better rewards
- [ ] 7.5 Class by class: spell changes, then the bot behaviour for that class

## Any time: Bot chat

Its own build order is in [Bot-Chat.md](Bot-Chat.md).

- [ ] B.1 The bridge without the game
- [ ] B.2 The hook in the server
- [ ] B.3 Game knowledge from the database
