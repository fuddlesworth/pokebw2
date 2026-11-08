# Cells and animations as JSON

The game's animated 2D graphics are NitroSystem files: cells, cell animations, multi-cells and multi-cell animations,
and for the Pokémon's battle sprites an MCSS file that says how to draw each cell. `tools/data/nitro_cells.py` reads each
of them as JSON and writes it back byte for byte; this page describes that JSON. The Nitro files stay the sources under
`res/`, since their JSON is about thirty times their size: to edit one, replace it with its JSON, and the build packs it
from there.

```sh
python3 tools/data/nitro_cells.py edit res/pokemon/bulbasaur/front_multi_cells.nmcr   # replace it with its JSON
python3 tools/data/nitro_cells.py dump FILE [OUTPUT.json]                              # print or write the JSON
python3 tools/data/nitro_cells.py pack INPUT.json FILE                                 # write the file from the JSON
python3 tools/data/nitro_cells.py check                                                # every file under res/ and back
```

`edit` names the JSON after the file, without its compression: `front_multi_cells.nmcr.json`,
`front_animations.nanr.json` for `front_animations.nanr.lz11`. The packers that read these files
(`pokemon_graphics.py`, `trainer_graphics.py`, `graphics_archive.py` and `item_icon_data.py`) pack a file from its JSON
where it has one, and stop at a file that has both. A JSON that packs to the game's file keeps the game's bytes; an
edited one is compressed again where the game reads it compressed (`GFL_G2DIOReadNANR` with `TRUE` in
`src/system/mcss.c`). `git checkout` of the file, with the JSON removed, undoes `edit`. Run `python3 configure.py`
after `edit`, as after adding any source file. A compressed file (`*.nanr.lz11`) is read decompressed, and `pack` writes
it uncompressed. All 15,343 such files under `res/`, the Pokémon's, the trainers' and the menus', come back byte for
byte. The trainers' `mcss.bin` has another layout, which this doesn't read.

`nitro_cells.py` was written by Michael Neuberger for [pokegrey](https://github.com/DeltaDX117/pokegrey).

## How a battle sprite is drawn

Each Pokémon's front and back sprite is a puppet: parts of one image, `front.png` or `back.png`, each moved by its own
animation. MCSS (`src/system/mcss.c`) draws it each frame through four files, from the outside in:

```
front_multi_animations.nmar   which pose to show, and for how long
        │  frame content "index" = a multi-cell
        ▼
front_multi_cells.nmcr        a pose: its parts, each a cell animation sequence at a position
        │  node "sequence" = a sequence of the cell animations
        ▼
front_animations.nanr.lz11    a part's motion: frames of a cell, moved, rotated and scaled
        │  frame content "index" = a cell
        ▼
front_info.bin (MCSS)         a cell: the quad of the image that it draws
```

`front_cells.ncer` has the same cells as OAMs, NitroSystem's sprite tiles. NitroSystem's multi-cell animation needs it,
but MCSS draws each cell from the MCSS file's quads, not from the OAMs, so a part's shape is its quad. The trainers' and
the menus' animated graphics have no MCSS file: they are drawn from their cells' OAMs.

Bulbasaur's front sprite, for example: its multi-cell animation has one sequence of four frames, 72, 72, 72 and 90 frames
long, showing multi-cell 0 three times and then multi-cell 1. Each multi-cell has ten nodes, one per part (head, legs,
feet, body, the bulb), and each node plays a sequence of the cell animations, which nudges its cell a pixel at a time.

## Common rules

- Every file has `"kind"`: `"cells"`, `"animations"`, `"multi_cells"`, `"multi_animations"`, `"mcss"` or
  `"musical_mcss"`.
- Every list is in the file's order, and an index means a place in a list, counted from 0.
- A field that has its usual value is left out, and `pack` gives it that value; each section lists them.
- `"labels"`, where a file has them, are the names that NitroSystem's converter kept (`head`, `leg_FR`, `CellAnime0`). No
  code in the game reads them, so they only document the file; their count needn't match anything.
- `"extension"` is the file's user extension block, as words: `[1]` or `[0]` in the game's files. Nothing in the game
  reads it.

| Unit | Meaning |
| --- | --- |
| pixels | positions and sizes in the Nitro files |
| 4096ths of a pixel | everything in an MCSS file: 4096 is one pixel, so `131072` is 32 pixels |
| 4096ths | a scale: 4096 is 1.0, 2048 half size, 8192 double |
| 65536ths of a turn | a rotation: 16384 is a quarter turn |
| frames | a duration, in the game's 60 frames a second, which MCSS can slow down or speed up |

## Cells (`.ncer`)

```json
{
    "kind": "cells",
    "mapping": "2d",
    "bounds": false,
    "cells": [
        {
            "radius": 14,
            "oams": [
                { "x": -28, "y": -27, "width": 32, "height": 16, "character": 8, "priority": 0, "palette": 0,
                  "affine": true, "double_size": true }
            ]
        }
    ],
    "labels": [ "head", "leg_FR" ],
    "extension": [ 1 ]
}
```

| Field | Meaning |
| --- | --- |
| `mapping` | How the cells' characters are laid out in VRAM: `"1d_32k"`, `"1d_64k"`, `"1d_128k"`, `"1d_256k"` or `"2d"` |
| `bounds` | Whether each cell has a bounding rectangle, `"bounds": { "max_x", "max_y", "min_x", "min_y" }` |
| `radius` | The cell's bounding circle, in 4-pixel units (`NNSi_G2dGetCellBoundingSphereR`) |
| `flags` | The cell's other attribute bits, left out when 0 |
| `oams` | The cell's sprite tiles, drawn together |

An OAM's fields:

| Field | Meaning | Left out when |
| --- | --- | --- |
| `x`, `y` | Its top left corner from the cell's origin, in pixels (-256 to 255, -128 to 127) | |
| `width`, `height` | Its size in pixels: 8×8, 16×16, 32×32, 64×64, 16×8, 32×8, 32×16, 64×32, 8×16, 8×32, 16×32 or 32×64 | |
| `character`, `priority`, `palette` | Its first tile in the character data, its priority 0 to 3, its palette 0 to 15 | |
| `affine` | Whether it is drawn through an affine matrix, `matrix` 0 to 31, with `double_size` | false |
| `hidden` | Hidden, for an OAM that isn't affine | false |
| `flip_h`, `flip_v` | Flipped, for an OAM that isn't affine | false |
| `unused` | The second attribute's bits 9 to 11, for an OAM that isn't affine | 0 |
| `mode` | `"normal"`, `"translucent"`, `"window"` or `"bitmap"` | `"normal"` |
| `mosaic`, `colors_256` | Mosaic, and 256 colors instead of 16 | false |

## Cell and multi-cell animations (`.nanr`, `.nmar`)

Both are banks of sequences, each a list of frames. In a cell animation a frame's `index` is a cell; in a multi-cell
animation it is a multi-cell.

```json
{
    "kind": "animations",
    "sequences": [
        {
            "element": "index_t",
            "play_mode": "forward_loop",
            "loop_start": 0,
            "frames": [
                { "duration": 6, "content": { "index": 0, "x": 0, "y": 0 } },
                { "duration": 6, "content": { "index": 0, "x": 0, "y": 1 } }
            ]
        }
    ],
    "labels": [ "head", "leg_FR" ],
    "extension": [ 1 ]
}
```

| Field | Meaning |
| --- | --- |
| `element` | What each frame's content holds: `"index"`, `"index_t"` (and a translation) or `"index_srt"` (and a scale, rotation and translation) |
| `play_mode` | `"forward"` (once), `"forward_loop"`, `"reverse"` (forward then back, once) or `"reverse_loop"` |
| `loop_start` | The frame a loop goes back to |
| `frames` | Each frame's `duration` in frames and its `content` |
| `kind` | The sequence's animation kind, left out when it is the file's: 1 in a cell animation, 2 in a multi-cell one |

A frame's content, by the sequence's `element`:

| `element` | `content` |
| --- | --- |
| `"index"` | `{ "index" }` |
| `"index_t"` | `{ "index", "x", "y" }`, the translation in pixels; `"unused"` only where the padding isn't the usual `0xbeef` |
| `"index_srt"` | `{ "index", "rotation", "scale_x", "scale_y", "x", "y" }`, rotation in 65536ths of a turn, scale in 4096ths |

```json
{ "duration": 7, "content": { "index": 1, "rotation": 0, "scale_x": 4096, "scale_y": 4096, "x": 0, "y": 0 } }
```

The file stores each different content once, in the order frames first use it, so frames with the same content share
it. In 19 of the game's files, such as the Doctor's and Serperior's, the converter shared less than that, and each of
their frames has `"slot"`, its place among the contents, so that they pack as the game has them; frames with the same
slot share it. A new or edited frame needs no slot.

## Multi-cells (`.nmcr`)

```json
{
    "kind": "multi_cells",
    "multi_cells": [
        {
            "cell_animations": 10,
            "nodes": [
                { "sequence": 6, "x": 16, "y": -2, "cell_animation": 0, "play_mode": "reset" },
                { "sequence": 5, "x": 6, "y": 0, "cell_animation": 1, "play_mode": "reset" }
            ]
        }
    ]
}
```

| Field | Meaning |
| --- | --- |
| `cell_animations` | How many cell animations the multi-cell runs at once |
| `nodes` | Its parts, drawn in order |
| `sequence` | The sequence of the cell animations that the node plays |
| `x`, `y` | Where the node is, in pixels from the multi-cell's origin |
| `cell_animation` | Which of the running cell animations plays it, so that two multi-cells can share one part's motion |
| `play_mode` | `"reset"` (the cell animation starts again when the multi-cell is shown) or `"continue"` |
| `flags` | The node's attribute bits 4 to 7, left out when 0x20, which NitroSystem's converter set in every node |

## MCSS files (`*_info.bin`)

The quads that MCSS draws for each cell, and which nodes keep moving while the idle animation stops the others
(`MCSSBin` in `src/system/mcss.c`). All values are in 4096ths of a pixel.

```json
{
    "kind": "mcss",
    "width": 41,
    "height": 41,
    "offset_x": 7,
    "offset_y": 4,
    "cells": [
        {
            "quad": { "x": -4096, "y": 6144, "width": 131072, "height": 98304, "s": 655360, "t": 0 },
            "quad2": { "x": -3840, "y": 4864, "width": 98304, "height": 65536, "s": 0, "t": 0 }
        }
    ],
    "idle_nodes": [],
    "unknown": 0
}
```

| Field | Meaning |
| --- | --- |
| `width`, `height`, `offset_x`, `offset_y` | The size of the sprite's image and its offset from the origin, in pixels |
| `cells` | One per cell, in the cells' order: the cell animations' `index` picks one |
| `quad` | Where the cell is drawn (`x`, `y`, `width`, `height`) and where it is in `front.png` or `back.png` (`s`, `t`, the top left corner) |
| `quad2` | A second quad, drawn before the first, or after it when the sprite is drawn reversed; left out when it is all 0 |
| `idle_nodes` | The nodes that keep animating during the idle animation, or `null` for none at all |
| `unknown` | A byte that `MCSSSys_GetStopCellFlags` returns to the battle, whose meaning isn't known yet |

The image is 256×128 pixels, so `s` and `t` stay below 1048576 and 524288.

## The musical's MCSS files (`musical/*_info.bin`)

The Pokémon Musical's older sprites (`MusicalMcssBin` in `src/ov209/musical_mcss.c`): each cell's quads as above, the
marker cells it carries, which aren't drawn but tell the musical where to put the props, and the markers.

| Field | Meaning |
| --- | --- |
| `cells[].marks` | The markers the cell carries, up to 8, by their place in `marks` |
| `marks` | Each marker's `kind` (which prop it holds), `x` and `y` in 4096ths of a pixel, `rotation` in units of 0x800, and an `unknown` word |

## Editing

- Durations, translations, rotations, scales and play modes can be changed freely, and frames added or removed: the
  contents and offsets are worked out again.
- An `index` must name something that exists: a cell (and its quad) for a cell animation, a multi-cell for a multi-cell
  animation. A node's `sequence` must be a sequence of the cell animations, and `cell_animation` below its multi-cell's
  `cell_animations`.
- MCSS indexes the MCSS file's cells by the same numbers as the `.ncer`'s cells, and every Pokémon's two files have
  the same number of cells, so a cell added to one belongs in the other.
- A quad must stay inside the 256×128 image; changing what a part looks like means drawing it in the PNG and pointing
  the quad at it.
