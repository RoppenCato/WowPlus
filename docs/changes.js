/*
 * What the changes page (index.html) shows. This is the only file to edit.
 *
 * One entry per change:
 *   title    what it is, in a few words
 *   text     one sentence a player understands
 *   details  optional list of finer points (shown when the entry is opened)
 *   kind     "feature" (new), "tweak" (a change to how the game was), or "fix"
 *   status   "confirmed" (seen working in play), "built" (made, not yet seen in play), or "planned"
 *   date     optional, when it was made
 */
window.CHANGELOG = {
  title: "WowPlus",
  theme: "warcraft",          // the look of the page: "arcade", "warcraft" or "maple"
  tagline: "World of Warcraft 3.3.5a as a single-player adventure: a world full of bots who talk like people, with enough left to chance that it can still surprise the one who built it.",
  intro: "Everything that is different from the original game, by area. Open an entry for the details. \"Built\" means it is made and the server runs with it, but it has not been seen working in play yet.",
  repo: "https://github.com/RoppenCato/WowPlus",
  updated: "2026-10-11",
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
      name: "Getting around",
      summary: "Less of the same road twice, without taking the world away.",
      items: [
        { title: "Sprint on Shift", kind: "feature", status: "confirmed", date: "2026-10-11",
          text: "Hold Shift to run 30 percent faster on foot or swimming, for as long as stamina lasts.",
          details: ["A bar of 6 seconds, refilling in 8.", "Run it empty and you are winded for a moment.", "Does nothing while mounted, in a travel form, in stealth, or with another speed boost.", "A puff of dust at your feet."] },
        { title: "The hearthstone has no cooldown", kind: "tweak", status: "built", date: "2026-10-10",
          text: "It takes you home whenever you like." },
        { title: "Coaches between inns", kind: "feature", status: "built", date: "2026-10-11",
          text: "Any innkeeper can send you to another inn on the same continent, for a fare like a flight's.",
          details: ["119 inns.", "The screen fades to black and you arrive.", "About a silver or two nearby at low level, around a gold for a long ride in Northrend."] },
        { title: "Room keys as teleports", kind: "feature", status: "built", date: "2026-10-11",
          text: "The key to your room in a city brings you there from anywhere." },
        { title: "The hearthstone no longer goes everywhere", kind: "tweak", status: "built", date: "2026-10-11",
          text: "A menu of every city, town and dungeon was tried for a day and removed: it took the world between places away." }
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
        { title: "Visitors, trophies and things that happen there", kind: "feature", status: "planned",
          text: "Companions who live there, guild mates dropping by, a neighbour, trophies on the wall, a stray cat." }
      ]
    },
    {
      name: "Bots",
      summary: "A hundred other players who are not there, made into company.",
      items: [
        { title: "Bots of your faction, around your level", kind: "tweak", status: "built", date: "2026-10-11",
          text: "All bots play your side; about half gather in your level range and zone, the rest stay spread out." },
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
        { title: "Companions", kind: "feature", status: "built", date: "2026-10-11",
          text: "Up to two guild mates you choose, called to your side and sent off again.",
          details: ["When you rest, one lights a campfire; they sit, eat, laugh, dance or say something.", "On the road, a gesture or a remark every few minutes."] },
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
          text: "300 people with a name, age, home, job, temper, humour, hobbies and a way of talking.",
          details: ["From Europe, Japan, China, the United States, Canada, Australia and New Zealand.", "They know what time it is where they live.", "Mostly 30 to 45 years old; 95 in 100 are simply nice.", "A Scandinavian answers Swedish in their own language.", "Some know each other outside the game: siblings, cousins, old friends."] },
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
        { title: "The same people for another game", kind: "feature", status: "planned",
          text: "The files that describe the people are written so MapleStoryPlus can use them too." }
      ]
    },
    {
      name: "Loot and items",
      summary: "More reasons to look at what dropped.",
      items: [
        { title: "A surprise in the loot", kind: "feature", status: "confirmed", date: "2026-10-10",
          text: "Enemies and chests can drop an extra piece of gear for your level, of any rarity.",
          details: ["Better odds from elites, rares and bosses.", "Leans toward what your class can use.", "Very rarely a mount or a companion pet."] },
        { title: "Random bonuses on gear", kind: "feature", status: "built", date: "2026-10-10",
          text: "Green and better gear can carry extra bonuses, more of them on better items; crafted gear always does." },
        { title: "Gear never binds", kind: "tweak", status: "built", date: "2026-10-10",
          text: "Anything you find can be handed on to a bot or another character." },
        { title: "Bags from enemies", kind: "feature", status: "built", date: "2026-10-11",
          text: "8, 10 and 12-slot bags can drop in the zones for levels 10 to 34." },
        { title: "Loot everything at once, junk sells itself", kind: "tweak", status: "built", date: "2026-10-10",
          text: "One loot picks up from every corpse nearby, and grey items turn into gold on the spot." },
        { title: "Loot that announces itself", kind: "feature", status: "built", date: "2026-10-11",
          text: "A blue or better drop appears on screen in its colour, with a beam and a flash that pulses longer for better items." },
        { title: "Big hits, and a record", kind: "feature", status: "built", date: "2026-10-11",
          text: "Critical hits are shown large, and beating your hardest hit ever gets a fanfare." }
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
        { title: "Dungeons alone give proper experience", kind: "fix", status: "built", date: "2026-10-11",
          text: "Dungeons scaled down for one player gave very little; that is doubled." }
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
        { title: "Starter weapons for sale", kind: "tweak", status: "built", date: "2026-10-11",
          text: "The weaponsmiths of the starting areas sell every basic weapon, a shield and a first wand for 2 silver." },
        { title: "Every profession, learned faster", kind: "tweak", status: "built", date: "2026-10-10",
          text: "No limit of two professions, three times the skill gain, three times the gathering." }
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
    {
      name: "Fixes",
      summary: "Things that went wrong and were put right.",
      items: [
        { title: "Crash when looting a grey item", kind: "fix", status: "built", date: "2026-10-11",
          text: "The server crashed when a looted item had already been sold by the junk-selling module." },
        { title: "No bots at low level", kind: "fix", status: "built", date: "2026-10-11",
          text: "Arena teams pinned 72 of 100 bots at level 70 to 80; the teams are gone and the bots spread out." },
        { title: "An Outland ring at a starter vendor", kind: "fix", status: "built", date: "2026-10-11",
          text: "Vendors in areas without a level were stocked as if they stood in a city; a zone's level now comes from its quests." },
        { title: "A first wand nobody could afford", kind: "fix", status: "built", date: "2026-10-11",
          text: "A price rule had made the level 5 wand cost 85 silver; it is 2 silver." },
        { title: "Furniture that vanished, and question-mark icons", kind: "fix", status: "built", date: "2026-10-11",
          text: "Plain furniture disappeared when placed, and keys and furniture had no icons." },
        { title: "Bots that talked too much", kind: "fix", status: "built", date: "2026-10-11",
          text: "Long answers to short messages, constant jokes and questions, emote spam, and canned whispers." },
        { title: "Realm shown as offline", kind: "fix", status: "confirmed", date: "2026-10-10",
          text: "The realm stayed marked offline after an unclean shutdown." }
      ]
    },
    {
      name: "Ideas waiting",
      summary: "Talked through, not built.",
      items: [
        { title: "Threat that grows where you do not go", kind: "feature", status: "planned", text: "Camps you clear stay quiet for a while; camps you ignore grow, and a champion moves in." },
        { title: "Camping, and a night that matters", kind: "feature", status: "planned", text: "A tent and a fire almost anywhere; more danger and better finds after dark." },
        { title: "A trial every ten levels", kind: "feature", status: "planned", text: "A challenge made for your class at 10, 20, 30, with a keepsake for winning." },
        { title: "A rival and a mentor", kind: "feature", status: "planned", text: "One bot who races you, one who looks out for you." },
        { title: "More quests from the world", kind: "feature", status: "planned", text: "Bounties on rares and dungeon bosses, courier runs, survey quests, work orders for crafters." },
        { title: "Enemies with a twist", kind: "feature", status: "planned", text: "One in fifteen is enraged, shielded or calls a friend; champions wind up heavy blows you can avoid." },
        { title: "The runner and rifts", kind: "feature", status: "planned", text: "A creature that flees with a pile of loot, and events that open nearby for ten minutes." },
        { title: "A difficulty you set yourself", kind: "feature", status: "planned", text: "Harder enemies for more experience and better loot odds, in steps." }
      ]
    }
  ]
};
