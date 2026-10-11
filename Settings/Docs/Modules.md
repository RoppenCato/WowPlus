# Modules: what can be added

AzerothCore is built to be extended with **modules**: separate packages that add or change a feature without touching the core code. `mod-playerbots` is one. This is an overview of what else exists, so we can see what is possible before writing something ourselves.

**None of these are installed or tested by us.** The descriptions are the authors' own, shortened. The list was collected on 2026-10-10 from GitHub: every repo tagged [`azerothcore-module`](https://github.com/topics/azerothcore-module) (483 at the time) plus the official [AzerothCore organisation](https://github.com/azerothcore). It is a selection, about 150 of them, not all. The official browsable list is the [AzerothCore catalogue](https://www.azerothcore.org/catalogue.html).

## Before adding one

- **Our core is the Playerbots fork, which runs behind standard AzerothCore.** A module written for the newest AzerothCore may not compile here and can need an older version of itself. Modules made for Playerbots (the first section) are the safest.
- **"Updated" shows the month of the last change.** Something untouched for a year or more may no longer build.
- **Stars** are GitHub stars, a rough sign of how many people use it.
- **Some need more than a module:** a client patch (new races, new battlegrounds), a client addon, or the Lua engine `mod-ale`. Noted where known; always read the module's own README.
- **Modules interact.** Two modules changing the same thing (two autobalancers, two transmog systems) will fight. Add one at a time and test.
- **How to install** is in [Server/README.md](../../Server/README.md#adding-other-modules). In short: put it in `Server/modules/`, rebuild, add its settings file to `Settings/`.
- **Take a backup first** (`Backup-Database.bat`). Most modules add tables or change data, and removing a module does not undo that.

## Good first candidates for us

A personal shortlist for a server that is mostly one player plus bots. To be decided, not decided.

| Module | Why |
|--------|-----|
| [mod-player-bot-level-brackets](https://github.com/DustinHendrickson/mod-player-bot-level-brackets) | Keeps bots spread evenly over the level ranges, so there is always someone at your level. |
| [mod-autobalance](https://github.com/azerothcore/mod-autobalance) or [mod-solocraft](https://github.com/azerothcore/mod-solocraft) | Makes dungeons and raids doable with fewer real players. Pick one, not both. |
| [mod-ah-bot](https://github.com/azerothcore/mod-ah-bot) | Fills the auction house, which is otherwise empty. **Installed 2026-10-11** (Playerbots does not fill it). |
| [mod-aoe-loot](https://github.com/azerothcore/mod-aoe-loot), [mod-learn-spells](https://github.com/azerothcore/mod-learn-spells) | Small comforts with little risk. |
| [mod-transmog](https://github.com/azerothcore/mod-transmog) | Change how gear looks. The most used module after the big systems. |
| [mod-ale](https://github.com/azerothcore/mod-ale) | Lua scripting. Lets us change behaviour with small script files and no rebuild, and many community scripts need it. |
| [mod-dungeon-clear](https://github.com/jrad7/mod-dungeon-clear) | A tank bot leads a full dungeon run. Listed by the Playerbots project itself. |

## Bots (made for Playerbots)

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-ollama-chat](https://github.com/DustinHendrickson/mod-ollama-chat) | Bots chat through a language model run locally with Ollama. Heavy on CPU/GPU. See also our [bot chat design](Bot-Chat.md). | 137 | 2026-10 |
| [mod-llm-chatter](https://github.com/Hokken/mod-llm-chatter) | AI-written bot conversations for mod-playerbots. | 85 | 2026-10 |
| [mod-pbc](https://github.com/deseven/mod-pbc) | Character-focused language model integration for Playerbots, aimed at roleplay. | 63 | 2026-10 |
| [mod-llm-guide](https://github.com/Hokken/mod-llm-guide) | An AI in-game assistant that answers questions about the game. | 17 | 2026-10 |
| [mod-ollama-bot-buddy](https://github.com/DustinHendrickson/mod-ollama-bot-buddy) | Lets a language model control what a bot does, not just what it says. | 46 | 2026-01 |
| [mod-ollama-bot-amigo](https://github.com/notOrrytrout/mod-ollama-bot-amigo) | Language-model-driven bots with questing behaviour. | 3 | 2026-09 |
| [mod-dungeon-clear](https://github.com/jrad7/mod-dungeon-clear) | Autonomous dungeon mode: a tank bot leads the whole run. | 108 | 2026-10 |
| [mod-player-bot-level-brackets](https://github.com/DustinHendrickson/mod-player-bot-level-brackets) | Keeps bots spread evenly across level ranges you set. | 62 | 2026-09 |
| [mod-rndbot-sync](https://github.com/Yuof/mod-rndbot-sync) | Keeps random bots in step with the highest online player in level, gear and progression. | 12 | 2026-09 |
| [mod-playerbots-city-life](https://github.com/vrzgames/mod-playerbots-city-life) | Fills cities and social hubs with lifelike bot crowds. | 13 | 2026-09 |
| [mod-playerbots-pvp-life](https://github.com/vrzgames/mod-playerbots-pvp-life) | World PvP, duel zones and faction activity for bots. | 8 | 2026-09 |
| [mod-playerbots-wintergrasp](https://github.com/vrzgames/mod-playerbots-wintergrasp) | Bots accept Wintergrasp queue and battle invitations. | 5 | 2026-09 |
| [mod-autonomous-altbots](https://github.com/Gutwormy/mod-autonomous-altbots) | Your alt characters stay active on their own as persistent bots. | 3 | 2026-08 |
| [mod-optimal-bot-raid](https://github.com/barnaclebarry/mod-optimal-bot-raid) | Puts together and directs raid groups of bots. | 8 | 2026-07 |
| [mod-rndbot-buffs](https://github.com/Rockhopper1776/mod-rndbot-buffs) | Idle random bots cast class buffs on real players nearby. | 3 | 2026-08 |
| [mod-world-buff-bots](https://github.com/Rockhopper1776/mod-world-buff-bots) | Simulates the classic world buff turn-ins on random timers. | 5 | 2026-08 |
| [mod-junk-to-gold](https://github.com/noisiver/mod-junk-to-gold) | Automatically sells looted gray items, which keeps bot bags from filling. Newer variant: [mod-junk-to-gold-plus](https://github.com/TrisBits/mod-junk-to-gold-plus). | 31 | 2025-02 |
| [mod-token-turnin](https://github.com/Zerathane/mod-token-turnin) | Turns tier tokens into gear automatically. | 17 | 2026-07 |
| [mod-ilvl-scaling](https://github.com/Maddnes95/mod-ilvl-scaling) | Scales creature difficulty and loot to the item level of the real players in the group, ignoring bots. | 5 | 2026-09 |
| [mod-rotation](https://github.com/Maddnes95/mod-rotation) | One-button rotation for the player: one spell that picks the right ability, all 30 specs. Made for the playerbot fork. | 9 | 2026-09 |
| [mod-gold-rush](https://github.com/Old-Man-Warcraft/mod-gold-rush) | Timed open-world resource hotspot events. | 4 | 2026-04 |

### Client addons for bots

These go in the game client's `Interface/AddOns/` folder, not on the server.

| Addon | What it does |
|-------|--------------|
| [MultiBot-Chatless](https://github.com/Wishmaster117/MultiBot-Chatless) | Buttons and panels for controlling your bots instead of typing commands. |
| [PlayerbotManager](https://github.com/Lichborne-AC/PlayerbotManager) | An in-game window for tracking bot gear and raid composition. |

## Playing alone or in a small group

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-autobalance](https://github.com/azerothcore/mod-autobalance) | Scales dungeon and raid enemies to the number of players inside. | 165 | 2026-10 |
| [mod-solocraft](https://github.com/azerothcore/mod-solocraft) | The other approach: strengthens the player instead when the group is too small. | 75 | 2026-09 |
| [mod-dungeon-scale](https://github.com/NathanHandley/mod-dungeon-scale) | Changes dungeon difficulty; a fork of AutoBalance. | 23 | 2026-05 |
| [mod-fortis-autobalance](https://github.com/FortisVoluntas/mod-fortis-autobalance) | An AutoBalance version made for the Playerbots fork. | 2 | 2025-10 |
| [mod-quickbalance](https://github.com/hallgaeuer/mod-quickbalance) | Fixed balance adjustments for raids and dungeons, no automatic scaling. | 1 | 2023-10 |
| [mod-open-world-difficulty](https://github.com/Stefan2102/mod-open-world-difficulty) | Scales the difficulty of creatures in the open world. | 3 | 2026-08 |
| [mod-item-level-scaling](https://github.com/Ildourol/mod-item-level-scaling) | Scales dungeon and raid loot to the player's progression. | 2 | 2026-10 |
| [mod-solo-lfg](https://github.com/azerothcore/mod-solo-lfg) | Use the dungeon finder alone. | 58 | 2025-02 |
| [mod-rdf-expansion](https://github.com/azerothcore/mod-rdf-expansion) | Queue for random dungeons of earlier expansions at a higher level. | 18 | 2025-02 |
| [mod-leech](https://github.com/ZhengPeiRu21/mod-leech) | Heals the player when dealing damage. | 15 | 2026-04 |
| [mod-auto-resurrect](https://github.com/Elmegaard/mod-auto-resurrect) | Resurrects you at the instance entrance when you release. | 5 | 2025-03 |
| [mod-quick-respawn](https://github.com/hallgaeuer/mod-quick-respawn) | Puts your ghost in front of the instance portal after dying. | 8 | 2025-04 |

## Progression and challenge

Changes to the shape of the whole game.

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-individual-progression](https://github.com/ZhengPeiRu21/mod-individual-progression) | Each player moves through Vanilla, then The Burning Crusade, then Wrath, unlocking content as they clear it. The best known overhaul. | 286 | 2026-10 |
| [mod-progression-system](https://github.com/azerothcore/mod-progression-system) | Server-wide content phases, as used by the ChromieCraft server. Uses [mod-zone-difficulty](https://github.com/azerothcore/mod-zone-difficulty). | 82 | 2026-10 |
| [mod-challenge-modes](https://github.com/ZhengPeiRu21/mod-challenge-modes) | Optional self-imposed rules such as hardcore, self-crafted or slow XP, with rewards. | 68 | 2025-11 |
| [mod-hardcore](https://github.com/Zindokar/mod-hardcore) | Simple hardcore mode: death is final. | 3 | 2026-08 |
| [mod-hard-modes](https://github.com/Conan1996-2/mod-hard-modes) | Server-wide hard modes. | 3 | 2026-04 |
| [mod-mythic-plus](https://github.com/silviu20092/mod-mythic-plus) | A Mythic Plus system: timed, scaling dungeons. | 31 | 2026-03 |
| [araxiaonline/mod-mythic-plus](https://github.com/araxiaonline/mod-mythic-plus) | Reuses Wrath dungeons as much harder content up to level 85 with new loot. Not like retail Mythic. | 20 | 2026-07 |
| [MythicPlus-extended](https://github.com/Ildourol/MythicPlus-extended) | Mythic+ with keystones, affixes, timers and a weekly vault. Lua, needs `mod-ale` and AIO. | 3 | 2026-09 |
| [mod-dungeon-master](https://github.com/trauntrow/mod-dungeon-master) | Randomly generated dungeons. | 62 | 2026-09 |
| [Delves](https://github.com/araxiaonline/Delves) | Custom single-player delves. Lua. | 23 | 2026-08 |
| [echoes-of-the-worldsoul](https://github.com/vibecoder99-cmd/echoes-of-the-worldsoul) | Long-term progression overhaul: gear attunement, permanent stat growth, optional Playerbots integration. | 16 | 2026-09 |
| [Prestige-and-Draft-Mode](https://github.com/Youpeoples/Prestige-and-Draft-Mode) | Prestige at max level, with an optional spell draft. Lua. | 20 | 2025-07 |
| [mod-crucible-wotlk](https://github.com/sbitkov/mod-crucible-wotlk) | Permanent character progression system. | 3 | 2026-09 |
| [mod-nemesis-system](https://github.com/Old-Man-Warcraft/mod-nemesis-system) | A creature that kills you in the open world becomes a named revenge target. | 16 | 2026-05 |
| [wrath-of-the-vanilla-v2](https://github.com/Hextv/wrath-of-the-vanilla-v2) | Limits the server to original (vanilla) content. | 13 | 2026-02 |
| [Azerothcore-Level-60-World](https://github.com/Brytenwally/Azerothcore-Level-60-World) | Scales Burning Crusade and Wrath content down to level 48-60. | 4 | 2026-07 |

## Classes, races and skills

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-arac](https://github.com/heyitsbench/mod-arac) | All races can be all classes. Includes client patches. | 58 | 2025-07 |
| [mod-uac](https://github.com/berubejd/mod-uac) | Any race, any class, designed to be easy to maintain and undo. | 15 | 2026-07 |
| [mod-worgoblin](https://github.com/heyitsbench/mod-worgoblin) | Worgen and Goblin as playable races. Needs a client patch. | 39 | 2026-10 |
| [mod-classless-wildcard](https://github.com/DustinHendrickson/mod-classless-wildcard) | Classless play: pick or draft abilities from any class, after the Ascension server. | 43 | 2026-10 |
| [mod-racial-trait-swap](https://github.com/azerothcore/mod-racial-trait-swap) | Swap your racial abilities for another race's. | 12 | 2026-09 |
| [mod-classic-deathknight](https://github.com/VenomekPL/mod-classic-deathknight) | Death Knights that start at level 1 in their race's starting zone. | 4 | 2026-09 |
| [mod-learn-spells](https://github.com/azerothcore/mod-learn-spells) | Learn new spells automatically on level-up, no trainer visit. | 47 | 2025-03 |
| [mod-learn-highest-talent](https://github.com/azerothcore/mod-learn-highest-talent) | Talent spells are learned at their highest rank. | 1 | 2025-02 |
| [mod-talentbutton](https://github.com/brian8544/mod-talentbutton) | Dual spec at level 10 and a button to reset talents anywhere. | 3 | 2026-06 |
| [mod-no-profession-limit](https://github.com/AlsoNotMehh/mod-no-profession-limit) | Removes the two-profession limit. | 13 | 2026-09 |
| [mod-instant-swing](https://github.com/Nevaden/mod-instant-swing) | "On next swing" attacks fire instantly. | 4 | 2026-06 |
| [mod-starting-pet](https://github.com/Gozzim/mod-starting-pet) | Hunters and warlocks start with a pet. | 6 | 2026-04 |
| [mod-npc-talent-template](https://github.com/azerothcore/mod-npc-talent-template) | An NPC that applies ready-made gear and talent sets. | 23 | 2026-10 |

For our own class and skill changes, these show how others did it. Changes to what a spell *does* are in the core's `Server/src/server/scripts/Spells/`; changes to how bots *use* a class are in `Server/modules/mod-playerbots/src/Ai/Class/`.

## Items and gear

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-transmog](https://github.com/azerothcore/mod-transmog) | Transmogrification: make gear look like other gear. | 195 | 2026-10 |
| [mod-transmog-plus](https://github.com/Stefan2102/mod-transmog-plus) | Transmog stored per equipment slot, so the look stays when gear is replaced. | 17 | 2026-10 |
| [mod-random-enchants](https://github.com/azerothcore/mod-random-enchants) | Looted, crafted and quest items can roll random extra enchantments. | 32 | 2026-09 |
| [mod-item-affixes](https://github.com/Nevaden/mod-item-affixes) | Items drop with affixes you roll for stats or ability changes. | 8 | 2026-09 |
| [mod-item-upgrade](https://github.com/silviu20092/mod-item-upgrade) | Upgrade individual items. | 25 | 2026-03 |
| [mod-reforging](https://github.com/silviu20092/mod-reforging) | Reforging: move part of one stat on an item to another. | 17 | 2026-03 |
| [mod-item-enchant-system](https://github.com/Opertol900/mod-item-enchant-system) | Lineage II-style item enchanting, with risk. | 2 | 2026-08 |
| [mod-npc-enchanter](https://github.com/azerothcore/mod-npc-enchanter) | An NPC that enchants your gear. | 44 | 2026-03 |
| [mod-weapon-visual](https://github.com/azerothcore/mod-weapon-visual) | Choose the glow effect on your weapon. | 18 | 2026-09 |
| [mod-costumes](https://github.com/azerothcore/mod-costumes) | Items that temporarily change what you look like. | 9 | 2025-02 |
| [mod_custom_loot](https://github.com/LykaonTitheus/mod_custom_loot) | Choose one item from a boss's loot table instead of hoping for a drop. | 2 | 2026-05 |
| [mod-attunement-skip](https://github.com/dr1s/mod-attunement-skip) | Grants raid attunements automatically at the right level. | 3 | 2026-05 |

## Everyday comfort

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-aoe-loot](https://github.com/azerothcore/mod-aoe-loot) | Loot all nearby corpses at once. Alternative: [TerraByte's version](https://github.com/TerraByte-tbwps/mod-aoe-loot). | 82 | 2026-10 |
| [mod-autoloot](https://github.com/Grim-Batol/mod-autoloot) | Loots creatures the moment they die. | 3 | 2026-07 |
| [mod-quest-loot-party](https://github.com/pangolp/mod-quest-loot-party) | Everyone in the group gets the quest item from one kill. | 29 | 2026-05 |
| [mod-reagent-bank](https://github.com/ZhengPeiRu21/mod-reagent-bank) | A separate bank for crafting materials. | 39 | 2026-04 |
| [mod-improved-bank](https://github.com/silviu20092/mod-improved-bank) | A better bank. | 14 | 2026-03 |
| [mod-abyssal-storage](https://github.com/thanhtong89/mod-abyssal-storage) | A limitless account-wide vault with automatic deposit, and withdrawal when crafting. | 7 | 2026-03 |
| [mod-autosort](https://github.com/silviu20092/mod-autosort) | Sorts your bags. | 4 | 2026-08 |
| [mod-auto-gather](https://github.com/thanhtong89/mod-auto-gather) | Gathers herbs and ore automatically when near, and shows both on the minimap. | 13 | 2026-02 |
| [mod-autofish](https://github.com/Flerp/mod-autofish) | A built-in, configurable fishing helper. | 11 | 2026-04 |
| [mod-flightmaster-whistle](https://github.com/silviu20092/mod-flightmaster-whistle) | An item that takes you to the nearest flight master. | 17 | 2026-08 |
| [mod-no-hearthstone-cooldown](https://github.com/BytesGalore/mod-no-hearthstone-cooldown) | Hearthstone without cooldown. | 22 | 2025-03 |
| [mod-travel-book](https://github.com/ShaneBair/mod-travel-book), [mod-travelers-recall](https://github.com/ThiagoSBarreto/mod-travelers-recall) | Teleport items or services that unlock as you explore. | 4 / 12 | 2026-09 |
| [mod-quest-radar](https://github.com/warblups/mod-quest-radar) | Shows quest objectives on the minimap. Module plus client addon. | 4 | 2026-09 |
| [mod-dungeon-quests](https://github.com/Hisha/mod-dungeon-quests) | An NPC at each dungeon entrance offers that dungeon's quests. | 3 | 2026-09 |
| [mod-skip-dk-starting-area](https://github.com/azerothcore/mod-skip-dk-starting-area) | Skip the Death Knight starting zone. | 21 | 2025-03 |
| [mod-npc-buffer](https://github.com/azerothcore/mod-npc-buffer) | An NPC that buffs you. | 24 | 2026-03 |
| [mod-npc-beastmaster](https://github.com/azerothcore/mod-npc-beastmaster) | An NPC that gives any class a pet, or hunters exotic pets. | 32 | 2026-05 |
| [mod-npc-free-professions](https://github.com/azerothcore/mod-npc-free-professions) | An NPC that gives professions with all recipes. | 18 | 2026-03 |
| [mod-assistant](https://github.com/noisiver/mod-assistant) | One NPC for heirlooms, glyphs, gems, race or faction change, and paid profession skill. | 38 | 2026-06 |
| [mod-character-services](https://github.com/Zerkenn/mod-character-services) | An NPC selling name, appearance, race and faction changes. | 8 | 2026-08 |
| [mod-account-bound](https://github.com/AlsoNotMehh/mod-account-bound) | Account-wide achievements, mounts, pets, titles, reputations and professions, each switchable. | 9 | 2026-09 |
| [mod-account-mounts](https://github.com/azerothcore/mod-account-mounts), [mod-account-achievements](https://github.com/azerothcore/mod-account-achievements) | The official single-purpose versions of the above. | 21 / 29 | 2026-09 / 2025-02 |
| [mod-shared-professions](https://github.com/thanhtong89/mod-shared-professions) | Professions are shared across your account. | 8 | 2026-02 |
| [mod-interact-key](https://github.com/Talotan/mod-interact-key) | A retail-style "interact" key, bound through a macro. | 4 | 2026-05 |

## Rates: XP, drops, gold

Simple rate changes need no module: they are settings in `Settings/worldserver.conf` (see [Settings/README.md](../README.md)). These add more control.

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-individual-xp](https://github.com/azerothcore/mod-individual-xp) | Each player sets their own XP rate with a command. | 29 | 2026-04 |
| [mod-individual-rates](https://github.com/Sphynxmods/mod-individual-rates) | The same idea for XP, skill, reputation, drops and gold. | 4 | 2026-09 |
| [mod-dynamic-xp](https://github.com/azerothcore/mod-dynamic-xp) | Different XP rates for different level ranges. | 23 | 2025-11 |
| [mod-weekend-xp](https://github.com/azerothcore/mod-weekend-xp), [mod-weekendbonus](https://github.com/noisiver/mod-weekendbonus) | Higher rates at weekends. | 22 / 18 | 2026-10 / 2025-02 |
| [mod-xpcatchup](https://github.com/Jellypowered/mod-xpcatchup) | Lower-level characters catch up faster. | 2 | 2026-09 |
| [mod-profession-experience](https://github.com/Tereneckla/mod-profession-experience) | Using professions gives XP. | 12 | 2026-07 |
| [mod-gathering-yield](https://github.com/VenomekPL/mod-gathering-yield) | About three times the yield from mining, herbalism, fishing and skinning. | 2 | 2026-10 |
| [mod-craftspeed](https://github.com/Day36512/mod-craftspeed) | Faster crafting casts. | 7 | 2025-11 |
| [mod-dynamic-loot-rates](https://github.com/hallgaeuer/mod-dynamic-loot-rates) | Separate loot rates inside dungeons and raids. | 6 | 2023-10 |
| [mod-money-for-kills](https://github.com/azerothcore/mod-money-for-kills) | Gold for kills. | 13 | 2026-03 |
| [mod-congrats-on-level](https://github.com/azerothcore/mod-congrats-on-level) | Rewards at chosen levels. | 28 | 2025-02 |
| [mod-reward-played-time](https://github.com/azerothcore/mod-reward-played-time) | Rewards for time played. | 23 | 2026-09 |

## Economy

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-ah-bot](https://github.com/azerothcore/mod-ah-bot) | A bot that puts items on the auction house and buys from it. | 117 | 2026-09 |
| [mod-ah-bot-plus](https://github.com/NathanHandley/mod-ah-bot-plus) | A reworked version with more control over what is listed and at what price. | 143 | 2026-07 |
| [mod-auctionsim](https://github.com/Moloch17/mod-auctionsim) | An auction house simulator. | 20 | 2026-10 |
| [Black-Market-Auction-House](https://github.com/Youpeoples/Black-Market-Auction-House) | The Mists of Pandaria Black Market, backported. Lua, needs `mod-ale`. | 37 | 2025-06 |
| [mod-guildfunds](https://github.com/noisiver/mod-guildfunds) | A share of looted and quest gold goes to the guild bank. | 7 | 2024-09 |
| [mod-tcg-vendors](https://github.com/lightninjay/mod-tcg-vendors) | Restores the trading card game and promotional item vendors. | 13 | 2026-06 |

## The world, guilds and housing

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-guildhouse](https://github.com/azerothcore/mod-guildhouse) | A guild house you buy and furnish with vendors and portals. | 54 | 2026-06 |
| [mod-guild-village](https://github.com/BeardBear33/mod-guild-village) | Guild villages, made for the Playerbots core. Expanded version: [Old-Man-Warcraft's](https://github.com/Old-Man-Warcraft/mod-guild-village). | 6 | 2026-04 |
| [mod-guild-levels](https://github.com/Old-Man-Warcraft/mod-guild-levels) | Guild experience, 25 guild levels and Cataclysm-style perks. | 11 | 2026-05 |
| [mod-playerhousing](https://github.com/dudemcbro/mod-playerhousing) | A private island per account with thousands of placeable furnishings. Includes a client addon. | 2 | 2026-10 |
| [mod-warband-camp](https://github.com/Ildourol/mod-warband-camp) | An account-wide camp with props, mailbox, training dummies and rested XP. | 9 | 2026-10 |
| [mod-camping](https://github.com/maluramichael/mod-camping) | `.camp` makes a campfire that gives a rest buff. | 2 | 2026-09 |
| [mod-TimeIsTime](https://github.com/dunjeon/mod-TimeIsTime) | Changes the speed of the day and night cycle. | 28 | 2025-06 |
| [mod-changeablespawnrates](https://github.com/justin-kaufmann/mod-changeablespawnrates) | Respawn times that follow how many players are around. | 9 | 2026-07 |
| [mod-dead-means-dead](https://github.com/kjack9/mod-dead-means-dead) | Change or remove creature respawn. | 7 | 2023-10 |
| [mod-brawlers-guild](https://github.com/avarishd/mod-brawlers-guild) | The Mists of Pandaria Brawler's Guild. | 12 | 2025-06 |
| [mod-hunts](https://github.com/Hisha/mod-hunts) | A hunter and prey system. | 5 | 2026-09 |
| [mod-creature-capture](https://github.com/thanhtong89/mod-creature-capture) | Capture any NPC and summon it as a guardian. | 9 | 2026-07 |
| [Treasure-Chest-System](https://github.com/zyggy123/Treasure-Chest-System) | Game masters hide treasure chests with hints for players to find. Lua. | 15 | 2026-07 |
| [mod-Faction-Free](https://github.com/gitdalisar/mod-Faction-Free) | Interact with the other faction: quests, reputations, achievements. | 34 | 2025-09 |
| [mod-boss-announcer](https://github.com/azerothcore/mod-boss-announcer) | Announces world boss kills. | 11 | 2026-03 |

## PvP

Only useful once other people play, or with bots that join battlegrounds.

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-cfbg](https://github.com/azerothcore/mod-cfbg) | Cross-faction battlegrounds, so queues fill with fewer players. | 24 | 2026-07 |
| [mod-1v1-arena](https://github.com/azerothcore/mod-1v1-arena) | One against one arena. | 27 | 2026-03 |
| [mod-arena-3v3-solo-queue](https://github.com/azerothcore/mod-arena-3v3-solo-queue) | Queue alone for 3v3. Still in development. | 13 | 2026-09 |
| [mod-bg-slaveryvalley](https://github.com/Helias/mod-bg-slaveryvalley), [mod-bg-twinpeaks](https://github.com/Helias/mod-bg-twinpeaks), [mod-bg-battle-for-gilneas](https://github.com/Helias/mod-bg-battle-for-gilneas) | Extra battlegrounds. Need a client patch. | 35 / 17 / 15 | 2026-09 |
| [mod-arena-tolviron](https://github.com/Helias/mod-arena-tolviron), [mod-arena-tigerspeak](https://github.com/Helias/mod-arena-tigerspeak) | Extra arenas. Need a client patch. | 11 / 10 | 2026-09 |
| [mod-pvp-titles](https://github.com/azerothcore/mod-pvp-titles) | The old PvP rank titles, by honorable kills. | 25 | 2025-02 |
| [mod-duel-reset](https://github.com/azerothcore/mod-duel-reset) | Health and cooldowns reset around duels. | 24 | 2026-09 |
| [mod-pvp-zones](https://github.com/azerothcore/mod-pvp-zones) | Rotating world PvP zones. | 8 | 2026-05 |

## Scripting and tools

For building our own things.

| Module | What it does | Stars | Updated |
|--------|--------------|------:|---------|
| [mod-ale](https://github.com/azerothcore/mod-ale) | The AzerothCore Lua Engine (successor of Eluna). Write game scripts in Lua, no rebuild. Many entries above marked "Lua" need it. | 192 | 2026-10 |
| [eluna-ts](https://github.com/azerothcore/eluna-ts) | Write those scripts in TypeScript. | 38 | 2024-06 |
| [mod-python-engine](https://github.com/privatecore/mod-python-engine), [mod-nodejs](https://github.com/airbreather/mod-nodejs) | The same idea for Python and for Node.js. | 11 / 2 | 2026-08 / 2026-10 |
| [mod-caio](https://github.com/sogladev/mod-caio) | Sends addon code and data from the server to the client, for custom game windows without the player installing anything. | 3 | 2026-04 |
| [skeleton-module](https://github.com/azerothcore/skeleton-module) | The official empty template for a new module. | 57 | 2026-09 |
| [mod-better-item-reloading](https://github.com/azerothcore/mod-better-item-reloading) | Reload changed items without restarting the server. Useful while tuning items. | 20 | 2026-07 |
| [mod-increment-cache-version](https://github.com/sogladev/mod-increment-cache-version) | Makes clients refresh their cache after each server start, so nobody has to delete the Cache folder. | 4 | 2026-01 |
| [mod-gomove](https://github.com/Tralenor/mod-gomove) | Move and place game objects in the world with the mouse. | 7 | 2026-01 |
| [mod-game-state-api](https://github.com/abutbul/mod-game-state-api) | A web API showing what is happening in the game. | 7 | 2025-07 |
| [mod-player-statistics](https://github.com/ShaneBair/mod-player-statistics), [mod-world-events](https://github.com/robbyczgw-cla/mod-world-events) | Log player and bot activity (boss kills, loot, level-ups) to a table for statistics. | 3 / 3 | 2026-09 / 2026-06 |
| [mod-discord-chat](https://github.com/kiwilabagency/mod-discord-chat) | Two-way bridge between game chat and Discord. | 2 | 2026-09 |
| [WotLKItemGenerator](https://github.com/STRHercules/WotLKItemGenerator) | A tool that generates large numbers of new items. | 3 | 2026-09 |

## Mostly for public servers

Not relevant for a private server, listed so they are not mistaken for something else: `mod-anticheat`, `mod-premium`, `mod-system-vip`, `mod-reward-shop`, `mod-multi-client-check`, `mod-ip-tracker`, `mod-recruit-friend`, `mod-server-auto-shutdown`, `mod-detailed-logging`, `mod-world-chat`, `mod-starter-guild`, `mod-breaking-news-override`.
