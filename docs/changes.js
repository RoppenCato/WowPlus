/*
 * What the changes page (index.html) shows. This is the only file to edit.
 *
 * One entry per change:
 *   title    what it is, in a few words
 *   text     one sentence a player understands
 *   details  optional list of finer points (shown when the entry is opened)
 *   date     optional, when it was made
 *
 * The page shows what the game has right now. So: no entries for ideas or plans, none for bugs of our
 * own that were fixed, and when something is changed again or taken out, its entry is rewritten or removed.
 *
 * kind ("feature", "tweak", "fix") and status ("confirmed", "built") may be noted per entry; they are only
 * shown, as labels with filters, if  showTags: true  is set below.
 */
window.CHANGELOG = {
  title: "WowPlus",
  theme: "warcraft",          // the look of the page: "arcade", "warcraft" or "maple"
  tagline: "World of Warcraft 3.3.5a as a single-player adventure: a world full of bots who talk like people, with enough left to chance that it can still surprise the one who built it.",
  intro: "What this server has that the original game does not, by area. Click a line for the details.",
  repo: "https://github.com/RoppenCato/WowPlus",
  updated: "2026-10-12",
  footer: "Built on AzerothCore and mod-playerbots.",

  categories: [
    {
      name: "The world",
      summary: "A sky that moves, weather that matters, and things out there that nobody placed by hand.",
      items: [
        { title: "A day and night every hour", kind: "feature", status: "built", date: "2026-10-11",
          text: "The sun rises and sets once per hour of play, and the minimap clock shows the world's time.",
          details: ["Only the sky and the clock follow it; daily resets and holidays stay on the real calendar.", "The length is a setting."] },
        { title: "More and heavier weather", kind: "tweak", status: "built", date: "2026-10-11",
          text: "It rains, snows or storms about 40 percent of the time, and often hard.",
          details: ["A spell of weather lasts 20 minutes.", "Zones the game gave no weather get it too: rain mostly, snow in the cold lands, sand in the deserts.", "Decided from the world seed and the time, so nobody knows the forecast."] },
        { title: "Champions", kind: "feature", status: "built", date: "2026-10-11",
          text: "One or two ordinary enemies in every zone are something more: named, bigger, tougher, with the silver dragon of a rare.",
          details: ["111 champions in 77 zones.", "About three times the health of their kind, a level higher, back after 4 to 8 hours.", "They always drop an extra item and give a first-kill bonus.", "Which enemies they are comes from the world seed: a new seed, a new set."] },
        { title: "Bounties", kind: "feature", status: "built", date: "2026-10-11",
          text: "Every champion has a price on its head: a real quest from the innkeepers of its zone.",
          details: ["Pays gold and experience by the champion's level.", "Bots mention where a champion has been seen."] },
        { title: "Rares are worth hunting", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Rares are tougher, always drop an extra item, and rare elites can be beaten alone.",
          details: ["Rares: twice the health, a quarter more damage.", "Rare elites: scaled down so one player can win."] },
        { title: "Surprise stock at vendors", kind: "feature", status: "built", date: "2026-10-11",
          text: "Vendors sometimes have one unusual item on the shelf, different every week.",
          details: ["Armour and weapon vendors: often a green, sometimes a blue, for the zone's level.", "Other speciality vendors: now and then a potion, elixir or recipe.", "Very rarely a mount or a pet.", "Not at inns, food sellers or in starting areas."] },
        { title: "Wandering merchants with a theme", kind: "feature", status: "built", date: "2026-10-11",
          text: "Vendors that walk a route carry a themed stock: beasts, mounts, potions, armour, weapons, recipes or oddities." },
        { title: "A journal that writes itself", kind: "feature", status: "built", date: "2026-10-11",
          text: "First kills, places discovered, levels, bounties, people met, letters and gifts are written down per character.",
          details: ["Two books: The World and People.", "Open with /journal or the book by the minimap.", "A new line flashes on screen when it is written."] }
      ]
    },
    {
      name: "Loot and items",
      summary: "More reasons to look at what dropped.",
      items: [
        { title: "Any enemy can drop something great", kind: "feature", status: "confirmed", date: "2026-10-10",
          text: "Every kill and every chest has a chance at an extra piece of gear for your level: usually green, sometimes rare, now and then epic, and once in a long while legendary.",
          details: ["The chance of an extra drop: 6 percent from an ordinary enemy, 20 from an elite, 35 from a chest, always from a rare or a champion.",
                    "What it is: about 92 in 100 green, 6 rare (blue), 2 epic, and 1 in 1000 legendary.",
                    "Elites, rares and bosses roll the rarity two to four times and keep the best.",
                    "It leans toward what your class can use.",
                    "Very rarely it is a mount or a companion pet instead."] },
        { title: "Bosses drop things they never did", kind: "feature", status: "built", date: "2026-10-12",
          text: "Every dungeon and raid boss keeps its own loot and also drops one of its new drops: about ten items from elsewhere in the game, one picked for each class.",
          details: ["What drops is random, whoever you are: what you cannot use is for an alt or a guild mate.", "The new drops match the boss in quality and item level, and differ from world to world."] },
        { title: "A loot book", kind: "feature", status: "built", date: "2026-10-12",
          text: "Look up what every boss in every dungeon and raid drops, the new drops included, and search by item name." },
        { title: "Great drops light up the screen", kind: "feature", status: "built", date: "2026-10-11",
          text: "A rare, epic or legendary drop appears in the middle of the screen in its colour, with a beam of light, a flash at the edges and a sound.",
          details: ["Two pulses for rare, four for epic, six for legendary."] },
        { title: "Random bonuses on gear", kind: "feature", status: "built", date: "2026-10-10",
          text: "Green and better gear can carry extra bonuses, more of them on better items; crafted gear always does." },
        { title: "Gear never binds", kind: "tweak", status: "built", date: "2026-10-10",
          text: "Anything you find can be handed on to a bot or another character." },
        { title: "Bags from enemies", kind: "feature", status: "built", date: "2026-10-11",
          text: "8, 10 and 12-slot bags can drop in the zones for levels 10 to 34." },
        { title: "Loot everything at once, junk sells itself", kind: "tweak", status: "built", date: "2026-10-10",
          text: "One loot picks up from every corpse nearby, and grey items turn into gold on the spot." },
        { title: "Your hardest hit is remembered", kind: "feature", status: "built", date: "2026-10-12",
          text: "Beat your hardest hit ever and the number pops up once in gold, with a sound and a line in chat." }
      ]
    },
    {
      name: "Getting around",
      summary: "Less of the same road twice, without taking the world away.",
      items: [
        { title: "Sprint on Shift", kind: "feature", status: "confirmed", date: "2026-10-11",
          text: "Hold Shift to run 30 percent faster on foot or swimming, for as long as stamina lasts.",
          details: ["A bar of 6 seconds, refilling in 8.", "Run it empty and you are winded for a moment.", "Does nothing while mounted, in a travel form, in stealth, or with another speed boost.", "A puff of dust at your feet."] },
        { title: "The hearthstone has no cooldown", kind: "tweak", status: "built", date: "2026-10-10",
          text: "It takes you home whenever you like. Home, and the keys to your rooms, are the only ways to travel by magic." },
        { title: "Coaches between inns", kind: "feature", status: "built", date: "2026-10-11",
          text: "Any innkeeper can send you to another inn on the same continent, for a fare like a flight's.",
          details: ["119 inns.", "The screen fades to black and you arrive.", "About a silver or two nearby at low level, around a gold for a long ride in Northrend."] },
        { title: "Room keys as teleports", kind: "feature", status: "built", date: "2026-10-11",
          text: "The key to your room in a city brings you there from anywhere." },
      ]
    },
    {
      name: "A room of your own",
      summary: "Buy a key, claim a spot in a city, and furnish it.",
      items: [
        { title: "Claim any spot in a city", kind: "feature", status: "confirmed", date: "2026-10-11",
          text: "An innkeeper sells the key; stand where you want your room and use it.",
          details: ["Ten cities.", "The first key costs 10 gold, the second 20, the third 30; Shattrath 1000, Dalaran 5000.", "The room can be moved; the furniture goes back into your bags."] },
        { title: "Furniture you place yourself", kind: "feature", status: "built", date: "2026-10-11",
          text: "33 pieces from the innkeepers, put down where you stand and picked up again.",
          details: ["Chairs, benches and stools you can sit on.", "Tables, rugs, braziers, lanterns, candles, a bookshelf, a wardrobe, banners and more.", "Only in the world while you are near, so it is there for you alone."] },
        { title: "Storage, mail and the guild vault at home", kind: "feature", status: "confirmed", date: "2026-10-11",
          text: "A chest that opens your bank, a mailbox, and a vault for your guild." },
        { title: "A bed, a fire, an anvil and a forge", kind: "feature", status: "built", date: "2026-10-11",
          text: "Sleep for rested experience, cook on your own fire, smith at home." },
      ]
    },
    {
      name: "Bots",
      summary: "A hundred other players who are not there, made into company.",
      items: [
        { title: "Bots of your side, whichever you play", kind: "feature", status: "built", date: "2026-10-12",
          text: "Log in on a Horde or an Alliance character and the world fills with bots of that side. Your guild mates and friends are always among them.",
          details: ["About half gather in your level range and zone, the rest stay spread out.", "After switching sides it takes a few minutes until all are there."] },
        { title: "A board of people looking for a group", kind: "feature", status: "built", date: "2026-10-12",
          text: "A list of who is free near your level, with their role and a note in their own words. Pick who comes along, or post your group and see who whispers.",
          details: ["Always a couple of tanks and healers to choose from; the list changes as people come and go.", "They take a minute to arrive and wait at the dungeon entrance.", "A raid from the board holds up to 10.", "After the run they say goodbye and go; the journal keeps who you ran with."] },
        { title: "People you know stay", kind: "feature", status: "built", date: "2026-10-12",
          text: "A bot you have talked with more than once lands on your friends list, and keeps its level like a guild mate does." },
        { title: "A ready check that means something", kind: "feature", status: "built", date: "2026-10-12",
          text: "A bot low on health or mana, dead or not there answers \"not ready\", sits down to drink and says why. A healer running dry speaks up by itself." },
        { title: "Marks the bots obey", kind: "tweak", status: "built", date: "2026-10-12",
          text: "Skull and moon buttons on the bot bar: the skull dies first, the moon is sheeped or trapped and left alone." },
        { title: "Back on their feet after a wipe", kind: "feature", status: "built", date: "2026-10-12",
          text: "Dead bots are with you again when you have run back, instead of lying there until someone resurrects them." },
        { title: "The loot is yours", kind: "tweak", status: "built", date: "2026-10-12",
          text: "Bots in your group do not loot: every kill is yours to open, in dungeons too." },
        { title: "Bots dress differently", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Most wear green, some have a few blues, a few are really well geared, and they pick the right armour for their class." },
        { title: "Passing buffs", kind: "feature", status: "built", date: "2026-10-11",
          text: "Now and then a bot near you casts its class buff on you." },
        { title: "Have the place to yourself", kind: "feature", status: "built", date: "2026-10-11",
          text: "\".bots away\" sends nearby bots elsewhere once; \".bots alone on\" keeps them away." },
        { title: "Recruit anyone into your guild", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Bots always accept an invitation, and one already in a bot guild leaves it first." },
        { title: "Guild mates keep up", kind: "feature", status: "built", date: "2026-10-11",
          text: "A bot in your guild is brought to your level, with gear to match, so it has caught up when you next log in." },
        { title: "Build your guild one person at a time", kind: "feature", status: "built", date: "2026-10-12",
          text: "Ask to meet someone and a bot walks up and introduces itself: name, where they live, how they play. Talk, say \"keep\" to invite them to your guild, or \"next\" for someone else." },
        { title: "Companions", kind: "feature", status: "built", date: "2026-10-11",
          text: "Any guild mate in your party is a companion. Pick who comes from a list of guild mates online, or call back whoever came last time.",
          details: ["When you rest, one lights a campfire; they sit, eat, laugh, dance or say something.", "On the road, a gesture or a remark every few minutes."] },
        { title: "One menu for everything", kind: "feature", status: "built", date: "2026-10-12",
          text: "A Menu button lists what you can ask of the bots and the server: group orders, marks, companions, the windows. Nothing to type or remember." },
        { title: "A bar of buttons", kind: "feature", status: "built", date: "2026-10-11",
          text: "Follow, Stay, Attack, Flee, Call, Dismiss, Away and Alone, instead of typed commands." },
        { title: "Quiet bots", kind: "tweak", status: "built", date: "2026-10-11",
          text: "No emote spam, no unasked whispers, no canned lines; a hello in party or guild chat when they join." }
      ]
    },
    {
      name: "Talking with bots",
      summary: "Bots answer through Claude, as the people who play them.",
      items: [
        { title: "Bots answer when you talk", kind: "feature", status: "confirmed", date: "2026-10-11",
          text: "Whisper a bot, speak near one, or talk in party, guild or General chat, and it answers in character.",
          details: ["A bot you name always answers; otherwise the one that answered last, or sometimes another.", "Bot commands like \"follow\" still work as commands."] },
        { title: "A person behind every bot", kind: "feature", status: "built", date: "2026-10-11",
          text: "300 people, each with a whole life: family and friends by name, a job, opinions, and something going on that moves along over the weeks.",
          details: ["From Europe, Japan, China, the United States, Canada, Australia and New Zealand.", "They know what time it is where they live.", "Mostly 30 to 45 years old; 95 in 100 are simply nice.", "A Scandinavian answers Swedish in their own language.", "Some know each other outside the game: siblings, cousins, old friends."] },
        { title: "Everyone types their own way", kind: "feature", status: "built", date: "2026-10-11",
          text: "Lower case or capitals, thx or thanks, few words or many, their own hello, goodbye and laugh. You can tell who is talking without the name.",
          details: ["A stranger keeps their life to themselves; a guild mate you have talked with tells you about theirs.", "They are players, not guides: short answers, no advice nobody asked for."] },
        { title: "They remember you", kind: "feature", status: "built", date: "2026-10-11",
          text: "Each bot keeps a few notes about what you talked about. Guild mates keep theirs for good." },
        { title: "Plain talk", kind: "tweak", status: "built", date: "2026-10-11",
          text: "A word gets a few words back. Jokes, questions and stories about their day only now and then." },
        { title: "Bots speak first", kind: "feature", status: "built", date: "2026-10-11",
          text: "A greeting when you log in, and something in guild chat every 5 to 10 minutes, sometimes answered by another bot." },
        { title: "They notice what happens", kind: "feature", status: "built", date: "2026-10-11",
          text: "A level-up, a death, a good drop, a rare or a boss: about one moment in three gets a word from someone." },
        { title: "Meeting in the world", kind: "feature", status: "built", date: "2026-10-11",
          text: "A guild mate or a bot you have talked with waves and says something when your paths cross." },
        { title: "Rumours", kind: "feature", status: "built", date: "2026-10-11",
          text: "Bots pass on true things they \"heard\": a vendor with something odd this week, where a champion or a rare roams." },
        { title: "Gifts by mail", kind: "feature", status: "built", date: "2026-10-11",
          text: "Every few hours of play a guild mate sends a few potions with a short letter." },
      ]
    },
    {
      name: "Levelling",
      summary: "Not slow alone, and new places pay better than grinding.",
      items: [
        { title: "Faster experience", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Kills and quests give one and a half times the experience, discovering an area three times." },
        { title: "Bots do not eat your experience", kind: "tweak", status: "built", date: "2026-10-11",
          text: "In a party of bots you keep 45 to 75 percent of a kill, where the game would leave you 28 to 50." },
        { title: "First-kill bonuses", kind: "feature", status: "built", date: "2026-10-11",
          text: "The first time you kill each rare, champion or boss you get a bonus worth most of a quest." },
        { title: "Rested three times as fast", kind: "tweak", status: "built", date: "2026-10-11",
          text: "A session after a break starts quick." },
        { title: "Proper experience in dungeons alone", kind: "tweak", status: "built", date: "2026-10-11",
          text: "A dungeon enemy scaled down for one player is worth about what an enemy outdoors is." }
      ]
    },
    {
      name: "Classes and talents",
      summary: "Keep the friction, remove the misery.",
      items: [
        { title: "Two talent sets for free, from level 10", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Dual specialisation without the 1000 gold and without waiting for level 40." },
        { title: "Free talent resets, from the talent window", kind: "tweak", status: "built", date: "2026-10-11",
          text: "A \"Reset talents\" button, no trainer and no cost." },
        { title: "Class quest abilities come from the trainer", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Abilities that needed a long class quest, miserable alone, are taught like any other." },
        { title: "A better first twenty levels", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Key abilities arrive earlier where a class had nothing to do.",
          details: ["Hunters have a starter pet from level 1; taming your own still comes at 10.", "Shamans get their totem relic without the quests.", "Rogues can dual wield from the start.", "Several class abilities moved earlier."] },
        { title: "Hunters: tame what could not be tamed", kind: "feature", status: "built", date: "2026-10-11",
          text: "Basilisks, hydras, stags, kodos, mammoths, foxes, worgs and more can be tamed, and so can rares and champions that are beasts.",
          details: ["Each new kind uses the abilities of the pet family it resembles most."] },
        { title: "Warlocks: bind any demon, keep an Infernal or a Doomguard", kind: "feature", status: "built", date: "2026-10-11",
          text: "An enslaved demon stays until you log out, and an Infernal or Doomguard stays in place of your ordinary demon.",
          details: ["One servant at a time.", "Ritual of Doom needs no helpers."] },
        { title: "Mages: a polymorph collection", kind: "feature", status: "built", date: "2026-10-11",
          text: "Tomes that turn enemies into a pig, serpent, turtle, black cat, rabbit or turkey can drop for mages." },
        { title: "Rogues: pockets worth picking, full speed in stealth", kind: "feature", status: "built", date: "2026-10-11",
          text: "A picked pocket often holds a junkbox, a junkbox often holds gear for your level, and stealth no longer slows you." },
        { title: "Shamans: four totems at once from level 20, Ghost Wolf indoors", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Call of the Elements comes ten levels earlier, and the wolf form works inside." },
        { title: "Druids: Travel Form at 14", kind: "tweak", status: "built", date: "2026-10-11",
          text: "Two levels earlier." },        { title: "Starter weapons for sale", kind: "tweak", status: "built", date: "2026-10-11",
          text: "The weaponsmiths of the starting areas sell every basic weapon, a shield and a first wand for 2 silver." },
        { title: "Every profession, learned faster", kind: "tweak", status: "built", date: "2026-10-10",
          text: "No limit of two professions, three times the skill gain, three times the gathering." }
      ]
    },
    {
      name: "New quests",
      summary: "Quests written for this world, offered by the people who already live in it.",
      items: [
        { title: "The Hatred of the Centaur", kind: "feature", status: "built", date: "2026-10-12",
          text: "Mulgore, levels 8 to 9, three quests: an old bull in Bloodhoof Village remembers the wandering years, Galak scouts are on the plains again, and Baine Bloodhoof tells why this land is not taken for granted.",
          details: ["Starts with Hulfnar Stonetotem in Bloodhoof Village."] },
        { title: "The White Stag and the Moon", kind: "feature", status: "built", date: "2026-10-12",
          text: "Mulgore, levels 7 to 10, three quests: Vorn Skyseer tells the tauren tale of Apa'ro the white stag, the moon who freed him, and what a hunter owes the hunted.",
          details: ["Starts with Vorn Skyseer in Bloodhoof Village."] },
        { title: "The Candle and the Dark", kind: "feature", status: "built", date: "2026-10-12",
          text: "Elwynn Forest, levels 6 to 9, Alliance, three quests: a miner locked out of the Fargodeep wants to know what kobolds are, why the dark is death to them, and who is driving them.",
          details: ["Starts with Edrin Halloway in Goldshire, a person new to the world.", "Uses two new kinds of task: going to a place, and hearing someone out."] },
        { title: "Something on the road", kind: "feature", status: "built", date: "2026-10-12",
          text: "Quests that start in one zone and end in the next and tell you what waits there. So far: Elwynn to Westfall and to Redridge, Dun Morogh to Loch Modan, Teldrassil to Darkshore, Durotar to the Barrens, Tirisfal to Silverpine." },
        { title: "The Ones Who Were Here First", kind: "feature", status: "built", date: "2026-10-12",
          text: "Dun Morogh, levels 7 to 9, Alliance, two quests: why the Frostmane trolls are in the mountains at all, and whose mountains they were.",
          details: ["Starts with Yori Crackhelm in Kharanos."] },
        { title: "Once of Our Blood", kind: "feature", status: "built", date: "2026-10-12",
          text: "Teldrassil, levels 6 to 8, Alliance, two quests: what a satyr is, heard from a priestess and then from the satyr himself, and the one who watches over fallen Starbreeze.",
          details: ["Starts with Orenthil Whisperwind in Dolanaar."] },
        { title: "The Admiral's War", kind: "feature", status: "built", date: "2026-10-12",
          text: "Durotar, levels 7 to 9, Horde, three quests: why there are humans in a keep on the orcs' coast, what they were ordered to do, and who should be told. Ends in Orgrimmar.",
          details: ["Starts with Magga in Razor Hill."] },
        { title: "What Sleeps Beneath", kind: "feature", status: "built", date: "2026-10-12",
          text: "Tirisfal Glades, levels 8 to 9, Horde, two quests: where the name Tirisfal comes from, and who lived in the glades long before Lordaeron and left in a hurry.",
          details: ["Starts with Claire Willower in Brill.", "Dark Cleric Beryl also sends you to an archbishop's grave nobody visits."] }
      ]
    },
    {
      name: "Dungeons, raids and PvP gear",
      summary: "Content made for groups, opened to one player and their bots.",
      items: [
        { title: "Dungeons and raids fit your group", kind: "tweak", status: "built", date: "2026-10-10",
          text: "Enemies are weaker when fewer players are inside." },
        { title: "The dungeon finder works alone", kind: "tweak", status: "built", date: "2026-10-10",
          text: "Queue by yourself and go in." },
        { title: "A bot can lead the run", kind: "feature", status: "built", date: "2026-10-10",
          text: "A tank bot can take a whole dungeon from start to finish." },
        { title: "No attunements", kind: "tweak", status: "built", date: "2026-10-10",
          text: "No keys or quest chains before a dungeon or raid, and raids can be entered without a raid group." },
        { title: "Honor and arena points from ordinary kills", kind: "feature", status: "built", date: "2026-10-10",
          text: "Enemies at your level give a little of both, and arena gear needs no rating." }
      ]
    },
    {
      name: "Auction house",
      summary: "A market that works with one real player.",
      items: [
        { title: "A stocked auction house", kind: "feature", status: "built", date: "2026-10-11",
          text: "Around 9000 auctions, shared by both factions." },
        { title: "The staples are always there", kind: "feature", status: "built", date: "2026-10-11",
          text: "Every crafting material, gem, glyph and enchant scroll is kept for sale and put up again when bought." },
        { title: "It buys from you", kind: "feature", status: "built", date: "2026-10-11",
          text: "What you put up is bought if the price is reasonable, usually within a few hours." }
      ]
    },
  ]
};
