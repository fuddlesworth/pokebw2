#!/usr/bin/env python3
"""The battle facilities' trainers and Pokémon in data/facilities/<facility>/, as pokeplatinum's res/trainers/frontier/:
dump writes them from the ROM once, and pack builds a facility's archives from them, which the build does.

    facility_data.py dump extract/b2_us/files data/facilities battle_subway
    facility_data.py pack data/facilities/battle_subway TRAINERS POKEMON

A facility has a trainers archive, each file a trainer class and the Pokémon sets the trainer picks from, and a
Pokémon archive, each file a set (BSubwayPokemonData in include/field/battle_facility.h); FACILITIES says which archives
and message files are a facility's. Its directory holds:

- trainers/<trainer>.json: a trainer's class, its sets by name, and its name and messages, which the text takes with
  \\from{facilities.<facility>.names} and \\from{facilities.<facility>.messages};
- pokemon/<set>.json: a set, named after its species and numbered from 1;
- order.json: the trainers and sets in archive order, which the files' names don't give.

facility.schema.json and set.schema.json, in data/facilities/, describe the fields.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from datajson import ROOT, DataError, label, load, load_schema, name, value, write  # noqa: E402
from make_constants import identifier  # noqa: E402
from narc import read_narc, write_narc  # noqa: E402
from text_data import message_lines  # noqa: E402
from text_sources import to_json  # noqa: E402

SET = struct.Struct("<5HBBHH")
STATS = ["hp", "attack", "defense", "speed", "special_attack", "special_defense"]
DATA_DIR = ROOT / "data/facilities"
# Each facility: its trainers' and Pokémon archives, the system message files of its trainers' names and messages, and
# how many messages a trainer has. The Battle Subway's are also the Trial House's (func_ov012_02162864)
FACILITIES = {
    "battle_subway": {"trainers": "a/2/1/2", "pokemon": "a/2/1/1", "names": 15, "messages": 376, "per_trainer": 3},
}


# Dumping


def set_json(data: bytes) -> dict:
    species, *moves, ev_flags, nature, item, form = SET.unpack(data)
    return {
        "$schema": "../../set.schema.json",
        "species": name("SPECIES_", species),
        "moves": [name("MOVE_", move) for move in moves],
        "ev_flags": {stat: bool(ev_flags >> i & 1) for i, stat in enumerate(STATS)},
        "nature": name("NATURE_", nature),
        "item": name("ITEM_", item),
        "form": form,
    }


def dump(files: Path, output: Path, facility: str):
    config = FACILITIES[facility]
    trainers = read_narc((files / config["trainers"]).read_bytes())
    sets = read_narc((files / config["pokemon"]).read_bytes())
    root = output / facility
    set_names = []
    count: dict[str, int] = {}
    for data in sets:
        if data[10] >> len(STATS):
            sys.exit("a set has EV flags beyond the six stats")
        stem = identifier(name("SPECIES_", SET.unpack(data)[0]).removeprefix("SPECIES_")).lower()
        count[stem] = count.get(stem, 0) + 1
        set_names.append(f"{stem}_{count[stem]}")
        write(root / "pokemon" / f"{set_names[-1]}.json", set_json(data))
    names = message_lines(files / "a/0/0/2", config["names"])
    messages = message_lines(files / "a/0/0/2", config["messages"])
    per = config["per_trainer"]
    trainer_names = []
    count = {}
    for index, data in enumerate(trainers):
        trainer_class, set_count, *picks = struct.unpack(f"<{len(data) // 2}H", data)
        if len(picks) != set_count:
            sys.exit(f"trainer {index} doesn't list its count of sets")
        class_name = name("TRAINER_CLASS_", trainer_class)
        trainer = {"$schema": "../../facility.schema.json", "class": class_name,
                   "pokemon": [set_names[pick] for pick in picks]}
        if index < len(names):
            trainer = {"$schema": trainer.pop("$schema"), "name": to_json(names[index]), **trainer,
                       "messages": [to_json(m) for m in messages[per * index:per * (index + 1)]]}
        base = (str(class_name).removeprefix("TRAINER_CLASS_").lower() + "_" +
                (identifier(names[index]).lower() if index < len(names) else str(index)))
        count[base] = count.get(base, 0) + 1
        trainer_names.append(base if count[base] == 1 else f"{base}_{count[base]}")
        write(root / "trainers" / f"{trainer_names[-1]}.json", trainer)
    write(root / "order.json", {"trainers": trainer_names, "pokemon": set_names})
    print(f"wrote {len(trainers)} trainers and {len(sets)} sets to {root}")


# Packing


def load_facility(facility: str) -> tuple[list[tuple[str, dict]], list[tuple[str, dict]]]:
    """Returns the facility's trainers and sets in archive order, by name, validated."""
    root = DATA_DIR / facility
    order = json.loads((root / "order.json").read_text())
    trainer_schema = load_schema(DATA_DIR / "facility.schema.json")
    set_schema = load_schema(DATA_DIR / "set.schema.json")
    trainers = [(n, load(root / "trainers" / f"{n}.json", trainer_schema)) for n in order["trainers"]]
    sets = [(n, load(root / "pokemon" / f"{n}.json", set_schema)) for n in order["pokemon"]]
    return trainers, sets


def pack(facility: str, trainers_output: Path, pokemon_output: Path):
    trainers, sets = load_facility(facility)
    set_index = {set_name: i for i, (set_name, _) in enumerate(sets)}
    trainer_files = []
    for trainer_name, data in trainers:
        where = label(DATA_DIR / facility / "trainers" / f"{trainer_name}.json")
        picks = []
        for set_name in data["pokemon"]:
            if set_name not in set_index:
                raise DataError(f"{where}: {set_name} is not a set of order.json")
            picks.append(set_index[set_name])
        trainer_files.append(struct.pack(f"<{2 + len(picks)}H", value(data["class"], where), len(picks), *picks))
    set_files = []
    for set_name, data in sets:
        where = label(DATA_DIR / facility / "pokemon" / f"{set_name}.json")
        moves = [value(move, where) for move in data["moves"]]
        ev_flags = sum(1 << i for i, stat in enumerate(STATS) if data["ev_flags"][stat])
        set_files.append(SET.pack(value(data["species"], where), *moves, ev_flags, value(data["nature"], where),
                                  value(data["item"], where), data["form"]))
    for output, members in ((trainers_output, trainer_files), (pokemon_output, set_files)):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def text_lines(facility: str, kind: str) -> list[str]:
    """The names or messages of a facility's trainers that have them, which come first, for the text."""
    trainers, _ = load_facility(facility)
    lines = []
    for trainer_name, data in trainers:
        if "name" not in data:
            break
        lines += [data["name"]] if kind == "names" else data["messages"]
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write a facility's data from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    dump_parser.add_argument("facility", choices=FACILITIES)
    pack_parser = commands.add_parser("pack", help="build a facility's archives")
    pack_parser.add_argument("root", type=Path, help="the facility's directory, such as data/facilities/battle_subway")
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE", help="the trainers and Pokémon archives")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output, args.facility)
        return
    try:
        pack(args.root.name, *args.outputs)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
