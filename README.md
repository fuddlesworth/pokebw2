# Pokémon Black 2 and White 2

A work-in-progress decompilation of Pokémon Black 2 and White 2.
Both USA/Europe ROMs rebuild byte for byte from the same source tree.

## Progress

[![Code](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=matched_code_percent&label=Code)](https://bytematch.dev/fuddlesworth/pokebw2)
[![Functions](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=matched_functions&label=Functions)](https://bytematch.dev/fuddlesworth/pokebw2)
[![Files](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=complete_units&label=Complete%20files)](https://bytematch.dev/fuddlesworth/pokebw2)

Progress updates automatically from CI. [ByteMatch](https://bytematch.dev/fuddlesworth/pokebw2) shows it for each
version, for ARM9 main and the overlays, and for the game's code and each library.

## Build

Requires Linux, WSL or macOS, Python 3.11+, ninja, and LLVM (`clang` and `llvm-objcopy` on your `PATH`).

1. Place your own ROM dumps in `orig/`, using the filenames below.
2. Configure and build:

   ```sh
   python3 configure.py
   ninja
   ```

The first run downloads the remaining tools. Every available version is built and checked against its SHA1.
ROMs are not included or provided.

| ROM (USA/Europe, NDSi Enhanced) | Input file | SHA1 |
| --- | --- | --- |
| Black 2 | `orig/baserom_b2_us.nds` | `e51e6dfb8678a3d19dcd2a10691b96a569ca0abb` |
| White 2 | `orig/baserom_w2_us.nds` | `b5d7490be7b415b8f1e672a53e978a9cc667e56a` |

Output: `build/pokeblack2_us.nds` and/or `build/pokewhite2_us.nds`.
See [Build details](docs/building.md) for macOS setup, toolchain options and modified builds.

## Contribute

Read [CONTRIBUTING.md](CONTRIBUTING.md) for the workflow and rules. Claim an overlay's issue before starting work.
CI compiles every C file for both versions; fork PRs must verify ROM matching locally.

## Documentation

| Guide | Covers |
| --- | --- |
| [Build details](docs/building.md) | Setup and build options |
| [Decompiling](docs/decompiling.md) | Workflow and tools |
| [Matching](docs/matching.md) | MWCC behavior and matching techniques |
| [Code organization](docs/code-organization.md) | Source layout, libraries and names |
| [Native port notes](docs/porting-notes.md) | Recovered platform assumptions and open porting questions |
| [Source files](docs/source-files.md) | Overlay files and boundary evidence |
| [Nonmatching functions](docs/nonmatching-functions.md) | Remaining mismatches |
| [Game data](docs/data.md) | Data formats and archives |
| [Cells and animations as JSON](docs/cells-and-animations.md) | The sprites' cells, animations and MCSS files as JSON |
| [Scripts](docs/scripts.md) | Trainer AI, battle animation and field scripts |
| [Configs and ROMs](docs/configs.md) | dsd configuration and ROM structure |
| [Tools](tools/README.md) | Tool reference |

## Credits

This project builds on the work of others:

- **Names:** [swan](https://github.com/ds-pokemon-hacking/swan) by DS Pokémon Hacking, the source of the symbol,
  type and field names imported here.
- **References:** [pret](https://github.com/pret)'s
  [pokeplatinum](https://github.com/pret/pokeplatinum), [pokeheartgold](https://github.com/pret/pokeheartgold),
  [pokediamond](https://github.com/pret/pokediamond) and [pokeemerald](https://github.com/pret/pokeemerald), whose
  layout, build and conventions this project follows; ntrtwl's
  [NitroSDK](https://github.com/ntrtwl/NitroSDK) and [NitroSystem](https://github.com/ntrtwl/NitroSystem)
  reconstructions; and [aletheia](https://github.com/Project-Holon/aletheia), the Black 1 decompilation, whose map of
  the SDK libraries' objects placed many of our library files.
- **Tools:** [ds-decomp](https://github.com/AetiasHax/ds-decomp) and [ds-rom](https://github.com/AetiasHax/ds-rom) by AetiasHax, used through our forks;
  the MWCC packages from [decomp.aetias.com](https://decomp.aetias.com);
  [objdiff](https://github.com/encounter/objdiff) and [sjiswrap](https://github.com/encounter/sjiswrap) by encounter;
  [wibo](https://github.com/decompals/wibo) by decompals; [decomp-permuter](https://github.com/simonlindholm/decomp-permuter)
  by simonlindholm; [Apicula](https://github.com/scurest/apicula) by scurest, for exporting models; and
  [ByteMatch](https://bytematch.dev) for progress tracking.
- **Data and assets:** the cell and animation converter `nitro_cells.py` by Michael Neuberger, from
  [pokegrey](https://github.com/DeltaDX117/pokegrey); the whole-model writer, new map models and texture imports by
  bjayers, from [Paper White 2](https://github.com/bjayers/paperwhite2); and the per-species folder layout of
  [pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion).

## License

[GNU GPL v3.0](LICENSE), including the symbol names imported from [swan](docs/code-organization.md#names).
