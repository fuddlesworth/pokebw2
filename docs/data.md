# Game data

The game keeps most of its data in NARC archives under `files/a/`, one numbered file per entry. The archives that are
built from source have their sources in `data/`, and the build packs each archive from them and checks that it matches
the original, so editing an entry is as easy as editing its file, and the matching build proves that the sources say
exactly what the ROM holds. The sources come in two kinds:

- **JSON data**, as pokeplatinum's `res/`: one file per entity, such as `data/pokemon/bulbasaur/data.json`, with a
  JSON schema beside it that documents every field and that editors use for completion and checking. A script packs
  the archives from the files (`DATA_PACKS` in `configure.py`), validating each file against its schema and naming
  the file and field of any mistake. Files are ordered by the constant lists, not by their names, so adding an entry
  is adding its constant and its file; the build notices new, removed and renamed files by itself.
- **Assembly**, for the field and trainer AI scripts, which are code more than data, as pret keeps them: one source
  file per entry, in archive order, assembled with the macros in `include/asm/` (`ARCHIVES` in `configure.py`).

Both use the same constants as the C code (see [Constant lists](#constant-lists)): the JSON by name, the assembly
through the C preprocessor. Each archive has a script that wrote its sources from the original, which documents the
format and can write them again.

| Archive | Sources | Contents | Script |
| --- | --- | --- | --- |
| `a/0/0/2` | `data/text/system/` | System messages | `tools/scripts/text_data.py` |
| `a/0/0/3` | `data/text/script/` | Script messages | `tools/scripts/text_data.py` |
| `a/0/1/2` | `data/zones/` (JSON) | Zone headers | `tools/scripts/zone_data.py` |
| `a/0/1/6`, `a/0/1/8`, `a/0/1/9`, `a/0/2/0`, `a/1/2/4` | `data/pokemon/` (JSON) | Species data, level-up moves, evolutions, baby species, egg moves | `tools/scripts/species_data.py` |
| `a/0/1/7` | `data/pokemon/growth_rates.csv` | Experience tables of the growth rates | `tools/scripts/species_data.py` |
| `a/0/2/1` | `data/moves/` (JSON) | Move data | `tools/scripts/move_data.py` |
| `a/0/2/4` | `data/items/` (JSON) | Item data | `tools/scripts/item_data.py` |
| `a/0/5/6` | `data/field_scripts/` | Field scripts, see [Scripts](scripts.md#field-scripts) | `tools/scripts/field_script.py` |
| `a/0/9/1`, `a/0/9/2`, `a/0/8/9`, `a/0/9/0` | `data/trainers/` (JSON) | Trainers, their parties, and the table of their messages | `tools/scripts/trainer_data.py` |
| `a/1/2/7` | `data/encounters/` (JSON) | Wild encounters | `tools/scripts/encounter_data.py` |
| `a/1/6/9` | `data/tr_ai/` | Trainer AI scripts, see [Scripts](scripts.md) | `tools/scripts/tr_ai_script.py` |

The text archives are packed by `text_data.py` from text files rather than assembled; `configure.py` lists them in
`TEXT_ARCHIVES`.

## Constant lists

The ID constants that name the game's data, such as species, moves, abilities, items, types, sound sequences, trainer
classes, trainers, zones, wild encounter tables, event flags and event variables, are lists in `data/constants/`, one
name per line. They are the source of truth: to add one, add it to its list, and to rename one, use
`tools/scripts/rename_constant.py OLD NEW`, which renames its uses too, and its data file where the data is named after
it. The build generates a header from each list, `constants/<list>.h` in `build/include/generated/`, which is on the
include path, so the C code, the data sources and the scripts all use the same names, and the generated header can never
disagree with its list. A line is a constant's full name, which takes the previous value plus one, or `NAME = value`
(decimal or `0x` hex); `#` starts a comment.

```
# Species, by national Pokédex number (bootstrapped from the ROM by make_constants.py)
SPECIES_NONE = 0
SPECIES_BULBASAUR
SPECIES_IVYSAUR
```

`tools/scripts/gen_constants.py` writes the headers, and its `load()` and `header_text()` give the scripts the same
constants without a build. The lists were written once from the game's own text, and from the sound archive's symbols,
by `tools/scripts/make_constants.py`, which can write them again; constants the game has no text for, such as
`ITEM_LAST` or `TYPE_NULL`, were added by hand. Constants whose names come from the code rather than from the game's
data, such as the battle and field script constants, stay hand-written headers in `include/constants/`.

Zones are named after their place name, which most zones share with others: the one the player can fly from, or else
the first, gets the place name alone (`ZONE_CASTELIA_CITY`); a Pokémon Center, gate, lab or gym, told by its music,
gets what it is (`ZONE_ASPERTIA_CITY_POKEMON_CENTER`, `ZONE_ASPERTIA_CITY_GYM`); and the rest are numbered from `_2` in
ID order (`ZONE_CASTELIA_CITY_12`). The code names a few itself, such as `ZONE_VICTORY_ROAD`, the one Escape Rope
leads out of (`ZONE_OVERRIDES` in `make_constants.py --zones`). A numbered zone is a placeholder: when its map shows
what it is, rename it.

The game has no names for its event flags and variables, so `flags.txt` (`EVENT_FLAG_*`) and `vars.txt`
(`EVENT_WORK_*`) only hold the ones named so far, each with its value. Name one there when the code or a script shows
what it does, then write the field scripts again (`field_script.py disasm`), so that they use the name.

The members of an archive that the code loads by number get a list of their own, `narc_<archive>.txt`, as pret's
`.naix` names: the archive's name, then what the member is, then the kind of file, from its magic, such as
`NARC_INTRO_PROFESSOR_NCLR` in `narc_intro.txt`. The archives have no file names (one of 308 does), so a member is
named only when the code shows what it is, and the list holds only those, each with its value.

## Text

`a/0/0/2` (system messages) and `a/0/0/3` (script messages) hold the game's text, one message file per entry. Their
sources are `data/text/system/NNNN.txt` and `data/text/script/NNNN.txt`, UTF-8, with one message per line, so the
line number (from 0) is the message's ID:

```
Listen up!\nThere's nothing wrong with making money!{be01}\nBut there are wrong ways to do it...
How serious are you willing to get\nin order to get what you want?
```

- `\n` is a line break within a message, and `\\`, `\{` and `\}` are a backslash and braces.
- `{TTTT}` or `{TTTT:a,b}` is a control code, its type in hex and its arguments: a placeholder for a name or number,
  a color, or `{be01}`, which waits for a button and scrolls.
- `\x{HHHH}` is a character that can't be shown as itself. Some messages end with `\x{ffff}`, an extra terminator that
  the original files have.
- A message that starts with `\c` is stored compressed, as the game stores trainers' names, among others.
- A last line `\pad{XX}` is not a message but the byte that fills the end of the file to a multiple of 4 bytes, which
  the original files have as leftovers rather than 0.
- A line `\from{species.name}` is not a message but stands for messages that come from the data in JSON, one per
  species, move or trainer, as pokeplatinum's text banks come from `res/`: see below.

The text that belongs to a species, a move or a trainer is in its JSON file, so renaming a Pokémon or rewriting a
trainer's lines is an edit in one place. The message files take it with `\from{...}` lines, which
`tools/scripts/text_sources.py` expands when the text is packed, among the messages that belong to nothing, such as
Egg and Bad Egg after the species' names:

| File | Line | Messages |
| --- | --- | --- |
| `0090_species_names.txt` | `\from{species.name}` | Each species' `name` |
| `0486_species_names_upper.txt` | `\from{species.name_upper}` | The same in capitals |
| `0483_species_names_with_article.txt` | `\from{species.name_with_article}` | The same with "a" or "an" (`name_article` where the first letter doesn't tell) |
| `0464_species_categories.txt` | `\from{species.category}` | Each species' `category` |
| `0442_pokedex_entries.txt` | `\from{species.pokedex_entry}` | Each species' `pokedex_entry`; the forms' entries after them stay text |
| `0403_move_names.txt` | `\from{moves.name}` | Each move's `name` |
| `0488_move_names_upper.txt` | `\from{moves.name_upper}` | The same in capitals |
| `0402_btl_main.txt` | `\from{moves.description}` | Each move's `description` |
| `0064_item_names.txt` | `\from{items.name}` | Each item's `name` |
| `0481_item_names_with_article.txt` | `\from{items.name_with_article}` | The same with its article (`name_article`, or the whole `name_with_article` where it isn't written as the others) |
| `0482_item_names_plural.txt` | `\from{items.name_plural}` | Each item's `name_plural` |
| `0063_item_descriptions.txt` | `\from{items.description}` | Each item's `description` |
| `0382_trainer_names.txt` | `\from{trainers.name}` | Each trainer's `name` |
| `0381_trainer_msg_load.txt` | `\from{trainers.messages}` | Each trainer's `messages`, in the order of the trainer message table |

In the JSON, a line break is a real one (`"\n"` in the JSON) rather than `\n`; control codes and other escapes are as
in the text files. The other languages' Pokédex entries and categories, which the game keeps for older species, stay
text.

The game encrypts each message with a key that depends on its ID, which `text_data.py` applies when packing. Files are
numbered by their ID in the archive, `NNNN_name.txt`, and named where something says what they are for: a script message
file after the place of the zone whose header names it (`0003_black_city.txt`), and a system message file after what it
holds, which the word set function that loads it or its contents show (`0403_move_names.txt`, `0027_natures.txt`), or
else after the source file or function that loads it (`0004_delete_save.txt`). The others keep their number until they
are known (`0001.txt`). `text_data.py unpack` keeps the names of the files it writes over. Both versions have the same
text.

## Species

Each species has a directory in `data/pokemon/`, named after its constant (`bulbasaur/` for `SPECIES_BULBASAUR`), as
pokeplatinum's `res/pokemon/`. Its `data.json` holds everything the game keeps about it in five archives: its record of
the species data (`a/0/1/6`, the 0x4c bytes that `PML_PersonalGetParam` reads), its level-up moves (`a/0/1/8`), its
evolutions (`a/0/1/9`), its baby species (`a/0/2/0`) and its egg moves (`a/1/2/4`). `data/pokemon/species.schema.json`
documents each field.

```json
{
    "$schema": "../species.schema.json",
    "name": "Bulbasaur",
    "category": "Seed Pokémon",
    "pokedex_entry": "For some time after its birth, it\ngrows by gaining nourishment from\nthe seed on its back.",
    "base_stats": {
        "hp": 45,
        ...
    },
    "types": [ "TYPE_GRASS", "TYPE_POISON" ],
    "abilities": [ "ABILITY_OVERGROW", "ABILITY_NONE" ],
    "hidden_ability": "ABILITY_CHLOROPHYLL",
    ...
    "learnset": {
        "by_level": [
            [ 1, "MOVE_TACKLE" ],
            [ 3, "MOVE_GROWL" ],
            ...
        ],
        "by_tm": [ "TM06", "TM09", ... ],
        "egg_moves": [ "MOVE_SKULL_BASH", "MOVE_CHARM", "MOVE_PETAL_DANCE", ... ],
        "by_tutor": [ "MOVE_GRASS_PLEDGE", "MOVE_SEED_BOMB", "MOVE_BIND", ... ]
    },
    "evolutions": [
        {
            "method": "EVO_METHOD_LEVEL",
            "param": 16,
            "species": "SPECIES_IVYSAUR"
        }
    ],
    "regional_dex_number": null,
    "baby_species": "SPECIES_BULBASAUR"
}
```

`tools/scripts/species_data.py pack` builds the archives, one entry per record, in this order:

- The species of `data/constants/species.txt`, from `SPECIES_NONE` (`none/`, an empty record) to Genesect.
- The extra records, `data/pokemon/extra/<record>.json`, which no species' forms point at, kept until their use is
  known. Some look like unused species, such as a Steel and Flying one with 600 base stats.
- The alternate forms that have records of their own, `data/pokemon/<species>/form_<n>.json`, in the order of
  `data/pokemon/forms.json`. The packer sets each species' first form record from it, so a species' `forms` only
  gives the number of forms and where their sprites start. Species whose forms only differ in looks, such as Unown,
  have no form records.

The baby species archive stops before the forms, which have no `baby_species`. The species data archive ends with a
table of the species' and extra records' Unova Pokédex numbers, which their files hold as `regional_dex_number`: its 301
numbers match the game's Unova Pokédex, from #000 for Victini to #300. The TMs and HMs a species learns are named, and
so are its tutor moves: the record keeps them as a bit per move of each tutor, and the packer finds each move's bit, the
type tutor's (the pledges, the starters' ultimate moves and Draco Meteor, in the order `PokeParty_GetTutorMoveID` tests
them) or its place in the shop table of the tutor of Driftveil City, Lentimas Town, Humilau City or Nacrene City in
`src/ov036/scrcmd_shop.c`. A mod that changes a tutor's table keeps every species' tutor moves. Three flags are named
after the species that have them: `underground` (Diglett and Dugtrio, tested by the battle animations), `asymmetric`
(species that don't look the same mirrored, such as Kingler and Absol) and `palette_forms` (Arceus, whose forms only
change its palette).

Evolutions are a method (`EVO_METHOD_*`), its parameter, and the species evolved into, at most seven. The parameter
depends on the method: a level, an item, a move, a species, or another value such as the beauty needed. The level-up
moves are in the order the game checks them, by level.

The experience tables (`a/0/1/7`) are `data/pokemon/growth_rates.csv`, a row per level from 0 to 100 and a column per
table, headed by its `GROWTH_*` constant; the two after the six rates, headed `EXTRA`, are copies of the first that
nothing names. Both versions have the same species data.

To add a species, add its constant to the end of `species.txt` and its directory with a `data.json`; to add a form
with a record of its own, add its `form_<n>.json` and its line to `forms.json`, and count it in the species' `forms`.
A species' name, category and Pokédex entry go into the text (see [Text](#text)); its sprites and cry aren't built
from source yet.

## Moves

Each move has a directory in `data/moves/`, named after its constant (`thunderbolt/` for `MOVE_THUNDERBOLT`), whose
`data.json` is its record of the move data (`a/0/2/1`, the 0x24 bytes that `PML_MoveGetParamCore` reads).
`tools/scripts/move_data.py pack` builds the archive in the order of `data/constants/moves.txt`, and
`data/moves/move.schema.json` documents each field.

```json
{
    "$schema": "../move.schema.json",
    "name": "Thunderbolt",
    "description": "A strong electric blast is loosed at the\ntarget. It may also leave the target\nwith paralysis.",
    "type": "TYPE_ELECTRIC",
    "quality": "MOVE_QUALITY_DAMAGE_INFLICT",
    "category": "MOVE_CATEGORY_SPECIAL",
    "power": 95,
    "accuracy": 100,
    "pp": 15,
    ...
    "inflicts": {
        "condition": "CONDITION_PARALYSIS",
        "chance": 10,
        ...
    },
    "effect": "BATTLE_EFFECT_PARALYZE_HIT",
    "target": "MOVE_TARGET_SELECTED",
    "stat_changes": [],
    "flags": [ "MOVE_FLAG_PROTECT", "MOVE_FLAG_MIRROR_MOVE" ]
}
```

`effect` is the battle effect the move runs (`BATTLE_EFFECT_*`), and `inflicts` the condition it inflicts with its
chance and duration. `stat_changes` holds up to three changes of the stats of `BATTLEMON_*_STAGE`. `quality` is the
class of the move's effect (`MOVE_QUALITY_*`), `target` which Pokémon it targets (`MOVE_TARGET_*`) and `flags` its
properties (`MOVE_FLAG_*`), such as contact, sound or being blocked by Protect; `include/constants/battle.h` names
each after the moves that have it. The record's "SS" marker, which every move has, is written by the packer. Both
versions have the same move data. A move's name and description go into the text (see [Text](#text)).

## Items

Each item has a directory in `data/items/`, named after its constant (`potion/` for `ITEM_POTION`), whose `data.json`
is its file of the item data (`a/0/2/4`, the 36 bytes of `ItemData` in `include/pml/item.h`) and its text: its name,
its name with an article, its plural and its description (see [Text](#text)). `tools/scripts/item_data.py pack`
builds the archive in the order of `data/constants/items.txt`, and `data/items/item.schema.json` documents each field.

```json
{
    "$schema": "../item.schema.json",
    "name": "Potion",
    "name_plural": "Potions",
    "description": "A spray-type medicine for wounds.\nIt restores the HP of one Pokémon by\njust 20 points.",
    "price": 30,
    "hold_effect": "HOLD_EFFECT_NONE",
    ...
    "field_pocket": 1,
    "battle_pocket": 4,
    ...
    "effects": {
        "sleep_heal": false,
        ...
        "hp_restore": true,
        ...
        "hp_restore_amount": 20,
        ...
    }
}
```

The price is in tens of Pokédollars, which `PML_ItemGetParam` multiplies by 10. An item has either a single `value` or
`effects`, what it does used on a Pokémon (`ItemParams`); the packer sets the record's work type from which. The pockets
are the bag's: 0 items, 1 medicine, 2 TMs and HMs, 3 berries and 4 key items outside battle, and in battle bits for
balls (1), battle items (2), HP and PP restoring (4) and status healing (8). An item's article is "a" or "an" by its
first letter unless `name_article` says otherwise ("an HP Up", "the Leftovers", none for Honey). The 20 unused items,
named "???", have constants of their own, as `ITEM_UNUSED_113`. Both versions have the same item data.

## Trainers

Each trainer is `data/trainers/<trainer>.json`, named after its constant (`elite_four_shauntal.json` for
`TRAINER_ELITE_FOUR_SHAUNTAL`), as pokeplatinum's `res/trainers/data/`. It holds the trainer's record (`a/0/9/1`) and
its party (`a/0/9/2`), and `tools/scripts/trainer_data.py pack` builds both archives in the order of
`data/constants/trainers.txt`. `data/trainers/trainer.schema.json` documents each field.

```json
{
    "$schema": "trainer.schema.json",
    "name": "Shauntal",
    "class": "TRAINER_CLASS_ELITE_FOUR_SHAUNTAL",
    "battle_style": "BTL_STYLE_SINGLE",
    "items": [ "ITEM_FULL_RESTORE" ],
    "ai_flags": [ "AI_FLAG_BASIC", "AI_FLAG_EVAL_ATTACK", "AI_FLAG_EXPERT" ],
    "heals": false,
    "money": 30,
    "reward": "ITEM_NONE",
    "party": [
        {
            "species": "SPECIES_COFAGRIGUS",
            "form": 0,
            "level": 56,
            "difficulty": 200,
            "gender": 0,
            "ability": 1,
            "item": "ITEM_NONE",
            "moves": [ "MOVE_WILL_O_WISP", ... ]
        },
        ...
    ],
    "messages": [
        {
            "type": 0,
            "text": "..."
        },
        ...
    ]
}
```

A trainer's `name` and `messages` go into the text (see [Text](#text)). The messages are also the trainer message table
(`a/0/8/9`), which pairs each with its trainer and type (0 before the battle, 1 when the trainer loses, 8 when it wins,
and other moments by number), and its offsets for each trainer (`a/0/9/0`), which `TrainerMsg_Load` reads; the packer
writes both, with the trainers in the order of `data/trainers/message_order.json`, then any others that have messages. A
party either gives every Pokémon an `item` or none, and `moves` or none; the packer sets the record's party kind
(`PARTY_ITEMS`, `PARTY_MOVES`) from that, and a Pokémon without moves gets the moves of its level. `ai_flags` are the
trainer AI scripts to run (`AI_FLAG_*`, see [Scripts](scripts.md)), `money` a multiplier of the prize money and `reward`
an item given after the battle. A Pokémon's `difficulty` sets its individual values, and `gender` and `ability` pick
them when not 0. `class` is a `TRAINER_CLASS_*` from `data/constants/trainer_classes.txt`, named after the class's name;
where several classes share one, after their only trainer, their sex or their ID, as `TRAINER_CLASS_SCHOOL_KID_F`
(`make_constants.py --trainer-classes`). `TRAINER_NONE` has no file: the packer writes the empty placeholder the game
has for it. Both versions have the same trainers.

Each trainer's constant in `data/constants/trainers.txt`, which the field scripts use, is its class and name, as
`TRAINER_YOUNGSTER_JIMMY`, without the class for the story characters, whose class is Pokémon Trainer
(`TRAINER_CHEREN`), and numbered from `_2` where a class and name repeat, as for rematches (`make_constants.py
--trainers`). To add a trainer, add its constant to the end of the list and its file.

## Wild encounters

Each wild encounter table (`a/1/2/7`, which a zone's header names, `GetZoneEncID`) is `data/encounters/<table>.json`,
named after its constant in `data/constants/encounters.txt` (`striaton_city.json` for `ENCOUNTERS_STRIATON_CITY`),
which names the tables after the place of the first zone that has them. `tools/scripts/encounter_data.py pack`
builds each version's archive in the order of the list, and `data/encounters/encounters.schema.json` documents each
field.

Black 2 and White 2 have different encounters in many places. A table's rates, and each group of its slots, is either
the same for both versions or an object with one for each:

```json
{
    "$schema": "encounters.schema.json",
    "all_year": {
        "rates": {
            "grass": 0,
            ...
            "surf": 10,
            "fishing": 50,
            ...
        },
        "grass": [],
        ...
        "surf": {
            "black2": [
                [ "SPECIES_BASCULIN", 45, 60 ],
                ...
            ],
            "white2": [
                [ "SPECIES_BASCULIN", 45, 60, 1 ],
                ...
            ]
        },
        ...
    }
}
```

A table (`EncData`) has a rate for each group of slots, the table's flags, and the slots: up to 12 each for grass,
dark grass and shaking grass, and up to 5 each for surfing, rippling water, fishing and rippling fishing; the packer
fills the rest with empty ones. A slot is a species, its lowest and highest level, and its form when not 0. A table is
`all_year`, or has four, `spring`, `summer`, `autumn` and `winter`. To add a table, add its constant to the end of the
list and its file, and name it in a zone's header.

## Zone headers

Each zone's header is `data/zones/<zone>.json`, named after its constant (`aspertia_city_gym.json` for
`ZONE_ASPERTIA_CITY_GYM`). `tools/scripts/zone_data.py pack` puts the 615 headers, 0x30 bytes each, into the single
entry of `a/0/1/2` in the order of `data/constants/zones.txt`, and `data/zones/zone.schema.json` documents each field
and the code that reads it.

```json
{
    "$schema": "zone.schema.json",
    "map_type": 16,
    ...
    "scripts": 2,
    "text": 4,
    "bgm": "SEQ_BGM_POKECEN",
    "encounters": null,
    "parent": "ZONE_BLACK_CITY",
    ...
    "fly_from": false,
    ...
}
```

A header names the zone's map, its scripts and script messages, its music (`SEQ_*`, or one for each season), its wild
encounter table (an `ENCOUNTERS_*` constant, or null), the zone it belongs to, its place name, its default weather and
camera, its battle background, what it allows (cycling, Escape Rope, flying from it), and where flying lands. Both
versions have the same zone headers.
