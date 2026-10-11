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

### The switch itself

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
