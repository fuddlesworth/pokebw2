#!/usr/bin/env python3
"""Count a complete file's code and data as matched in an objdiff progress report.

    complete_report.py build/b2_us/report.objdiff.json build/b2_us/report.json

objdiff scores a function by the text of its instructions, symbol names included, so a call that names its target
differently in the original's object and in ours is a mismatch even where both names are one address: a name the
configs haven't caught up with (func_ov167_0219f6ec, where the C calls BtlFaintRec_NextTurn), the runtime's two names
for one routine (_ull_mul and _ll_mul), or a string literal the compiler names (@1776). No option of `report generate`
changes that. For an incomplete file objdiff's score is the evidence there is. A complete file is linked from our
compiled object, though, and both ROMs are checked byte for byte, so each of its functions and each byte of its data
matches. This marks them so, and recomputes the measures of the report and of each category from its units.

The bytes that no source file holds are dsd's gaps (_dsd_gap@MODULE_N), which link the original's bytes. A gap that
holds only what the linker writes is no file to decompile, so it is left out of the report, though it stays in the
link: an autoload's empty static-initializer list, whose terminator starts at the autoload's sinit_start and whose
zero fill reaches its first object, and the zero fill that aligns a module section's end to 32 bytes. Each gap is
placed from the delinks and checked against the object dsd wrote for it; a gap that can't be placed, that holds a
nonzero byte, or that is anywhere else stays in the report as a file still to find.
"""
import argparse
import json
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.decomp.dsd_config import MIGRATED_MODULES, MIGRATED_SECTIONS  # noqa: E402

SECTION_RE = re.compile(r"^\s+(\.[\w.]+)\s+start:0x([0-9a-f]+) end:0x([0-9a-f]+)")
# The autoloads' metadata as dsd extracts it, which records where each one's static initializers start
AUTOLOAD_YAML = {"itcm": "itcm", "dtcm": "dtcm", "autoload_2": "unk_autoload_2", "autoload_3": "unk_autoload_3"}

# Byte counts, which the report writes as strings
BYTES = ["total_code", "matched_code", "total_data", "matched_data", "complete_code", "complete_data"]
COUNTS = ["total_functions", "matched_functions", "total_units", "complete_units"]


def percent(part: float, whole: float) -> float:
    return part / whole * 100 if whole else 100.0


def aggregate(units: list[dict]) -> dict:
    """Returns the measures of a group of units, as objdiff adds them up: sums, and the fuzzy match weighted by code."""
    sums = {key: sum(int(u["measures"].get(key, 0)) for u in units) for key in BYTES + COUNTS}
    fuzzy = sum(u["measures"].get("fuzzy_match_percent", 0.0) * int(u["measures"].get("total_code", 0)) for u in units)
    return {
        "fuzzy_match_percent": fuzzy / sums["total_code"] if sums["total_code"] else 100.0,
        "total_code": str(sums["total_code"]),
        "matched_code": str(sums["matched_code"]),
        "matched_code_percent": percent(sums["matched_code"], sums["total_code"]),
        "total_data": str(sums["total_data"]),
        "matched_data": str(sums["matched_data"]),
        "matched_data_percent": percent(sums["matched_data"], sums["total_data"]),
        "total_functions": sums["total_functions"],
        "matched_functions": sums["matched_functions"],
        "matched_functions_percent": percent(sums["matched_functions"], sums["total_functions"]),
        "complete_code": str(sums["complete_code"]),
        "complete_code_percent": percent(sums["complete_code"], sums["total_code"]),
        "complete_data": str(sums["complete_data"]),
        "complete_data_percent": percent(sums["complete_data"], sums["total_data"]),
        "total_units": sums["total_units"],
        "complete_units": sums["complete_units"],
    }


def read_delinks(delinks: Path) -> tuple[list, list]:
    """A delinks.txt's module sections and its files' sections, each as (section, start, end)."""
    module, files, source = [], [], None
    for line in delinks.read_text().splitlines():
        if line.endswith(":") and not line[0].isspace():
            source = line[:-1]
        elif m := SECTION_RE.match(line):
            entry = (m.group(1), int(m.group(2), 16), int(m.group(3), 16))
            (files if source else module).append(entry)
    return module, files


def section_gaps(module: list, files: list) -> list[tuple[str, int, int, int]]:
    """The ranges of a module's sections that no file holds, in the order dsd numbers its gaps: by section as the
    module lists them, then by address, as (section, start, end, the section's end)."""
    gaps = []
    for name, start, end in module:
        pos = start
        for _, file_start, file_end in sorted(f for f in files if f[0] == name):
            if file_start > pos:
                gaps.append((name, pos, file_start, end))
            pos = max(pos, file_end)
        if pos < end:
            gaps.append((name, pos, end, end))
    return gaps


def object_bytes(path: Path) -> tuple[int, bytes] | None:
    """The size of an ELF object's allocated sections and the bytes of those that have contents (none for .bss)."""
    data = path.read_bytes()
    if data[:4] != b"\x7fELF":
        return None
    shoff, = struct.unpack_from("<I", data, 0x20)
    shentsize, shnum = struct.unpack_from("<HH", data, 0x2e)
    size, contents = 0, b""
    for i in range(shnum):
        _, kind, flags, _, offset, length = struct.unpack_from("<IIIIII", data, shoff + i * shentsize)
        if flags & 2:  # SHF_ALLOC
            size += length
            if kind != 8:  # SHT_NOBITS
                contents += data[offset:offset + length]
    return size, contents


def sinit_start(root: Path, version: str, module: str) -> int | None:
    name = AUTOLOAD_YAML.get(module)
    path = root / "extract" / version / "arm9" / f"{name}.yaml"
    if name and path.exists():
        m = re.search(r"^sinit_start: (\d+)$", path.read_text(), re.M)
        return int(m.group(1)) if m else None
    return None


def linker_output_gaps(root: Path, version: str) -> set[str]:
    """The gap units that hold only what the linker writes, as the module docstring says."""
    arm9 = root / "config" / version / "arm9"
    paths = {"main": arm9 / "delinks.txt"}
    for delinks in [*arm9.glob("*/delinks.txt"), *arm9.glob("overlays/*/delinks.txt")]:
        paths[delinks.parent.name] = delinks
    modules = {name: read_delinks(path) for name, path in paths.items()}
    # Main's files hold the sections that dsd migrates to the DTCM and the LTD autoload, under their own names
    for section, start, end in modules["main"][1]:
        if section in MIGRATED_MODULES and MIGRATED_MODULES[section] in modules:
            modules[MIGRATED_MODULES[section]][1].append((MIGRATED_SECTIONS[section], start, end))
    found = set()
    for module, (sections, files) in modules.items():
        sinit = sinit_start(root, version, module)
        for i, (_, start, end, section_end) in enumerate(section_gaps(sections, files)):
            name = f"_dsd_gap@{module}_{i}"
            path = root / "build" / version / "delinks" / f"{name}.o"
            placed = object_bytes(path) if path.exists() else None
            if placed is None or placed[0] != end - start or any(placed[1]):
                continue
            if start == sinit or (end == section_end and end % 32 == 0):
                found.add(name)
    return found


def complete_unit(unit: dict):
    """Marks every function, section and byte of a complete unit as matched."""
    m = unit["measures"]
    m["matched_code"], m["matched_data"] = m.get("total_code", "0"), m.get("total_data", "0")
    m["matched_functions"] = m.get("total_functions", 0)
    m["fuzzy_match_percent"] = m["matched_code_percent"] = 100.0
    m["matched_data_percent"] = m["matched_functions_percent"] = 100.0
    for item in unit.get("functions", []) + unit.get("sections", []):
        item["fuzzy_match_percent"] = 100.0


def complete_report(report: dict, linker_gaps: set[str] = frozenset()) -> dict:
    units = report["units"] = [u for u in report["units"] if u["name"] not in linker_gaps]
    for unit in units:
        if unit.get("metadata", {}).get("complete"):
            complete_unit(unit)
    report["measures"] = aggregate(units)
    for category in report.get("categories", []):
        members = [u for u in units if category["id"] in u.get("metadata", {}).get("progress_categories", [])]
        category["measures"] = aggregate(members)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", type=Path, help="the report that objdiff-cli wrote")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    # The report is build/<version>/report.objdiff.json
    gaps = linker_output_gaps(ROOT, args.input.parent.name)
    args.output.write_text(json.dumps(complete_report(json.loads(args.input.read_text()), gaps)))


if __name__ == "__main__":
    main()
