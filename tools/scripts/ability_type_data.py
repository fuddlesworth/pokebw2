#!/usr/bin/env python3
"""Write the abilities' and types' text as JSON, one file per ability and per type, from the ROM's text, once:

    ability_type_data.py extract/b2_us/files data

data/abilities/<ability>.json has an ability's name and description, and data/types/<type>.json a type's name, named
after their constants. They have no archive of their own: the text files take them with \\from{abilities.name} and the
like (see text_sources.py), which validates them against ability.schema.json and type.schema.json.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from datajson import write  # noqa: E402
from text_data import message_lines  # noqa: E402
from text_sources import ordered, to_json  # noqa: E402

# Files of the system messages (a/0/0/2) with the abilities' names and descriptions and the types' names
ABILITY_NAMES, ABILITY_DESCRIPTIONS, TYPE_NAMES = 374, 375, 398


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", type=Path, help="the extracted files/ directory")
    parser.add_argument("output", type=Path, help="the data directory")
    args = parser.parse_args()
    text = args.files / "a/0/0/2"
    names, descriptions = message_lines(text, ABILITY_NAMES), message_lines(text, ABILITY_DESCRIPTIONS)
    for i, ability in enumerate(ordered("abilities")):
        write(args.output / "abilities" / f"{ability.removeprefix('ABILITY_').lower()}.json",
              {"$schema": "ability.schema.json", "name": to_json(names[i]), "description": to_json(descriptions[i])})
    type_names = message_lines(text, TYPE_NAMES)
    for i, type_name in enumerate(type_names):
        write(args.output / "types" / f"{ordered('types')[i].removeprefix('TYPE_').lower()}.json",
              {"$schema": "type.schema.json", "name": to_json(type_name)})
    print(f"wrote {len(names)} abilities and {len(type_names)} types to {args.output}")


if __name__ == "__main__":
    main()
