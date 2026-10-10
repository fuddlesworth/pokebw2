#!/usr/bin/env python3
"""Write the wild encounters (archive a/1/2/7) as editable sources, one file per encounter table, with the macros of
include/asm/encounters.inc. Black 2 and White 2 have different encounters in many places; where a table's rates or a
group of its slots differ, the file has both, under #ifdef BLACK2. The build assembles each version's back into its
archive and checks that it matches.

    encounter_data.py extract data/encounters    # reads extract/b2_us and extract/w2_us

Files are named after the place of the zone whose header names the table (GetZoneEncID).
"""
import argparse
import struct
from pathlib import Path

from make_constants import identifier
from msgdata import read_archive_file
from narc import read_narc
from gen_constants import constant_names, name

TABLE_SIZE = 0xE8
GROUPS = [("GrassEncounters", 12), ("DarkGrassEncounters", 12), ("ShakingGrassEncounters", 12),
          ("SurfEncounters", 5), ("RipplingSurfEncounters", 5), ("FishingEncounters", 5),
          ("RipplingFishingEncounters", 5)]
SEASONS = ["Spring", "Summer", "Autumn", "Winter"]
RATE_NAMES = ["grass", "dark_grass", "shaking_grass", "surf", "rippling_surf", "fishing", "rippling_fishing"]
PLACE_NAMES = 109
ZONE_SIZE = 48
NO_ENCOUNTERS = 0x1FFF


def rates_line(table: bytes) -> str:
    args = [f"{n}={v}" for n, v in zip(RATE_NAMES, table[:7])]
    if table[7]:
        args.append(f"flags={table[7]}")
    return f"    EncounterRates {', '.join(args)}"


def slot_lines(table: bytes, offset: int, count: int, species_names: dict[int, str]) -> list[str]:
    lines = []
    # The group fills the slots after the last one it lists with empty ones
    while count and not any(table[offset + 4 * (count - 1):offset + 4 * count]):
        count -= 1
    for i in range(count):
        value, low, high = struct.unpack_from("<HBB", table, offset + 4 * i)
        species, form = value & 0x7FF, value >> 11
        line = f"    Encounter {name(species_names, species)}, {low}, {high}"
        lines.append(line + (f", form={form}" if form else ""))
    return lines


def versioned(black: list[str], white: list[str]) -> list[str]:
    if black == white:
        return black
    return ["#ifdef BLACK2", *black, "#else", *white, "#endif"]


def write_file(black: bytes, white: bytes, species_names: dict[int, str], title: str) -> str:
    if len(black) != len(white):
        raise ValueError(f"{title}: the versions have different numbers of seasons")
    seasons = len(black) // TABLE_SIZE
    lines = ['#include "asm/encounters.inc"', "", f"// {title}"]
    for season in range(seasons):
        b = black[TABLE_SIZE * season:TABLE_SIZE * (season + 1)]
        w = white[TABLE_SIZE * season:TABLE_SIZE * (season + 1)]
        lines.append("")
        if seasons > 1:
            lines.append(f"// {SEASONS[season]}")
        lines += versioned([rates_line(b)], [rates_line(w)])
        offset = 8
        for group, count in GROUPS:
            lines.append(f"    {group}")
            lines += versioned(slot_lines(b, offset, count, species_names), slot_lines(w, offset, count, species_names))
            offset += 4 * count
        lines.append("    EncountersEnd")
    return "\n".join(lines) + "\n"


def table_places(extract: Path) -> dict[int, str]:
    """Returns the place name of each encounter table, from the zones whose headers name it."""
    zones = read_narc((extract / "files/a/0/1/2").read_bytes())[0]
    places = read_archive_file(extract / "files/a/0/0/2", PLACE_NAMES)
    names = {}
    for zone in range(len(zones) // ZONE_SIZE):
        header = zones[ZONE_SIZE * zone:ZONE_SIZE * (zone + 1)]
        table = struct.unpack_from("<H", header, 0x14)[0] & 0x1FFF
        place = struct.unpack_from("<H", header, 0x1A)[0] & 0x3FF
        if table != NO_ENCOUNTERS and place < len(places):
            names.setdefault(table, places[place])
    return names


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("extract", type=Path, help="the directory with the extracted versions, b2_us and w2_us")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    black = read_narc((args.extract / "b2_us/files/a/1/2/7").read_bytes())
    white = read_narc((args.extract / "w2_us/files/a/1/2/7").read_bytes())
    places = table_places(args.extract / "b2_us")
    species_names = constant_names("species.h", "SPECIES_")
    args.output.mkdir(parents=True, exist_ok=True)
    for index, (b, w) in enumerate(zip(black, white, strict=True)):
        place = places.get(index)
        stem = identifier(place).lower() if place else "unused"
        title = place or "No zone's header names this table"
        (args.output / f"{index:04d}_{stem}.s").write_text(write_file(b, w, species_names, title))
    print(f"wrote {len(black)} files to {args.output}")


if __name__ == "__main__":
    main()
