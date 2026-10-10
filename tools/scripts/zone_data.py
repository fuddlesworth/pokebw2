#!/usr/bin/env python3
"""Write the zone headers (archive a/0/1/2) as an editable source, one ZoneHeader line per zone, with the macro of
include/asm/zone_header.inc. The archive holds all 615 headers in one member, so they are one file. The build
assembles it back and checks that it matches.

    zone_data.py extract/b2_us/files data/zones
"""
import argparse
import struct
from pathlib import Path

from msgdata import read_archive_file
from narc import read_narc
from gen_constants import constant_names, name

HEADER_SIZE = 0x30
PLACE_NAMES = 109
# Values most zones have, which the macro takes when an argument is left out
DEFAULTS = {"encounters": 0x1FFF, "flag11": 1, "cam_bound": 0xFFFF}


def zone_line(header: bytes, sequences: dict[int, str]) -> str:
    (map_type, npc_cache, area, matrix, scripts, init_scripts, text, spring, summer, autumn, winter, enc, entities,
     parent, place, env, flags, cam_bound, icon, fly_x, fly_y, fly_z) = struct.unpack("<BBHHHHH4HHHHHHHHHiii", header)
    fields = {
        "map_type": map_type, "npc_cache": npc_cache, "area": area, "matrix": matrix, "scripts": scripts,
        "text": text,
    }
    if init_scripts != scripts + 1:
        fields["init_scripts"] = init_scripts
    if spring == summer == autumn == winter:
        fields["bgm"] = name(sequences, spring)
    else:
        fields.update(bgm_spring=name(sequences, spring), bgm_summer=name(sequences, summer),
                      bgm_autumn=name(sequences, autumn), bgm_winter=name(sequences, winter))
    fields.update(
        encounters=enc & 0x1FFF, enc_slot=enc >> 13, entities=entities, parent=parent, place=place & 0x3FF,
        place_display=place >> 10, weather=env & 0x3F, projection=env >> 6 & 7, camera=env >> 9,
        transition=flags & 0x1F, battle_bg=flags >> 5 & 0x1F, cycling=flags >> 10 & 1, flag11=flags >> 11 & 1,
        escape_rope=flags >> 12 & 1, fly_from=flags >> 13 & 1, cycle_surf_bgm=flags >> 14 & 1,
        entralink_warp=flags >> 15, cam_bound=cam_bound, name_icon=icon & 0x1FFF, easy_level=icon >> 13,
        fly_x=fly_x, fly_y=fly_y, fly_z=fly_z,
    )
    args = []
    for key, value in fields.items():
        if isinstance(value, str) or value != DEFAULTS.get(key, 0) or key in ("scripts", "text"):
            shown = f"{value:#x}" if key in ("encounters", "cam_bound") and isinstance(value, int) else value
            args.append(f"{key}={shown}")
    return f"    ZoneHeader {', '.join(args)}"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", type=Path, help="the extracted files/ directory")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    (data,) = read_narc((args.files / "a/0/1/2").read_bytes())
    places = read_archive_file(args.files / "a/0/0/2", PLACE_NAMES)
    sequences = constant_names("sound.h", "SEQ_")
    lines = ['#include "asm/zone_header.inc"', ""]
    for zone in range(len(data) // HEADER_SIZE):
        header = data[HEADER_SIZE * zone:HEADER_SIZE * (zone + 1)]
        place = struct.unpack_from("<H", header, 0x1A)[0] & 0x3FF
        lines.append(f"// Zone {zone}: {places[place] if place < len(places) else place}")
        lines.append(zone_line(header, sequences))
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "0000_zone_headers.s").write_text("\n".join(lines) + "\n")
    print(f"wrote {len(data) // HEADER_SIZE} zones to {args.output}")


if __name__ == "__main__":
    main()
