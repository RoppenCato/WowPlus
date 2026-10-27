# WowPlus Dialogue

Built on 2026-10-27, from the plan in [Addons-Plan.md](Addons-Plan.md) (step 2), in the spirit of the Immersion addon. Written fresh for 3.3.5; nothing of Immersion's code is used. **Not tried in the game yet.**

## What it does

When you talk to someone, the game's gossip and quest windows stay closed. Instead a panel low on the screen shows the person up close, with their name, and what they say comes a page at a time: a paragraph, or part of a long one cut at the end of a sentence.

| Key | Does |
|---|---|
| Space, or a click on the panel | the next page |
| 1 to 9 | an answer, when they have finished speaking |
| Esc | walk away |
| mouse wheel over the answers | more answers, when there are over nine |

The answers come up above the panel when the last page is shown:

- **Gossip**: quests to hand in, quests to take (with their level), then the person's other options (buy, train, fly, the server's own options), and Goodbye.
- **A quest offered**: the story, then "What to do", then *I will do it* or *Not now*. The rewards are shown with the answers; hover for the tooltip.
- **Asked how it goes**: what was asked for, then *Here it is* (only when you have it all) or *Not yet*.
- **Done**: thanks, and the rewards. If there is a choice, click the one you want (it lights up), then *Take the one I picked*.

Drag the panel to move it; `/dialogue reset` puts it back. `/dialogue off` gives back the game's own windows, `/dialogue on` takes them again (remembered between sessions).

## How it works

- The game's windows are kept from opening by taking away the events that open them (`GOSSIP_SHOW` from the gossip window; `QUEST_GREETING`, `QUEST_DETAIL`, `QUEST_PROGRESS`, `QUEST_COMPLETE` from the quest window). Everything else they do (money confirmations, codes to type) is left with them.
- The 3.3.5 game cannot let a window listen to keys without taking all of them, so Space and 1-9 are bound to the panel only while it is open, and not in combat. While it is open, Space does not jump.
- The game says goodbye between two windows as well (taking a quest from gossip closes the gossip), so the panel only closes when nothing new has opened a moment later.
- The person talks: the model plays the talking animation, a question or an exclamation at the end of a page gets its own gesture. If this client cannot play animations on a model, the person just stands there.
- For [WowPlus Voice](Addons-Plan.md) (step 3) it calls `WowPlusDialogue_OnSay(kind, who, quest title, text)` whenever someone starts to speak.

## Checked outside the game

`python Settings/Scripts/Test-Addon.py WowPlusDialogue "event GOSSIP_SHOW" "texts" "click WowPlusDialogueChoice2" ...` runs the addon against a stand-in for the game and prints what the panel says. It finds Lua errors; it cannot show whether it looks right or whether the game has every function used.

## To try

1. Talk to an innkeeper: does the panel come up with their face and words, and the game's window stay closed? Space, then pick an option with a number key.
2. Take a quest from someone with gossip and quests (gossip first, then the quest). Read with Space, accept with 1.
3. Hand in a quest with a choice of rewards: click one, then 1.
4. A vendor and a flight master through their gossip: does their own window open as usual?
5. Esc in the middle of a conversation, and walk away from someone mid-talk: does the panel close?
6. `/dialogue off`: the old windows are back. `/dialogue on`.
7. Does the face show, and does it move as it talks? (If not, tell me: the camera or the animation needs another way.)
