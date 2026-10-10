#!/usr/bin/env python3
"""Write the species data ("personal" data, archive a/0/1/6) as editable sources, one file per species or form, with
the macros of include/asm/personal.inc. The build assembles them back into the archive and checks that it matches.

    personal_data.py extract/b2_us/files/a/0/1/6 data/personal

Records 1 to 649 are the species by national Pokédex number and record 0 is empty. Records 685 to 708 are alternate
forms, which their species' Forms field points at; records 650 to 684 are extra ones that no Forms field points at.
The last member is a table of 16-bit values, kept as it is until its meaning is known.
"""
import argparse
import re
import struct
from pathlib import Path

from gen_constants import header_text
from narc import read_narc

RECORD_SIZE = 0x4C
SPECIES_COUNT = 650


def constant_names(header: str, prefix: str) -> dict[int, str]:
    """Returns the first name of each value of the constants with a prefix in a constants header."""
    names = {}
    for match in re.finditer(rf"^#define ({prefix}\w+) (0x[0-9a-fA-F]+|\d+)(?:\s*//.*)?$",
                             header_text(f"constants/{header}"), re.MULTILINE):
        names.setdefault(int(match.group(2), 0), match.group(1))
    return names


def flag_names(header: str, prefix: str, flags: int) -> str:
    """Returns flags as an OR of the constants with a prefix, defined as (1 << N) in a constants header."""
    names = {}
    for match in re.finditer(rf"^#define ({prefix}\w+) \(1 << (\d+)\)", header_text(f"constants/{header}"), re.M):
        names[int(match.group(2))] = match.group(1)
    parts = [names.get(bit, f"(1 << {bit})") for bit in range(32) if flags >> bit & 1]
    return " | ".join(parts) if parts else "0"


def name(names: dict[int, str], value: int) -> str:
    return names.get(value, str(value))


def machine_names(words: tuple[int, ...]) -> list[str]:
    bits = words[0] | (words[1] << 32) | (words[2] << 64) | (words[3] << 96)
    names = []
    for bit in range(128):
        if bits >> bit & 1:
            names.append(f"TM{bit + 1:02d}" if bit < 95 else f"HM{bit - 94:02d}")
    return names


def write_record(record: bytes, names: dict[str, dict[int, str]], title: str) -> str:
    hp, atk, defense, speed, spatk, spdef, type1, type2, catch_rate, stage, evs = struct.unpack_from("<10BH", record, 0)
    items = struct.unpack_from("<3H", record, 0x0C)
    gender, hatch, friendship, growth, egg1, egg2, ability1, ability2, hidden, flee = struct.unpack_from("<10B",
                                                                                                         record, 0x12)
    first_form, sprite_offset, form_count, color, base_exp, height, weight = struct.unpack_from("<HHBBHHH", record,
                                                                                                0x1C)
    machines = struct.unpack_from("<4I", record, 0x28)
    tutors = struct.unpack_from("<5I", record, 0x38)
    if evs >> 13 or color >> 8:
        raise ValueError(f"{title}: unknown bits set")

    gender_name = {0: "GENDER_RATIO_MALE_ONLY", 254: "GENDER_RATIO_FEMALE_ONLY", 255: "GENDER_RATIO_GENDERLESS"}
    ev_values = [evs >> shift & 3 for shift in range(0, 12, 2)]
    ev_flag = f", underground={evs >> 12}" if evs >> 12 else ""
    color_flags = "".join(f", {flag}=1" for bit, flag in ((6, "asymmetric"), (7, "palette_forms")) if color >> bit & 1)
    lines = [
        '#include "asm/personal.inc"',
        "",
        f"// {title}",
        f"    BaseStats {hp}, {atk}, {defense}, {speed}, {spatk}, {spdef}",
        f"    Types {name(names['type'], type1)}, {name(names['type'], type2)}",
        f"    CatchRate {catch_rate}",
        f"    EvolutionStage {stage}",
        f"    EvYields {', '.join(map(str, ev_values))}{ev_flag}",
        f"    HeldItems {', '.join(name(names['item'], item) for item in items)}",
        f"    GenderRatio {gender_name.get(gender, gender)}",
        f"    HatchCycles {hatch}",
        f"    BaseFriendship {friendship}",
        f"    GrowthRate {name(names['growth'], growth)}",
        f"    EggGroups {name(names['egg'], egg1)}, {name(names['egg'], egg2)}",
        f"    Abilities {', '.join(name(names['ability'], a) for a in (ability1, ability2, hidden))}",
        f"    FleeRate {flee}",
        f"    Forms {first_form}, {sprite_offset}, {form_count}",
        f"    Color {name(names['color'], color & 0x3F)}{color_flags}",
        f"    BaseExp {base_exp}",
        f"    HeightWeight {height}, {weight}",
        f"    Machines {', '.join(machine_names(machines))}".rstrip(),
        f"    Tutors {', '.join(f'{t:#010x}' for t in tutors)}",
        "",
    ]
    return "\n".join(lines)


def record_names(records: list[bytes], species_names: dict[int, str]) -> tuple[dict[int, str], dict[int, str]]:
    """Returns the file name stem and the title of each species record: a species, a form of the species whose Forms
    field points at it, or an extra record. The archives that hold one entry per species record, such as the
    evolutions and the level-up moves, name their files the same way."""
    titles = {}
    files = {}
    for index in range(min(SPECIES_COUNT, len(records))):
        species = species_names[index]
        files[index] = species.removeprefix("SPECIES_").lower()
        titles[index] = species
        first_form, _, count = struct.unpack_from("<HHB", records[index], 0x1C)
        for form in range(1, count if first_form else 0):
            record = first_form + form - 1
            files[record] = f"{files[index]}_form{form}"
            titles[record] = f"{species}, form {form}"
    for index in range(len(records)):
        if index not in files:
            # Records 650 to 684 come after the species, and no species' Forms field points at them
            files[index] = "extra"
            titles[index] = f"Record {index}, which no species' Forms field points at; its use isn't known yet"
    return files, titles


def write_table(data: bytes) -> str:
    values = struct.unpack(f"<{len(data) // 2}H", data)
    lines = ["// A table of 16-bit values after the species records, 999 for none; its meaning isn't known yet", ""]
    for i in range(0, len(values), 16):
        lines.append("    .2byte " + ", ".join(str(v) for v in values[i:i + 16]))
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    members = read_narc(args.archive.read_bytes())
    names = {
        "species": constant_names("species.h", "SPECIES_"),
        "type": constant_names("types.h", "TYPE_"),
        "item": constant_names("items.h", "ITEM_"),
        "ability": constant_names("abilities.h", "ABILITY_"),
        "growth": constant_names("pokemon.h", "GROWTH_"),
        "egg": constant_names("pokemon.h", "EGG_GROUP_"),
        "color": constant_names("pokemon.h", "COLOR_"),
    }
    records = [m for m in members if len(m) == RECORD_SIZE]

    files, titles = record_names(records, names["species"])
    args.output.mkdir(parents=True, exist_ok=True)
    for index, member in enumerate(members):
        if len(member) == RECORD_SIZE:
            text = write_record(member, names, titles[index])
            path = args.output / f"{index:04d}_{files[index]}.s"
        else:
            text = write_table(member)
            path = args.output / f"{index:04d}_table.s"
        path.write_text(text)
    print(f"wrote {len(members)} files to {args.output}")


if __name__ == "__main__":
    main()
