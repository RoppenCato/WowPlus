# The plan

What WowPlus is meant to become, the ideas for getting there, and for each idea what already exists and what would have to be built.

**Status: planning. Nothing in this document is built or installed yet.** Ideas are Robin's, from 2026-10-10. The "how" notes are Claude's assessment from reading the server code and database; where something is not verified it says so.

## The vision

A **single-player World of Warcraft**, with bots as company and as help.

- Everything in the game can be done alone, or with a group of bots: quests, dungeons, raids.
- Killing things and opening chests is always worth it, because anything can drop something useful for your level.
- Gold matters, vendors and chests hold surprises, professions are worth doing alone.
- Classes and travel are tuned for one player, without making the world feel smaller.

## How to read this

Every idea is marked with what kind of work it is, from cheapest to most expensive:

| Mark | Means | Where it lives | Rebuild? |
|------|-------|----------------|----------|
| **Setting** | A line in a settings file | `Settings/*.conf` | No, restart |
| **Module** | An existing community module, see [Modules.md](Modules.md) | `Server/modules/` | Yes |
| **SQL** | A change to game data | `Settings/SQL/` | No, restart |
| **Lua** | A small script, needs the Lua engine module `mod-ale` | to be decided | No |
| **New code** | C++ we write ourselves | Our own module, see below | Yes |
| **Client patch** | A file the game itself must load | `Client/*/Data/` | No |

Two things decide a lot of what follows:

- **The client only knows what it shipped with.** The server can change what a spell *does* or what an item's stats *are*, and item tooltips follow, because the server sends item data to the game. Spell tooltips, talent trees and spell level requirements are read from the game's own files, so changing those so that they *show* correctly needs a client patch. We are the only player and already run a patched client, so this is possible, just more work.
- **Our own code should be one module of our own** (working name `mod-wowplus`) in `Server/modules/`, with each feature behind a setting. That keeps our changes out of the core and out of `mod-playerbots`, so `Update-Server.bat` keeps merging cleanly.

## 1. Dungeons and raids alone or with bots

*Idea: able to run dungeons and raids completely by myself, or with bots. Bosses can be done solo and progress is easy.*

There are two separate problems: getting **in**, and the fights being **doable**.

| Part | How | Mark |
|------|-----|------|
| Enter a raid without being in a raid group | `Instance.IgnoreRaid = 1` in `worldserver.conf` | Setting |
| Enter regardless of level | `Instance.IgnoreLevel = 1` | Setting |
| Raid quests outside a raid group | `Quests.IgnoreRaid = 1` | Setting |
| Use the dungeon finder alone | [mod-solo-lfg](https://github.com/azerothcore/mod-solo-lfg) | Module |
| Shorter lockouts | `Rate.InstanceResetTime`, or [mod-instance-reset](https://github.com/azerothcore/mod-instance-reset) | Setting / Module |
| Fill a group with bots | Playerbots does this already: invite random bots, or create a bot of a chosen class with the `addclass` command (`Playerbots.AddClassCommand = 1`). Bots have dungeon and raid tactics (`Playerbots.ApplyInstanceStrategies = 1`). | Already there |
| A tank bot that leads the whole dungeon | [mod-dungeon-clear](https://github.com/jrad7/mod-dungeon-clear) | Module |
| Make the enemies weaker when the group is small | [mod-autobalance](https://github.com/azerothcore/mod-autobalance) scales enemies to the number of players inside. There is a version for the Playerbots fork, [mod-fortis-autobalance](https://github.com/FortisVoluntas/mod-fortis-autobalance). | Module |
| Make the player stronger instead | [mod-solocraft](https://github.com/azerothcore/mod-solocraft) | Module |
| Scale to the real player's gear, ignoring bots | [mod-ilvl-scaling](https://github.com/Maddnes95/mod-ilvl-scaling) | Module |

What no module solves: **boss mechanics that need several people.** Tank swaps, mind control, "stand in two places at once", fights that split the raid. Scaling the numbers does not help there. With a full bot group most of it works because bots fill the roles; truly alone, each such boss needs its own fix.

To settle:

- **Alone, or always with bots?** "Solo with a bot group" is far less work than "literally alone", because the bots carry the mechanics. Suggestion: aim for bot groups in raids, and truly solo for 5-player dungeons.
- **Scale enemies down or the player up?** Autobalance keeps your character feeling normal and makes the dungeon fit. Solocraft makes you a superhero inside. Do bots count as players for the scaling? If yes, a full bot group gets no help at all, which is probably right.
- **How hard should it be?** "Progress easily" suggests erring on the easy side, with a setting to turn it up.

## 2. Loot around my level from anything

*Idea: mobs, dungeon enemies and chests always have a chance to drop equipment around my level. Put all items in lists sorted by level, and add those lists as an extra drop chance to everything of that level. A level 10 boar could drop any green, blue, epic or legendary for level 10 to 13. Special rules at 60, 70 and 80, where you should start seeing the raid-level items before you get there.*

**This fits how the game already works.** Every creature has a loot list, and a loot list can point to shared lists (`reference_loot_template`, 24,000 rows) with a chance. That is exactly how "world drop" greens already work. The idea extends that system instead of fighting it.

The raw material in our database: 21,440 weapons and armour pieces with a level requirement. By rarity, counting all weapons and armour: 7,888 green, 5,482 blue, 9,784 epic, 30 legendary. Many of those are test items, PvP gear, quest rewards or class sets, so the lists need filtering.

Two ways to build it:

| | A. Lists in the database | B. A loot rule in our own module |
|---|---|---|
| How | Generate one shared list per level from `item_template`, and add it to the loot of every creature of that level. | When loot is created, the server asks our code, which adds an extra roll from an item pool it built at startup. The core has hooks for this (`OnAfterRefCount`, `OnBeforeDropAddItem`, `OnPlayerCreatureKill`). |
| Mark | SQL (generated by a script) | New code |
| Follows | The **creature's** level | The **player's** level, or the creature's, our choice |
| Chests | Needs a level per chest, which chests do not have | Works: uses the player's level or the zone's |
| Tuning | Regenerate and re-import the lists | Change a setting, restart |
| Smart rules ("only armour I can wear", "more from elites") | Hard | Easy |
| Changes to existing data | Tens of thousands of rows | None |

**Suggestion: B.** It is a small piece of code, it leaves the game's own loot untouched, and every rule below becomes a setting.

Rules to decide, with a first proposal:

| Rule | Proposal |
|------|----------|
| Level window | Items requiring your level up to 3 above it. At 10: items for 10 to 13. |
| Whose level | The player's when the creature is within about 5 levels of the player; nothing from grey creatures, so farming low zones gives nothing. |
| Chance by rarity | Roughly: green common, blue uncommon, epic rare, legendary almost never. Real numbers after trying it. |
| Chance by enemy | Normal creature lowest; elites, rare spawns and bosses much higher. A dungeon boss should feel like a jackpot. |
| The 60 / 70 / 80 steps | At those levels "required level" stops meaning much: all raid gear says 60, 70 or 80 and differs by **item level** instead. So above 50 the lists should be banded by item level. From 50 to 60, a small chance at level 60 dungeon and first-raid items; the same from 60 to 70 and 70 to 80. At 80, bands that open as your own gear improves. |
| What you can use | Option: weight drops towards armour types and weapons your class can use, without making everything perfect. |
| Left out | Quest items, test and unused items, PvP sets, items that start quests. To decide: class tier sets, crafted items (see professions), legendaries that belong to a questline. |
| Bots | A bot in your group rolls like a player today. Decide whether bots may win these drops. |

**Mounts (added 2026-10-10).** Mounts get a table of their own with its own chance, rolled separately from gear. The point is that mounts which are normally a one-in-a-hundred boss drop, or worse, are always possible from ordinary play. In this version of the game a mount is an item that teaches a spell when used, so mounts can be dropped like any other item. Same module, same hook; the open questions are in the [implementation list](Implementation-List.md).

Things to watch: it will flood bags with gear, so it pairs well with auto-selling junk ([mod-junk-to-gold](https://github.com/noisiver/mod-junk-to-gold)) and with gold being useful (section 8). It also makes quest rewards and dungeon loot tables matter less, which may or may not be wanted.

## 3. Weapons and armour of any rarity

*Idea: any weapon or armour piece could come in any rarity. Is that even possible?*

**Not directly.** Rarity is a fixed property of each item: every "Worn Shortsword" in the world is the same colour with the same stats. The server cannot hand out one copy as blue and another as epic.

What is possible:

| Way | What you get | Mark |
|-----|--------------|------|
| **Random bonuses on drops.** The game already has this ("of the Bear", "of the Eagle"; 3,468 items use it). Modules extend it to everything: [mod-random-enchants](https://github.com/azerothcore/mod-random-enchants) adds up to three random enchantments in tiers to looted, crafted and quest items; [mod-item-affixes](https://github.com/Nevaden/mod-item-affixes) adds affixes you can re-roll. | Two copies of the same item differ, and a lucky one is much better. The name colour stays the same. | Module |
| **Upgrading.** [mod-item-upgrade](https://github.com/silviu20092/mod-item-upgrade), [mod-reforging](https://github.com/silviu20092/mod-reforging) | You raise an item you like, for gold. A gold sink too. | Module |
| **Generated copies.** Create new items: the same sword as green, blue and epic, with scaled stats. A tool for mass-generating items exists ([WotLKItemGenerator](https://github.com/STRHercules/WotLKItemGenerator)). | Real rarities with the right colours. | SQL, probably plus Client patch |

The catch with generated copies: the 3.3.5a client keeps a small list of all items of its own, and items missing from it are known to show without their icon or look. That would need a client patch with the new items added. Not verified by us yet; a five-minute test with one custom item would tell.

**Suggestion:** start with random bonuses, which gives most of the "this drop could be special" feeling for no new code. Treat generated copies as a later experiment.

## 4. Vendors with surprises

*Idea: interesting things to buy from vendors, balanced, and kept fresh by randomising every day or similar.*

What the game has built in:

- **Limited stock.** Each vendor item (`npc_vendor`, 38,000 rows) has a maximum count and a restock time. "Only 1 in stock, back in 6 hours" needs no code.
- **Stock that depends on the calendar.** `game_event_npc_vendor` gives a vendor extra items only while a game event runs. Seven events, one per weekday, would give a weekly rotation.

| Step | What | Mark |
|------|------|------|
| 1 | A few vendors with rare items in limited stock | SQL |
| 2 | Weekday rotation through game events | SQL |
| 3 | Truly random stock, re-rolled daily from the level lists in section 2 | New code or Lua |
| 4 | A travelling merchant who stands in a different town each day | New code or Lua |

Pricing: the simplest balance is the item's own vendor price times a factor per rarity, paid in gold. Special currencies (tokens, badges) are possible but the price list for those lives in the client, so new ones need a client patch.

Related modules to borrow from: [Black-Market-Auction-House](https://github.com/Youpeoples/Black-Market-Auction-House) (Lua), [mod-tcg-vendors](https://github.com/lightninjay/mod-tcg-vendors), [mod-reward-shop](https://github.com/azerothcore/mod-reward-shop).

To settle: what counts as "interesting"? Gear from section 2's lists, recipes that normally drop in raids, mounts and pets, bags, profession materials, consumables you cannot craft. A mix, with the rare things rare.

## 5. Chests worth opening

*Idea: make chests in the world interesting.*

There are 1,347 chest objects in the game data, with their own loot lists (`gameobject_loot_template`, 17,900 rows). Most hold little.

| Idea | Mark |
|------|------|
| Chests use the same level-based loot as section 2, with a better chance than a normal creature | New code (same module) |
| Chest tiers: a small chance that a chest is a "rich" one | New code |
| More chests: add spawns in caves, camps and dead ends | SQL |
| Turn a share of other world objects into chests | [mod-go-spawn-replacer](https://github.com/Day36512/mod-go-spawn-replacer), Module |
| Hidden treasure with hints | [Treasure-Chest-System](https://github.com/zyggy123/Treasure-Chest-System), Lua |
| Locked chests that need a key sold by the surprise vendors | SQL |

## 6. Professions for one player

*Ideas: balance professions around solo play. Boost the gear you can make. Is this possible?*

Yes. Crafted items are ordinary items, so their stats can be changed with SQL, and tooltips follow.

The solo problems and what addresses them:

| Problem alone | Fix | Mark |
|---------------|-----|------|
| Only two professions, and no guild to cover the rest | `MaxPrimaryTradeSkill` in `worldserver.conf`, or [mod-no-profession-limit](https://github.com/AlsoNotMehh/mod-no-profession-limit) | Setting |
| Levelling a profession is slow | `SkillGain.Crafting`, `SkillGain.Gathering`, `SkillChance.*` | Setting |
| Gathering takes forever for one person | [mod-gathering-yield](https://github.com/VenomekPL/mod-gathering-yield), [mod-auto-gather](https://github.com/thanhtong89/mod-auto-gather) | Module |
| Materials normally come from other players through the auction house | An auction house bot ([mod-ah-bot](https://github.com/azerothcore/mod-ah-bot), [mod-ah-bot-plus](https://github.com/NathanHandley/mod-ah-bot-plus)); to check first what Playerbots already does there. Or vendors that sell materials (section 4). | Module / SQL |
| Bags fill with materials | [mod-reagent-bank](https://github.com/ZhengPeiRu21/mod-reagent-bank) | Module |
| Good recipes drop in raids or need reputation | Put them on trainers or the surprise vendors | SQL |
| Crafted gear is worse than drops | Raise the stats of crafted items, or let crafted items always roll a random bonus (mod-random-enchants covers crafted items) | SQL / Module |
| Professions give nothing while levelling | [mod-profession-experience](https://github.com/Tereneckla/mod-profession-experience) | Module |
| Long cooldowns (transmutes, cloth) | To look at | SQL |

To settle: with loot raining from section 2, crafted gear needs a reason to exist. Options: crafted gear is the *reliable* way to fill a slot you had no luck with; crafted items always get a guaranteed strong bonus; or professions make things drops cannot (bags, enchants, gems, consumables, upgrade materials).

## 7. Attunements

*Idea: remove attunements or make them a lot easier.*

All entry requirements are in one table, `dungeon_access_requirements` (35 rows): keys, quests, achievements and item level checks per dungeon and raid.

| Way | Mark |
|-----|------|
| Delete or relax the rows we do not want | SQL |
| Grant attunements automatically at the right level | [mod-attunement-skip](https://github.com/dr1s/mod-attunement-skip), Module |

Suggestion: SQL. It is 35 rows and we can keep the few that are fun (a key you find) and drop the chores.

## 8. Gold that matters

*Idea: gold gained in different ways, and fun ways to spend it.*

| Earning | Mark |
|---------|------|
| More gold from kills and quests: `Rate.Drop.Money`, `Rate.RewardBonusMoney` | Setting |
| Gold per kill by creature type: [mod-money-for-kills](https://github.com/azerothcore/mod-money-for-kills) | Module |
| Junk sold automatically: [mod-junk-to-gold](https://github.com/noisiver/mod-junk-to-gold) | Module |
| Selling the flood of gear from section 2 | Comes for free |

| Spending | Mark |
|----------|------|
| The surprise vendors and travelling merchant (section 4) | see there |
| Upgrading and reforging gear (section 3) | Module |
| Re-rolling random bonuses | Module |
| Transmog: [mod-transmog](https://github.com/azerothcore/mod-transmog) | Module |
| A home: [mod-guildhouse](https://github.com/azerothcore/mod-guildhouse), [mod-playerhousing](https://github.com/dudemcbro/mod-playerhousing), [mod-warband-camp](https://github.com/Ildourol/mod-warband-camp) | Module |
| Travel services (section 10) | see there |
| Hiring bots: a fee for a geared mercenary group for a dungeon | New code, an idea of our own |
| Chest keys, profession recipes | SQL |

The balance question is one question: **how many gold per hour should a player of level X make, and what should the best thing on sale cost?** Worth writing the answer down before tuning any single source.

## 9. Classes, skills and class quests

*Ideas: rebalance skills. Give some skills at other levels than now. Make class quests give good rewards, and give the skills themselves at a sensible level (example: the warlock's level 60 mount).*

| Change | How | Mark |
|--------|-----|------|
| The level at which a trainer teaches a spell | `trainer_spell` (6,400 rows) has a required level per spell | SQL |
| Learn spells automatically on level-up | [mod-learn-spells](https://github.com/azerothcore/mod-learn-spells) | Module |
| What a class quest rewards (items, a spell, gold) | `quest_template` | SQL |
| What a spell does (damage, cooldown, cost, effect) | The core's spell scripts in `Server/src/server/scripts/Spells/`, and its table for overriding spell data | New code / SQL |
| How the spell's tooltip reads | The client's own spell file | Client patch |
| Talents and talent trees | The client's own files | Client patch |
| How bots play a class | `Server/modules/mod-playerbots/src/Ai/Class/` | New code |

Points to keep in mind:

- **Tooltips.** A changed spell works as changed, but its tooltip shows the old numbers until the client is patched. For a handful of spells that is tolerable with a note; for a real rebalance we need the client patch pipeline.
- **Bots.** Bots look spells up by name and learn them from trainers (`Playerbots.AutoLearnTrainerSpells = 1`), so changed levels are picked up. If a spell's behaviour changes a lot, the bot rotation for that class may need adjusting.
- **Class quests.** In this version of the game the warlock and paladin mounts are simply bought from the trainer; the old quest chains still exist but are optional. The idea would be: the skill arrives at its level without a chore, and the quest chain stays as an optional adventure with a better reward (a unique look, a strong item, a bonus).
- **Reference.** The MapleStoryPlus project has `Class-Changes.md` for goals per class. The same document here would hold the "what and why" per class before anything is changed.

Community examples of big class changes: [mod-classless-wildcard](https://github.com/DustinHendrickson/mod-classless-wildcard), [mod-racial-trait-swap](https://github.com/azerothcore/mod-racial-trait-swap), [mod-talentbutton](https://github.com/brian8544/mod-talentbutton) (dual spec at level 10).

## 10. Travel

*Idea: improve getting around without making it boring or killing immersion.*

The tension: every shortcut removes a bit of world. The ideas that keep the world are the ones you **earn by having been there**.

| Idea | Keeps immersion? | Mark |
|------|------------------|------|
| A teleport service that only reaches places you have discovered: [mod-travelers-recall](https://github.com/ThiagoSBarreto/mod-travelers-recall), [mod-travel-book](https://github.com/ShaneBair/mod-travel-book) | Yes | Module |
| A whistle to the nearest flight master: [mod-flightmaster-whistle](https://github.com/silviu20092/mod-flightmaster-whistle) | Yes | Module |
| Shorter hearthstone cooldown, or several home points: [mod-no-hearthstone-cooldown](https://github.com/BytesGalore/mod-no-hearthstone-cooldown) | Mostly | Module |
| Riding earlier (the riding skills are in `trainer_spell` with a required level; whether the game also checks the level elsewhere is to verify) | Yes | SQL |
| Faster ground mounts | Yes | To look at |
| Instant flights: `InstantFlightPaths = 1` | No, but it is one line to try | Setting |
| All flight paths known from the start: `AllFlightPaths = 1` | No | Setting |
| Ships and zeppelins that leave when you board | Yes | New code, to look at |
| Bots that come to you instead of you fetching them: already there (`Playerbots.SummonWhenGroup = 1`) | n/a | Already there |

## 11. A hub for the solo player

*Idea (2026-10-10): one place where you get and do the things a solo player otherwise cannot, or only with a lot of running: vellums and rods for enchanting, other rare or annoying items. Maybe also all the vendors for PvP gear, tier gear and the like, so you do not have to run everywhere.*

**Technically easy.** Placing an NPC is one row in the database, and no client patch is needed: the game gets NPC names and vendor lists from the server. An existing vendor can simply be given a second spawn in the hub and brings its whole stock and prices with it. New vendors of our own are a few rows more.

What could be there:

| Group | Contents | Mark |
|-------|----------|------|
| The things one profession needs from another | Enchanting rods (made by blacksmiths), vellums (inscription), glyphs for every class, alchemist stones, engineering parts, gems cut to order | SQL: a vendor of our own |
| All the gear vendors in one row | Tier token vendors, emblem vendors, PvP vendors of every season, heirlooms. Second spawns of the existing NPCs. | SQL |
| Every trainer | All classes, all professions including cooking, fishing and first aid, riding, weapon masters for every weapon | SQL |
| Services | Bank, mailbox, auction house, repair, barber, stable master, innkeeper, talent reset, training dummies | SQL |
| Our own additions | The surprise vendors and the travelling merchant's home (section 4), an enchanter NPC ([mod-npc-enchanter](https://github.com/azerothcore/mod-npc-enchanter)), transmog ([mod-transmog](https://github.com/azerothcore/mod-transmog)), portals | SQL / Module |
| Life | Bots that hang around, so it is not a ghost town ([mod-playerbots-city-life](https://github.com/vrzgames/mod-playerbots-city-life)) | Module |

Things to think through:

- **PvP gear costs honor and arena points, which a solo player barely earns.** Putting the vendors in one place does not help if you cannot pay. Options: bots already queue for battlegrounds, so honor can be earned with them; or the hub sells PvP gear for gold; or PvP gear is simply in the loot pools (it is left out today).
- **Tier gear** already has a second way in: the loot system drops tier pieces directly. The token vendors still matter for the tokens that raid bosses drop.
- **It can hollow out the world.** If everything is in one room, the capitals and the travel system (section 10) lose their purpose. Three ways to keep some of it: the hub only carries what is *annoying* and the cities keep what is *normal*; or services unlock as you level; or you **buy each service for gold**, so the hub starts empty and grows, which also gives gold a purpose (section 8).
- **A hub you furnish yourself exists as a module:** [mod-guildhouse](https://github.com/azerothcore/mod-guildhouse) gives a private place where you buy vendors, trainers, a bank, portals and so on, one by one, for gold. It may be most of this idea ready-made; the place and the list of what can be bought would be adjusted to ours.
- **Where.** A place of its own that you teleport to (the classic choice is the unused "GM Island"; the guildhouse module uses it), or inside an existing city such as Dalaran or Shattrath, or a quiet neutral spot that already exists in the world (Caverns of Time, a corner of Booty Bay). It would be the first entry in the hearthstone menu.
- **Both factions.** Some vendors only talk to one faction. For a hub of our own they can be set neutral.

## 12. Talking to bots

Designed separately in [Bot-Chat.md](Bot-Chat.md). It belongs in this plan because a single-player world needs company that answers.

## What depends on what

- **The level item lists (section 2) are the foundation.** Chests (5), random vendor stock (4) and the gold balance (8) all draw from them. Build them once, in our module.
- **Solo dungeons (1) and attunements (7) are independent** and mostly settings and SQL. They can go first.
- **Class changes (9) are the largest and most open-ended**, and the only part that really needs client patches. Last, and one class at a time.
- **Bot gear must keep up.** If the player's gear gets much better through sections 2 and 3, bots look weak next to it. Bots are geared by settings such as `Playerbots.AutoGearQualityLimit` and `Playerbots.RandomGearQualityLimit` (both 3 = blue by default); those are the dials.

## Suggested order

| Phase | What | Kind of work |
|-------|------|--------------|
| 0 | Foundations: decide on our own module and on the Lua engine; generate reference lists of items, creatures and chests into `Settings/Reference/` like in MapleStoryPlus; get into the habit of a backup before each phase | Setup |
| 1 | Solo access: raid without a raid group, attunements relaxed, solo dungeon finder, one scaling module tried in a dungeon with and without bots | Settings, SQL, 2 to 3 modules |
| 2 | The loot system: item lists, the extra drop roll, the rules table above, chests | New code |
| 3 | Random bonuses on items, then upgrading | Modules |
| 4 | Gold: decide the gold-per-hour target, then vendors with limited stock, then rotating and random stock | SQL, then new code |
| 5 | Professions | Settings, SQL, modules |
| 6 | Travel | Modules, settings |
| 7 | Classes: write `Class-Changes.md`, set up the client patch pipeline, then class by class | SQL, new code, client patch |
| any time | Bot chat, see its own build order | New code |

Each phase ends with playing it for a while before starting the next. Several of these ideas change how the others feel.

## Decisions waiting

1. Raids: with a bot group, or truly alone?
2. Scaling: enemies down (autobalance) or player up (solocraft)?
3. Loot: way A (database lists) or way B (our module)? The proposal is B.
4. Loot: follow the player's level or the creature's? May bots win the extra drops?
5. Loot: are class tier sets, crafted items and questline legendaries in the lists?
6. Rarity: random bonuses first, generated copies later?
7. What makes crafted gear worth it next to the new drops?
8. The gold-per-hour target and the price of the best things.
9. Do we take on client patches (needed for honest spell tooltips and any talent change)?
10. Lua engine (`mod-ale`) for quick scripts, or everything in our own C++ module?
