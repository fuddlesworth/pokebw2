#!/usr/bin/env python3
"""The trainers in data/trainers/, one JSON file per trainer with its party, as pokeplatinum's res/trainers/data/: dump
writes them from the ROM once, and pack builds the trainer (a/0/9/1) and party (a/0/9/2) archives from them, which the
build does.

    trainer_data.py dump extract/b2_us/files data/trainers
    trainer_data.py pack data/trainers TRAINERS PARTIES

Each trainer is data/trainers/<trainer>.json, named after its constant (smasher_elena.json for
TRAINER_SMASHER_ELENA), and the archives have them in the order of data/constants/trainers.txt. TRAINER_NONE has no
file: its entries are the empty placeholder the game has, a 16-byte record and a 6-byte party. trainer.schema.json
describes each field.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from gen_constants import load as load_list  # noqa: E402
from narc import read_narc, write_narc  # noqa: E402

TRAINER = struct.Struct("<4B4HIBBH")
assert TRAINER.size == 20
MON = struct.Struct("<4BHH")
NONE_TRAINER, NONE_PARTY = bytes(16), bytes(6)
PARTY_MOVES, PARTY_ITEMS = 1, 2


def file_name(trainer: str) -> str:
    return trainer.removeprefix("TRAINER_").lower() + ".json"


def ordered_trainers() -> list[str]:
    trainers = sorted(load_list("trainers").items(), key=lambda item: item[1])
    if [v for _, v in trainers] != list(range(len(trainers))) or trainers[0][0] != "TRAINER_NONE":
        sys.exit("trainers.txt doesn't number the trainers from TRAINER_NONE = 0 in order")
    return [trainer for trainer, _ in trainers]


def trainer_json(trainer: bytes, party: bytes) -> dict:
    kind, class_, style, count, *items, ai, heals, money, reward = TRAINER.unpack(trainer)
    size = MON.size + (2 if kind & PARTY_ITEMS else 0) + (8 if kind & PARTY_MOVES else 0)
    if len(party) != size * count or heals > 1:
        raise ValueError("unexpected trainer record")
    mons = []
    for i in range(count):
        entry = party[size * i:size * (i + 1)]
        difficulty, gender_ability, level, pad, species, form = MON.unpack_from(entry)
        if pad:
            raise ValueError("party padding byte set")
        mon = {"species": name("SPECIES_", species), "form": form, "level": level, "difficulty": difficulty,
               "gender": gender_ability & 0xF, "ability": gender_ability >> 4}
        offset = MON.size
        if kind & PARTY_ITEMS:
            mon["item"] = name("ITEM_", struct.unpack_from("<H", entry, offset)[0])
            offset += 2
        if kind & PARTY_MOVES:
            mon["moves"] = [name("MOVE_", m) for m in struct.unpack_from("<4H", entry, offset) if m]
        mons.append(mon)
    return {
        "$schema": "trainer.schema.json",
        "class": name("TRAINER_CLASS_", class_),
        "battle_style": name("BTL_STYLE_", style),
        "items": [name("ITEM_", item) for item in items if item],
        "ai_flags": [name("AI_FLAG_", 1 << bit) for bit in range(32) if ai >> bit & 1],
        "heals": bool(heals),
        "money": money,
        "reward": name("ITEM_", reward),
        "party": mons,
    }


def dump(files: Path, output: Path):
    trainers = read_narc((files / "a/0/9/1").read_bytes())
    parties = read_narc((files / "a/0/9/2").read_bytes())
    names = ordered_trainers()
    if not len(trainers) == len(parties) == len(names) or (trainers[0], parties[0]) != (NONE_TRAINER, NONE_PARTY):
        sys.exit("the archives don't have one entry per trainer of trainers.txt, from the empty TRAINER_NONE")
    for trainer_name, trainer, party in list(zip(names, trainers, parties))[1:]:
        write(output / file_name(trainer_name), trainer_json(trainer, party))
    print(f"wrote {len(trainers) - 1} trainers to {output}")


def trainer_bytes(data: dict, where: str) -> tuple[bytes, bytes]:
    party = data["party"]
    has_items = {"item" in mon for mon in party}
    has_moves = {"moves" in mon for mon in party}
    if len(has_items) > 1 or len(has_moves) > 1:
        raise DataError(f"{where}: either every Pokémon of the party has an item, or none; the same for moves")
    kind = (PARTY_ITEMS if True in has_items else 0) | (PARTY_MOVES if True in has_moves else 0)
    items = [value(item, where) for item in data["items"]] + [0] * (4 - len(data["items"]))
    ai = 0
    for flag in data["ai_flags"]:
        ai |= value(flag, where)
    record = TRAINER.pack(kind, value(data["class"], where), value(data["battle_style"], where), len(party), *items,
                          ai, data["heals"], data["money"], value(data["reward"], where))
    entries = []
    for mon in party:
        entry = MON.pack(mon["difficulty"], mon["gender"] | mon["ability"] << 4, mon["level"], 0,
                         value(mon["species"], where), mon["form"])
        if kind & PARTY_ITEMS:
            entry += struct.pack("<H", value(mon["item"], where))
        if kind & PARTY_MOVES:
            moves = [value(move, where) for move in mon["moves"]]
            entry += struct.pack("<4H", *moves, *[0] * (4 - len(moves)))
        entries.append(entry)
    return record, b"".join(entries)


def pack(root: Path, trainers_output: Path, parties_output: Path):
    schema = load_schema(root / "trainer.schema.json")
    trainers, parties = [NONE_TRAINER], [NONE_PARTY]
    for trainer_name in ordered_trainers()[1:]:
        path = root / file_name(trainer_name)
        record, party = trainer_bytes(load(path, schema), label(path))
        trainers.append(record)
        parties.append(party)
    for output, members in ((trainers_output, trainers), (parties_output, parties)):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/trainers/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the trainer and party archives from data/trainers/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE", help="the trainer and party archives")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, *args.outputs)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
