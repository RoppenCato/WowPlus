# Ideas

Things we might do. Not decided, not planned. When an idea is taken up it moves to the [plan](Plan.md) and the [implementation list](Implementation-List.md).

## Keeping the mystery: an adventure switch

Robin, 2026-10-11: *"For the creator the mystery is gone. All the implementations we do now I know about, reducing the interest for myself to play. I want some sort of modular design with randomness which can be switched on. We do all these changes now and fix it the way I think it should be, but we keep in mind we want some sort of adventurous switch to change things up."*

The problem has two halves, and they need different answers:

- **You know the rules.** You decided the drop rates, the ladder, what the hub sells.
- **You know the content.** You know there is a hub, where it is, and who stands in it.

Randomness fixes the first. Only things you have not seen fix the second.

### The design rule to build by, starting now

So that a switch is possible later without rebuilding everything:

> **Every feature has a baseline, a range, and a seed.**
> The baseline is the value Robin tuned. The range is how far it may wander and still be fair. The seed decides where in the range it lands.

With the switch off, everything sits at its baseline: the game exactly as designed. With it on, each value is drawn from its range using the seed, so the same seed always gives the same world and a new seed gives a new one.

This costs little if done from the start: settings get a baseline and a range instead of one number, and lists (which ability at which milestone, what a vendor may stock) get more candidates than slots. What we have built so far is mostly in that shape already: the loot rule draws from pools, the hub menus are data, the class ladder is a table.

### Ideas, from small to large

**1. Sealed dice: Claude rolls, Robin does not look.**
The server picks values inside the agreed ranges at the start of a "season" and does not print them. Drop chances, which rarity is generous this season, how much honor a boss gives. Robin sets the ranges and knows only that the dice were rolled. The values are written to a sealed file, to be opened when the season ends. The cheapest idea here, and it restores "I wonder what the rates are".

**2. A world seed per character.**
Each new character rolls a seed at creation, and the seed reshapes that character's world: which abilities sit at which milestone (within what makes sense for the class), which zones are rich in rares, which vendors exist. Two mages play differently. This is the "randomizer" idea known from other games, and the reason for the baseline-range-seed rule above.

**3. A draft instead of a ladder.**
At each milestone the trainer offers three abilities from that level's pool and you choose one. You designed the pools, but not which three come up, and not what you gave up. Community modules do versions of this ([mod-classless-wildcard](https://github.com/DustinHendrickson/mod-classless-wildcard), [Prestige-and-Draft-Mode](https://github.com/Youpeoples/Prestige-and-Draft-Mode)); ours would stay inside the class.

**4. Unidentified items.**
An extra drop arrives without showing its random bonuses. They are revealed when you have it identified in the hub, for gold, or by equipping it. A small change to the bonus rule we already have, a gold sink, and every blue becomes a question.

**5. Things that move.**
A travelling merchant whose town changes daily. A treasure goblin that appears somewhere in your level range, flees when hit and drops a pile if you catch it. A rare elite that roams between zones. Chests that are only "rich" on some days. You know they exist; you never know where.

**6. Mutators: a weekly or per-zone twist.**
Like the affixes of Mythic+ dungeons. This week undead are stronger and drop double; in this zone everything is one level higher; today elites explode. Announced when you log in, or discovered. Some could be opt-in challenges taken at the hub for a better reward ([mod-challenge-modes](https://github.com/ZhengPeiRu21/mod-challenge-modes) is the nearest existing thing).

**7. Rumours from the bots.**
Bots know the seed. Ask one in the bot chat and it may have "heard of a merchant near Ratchet" or "seen something odd in the Barrens". The mystery is then found by talking and looking, not by reading a settings file. Ties the [bot chat](Bot-Chat.md) to everything in point 5.

**8. Sealed content: Claude builds, Robin discovers.**
The only idea that restores mystery about *content*. Robin gives a budget and limits ("three secrets per level range, nothing stronger than an epic of that level, nothing that touches saves"). Claude designs and builds them without describing them: a hidden vendor, a named rare with its own drop, a chest behind a waterfall, a short quest from an NPC that was not there before. The files go in a folder marked as spoilers, the commit messages say only "sealed content, batch 3", and the game is where Robin finds out.
What makes this workable: Claude can test through the server log and the database without Robin seeing the result, as was done for the loot rule. What it needs: trust in the limits, and a way to break the seal when something goes wrong.

**9. Letters.**
A lighter form of 8. Now and then a letter arrives in the in-game mailbox with a hint, a map coordinate or an item, written and sent on a schedule Robin does not know.

### Robin's steer, 2026-10-11

*"I like the idea of the world feeling exciting and mysterious."*

| Idea | Verdict | Robin's words, shortened |
|------|---------|--------------------------|
| Class changes and spell levels | **Out.** Stay with our tuned version. | "Probably better kept with our tuned version." |
| 1. Sealed dice | Yes, bounded | "Fun variations, not 'almost no drops'. And not overpowered." |
| 2. World seed | Yes, as **modes chosen in the Settings folder** | "Randomize it there by different modes, that change the world in different ways, described in words and not exact content." |
| 3. Draft | **No** | |
| 4. Unidentified items | Yes, **optional** | "Should be optional to have or not." |
| 5. Things that move | **Yes** | "Makes the world feel more alive. Wandering vendors, new vendors in towns." |
| 6. Mutators | Maybe, **on zones** | "More like modifications that can happen to zones." |
| 7. Rumours from bots | **Yes, strongly** | "We should try to utilise the bots a lot for these sorts of things." |
| 8. Sealed content | Partly | "More in lines of you randomising the items, or choosing mobs." |
| 9. Letters | Interesting | |

Three rules that came with it:

1. **The system is known, the instance is secret.** *"Things existing but never seeing them or knowing about their existence: I'd miss a whole system. The idea itself must be obvious so you know what to look for. It's not fun to know 'there's a big troll in this zone', but more fun hearing from a bot 'I'd be careful walking in these woods'."* So Robin always knows what *kinds* of things the world can do. What is never told is where, which, and when. Nothing is hidden so well that it could be missed entirely: every secret has a hint somewhere.
2. **Never take control of the character away.** No twist may stun, mind-control, fear, slow to a crawl, or otherwise make the player a spectator. Twists change the world, not the hands on the keyboard.
3. **Variation, not deprivation, and not power.** A roll may make something different or a little better or worse. It may not make a system pointless ("almost no drops") or hand out more than the tuned game would.

### The shape this gives: a living world in known systems

Seven systems. Robin knows all seven exist; the seed decides everything inside them.

| System | What Robin knows | What the seed decides |
|--------|------------------|-----------------------|
| **Fortunes** | Luck is not the same everywhere. Some zones and some kinds of enemy are a little more generous, some a little less. | Which. The total stays the same: what one zone loses another gains, and nothing drops below a floor. |
| **Champions** | In every zone a few ordinary enemies are more than they look: tougher, and carrying something worth having. | Which enemies, and what each carries: an item rolled from the loot pools for that level, a notch above what the zone normally gives. |
| **Wanderers** | Merchants travel. Some towns have a trader who was not there before, and one roams the roads. | Who, where, when they move on, and what they stock from the lists the surprise vendors draw on. |
| **Zone moods** | A zone can be in a mood: the dead are restless, the veins run rich, the predators hunt in packs. | Which zone has which mood, and for how long. A mood makes some enemies stronger, more numerous or more rewarding. It never touches the player's control. |
| **Unidentified items** *(optional)* | An extra drop may hide what its bonuses are until it is identified. | Nothing new; it hides what the bonus rule already rolled. Its own switch, off unless wanted. |
| **Rumours** | Bots hear things, and say them. | Which bot knows what. A bot in the right zone might warn about the woods, mention a trader on the road north, or complain that the boars have been dropping nothing lately. This is how every other system is found. |
| **Letters** | Now and then the mail brings something. | When, from whom, and whether it is a hint, a coin or a small gift. |

Rumours are the keystone. Without them the other systems are either invisible (rule 1 fails) or listed in a document (the mystery fails). With them, the way to learn what the world is doing is to talk to the people in it.

### Modes, chosen in the Settings folder

One small file in `Settings/`, for example `world.conf`, with two lines: the mode and the seed. Each mode is described in words, never in numbers:

| Mode | In words |
|------|----------|
| Calm | The tuned game. Nothing moves, nothing varies. For testing. |
| Living | The default adventure. Fortunes shift gently, a few champions per zone, merchants travel, moods come and go. |
| Bountiful | A generous year. Fortunes lean kind, more wanderers with better stock, moods are mostly the rewarding ones. |
| Perilous | A hard year. More champions and stronger ones, moods are mostly the dangerous ones, and the rewards rise with the danger. |
| Merchant's year | Trade is everything. Many wanderers, rare stock, shifting prices. Fewer champions. |
| Strange | The widest ranges of all, within the three rules. For when everything else feels familiar. |

And a seed: a number, or the word `roll` to have a new one picked at the next start. The seed is never printed. Rolling again gives a different world inside the same mode.

What each mode does to each system is a table of ranges in our module. Robin tunes those ranges once, knowing what a mode is *for*, and then stops looking.

### What "sealed content" becomes

Not hidden systems, which Robin ruled out, but hidden *choices inside known systems*: which boar is the champion, what it carries, where the wanderer stands today. The seed makes most of them. Claude can make some by hand where a roll would be bland: choosing a fitting enemy for a zone, writing its rumour, picking an item that suits it. Those go in a data file Robin does not need to open, and everything in it obeys the three rules.

### A possible order

1. **Wanderers,** because the surprise vendors are already planned and this is the same work with a seed.
2. **Champions,** on top of the loot pools that exist.
3. **Rumours, simple version:** bots say fixed lines in ordinary chat when near something. Needs no Claude connection; the bot module already has scripted speech.
4. **Fortunes,** inside the loot rule.
5. **Zone moods.**
6. **Rumours, full version,** when the [bot chat](Bot-Chat.md) exists: ask a bot and it answers from what the seed has done.
7. **Unidentified items** and **letters,** whenever wanted.

### The switch itself (first sketch, replaced by the modes above)

A single setting with three positions is probably enough:

| Position | What it means |
|----------|---------------|
| Off | Everything at its baseline. The game as designed. For testing and tuning. |
| Seeded | Values drawn from their ranges, moving things placed by the seed. Repeatable: the same seed is the same world. |
| Sealed | As Seeded, and the seed and the rolled values are not shown anywhere. |

Per character rather than for the whole server, so that a test character can stay at baseline while a "real" one plays the adventure.

### To think about

- **Which half matters more to you:** not knowing the numbers (1 to 7), or not knowing what exists (8 and 9)?
- **How much unfairness is fun?** A seed can be unlucky: a mage whose escape tool comes at 60. Tight ranges are safe and dull, wide ones are exciting and sometimes miserable. By your own rule, a seed must not create misery, only friction.
- **Who holds the seal?** If you can read the sealed file, will you? It could be kept out of the repo, or in a form that is awkward to read on purpose.
- **Seasons.** Is the natural unit a character, or a period of time after which the dice are rolled again?

## Other ideas

*Nothing here yet.*
