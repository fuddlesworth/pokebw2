#!/usr/bin/env python3
"""Write the move data (archive a/0/2/1) as editable sources, one file per move, with the macros of
include/asm/move_data.inc. The build assembles them back into the archive and checks that it matches.

    move_data.py extract/b2_us/files/a/0/2/1 data/moves
"""
import argparse
import struct
from pathlib import Path

from narc import read_narc
from gen_constants import constant_names, flag_names, name

RECORD_SIZE = 0x24


def write_record(record: bytes, names: dict[str, dict[int, str]], title: str) -> str:
    (type_, quality, category, power, accuracy, pp, priority, hits, condition, chance, duration, min_turns, max_turns,
     crit, flinch, effect, drain, heal, target) = struct.unpack_from("<6BbBH6BHbbB", record, 0)
    stats = record[0x15:0x18]
    stages = struct.unpack_from("<3b", record, 0x18)
    chances = record[0x1B:0x1E]
    marker = record[0x1E:0x20]
    flags = struct.unpack_from("<I", record, 0x20)[0]
    if marker != b"SS":
        raise ValueError(f"{title}: no SS marker")

    changes = []
    for i in range(3):
        if stats[i] or stages[i] or chances[i]:
            changes.append(f"stat{i + 1}={name(names['stat'], stats[i])}, stages{i + 1}={stages[i]}, "
                           f"chance{i + 1}={chances[i]}")
    lines = [
        '#include "asm/move_data.inc"',
        "",
        f"// {title}",
        f"    Type {name(names['type'], type_)}",
        f"    Quality {name(names['quality'], quality)}",
        f"    Category {name(names['category'], category)}",
        f"    Power {power}",
        f"    Accuracy {accuracy}",
        f"    PP {pp}",
        f"    Priority {priority}",
        f"    Hits {hits & 0xF}, {hits >> 4}",
        f"    Inflicts {name(names['condition'], condition) if condition else 0}, {chance}, {duration}, {min_turns}, "
        f"{max_turns}",
        f"    CritStage {crit}",
        f"    FlinchChance {flinch}",
        f"    Effect {name(names['effect'], effect)}",
        f"    DrainHeal {drain}, {heal}",
        f"    Target {name(names['target'], target)}",
        f"    StatChanges {', '.join(changes)}".rstrip(),
        "    Marker",
        f"    Flags {flag_names('battle.h', 'MOVE_FLAG_', flags)}",
        "",
    ]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    members = read_narc(args.archive.read_bytes())
    names = {
        "move": constant_names("moves.h", "MOVE_"),
        "type": constant_names("types.h", "TYPE_"),
        "category": constant_names("battle.h", "MOVE_CATEGORY_"),
        "target": constant_names("battle.h", "MOVE_TARGET_"),
        "quality": constant_names("battle.h", "MOVE_QUALITY_"),
        "condition": constant_names("battle.h", "CONDITION_"),
        "effect": constant_names("move_effects.h", "BATTLE_EFFECT_"),
        "stat": {k: v for k, v in constant_names("battle.h", "BATTLEMON_").items() if v.endswith("_STAGE")},
    }
    args.output.mkdir(parents=True, exist_ok=True)
    for index, member in enumerate(members):
        if len(member) != RECORD_SIZE:
            raise SystemExit(f"move {index} is {len(member)} bytes")
        move = names["move"].get(index, f"MOVE_{index}")
        path = args.output / f"{index:04d}_{move.removeprefix('MOVE_').lower()}.s"
        path.write_text(write_record(member, names, move))
    print(f"wrote {len(members)} files to {args.output}")


if __name__ == "__main__":
    main()
