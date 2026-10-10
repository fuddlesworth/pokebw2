# Game data

The game keeps most of its data in NARC archives under `files/a/`, one numbered file per entry. The archives that are
built from source have their sources in `data/`, and the build packs each archive from them and checks that it matches
the original, so editing an entry is as easy as editing its file, and the matching build proves that the sources say
exactly what the ROM holds. The sources come in two kinds:

- **JSON data**, as pokeplatinum's `res/`: one file per entity, such as `data/pokemon/bulbasaur/data.json`, with a
  JSON schema beside it that documents every field and that editors use for completion and checking. A script packs
  the archives from the files (`DATA_PACKS` in `configure.py`), validating each file against its schema and naming
  the file and field of any mistake. Files are ordered by the constant lists, not by their names, so adding an entry
  is adding its constant and its file.
- **Assembly**, for the scripts and the data not moved to JSON yet: one source file per entry, in archive order,
  assembled with the macros in `include/asm/` (`ARCHIVES` in `configure.py`).

Both use the same constants as the C code (see [Constant lists](#constant-lists)): the JSON by name, the assembly
through the C preprocessor. Each archive has a script that wrote its sources from the original, which documents the
format and can write them again.

| Archive | Sources | Contents | Script |
| --- | --- | --- | --- |
| `a/0/0/2` | `data/text/system/` | System messages | `tools/scripts/text_data.py` |
| `a/0/0/3` | `data/text/script/` | Script messages | `tools/scripts/text_data.py` |
| `a/0/1/2` | `data/zones/` | Zone headers | `tools/scripts/zone_data.py` |
| `a/0/1/6`, `a/0/1/8`, `a/0/1/9`, `a/0/2/0` | `data/pokemon/` (JSON) | Species data, level-up moves, evolutions, baby species | `tools/scripts/species_data.py` |
| `a/0/1/7` | `data/pokemon/growth_rates.csv` | Experience tables of the growth rates | `tools/scripts/species_data.py` |
| `a/0/2/1` | `data/moves/` | Move data | `tools/scripts/move_data.py` |
| `a/0/5/6` | `data/field_scripts/` | Field scripts, see [Scripts](scripts.md#field-scripts) | `tools/scripts/field_script.py` |
| `a/0/9/1`, `a/0/9/2` | `data/trainers/` | Trainers and their parties | `tools/scripts/trainer_data.py` |
| `a/1/2/7` | `data/encounters/` | Wild encounters | `tools/scripts/encounter_data.py` |
| `a/1/6/9` | `data/tr_ai/` | Trainer AI scripts, see [Scripts](scripts.md) | `tools/scripts/tr_ai_script.py` |

The text archives are packed by `text_data.py` from text files rather than assembled; `configure.py` lists them in
`TEXT_ARCHIVES`.

## Constant lists

The ID constants that name the game's data, such as species, moves, abilities, items, types, sound sequences, trainer
classes, trainers, zones, event flags and event variables, are lists in `data/constants/`, one name per line. They are
the source of truth: to add one, add it to its list, and to rename one, use `tools/scripts/rename_constant.py OLD NEW`,
which renames its uses too. The build generates a header from each list, `constants/<list>.h` in
`build/include/generated/`, which is on the include path, so the C code, the data sources and the scripts all use the
same names, and the generated header can never disagree with its list. A line is a constant's full name, which takes the
previous value plus one, or `NAME = value` (decimal or `0x` hex); `#` starts a comment.

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

The game encrypts each message with a key that depends on its ID, which `text_data.py` applies when packing. Files
are numbered by their ID in the archive, `NNNN_name.txt`, and named where something says what they are for: a script
message file after the place of the zone whose header names it (`0003_black_city.txt`), and a system message file
after what it holds, which the word set function that loads it or its contents show (`0403_move_names.txt`,
`0027_natures.txt`), or else after the source file or function that loads it (`0004_delete_save.txt`). The others keep their number until they are known (`0001.txt`). `text_data.py unpack` keeps
the names of the files it writes over. Both versions have the same text.

## Species

Each species has a directory in `data/pokemon/`, named after its constant (`bulbasaur/` for `SPECIES_BULBASAUR`), as
pokeplatinum's `res/pokemon/`. Its `data.json` holds everything the game keeps about it in four archives: its record of
the species data (`a/0/1/6`, the 0x4c bytes that `PML_PersonalGetParam` reads), its level-up moves (`a/0/1/8`), its
evolutions (`a/0/1/9`) and its baby species (`a/0/2/0`). `data/pokemon/species.schema.json` documents each field.

```json
{
    "$schema": "../species.schema.json",
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
        "tutors": [ "0x00000001", "0x00000040", "0x00000000", "0x0000040f", "0x00002002" ]
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
numbers match the game's Unova Pokédex, from #000 for Victini to #300. The TMs and HMs a species learns are named; the
tutor moves are still bit masks. Three flags are named after the species that have them: `underground` (Diglett and
Dugtrio, tested by the battle animations), `asymmetric` (species that don't look the same mirrored, such as Kingler
and Absol) and `palette_forms` (Arceus, whose forms only change its palette).

Evolutions are a method (`EVO_METHOD_*`), its parameter, and the species evolved into, at most seven. The parameter
depends on the method: a level, an item, a move, a species, or another value such as the beauty needed. The level-up
moves are in the order the game checks them, by level.

The experience tables (`a/0/1/7`) are `data/pokemon/growth_rates.csv`, a row per level from 0 to 100 and a column per
table, headed by its `GROWTH_*` constant; the two after the six rates, headed `EXTRA`, are copies of the first that
nothing names. Both versions have the same species data.

To add a species, add its constant to the end of `species.txt` and its directory with a `data.json`; to add a form
with a record of its own, add its `form_<n>.json` and its line to `forms.json`, and count it in the species' `forms`.
The sprites, cries and names that the other archives hold aren't built from source yet.

## Move data

`a/0/2/1` holds one 0x24-byte record per move, by move ID, which `PML_MoveGetParamCore` reads. Its sources are
`data/moves/NNNN_name.s`, with the macros of `include/asm/move_data.inc`, again one per field and in its order:

```
#include "asm/move_data.inc"

// MOVE_THUNDERBOLT
    Type TYPE_ELECTRIC
    Quality MOVE_QUALITY_DAMAGE_INFLICT
    Category MOVE_CATEGORY_SPECIAL
    Power 95
    Accuracy 100
    PP 15
    Priority 0
    Hits 0, 0
    Inflicts CONDITION_PARALYSIS, 10, 1, 0, 0
    CritStage 0
    FlinchChance 0
    Effect BATTLE_EFFECT_PARALYZE_HIT
    DrainHeal 0, 0
    Target MOVE_TARGET_SELECTED
    StatChanges
    Marker
    Flags MOVE_FLAG_PROTECT | MOVE_FLAG_MIRROR_MOVE
```

`Effect` is the battle effect the move runs (`BATTLE_EFFECT_*`), and `Inflicts` the condition it inflicts with its
chance and duration. `StatChanges` takes up to three changes as `statN=`, `stagesN=` and `chanceN=`, with the stats
of `BATTLEMON_*_STAGE`. `Quality` is the class of the move's effect (`MOVE_QUALITY_*`), `Target` which Pokémon it
targets (`MOVE_TARGET_*`) and `Flags` its properties (`MOVE_FLAG_*`), such as contact, sound or being blocked by
Protect; `include/constants/battle.h` names each after the moves that have it. Both versions have the same move
data.

## Trainers

A trainer is an entry of `a/0/9/1` and its party the entry of `a/0/9/2` with the same ID. Both come from one file,
`data/trainers/NNNN_name.s`, named after the trainer's ID and name: the file's `.trainer` section goes into the first
archive and its `.party` section into the second (`ARCHIVES` names the section of each).

```
#include "asm/trainer.inc"

// Elite Four Shauntal
    Trainer class=TRAINER_CLASS_ELITE_FOUR_SHAUNTAL, party=PARTY_MOVES | PARTY_ITEMS, item1=ITEM_FULL_RESTORE, ai=AI_FLAG_BASIC | AI_FLAG_EVAL_ATTACK | AI_FLAG_EXPERT, money=30
    PartyMon level=56, species=SPECIES_COFAGRIGUS, difficulty=200, ability=1, move1=MOVE_WILL_O_WISP, ...
    ...
    PartyMon level=58, species=SPECIES_CHANDELURE, difficulty=250, ability=2, item=ITEM_SITRUS_BERRY, ...
    PartyEnd
```

The macros take keyword arguments, and leave out the ones that are 0. `party` says what each party entry holds besides
the Pokémon: `PARTY_MOVES`, `PARTY_ITEMS`, both, or neither, in which case a Pokémon gets the moves of its level and
no item. `PartyEnd` counts the party for the trainer record. `style` is the battle style (`BTL_STYLE_*`), `ai` the
trainer AI scripts to run (`AI_FLAG_*`, see [Scripts](scripts.md)), `money` a multiplier of the prize money and
`reward` an item given after the battle. A Pokémon's `difficulty` sets its individual values, and `gender` and
`ability` pick them when not 0. `class` is a `TRAINER_CLASS_*` from `data/constants/trainer_classes.txt`, named after
the class's name; where several classes share one, after their only trainer, their sex or their ID, as
`TRAINER_CLASS_SCHOOL_KID_F` (`make_constants.py --trainer-classes`). Both versions have the same trainers.

Each trainer's ID, the number of its file, is a `TRAINER_*` from `data/constants/trainers.txt`, which the field
scripts use: its class and name, as `TRAINER_YOUNGSTER_JIMMY`, without the class for the story characters, whose
class is Pokémon Trainer (`TRAINER_CHEREN`), and numbered from `_2` where a class and name repeat, as for rematches
(`make_constants.py --trainers`).

## Wild encounters

`a/1/2/7` holds the wild encounter tables, one per entry, which a zone's header names (`GetZoneEncID`). Their sources
are `data/encounters/NNNN_place.s`, named after the place of that zone. Black 2 and White 2 have different encounters
in many places, so where a table's rates or a group of its slots differ, the file has both under `#ifdef BLACK2`:

```
#include "asm/encounters.inc"

// Striaton City

    EncounterRates grass=0, dark_grass=0, shaking_grass=0, surf=10, rippling_surf=1, fishing=50, rippling_fishing=1
    GrassEncounters
    DarkGrassEncounters
    ShakingGrassEncounters
    SurfEncounters
#ifdef BLACK2
    Encounter SPECIES_BASCULIN, 45, 60
    ...
#else
    Encounter SPECIES_BASCULIN, 45, 60, form=1
    ...
#endif
    ...
    EncountersEnd
```

A table (`EncData`) has a rate for each group of slots and then the slots: 12 each for grass, dark grass and shaking
grass, and 5 each for surfing, rippling water, fishing and rippling fishing. A slot is a species, its lowest and
highest level, and its form. A group fills the slots it doesn't list with empty ones, and listing too many is an
error. A table with four seasons has four of these, for spring, summer, autumn and winter.

## Zone headers

`a/0/1/2` holds one 0x30-byte header per zone, all 615 in a single entry, so their source is one file,
`data/zones/0000_zone_headers.s`, with a `ZoneHeader` line per zone in order and the place's name in a comment:

```
// Zone 1: Black City
    ZoneHeader map_type=16, npc_cache=24, area=283, matrix=13, scripts=2, text=4, bgm=SEQ_BGM_POKECEN, ...
```

A header names the zone's map, its scripts and script messages, its music for each season (`SEQ_*`, or `bgm` for all
four), its wild encounters (the number of a `data/encounters/` file), its place name, its default weather and camera,
its battle background, what it allows (cycling, Escape Rope, flying from it), and where flying lands.
`include/asm/zone_header.inc` describes each argument and the code that reads it. Arguments left out take the value
most zones have. Both versions have the same zone headers.
