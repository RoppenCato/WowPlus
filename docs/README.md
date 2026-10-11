# The changes page

A single web page that lists everything this project changes in the game, by area, with search and filters. GitHub can show it as a website (GitHub Pages).

| File | What it is |
|---|---|
| `index.html` | The page. A template: it holds no content of its own and never needs editing. |
| `changes.js` | The content: the project's name, and every change with its area, a sentence, optional details, its kind and its status. This is the only file to edit. |

## Looking at it

Double-click `index.html`. It works straight from the folder, no web server needed.

## Putting it on the web (once)

1. On GitHub, open the repository, then **Settings**, then **Pages**.
2. Under "Build and deployment", set **Source** to "Deploy from a branch".
3. Set **Branch** to `main` and the folder to `/docs`, and press **Save**.
4. After a minute the page is at `https://<your GitHub name>.github.io/<repository name>/`. GitHub shows the address on that same settings page.

Two things to know first:

- **The page becomes public**, readable by anyone who has the address, even though it only describes the project.
- **A private repository** can only use GitHub Pages on a paid GitHub plan.

After that, every push that changes `changes.js` updates the page by itself within a minute or two.

## Adding a change

Open `changes.js` and add an entry to the right area's `items`:

```js
{ title: "Sprint on Shift", kind: "feature", status: "built", date: "2026-10-11",
  text: "Hold Shift to run faster on foot, for as long as stamina lasts.",
  details: ["A bar of 6 seconds, refilling in 8.", "Does nothing while mounted."] },
```

| Field | Meaning |
|---|---|
| `title` | What it is, in a few words. |
| `text` | One sentence a player understands. |
| `details` | Optional. A list of finer points, shown when the entry is opened. |
| `kind` | `"feature"` (new), `"tweak"` (a change to how the game was), or `"fix"`. Left out: feature. |
| `status` | `"confirmed"` (seen working in play), `"built"` (made, not yet seen in play), or `"planned"`. Left out: built. |
| `date` | Optional. |

A new area is a new block in `categories` with a `name`, a `summary` and its `items`. The colours are given out automatically; an area can ask for its own with `colour: "#ff8800"`.

Also change `updated` at the top of the file.

The page is written in plain text with quotes around every value. A missing comma or quote makes the whole page come up empty; if that happens, the last thing edited is where to look.

## Using it for another project

Copy `index.html` as it is, and write a `changes.js` for that project next to it. Nothing in `index.html` names this project.