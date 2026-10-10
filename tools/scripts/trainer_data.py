#!/usr/bin/env python3
"""Write the trainers (archive a/0/9/1) and their parties (a/0/9/2) as editable sources, one file per trainer that
holds both, with the macros of include/asm/trainer.inc. The build assembles each file's two sections back into the two
archives and checks that they match.

    trainer_data.py extract/b2_us/files data/trainers

Files are named after the trainer's index and name, and say its class, from the game's text.
"""
import argparse
import re
import struct
from pathlib import Path

from make_constants import identifier
from msgdata import read_archive_file
from narc import read_narc
from gen_constants import constant_names, header_text, name

TRAINER_SIZE = 20
# Files of the system message archive (a/0/0/2) with the trainers' names and their classes' names
TRAINER_NAMES = 382
CLASS_NAMES = 383
STYLES = {0: "BTL_STYLE_SINGLE", 1: "BTL_STYLE_DOUBLE", 2: "BTL_STYLE_TRIPLE", 3: "BTL_STYLE_ROTATION"}
PARTY_KINDS = {0: "0", 1: "PARTY_MOVES", 2: "PARTY_ITEMS", 3: "PARTY_MOVES | PARTY_ITEMS"}


def ai_flag_names(flags: int) -> str:
    names = {}
    for match in re.finditer(r"^#define (AI_FLAG_\w+) \(1 << (\d+)\)$", header_text("constants/tr_ai.h"), re.M):
        names[int(match.group(2))] = match.group(1)
    parts = [names.get(bit, f"(1 << {bit})") for bit in range(32) if flags >> bit & 1]
    return " | ".join(parts) if parts else "0"


def write_trainer(trainer: bytes, party: bytes, names: dict, title: str, class_name: str) -> str:
    kind, class_, style, count = trainer[0:4]
    items = struct.unpack_from("<4H", trainer, 4)
    ai, heals, money, reward = struct.unpack_from("<IBBH", trainer, 0xC)
    args = [f"class={name(names['class'], class_)}"]
    if kind:
        args.append(f"party={PARTY_KINDS[kind]}")
    if style:
        args.append(f"style={STYLES[style]}")
    args += [f"item{i + 1}={name(names['item'], item)}" for i, item in enumerate(items) if item]
    if ai:
        args.append(f"ai={ai_flag_names(ai)}")
    if heals:
        args.append(f"heals={heals}")
    args.append(f"money={money}")
    if reward:
        args.append(f"reward={name(names['item'], reward)}")
    lines = ['#include "asm/trainer.inc"', "", f"// {class_name} {title}", f"    Trainer {', '.join(args)}"]

    size = 8 + (2 if kind & 2 else 0) + (8 if kind & 1 else 0)
    if len(party) != size * count:
        raise ValueError(f"{title}: a party of {len(party)} bytes for {count} Pokémon of {size}")
    for i in range(count):
        entry = party[size * i:size * (i + 1)]
        difficulty, gender_ability, level, pad, species, form = struct.unpack_from("<4BHH", entry, 0)
        if pad:
            raise ValueError(f"{title}: padding byte {pad}")
        mon = [f"level={level}", f"species={name(names['species'], species)}"]
        if form:
            mon.append(f"form={form}")
        if difficulty:
            mon.append(f"difficulty={difficulty}")
        if gender_ability & 0xF:
            mon.append(f"gender={gender_ability & 0xF}")
        if gender_ability >> 4:
            mon.append(f"ability={gender_ability >> 4}")
        offset = 8
        if kind & 2:
            item = struct.unpack_from("<H", entry, offset)[0]
            if item:
                mon.append(f"item={name(names['item'], item)}")
            offset += 2
        if kind & 1:
            moves = struct.unpack_from("<4H", entry, offset)
            mon += [f"move{j + 1}={name(names['move'], m)}" for j, m in enumerate(moves) if m]
        lines.append(f"    PartyMon {', '.join(mon)}")
    lines += ["    PartyEnd", ""]
    return "\n".join(lines)


def write_empty(trainer: bytes, party: bytes) -> str:
    return "\n".join([
        "// No trainer: the record of ID 0, which is shorter than the others",
        '    .section .trainer, "a"',
        f"    .space {len(trainer)}",
        '    .section .party, "a"',
        f"    .space {len(party)}",
        "",
    ])


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", type=Path, help="the extracted files/ directory")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    trainers = read_narc((args.files / "a/0/9/1").read_bytes())
    parties = read_narc((args.files / "a/0/9/2").read_bytes())
    trainer_names = read_archive_file(args.files / "a/0/0/2", TRAINER_NAMES)
    class_names = read_archive_file(args.files / "a/0/0/2", CLASS_NAMES)
    names = {
        "species": constant_names("species.h", "SPECIES_"),
        "item": constant_names("items.h", "ITEM_"),
        "move": constant_names("moves.h", "MOVE_"),
        "class": constant_names("trainer_classes.h", "TRAINER_CLASS_"),
    }
    args.output.mkdir(parents=True, exist_ok=True)
    for index, (trainer, party) in enumerate(zip(trainers, parties, strict=True)):
        if len(trainer) != TRAINER_SIZE:
            if any(trainer) or any(party):
                raise SystemExit(f"trainer {index} is {len(trainer)} bytes and not empty")
            (args.output / f"{index:04d}_none.s").write_text(write_empty(trainer, party))
            continue
        title = trainer_names[index]
        stem = identifier(title).lower() or "unnamed"
        class_name = class_names[trainer[1]].replace("⒆⒇", "Pkmn")
        text = write_trainer(trainer, party, names, title, class_name)
        (args.output / f"{index:04d}_{stem}.s").write_text(text)
    print(f"wrote {len(trainers)} files to {args.output}")


if __name__ == "__main__":
    main()
