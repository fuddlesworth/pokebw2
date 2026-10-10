#!/usr/bin/env python3
"""Write the constant lists in data/constants/ for moves, abilities, items, species and types, named after the game's
own text, for sound sequences, named after the sound archive's symbols, and for trainer classes. This bootstraps the
lists from the ROM once; after that the committed lists are the source of truth, edited by hand, and the build
generates the headers from them with gen_constants.py.

    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --sdat extract/b2_us/files/swan_sound_data.sdat
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --trainer-classes extract/b2_us
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --trainers extract/b2_us

Names are the English names in upper case, with words split at spaces, hyphens and capitals inside a word, so that
"ThunderPunch" becomes MOVE_THUNDER_PUNCH. Items named "???" are unused and get no constant.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from msgdata import read_archive_file  # noqa: E402

# (list, prefix, file in the system message archive, number of IDs or None for all lines, description)
TABLES = [
    ("moves.txt", "MOVE", 403, None, "Moves"),
    ("abilities.txt", "ABILITY", 374, None, "Abilities"),
    ("items.txt", "ITEM", 64, None, "Items"),
    # The lines after the national Pokédex name eggs and other things that are not species
    ("species.txt", "SPECIES", 90, 650, "Species, by national Pokédex number"),
    ("types.txt", "TYPE", 398, None, "Types"),
]

# Names for the IDs whose text is not a name, or is shared with another ID. The item descriptions tell them apart.
OVERRIDES = {
    "MOVE": {0: "NONE"},
    "ABILITY": {0: "NONE"},
    "ITEM": {
        0: "NONE",
        621: "XTRANSCEIVER_MALE",
        626: "XTRANSCEIVER_FEMALE",
        628: "DNA_SPLICERS_FUSE",
        629: "DNA_SPLICERS_SEPARATE",
        636: "DROPPED_ITEM_MALE",
        637: "DROPPED_ITEM_FEMALE",
    },
    "SPECIES": {0: "NONE"},
}

# The font draws the female and male signs with these characters
CHARACTERS = {"é": "e", "⑮": "_F", "⑭": "_M"}


def identifier(name: str) -> str:
    for char, replacement in CHARACTERS.items():
        name = name.replace(char, replacement)
    name = re.sub(r"(?<=[a-z])(?=[A-Z])", "_", name)
    name = re.sub(r"['.]", "", name)
    name = re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_")
    return name.upper()


def write_list(path: Path, names: dict[int, str], description: str, hexadecimal: bool = False):
    """Writes a gen_constants.py list: a value is only written where it does not follow the previous name's."""
    lines = [f"# {description} (bootstrapped from the ROM by make_constants.py)"]
    previous = None
    for i, name in sorted(names.items()):
        value = f" = {i:#x}" if hexadecimal else f" = {i}"
        lines.append(f"{name}{'' if previous is not None and i == previous + 1 else value}")
        previous = i
    path.write_text("\n".join(lines) + "\n")


def sound_sequence_names(sdat: bytes) -> list[str | None]:
    """Returns the names of the sequences in a sound archive's symbol block, by sequence number."""
    symb_offset, symb_size = struct.unpack_from("<II", sdat, 0x10)
    symb = sdat[symb_offset:symb_offset + symb_size]
    record = struct.unpack_from("<I", symb, 8)[0]
    names = []
    for i in range(struct.unpack_from("<I", symb, record)[0]):
        offset = struct.unpack_from("<I", symb, record + 4 + 4 * i)[0]
        names.append(symb[offset:symb.index(b"\0", offset)].decode() if offset else None)
    return names


def write_sound_list(path: Path, sdat: bytes):
    names = {i: name for i, name in enumerate(sound_sequence_names(sdat)) if name}
    write_list(path, names, "Sound sequences, with the names that the sound archive's symbols give them",
               hexadecimal=True)


# Files of the system message archive with the trainers' names and the trainer classes' names
TRAINER_NAMES = 382
TRAINER_CLASS_NAMES = 383
# Black 2's table of trainer classes that TrainerClass_GetSex reads, 4 bytes per class with the sex in the second
TRAINER_CLASS_TABLE = 0x02092394


def trainer_class_names(extract: Path) -> dict[int, str]:
    """Returns the name of each trainer class. A class name the game uses for several classes gets the name of the one
    trainer of the class, then the sex the game gives the class (_M or _F), then the class ID, until it is unique."""
    import yaml
    from narc import read_narc

    files = extract / "files"
    classes = [identifier(n.replace("⒆⒇", "Pkmn")) or "NONE" for n in
               read_archive_file(files / "a/0/0/2", TRAINER_CLASS_NAMES)]
    trainer_names = read_archive_file(files / "a/0/0/2", TRAINER_NAMES)
    users: dict[int, set[str]] = {}
    for i, trainer in enumerate(read_narc((files / "a/0/9/1").read_bytes())):
        if len(trainer) == 20:
            if identifier(trainer_names[i]):
                users.setdefault(trainer[1], set()).add(identifier(trainer_names[i]))
    arm9 = (extract / "arm9" / "arm9.bin").read_bytes()
    base = yaml.safe_load((extract / "arm9" / "arm9.yaml").read_text())["base_address"]
    sexes = [arm9[TRAINER_CLASS_TABLE - base + 4 * c + 1] for c in range(len(classes))]

    names = dict(enumerate(classes))

    def duplicates():
        groups: dict[str, list[int]] = {}
        for c, name in names.items():
            groups.setdefault(name, []).append(c)
        return [group for group in groups.values() if len(group) > 1]

    for group in duplicates():
        for c in group:
            if len(users.get(c, ())) == 1:
                names[c] += "_" + next(iter(users[c]))
    for group in duplicates():
        if len({sexes[c] for c in group}) > 1:
            for c in group:
                names[c] += "_F" if sexes[c] else "_M"
    for group in duplicates():
        for c in group:
            names[c] += f"_{c}"
    return names


# The class of the rivals and other story characters, which their names say enough without
GENERIC_CLASS = "PKMN_TRAINER"


def trainer_names(extract: Path) -> dict[int, str]:
    """Returns the name of each trainer: its class and its name, as TRAINER_YOUNGSTER_JIMMY, without the class for
    story characters (TRAINER_CHEREN), and with the ID for one without a name. Trainers with the same class and name,
    such as rematches, get _2, _3 and so on in ID order."""
    from narc import read_narc

    files = extract / "files"
    classes = [identifier(n.replace("⒆⒇", "Pkmn")) for n in read_archive_file(files / "a/0/0/2", TRAINER_CLASS_NAMES)]
    texts = read_archive_file(files / "a/0/0/2", TRAINER_NAMES)
    names: dict[int, str] = {}
    count: dict[str, int] = {}
    for i, trainer in enumerate(read_narc((files / "a/0/9/1").read_bytes())):
        if i == 0:
            names[i] = "TRAINER_NONE"
            continue
        trainer_class = classes[trainer[1]]
        name = identifier(texts[i])
        parts = [] if trainer_class == GENERIC_CLASS and name else [trainer_class]
        parts.append(name or str(i))
        base = "TRAINER_" + "_".join(parts)
        count[base] = count.get(base, 0) + 1
        names[i] = base if count[base] == 1 else f"{base}_{count[base]}"
    if len(set(names.values())) != len(names):
        sys.exit("two trainers have the same name")
    return names


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("archive", type=Path, help="the system message archive, files/a/0/0/2")
    parser.add_argument("output", type=Path, help="the directory for the lists, data/constants")
    parser.add_argument("--sdat", type=Path, help="the sound archive, files/swan_sound_data.sdat, for sound.txt")
    parser.add_argument("--trainer-classes", type=Path, metavar="EXTRACT",
                        help="write only trainer_classes.txt, from an extracted version such as extract/b2_us")
    parser.add_argument("--trainers", type=Path, metavar="EXTRACT",
                        help="write only trainers.txt, from an extracted version such as extract/b2_us")
    args = parser.parse_args()
    if args.trainer_classes:
        names = {c: f"TRAINER_CLASS_{name}" for c, name in trainer_class_names(args.trainer_classes).items()}
        write_list(args.output / "trainer_classes.txt", names, "Trainer classes, told apart by trainer, sex or ID")
    if args.trainers:
        write_list(args.output / "trainers.txt", trainer_names(args.trainers),
                   "Trainers, by class and name, numbered where they repeat")
    if args.trainer_classes or args.trainers:
        return
    if args.sdat:
        write_sound_list(args.output / "sound.txt", args.sdat.read_bytes())

    for list_file, prefix, file, count, description in TABLES:
        lines = read_archive_file(args.archive, file)[:count]
        overrides = OVERRIDES.get(prefix, {})
        names = {}
        for i, text in enumerate(lines):
            if i in overrides:
                names[i] = f"{prefix}_{overrides[i]}"
            elif text != "???":
                names[i] = f"{prefix}_{identifier(text)}"
        seen = {}
        for i, name in names.items():
            if name in seen:
                sys.exit(f"{name} is both {seen[name]} and {i}, add an override")
            seen[name] = i
        write_list(args.output / list_file, names, description)


if __name__ == "__main__":
    main()
