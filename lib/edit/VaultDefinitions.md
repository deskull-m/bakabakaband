# Vault definitions

`VaultDefinitions.jsonc` contains an ordered `vaults` array. Comments are allowed.
Each record has the following required fields; unknown fields are rejected.

| Field | Meaning |
| --- | --- |
| `id` | Unique, increasing ID, 0–32767. Gaps are allowed. |
| `name` | Nonempty vault name (shared by Japanese and English builds). |
| `type` | Room type: 7 (lesser), 8 (greater), 17, 18, 19 or 20. |
| `rating` | Nonnegative rating, preserved from the old definition. |
| `height`, `width` | Dimensions in cells, 1–242 each (`MAX_HGT`/`MAX_WID`). |
| `layout` | Exactly `height` strings, each exactly `width` printable ASCII bytes. |

The following fields are optional (bakabakaband extensions, preserved from the old
`N/X/D/F/T/O` text format). They are omitted when at their default.

| Field | Meaning | Default |
| --- | --- | --- |
| `min_depth` | Minimum dungeon depth for generation (`T:MINDEPTH_n`). | 0 |
| `max_depth` | Maximum dungeon depth for generation (`T:MAXDEPTH_n`). | 999 |
| `rarity` | Generation rarity (`T:RARITY_n`). | 1 |
| `flags` | Vault flags array; currently only `NO_ROTATION` (`O:`). | `[]` |
| `features` | Per-symbol terrain / monster overrides (`F:`). | `[]` |

Each `features` entry is an object:

| Field | Meaning |
| --- | --- |
| `symbol` | The single map character this override applies to. |
| `feat` | Terrain ID placed for `symbol` (`feature_list`). |
| `appearance` | Apparent terrain ID (`feature_ap_list`); optional, defaults to `feat`. |
| `monster` | Monster tag (e.g. `COLOSSUS`) or numeric monrace ID as a string; optional (`place_monster_list`). |

Leading and trailing spaces in each layout string are significant. Do not trim
them. Escape quotes and backslashes using JSON syntax. The reader concatenates
rows without newlines. It checks dimensions, IDs, types and fields before storing
the record; errors identify the vault ID and JSON field path.
The root must be an object containing a nonempty `vaults` array. The CI validation
tool also checks ID ordering/uniqueness and layout dimensions after JSON Schema
validation, so these cross-field errors are rejected before running the game.

Map symbols retain their existing meanings in `src/room/rooms-vault.cpp`: `%`
marks the outer wall, `#` granite, `$` glass, `X` permanent rock, `Y` permanent
glass, `+` secret doors, `-` secret glass doors, `'` curtains, `^` traps, `*`
treasure or traps, and `&`, `@`, `9`, `8`, `,` monster/treasure placements.
Terrain symbols and unmarked floor cells are unchanged by this migration.

## Migration notes (bakabakaband)

The data was converted from bakabakaband's own `VaultDefinitions.txt` (992 records),
which is far larger than the upstream (Hengband) vault set and uses the extra
`F:`/`T:`/`O:` directives above. The conversion preserves the game's existing
behavior exactly, with the following faithful adjustments:

- **Duplicate IDs (last wins).** Four IDs appeared twice in the text file
  (264 `GARDEN2`→`GARDEN3`, 381 `cathedral2`→`cathedral3`, 457 `Doors room6`→
  `Doors room7`, 748 `SymetricRoom(Fish)`→`Corridor`). The old reader overwrote
  the earlier record, so only the later one ever loaded. The JSONC keeps only the
  later record (988 records total). The shadowed earlier records are dropped.
- **Malformed layouts (9 records).** Where a record's concatenated `D:` text did
  not equal `height × width`, the old reader either ignored the trailing bytes
  (when longer) or read past the buffer (when shorter). The JSONC reproduces the
  old concatenated bytes and re-slices them into `height` rows of `width`:
  longer texts are truncated (IDs 257, 299, 858, 864 — behavior-preserving, the
  tail was never read), and shorter texts are space-padded (IDs 350, 354, 362,
  518, 923 — the former out-of-bounds read becomes defined outside-vault spaces).
  Correcting the intended visual design would change gameplay and is left to a
  separate change.

The definition hash changes with the serialization format; this does not change
vault IDs or the in-memory definitions. No legacy text fallback is provided.
