# Reference lists

Lists of what is in the game, for planning and for looking things up. Built from our own database and, where the database lacks something, from outside sources. They are snapshots: regenerate after a server update if exact numbers matter.

Files are tab-separated (`.tsv`) and open in Excel or any text editor.

| File | What it is | Built from |
|------|------------|------------|
| `Rares.tsv` | Every rare and rare elite enemy placed in the open world: 391 of them, with level, continent, zone, number of spawn points and whether it has loot of its own. | Our database for everything except the zone. Our data does not record which zone a spawn is in, so the zone comes from the [Rare Mobs by Zone](https://everquest.allakhazam.com/wiki/Rare_Mobs_by_Zone) list on the Allakhazam wiki, matched by name. Made 2026-10-11. |

## Rares.tsv: how far to trust the zone

The `zone_from` column says where the zone came from:

| Value | How many | Meaning |
|-------|---------:|---------|
| `list` | 373 | The wiki list names this rare in this zone. |
| `nearest` | 14 | Not on the wiki list. The zone is that of the nearest listed rare on the same continent: a guess, usually right, sometimes the neighbouring zone. |
| `unknown` | 4 | Not on the list and nothing listed nearby (Skarr the Unbreakable, Mushgog, The Razza, Vern). |

The wiki list does not say which version of the game it describes. It agrees with our 3.3.5a data for 373 of its 378 names, which is good evidence that it fits.

What the table shows at a glance: the old world is thick with rares (The Barrens alone has 33), while Outland and Northrend zones have about three each.
