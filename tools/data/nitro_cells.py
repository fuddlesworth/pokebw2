#!/usr/bin/env python3
"""NitroSystem's cell, cell animation, multi-cell and multi-cell animation files, and the battle sprites' MCSS files,
as JSON that writes them back byte for byte.

    nitro_cells.py edit FILE...
    nitro_cells.py dump FILE [OUTPUT.json]
    nitro_cells.py pack INPUT.json OUTPUT
    nitro_cells.py check [ROOT...]

The files stay the sources under res/, since their JSON is thirty times their size. edit replaces a file with its JSON,
named as the file is without its compression and with .json (front_animations.nanr.lz11 becomes
front_animations.nanr.json), and the packers that read the file (pokemon_graphics.py, trainer_graphics.py,
graphics_archive.py, item_icon_data.py) pack it from that JSON instead; a JSON that packs to the game's file keeps the
game's bytes, compression and all. git checkout brings the file back.

The four Nitro formats are the ones NNS_G2dGetUnpackedCellBank, NNS_G2dGetUnpackedAnimBank,
NNS_G2dGetUnpackedMultiCellBank and NNS_G2dGetUnpackedMCAnimBank read (lib/nnsys/src/g2d/, with the structs of
lib/nnsys/include/nnsys/g2d.h): a header and its blocks, the bank (CEBK, ABNK or MCBK), then the labels (LABL) and the
user extension (UEXT) where the file has them. Offsets in the files are resolved into nesting, and each format's bank is
written again as NitroSystem's converter laid it out:

- a cell bank (.ncer, RECN) is its cells, each with its OAMs, whose three attributes are split into their fields;
- a cell or multi-cell animation bank (.nanr, RNAN; .nmar, RAMN) is its sequences, each with its frames: the frame's
  duration and content, an index, an index with a translation, or an index with a rotation, scale and translation, by
  the sequence's element type. The converter stored each different content once in the file, in the order that frames
  first use it, with the contents of each sequence padded to 4 bytes with 0xcc. In 19 of the game's 5,688 files frames
  don't share every content they could, and their JSON gives each frame's place in the contents as "slot";
- a multi-cell bank (.nmcr, RCMN) is its multi-cells, each with its nodes: the cell animation sequence that the node
  plays, its position, the cell animation that plays it and its play mode.

docs/cells-and-animations.md describes the JSON, with its units and the values it leaves out.

A battle sprite's MCSS file (*_info.bin, the bin of MCSSLoadInfo in src/system/mcss.c) is its image's size and offset,
each cell's quads, which are what MCSS draws for the cells that the animations index (a position and size, and the
texture coordinates of the sprite's image), and the cells that keep animating while its idle animation stops the others,
a list ended by a 0 and padded to 4 bytes. The musical's MCSS files (musical/*_info.bin, MusicalMcssBin in
src/ov209/musical_mcss.c) are each cell's quads and the marker cells it carries, which aren't drawn but place the props,
then the markers.

check reads every file of these kinds under each root (res/ by default), compressed ones (.nanr.lz11) decompressed,
converts it to JSON and back, and reports any that don't come back byte for byte, and packs every such file's JSON.

Written by Michael Neuberger for pokegrey (github.com/DeltaDX117/pokegrey), with the packers' JSON added here.
"""
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import dumps  # noqa: E402
from tools.data.nitro_2d import decompress  # noqa: E402

BEEF = 0xBEEF
KINDS = {b"RECN": "cells", b"RNAN": "animations", b"RCMN": "multi_cells", b"RAMN": "multi_animations"}
MAGICS = {kind: magic for magic, kind in KINDS.items()}
BANKS = {"cells": b"CEBK", "animations": b"ABNK", "multi_cells": b"MCBK", "multi_animations": b"ABNK"}
ELEMENTS = ["index", "index_srt", "index_t"]
ELEMENT_SIZES = {"index": 2, "index_srt": 16, "index_t": 8}
PLAY_MODES = ["invalid", "forward", "forward_loop", "reverse", "reverse_loop"]
MAPPINGS = ["1d_32k", "1d_64k", "1d_128k", "1d_256k", "2d"]
# A multi-cell node's play mode, in its attributes' low 4 bits: whether its cell animation restarts when the multi-cell
# animation shows the node, or continues (NNSi_G2dGetMultiCellNodePlayMode). NitroSystem's converter set bit 5 in every
# node of the game's files, for a reason unknown, which "flags" overrides
NODE_PLAY_MODES = ["reset", "continue"]
NODE_FLAGS = 0x20


def pad4(blob: bytes) -> bytes:
    return blob + bytes(-len(blob) % 4)


def block_id(kind: bytes) -> bytes:
    """Returns a block's kind as the file stores it, a little-endian word ("CEBK" is b"KBEC"); the file's own magic
    is stored as it reads."""
    return kind[::-1]


# The file and its blocks


def read_blocks(blob: bytes) -> tuple[str, list[tuple[bytes, bytes]]]:
    magic, order, version, size, header_size, count = struct.unpack_from("<4sHHIHH", blob)
    if magic not in KINDS or order != 0xFEFF or version != 0x100 or size != len(blob) or header_size != 16:
        raise ValueError(f"not a version 1.0 NitroSystem cell or animation file: {magic!r}")
    blocks, offset = [], header_size
    for _ in range(count):
        kind, block_size = struct.unpack_from("<4sI", blob, offset)
        blocks.append((kind[::-1], blob[offset + 8:offset + block_size]))
        offset += block_size
    if offset != len(blob):
        raise ValueError("the blocks don't fill the file")
    return KINDS[magic], blocks


def write_blocks(kind: str, blocks: list[tuple[bytes, bytes]]) -> bytes:
    body = b"".join(struct.pack("<4sI", block_id(name), 8 + len(data)) + data for name, data in blocks)
    return struct.pack("<4sHHIHH", MAGICS[kind], 0xFEFF, 0x100, 16 + len(body), 16, len(blocks)) + body


def read_labels(data: bytes) -> list[str]:
    """Returns a LABL block's names: a table of offsets into the names that follow it, each ending with a 0."""
    for count in range(1, len(data) // 4 + 1):
        offsets = struct.unpack_from(f"<{count}I", data)
        names = data[4 * count:]
        if offsets[0] != 0 or not names.endswith(b"\0") or names.count(b"\0") != count:
            continue
        ends = [i for i, byte in enumerate(names) if byte == 0]
        if list(offsets) == [0] + [end + 1 for end in ends[:-1]]:
            return [name.decode("ascii") for name in names[:-1].split(b"\0")]
    raise ValueError("can't read the labels")


def write_labels(labels: list[str]) -> bytes:
    offsets, names = [], b""
    for label in labels:
        offsets.append(len(names))
        names += label.encode("ascii") + b"\0"
    return struct.pack(f"<{len(offsets)}I", *offsets) + names


def read_extras(blocks: list[tuple[bytes, bytes]], bank: bytes) -> dict:
    """Returns the labels and the user extension, which follow the bank in this order where the file has them."""
    names = [name for name, _ in blocks]
    if names[0] != bank or names[1:] not in ([], [b"LABL"], [b"LABL", b"UEXT"]):
        raise ValueError(f"unexpected blocks {names}")
    extras = {}
    for name, data in blocks[1:]:
        if name == b"LABL":
            extras["labels"] = read_labels(data)
        else:
            extras["extension"] = list(struct.unpack(f"<{len(data) // 4}I", data))
            if len(data) % 4:
                raise ValueError("a user extension that isn't whole words")
    return extras


def write_extras(data: dict) -> list[tuple[bytes, bytes]]:
    blocks = []
    if "labels" in data:
        blocks.append((b"LABL", write_labels(data["labels"])))
    if "extension" in data:
        blocks.append((b"UEXT", struct.pack(f"<{len(data['extension'])}I", *data["extension"])))
    return blocks


# Cells


# An OAM's width and height in pixels by its shape (square, wide, tall) and size; shape 3 is prohibited
OAM_SIZES = [[(8, 8), (16, 16), (32, 32), (64, 64)], [(16, 8), (32, 8), (32, 16), (64, 32)],
             [(8, 16), (8, 32), (16, 32), (32, 64)]]
OAM_MODES = ["normal", "translucent", "window", "bitmap"]


def read_oam(attr0: int, attr1: int, attr2: int) -> dict:
    """Returns an OAM's fields: its position from the cell's origin, its size in pixels, its first character,
    priority and palette, and the flags it sets: affine with its matrix and double size, or else hidden, flips and the
    second attribute's unused bits; mode, mosaic and 256 colors."""
    x, y, shape, size = attr1 & 0x1FF, attr0 & 0xFF, attr0 >> 14, attr1 >> 14
    oam = {"x": x - 512 if x >= 256 else x, "y": y - 256 if y >= 128 else y}
    if shape < 3:
        oam["width"], oam["height"] = OAM_SIZES[shape][size]
    else:
        oam["shape"], oam["size"] = shape, size
    oam.update({"character": attr2 & 0x3FF, "priority": attr2 >> 10 & 3, "palette": attr2 >> 12})
    if attr0 >> 8 & 1:
        oam.update({"affine": True, "matrix": attr1 >> 9 & 0x1F, "double_size": bool(attr0 >> 9 & 1)})
    else:
        oam.update({"hidden": bool(attr0 >> 9 & 1), "flip_h": bool(attr1 >> 12 & 1), "flip_v": bool(attr1 >> 13 & 1),
                    "unused": attr1 >> 9 & 7})
    oam.update({"mode": OAM_MODES[attr0 >> 10 & 3], "mosaic": bool(attr0 >> 12 & 1),
                "colors_256": bool(attr0 >> 13 & 1)})
    defaults = {"matrix": 0, "double_size": False, "hidden": False, "flip_h": False, "flip_v": False, "unused": 0,
                "mode": "normal", "mosaic": False, "colors_256": False}
    return {name: value for name, value in oam.items() if defaults.get(name, None) != value}


def write_oam(oam: dict) -> bytes:
    if "width" in oam:
        shape, size = next((shape, sizes.index((oam["width"], oam["height"])))
                           for shape, sizes in enumerate(OAM_SIZES) if (oam["width"], oam["height"]) in sizes)
    else:
        shape, size = oam["shape"], oam["size"]
    attr0 = oam["y"] & 0xFF | OAM_MODES.index(oam.get("mode", "normal")) << 10 | oam.get("mosaic", False) << 12
    attr0 |= oam.get("colors_256", False) << 13 | shape << 14
    attr1 = oam["x"] & 0x1FF | size << 14
    if oam.get("affine", False):
        attr0 |= 1 << 8 | oam.get("double_size", False) << 9
        attr1 |= oam.get("matrix", 0) << 9
    else:
        attr0 |= oam.get("hidden", False) << 9
        attr1 |= oam.get("unused", 0) << 9 | oam.get("flip_h", False) << 12 | oam.get("flip_v", False) << 13
    attr2 = oam["character"] | oam["priority"] << 10 | oam["palette"] << 12
    return struct.pack("<3H", attr0, attr1, attr2)


def read_cells(blocks) -> dict:
    data = blocks[0][1]
    count, bank_attr, cell_offset, mapping, vram, strings, extended = struct.unpack_from("<HHIIIII", data)
    if bank_attr & ~1 or vram or strings or extended:
        raise ValueError("a cell bank with VRAM transfer data, other attributes or packed extended data")
    with_bounds = bool(bank_attr & 1)
    cell_size = 16 if with_bounds else 8
    oams_start = cell_offset + count * cell_size
    cells, expected = [], 0
    for i in range(count):
        oam_count, cell_attr, oam_offset = struct.unpack_from("<HHI", data, cell_offset + i * cell_size)
        if oam_offset != expected:
            raise ValueError("a cell's OAMs aren't in order")
        expected += 6 * oam_count
        cell = {"radius": cell_attr & 0x3F}
        if cell_attr & ~0x3F:
            cell["flags"] = cell_attr & ~0x3F
        if with_bounds:
            cell["bounds"] = dict(zip(("max_x", "max_y", "min_x", "min_y"),
                                      struct.unpack_from("<4h", data, cell_offset + i * cell_size + 8)))
        cell["oams"] = [read_oam(*struct.unpack_from("<3H", data, oams_start + oam_offset + 6 * j))
                        for j in range(oam_count)]
        cells.append(cell)
    if cell_offset != 24:
        raise ValueError("cells that don't follow the bank's header")
    return {"mapping": MAPPINGS[mapping], "bounds": with_bounds, "cells": cells, **read_extras(blocks, b"CEBK")}


def write_cells(cells: dict) -> list[tuple[bytes, bytes]]:
    table, oams = b"", b""
    for cell in cells["cells"]:
        table += struct.pack("<HHI", len(cell["oams"]), cell["radius"] | cell.get("flags", 0), len(oams))
        if cells["bounds"]:
            bounds = cell["bounds"]
            table += struct.pack("<4h", bounds["max_x"], bounds["max_y"], bounds["min_x"], bounds["min_y"])
        oams += b"".join(write_oam(oam) for oam in cell["oams"])
    header = struct.pack("<HHIIIII", len(cells["cells"]), int(cells["bounds"]), 24, MAPPINGS.index(cells["mapping"]),
                         0, 0, 0)
    return [(b"CEBK", pad4(header + table + oams))] + write_extras(cells)


# Animations


def read_content(data: bytes, offset: int, element: str) -> dict:
    if element == "index":
        return {"index": struct.unpack_from("<H", data, offset)[0]}
    if element == "index_t":
        index, unused, x, y = struct.unpack_from("<HHhh", data, offset)
        content = {"index": index, "x": x, "y": y}
        unused = None if unused == BEEF else unused
    else:
        index, rotation, scale_x, scale_y, x, y = struct.unpack_from("<HHiihh", data, offset)
        content = {"index": index, "rotation": rotation, "scale_x": scale_x, "scale_y": scale_y, "x": x, "y": y}
        unused = None
    if unused is not None:
        content["unused"] = unused
    return content


def write_content(content: dict, element: str) -> bytes:
    if element == "index":
        return struct.pack("<H", content["index"])
    if element == "index_t":
        return struct.pack("<HHhh", content["index"], content.get("unused", BEEF), content["x"], content["y"])
    return struct.pack("<HHiihh", content["index"], content["rotation"], content["scale_x"], content["scale_y"],
                       content["x"], content["y"])


def content_layout(sequences: list[dict]) -> list[list[int]]:
    """Returns where each frame's content goes in the contents, as the converter laid them out: each different content
    once, in the order of first use, the contents of each sequence padded to 4 bytes, unless a frame gives its slot,
    which frames of the same slot share."""
    places, slots, offset = [], {}, 0
    for sequence in sequences:
        size = ELEMENT_SIZES[sequence["element"]]
        offsets = []
        for frame in sequence["frames"]:
            if "slot" in frame:
                key = ("slot", frame["slot"])
            else:
                key = (sequence["element"], json.dumps(frame["content"], sort_keys=True))
            if key not in slots:
                slots[key] = offset
                offset += size
            offsets.append(slots[key])
        offset += -offset % 4
        places.append(offsets)
    return places


def read_animations(blocks, file_kind: int) -> dict:
    data = blocks[0][1]
    count, frame_count, sequence_offset, frame_offset, content_offset, strings, extended = (
        struct.unpack_from("<HHIIIII", data))
    if strings or extended or sequence_offset != 24:
        raise ValueError("an animation bank with packed labels or extended data, or an unusual layout")
    sequences, places, expected = [], [], 0
    for i in range(count):
        frames, loop_start, anim_type, play_mode, frames_at = struct.unpack_from("<HHIII", data,
                                                                              sequence_offset + 16 * i)
        if frames_at != 8 * expected:
            raise ValueError("a sequence's frames aren't in order")
        expected += frames
        element = ELEMENTS[anim_type & 0xFFFF]
        sequence = {"element": element, "play_mode": PLAY_MODES[play_mode], "loop_start": loop_start, "frames": []}
        if anim_type >> 16 != file_kind:
            sequence["kind"] = anim_type >> 16
        offsets = []
        for j in range(frames):
            content_at, duration, pad = struct.unpack_from("<IHH", data, frame_offset + frames_at + 8 * j)
            if pad != BEEF:
                raise ValueError("a frame without the converter's padding")
            sequence["frames"].append({"duration": duration,
                                       "content": read_content(data, content_offset + content_at, element)})
            offsets.append(content_at)
        sequences.append(sequence)
        places.append(offsets)
    if frame_count != expected:
        raise ValueError("the frame count doesn't match the sequences")
    if content_layout(sequences) != places:
        slot_of = {}
        for sequence, offsets in zip(sequences, places):
            for frame, offset in zip(sequence["frames"], offsets):
                frame["slot"] = slot_of.setdefault(offset, len(slot_of))
        if content_layout(sequences) != places:
            raise ValueError("can't lay out the contents as the file has them")
    return {"sequences": sequences, **read_extras(blocks, b"ABNK")}


def write_animations(animations: dict, file_kind: int) -> list[tuple[bytes, bytes]]:
    sequences = animations["sequences"]
    places = content_layout(sequences)
    table, frames, contents = b"", b"", bytearray()
    for sequence, offsets in zip(sequences, places):
        anim_type = sequence.get("kind", file_kind) << 16 | ELEMENTS.index(sequence["element"])
        table += struct.pack("<HHIII", len(sequence["frames"]), sequence["loop_start"], anim_type,
                             PLAY_MODES.index(sequence["play_mode"]), len(frames))
        for frame, offset in zip(sequence["frames"], offsets):
            frames += struct.pack("<IHH", offset, frame["duration"], BEEF)
            content = write_content(frame["content"], sequence["element"])
            if len(contents) < offset + len(content):
                contents.extend(bytes(offset + len(content) - len(contents)))
            contents[offset:offset + len(content)] = content
        contents.extend(b"\xcc" * (-len(contents) % 4))
    frame_offset = 24 + len(table)
    header = struct.pack("<HHIIIII", len(sequences), len(frames) // 8, 24, frame_offset, frame_offset + len(frames),
                         0, 0)
    return [(b"ABNK", pad4(header + table + frames + bytes(contents)))] + write_extras(animations)


# Multi-cells


def read_multi_cells(blocks) -> dict:
    data = blocks[0][1]
    count, pad, cell_offset, node_offset, strings, extended = struct.unpack_from("<HHIIII", data)
    if pad != BEEF or strings or extended or cell_offset != 20:
        raise ValueError("a multi-cell bank with labels, extended data or an unusual layout")
    multi_cells, expected = [], 0
    for i in range(count):
        nodes, cell_animations, nodes_at = struct.unpack_from("<HHI", data, cell_offset + 8 * i)
        if nodes_at != 8 * expected:
            raise ValueError("a multi-cell's nodes aren't in order")
        expected += nodes
        multi_cell = {"cell_animations": cell_animations, "nodes": []}
        for j in range(nodes):
            sequence, x, y, attr = struct.unpack_from("<HhhH", data, node_offset + nodes_at + 8 * j)
            node = {"sequence": sequence, "x": x, "y": y, "cell_animation": attr >> 8,
                    "play_mode": NODE_PLAY_MODES[attr & 0xF]}
            if attr & 0xF0 != NODE_FLAGS:
                node["flags"] = attr & 0xF0
            multi_cell["nodes"].append(node)
        multi_cells.append(multi_cell)
    if node_offset != cell_offset + 8 * count:
        raise ValueError("nodes that don't follow the multi-cells")
    return {"multi_cells": multi_cells, **read_extras(blocks, b"MCBK")}


def write_multi_cells(multi_cells: dict) -> list[tuple[bytes, bytes]]:
    table, nodes = b"", b""
    for multi_cell in multi_cells["multi_cells"]:
        table += struct.pack("<HHI", len(multi_cell["nodes"]), multi_cell["cell_animations"], len(nodes))
        nodes += b"".join(struct.pack("<HhhH", node["sequence"], node["x"], node["y"],
                                      node["cell_animation"] << 8 | node.get("flags", NODE_FLAGS)
                                      | NODE_PLAY_MODES.index(node["play_mode"])) for node in multi_cell["nodes"])
    count = len(multi_cells["multi_cells"])
    header = struct.pack("<HHIIII", count, BEEF, 20, 20 + 8 * count, 0, 0)
    return [(b"MCBK", pad4(header + table + nodes))] + write_extras(multi_cells)


READERS = {"cells": read_cells, "animations": lambda blocks: read_animations(blocks, 1),
           "multi_cells": read_multi_cells, "multi_animations": lambda blocks: read_animations(blocks, 2)}
WRITERS = {"cells": write_cells, "animations": lambda data: write_animations(data, 1),
           "multi_cells": write_multi_cells, "multi_animations": lambda data: write_animations(data, 2)}


def read_nitro(blob: bytes) -> dict:
    kind, blocks = read_blocks(blob)
    return {"kind": kind, **READERS[kind](blocks)}


def write_nitro(data: dict) -> bytes:
    body = {key: value for key, value in data.items() if key != "kind"}
    return write_blocks(data["kind"], WRITERS[data["kind"]](body))


# MCSS files

QUAD = ("x", "y", "width", "height", "s", "t")


def read_mcss(blob: bytes) -> dict:
    count, width, height, offset_x, offset_y = struct.unpack_from("<IHHhh", blob)
    cells = []
    for i in range(count):
        values = struct.unpack_from("<12i", blob, 12 + 48 * i)
        cell = {"quad": dict(zip(QUAD, values[:6]))}
        if any(values[6:]):
            cell["quad2"] = dict(zip(QUAD, values[6:]))
        cells.append(cell)
    tail = (12 + 48 * count) & ~3
    idle_count, unknown = blob[tail], blob[tail + 1]
    idle_nodes = list(blob[tail + 2:tail + 2 + (0 if idle_count == 0xFF else idle_count)])
    data = {"kind": "mcss", "width": width, "height": height, "offset_x": offset_x, "offset_y": offset_y,
            "cells": cells, "idle_nodes": None if idle_count == 0xFF else idle_nodes, "unknown": unknown}
    rest = blob[tail + 2 + len(idle_nodes):]
    if rest != bytes(len(rest)) or len(rest) != 4 - (2 + len(idle_nodes)) % 4:
        raise ValueError("an MCSS file with more after its idle cells than a 0 and padding")
    return data


def write_mcss(data: dict) -> bytes:
    blob = struct.pack("<IHHhh", len(data["cells"]), data["width"], data["height"], data["offset_x"], data["offset_y"])
    for cell in data["cells"]:
        quad2 = cell.get("quad2", dict.fromkeys(QUAD, 0))
        blob += struct.pack("<12i", *(cell["quad"][name] for name in QUAD), *(quad2[name] for name in QUAD))
    idle = data["idle_nodes"]
    tail = bytes([0xFF if idle is None else len(idle), data["unknown"]]) + bytes(idle or []) + b"\0"
    return pad4(blob + tail)


# The musical's MCSS files (musical/*_info.bin, MusicalMcssBin in src/ov209/musical_mcss.c): each cell's quads and the
# marker cells it carries, which aren't drawn but place the props, then the markers' table

MARK = ("unknown", "kind", "x", "y", "rotation")


def read_musical_mcss(blob: bytes) -> dict:
    count = struct.unpack_from("<I", blob)[0]
    cells = []
    for i in range(count):
        values = struct.unpack_from("<12i", blob, 4 + 56 * i)
        cell = {"quad": dict(zip(QUAD, values[:6]))}
        if any(values[6:]):
            cell["quad2"] = dict(zip(QUAD, values[6:]))
        marks = blob[4 + 56 * i + 48:4 + 56 * (i + 1)]
        cell["marks"] = list(marks.rstrip(b"\xff"))
        if b"\xff" in bytes(cell["marks"]):
            raise ValueError("a cell's markers with a gap")
        cells.append(cell)
    table = blob[4 + 56 * count:]
    if len(table) % 20:
        raise ValueError("a markers' table that isn't whole markers")
    marks = [dict(zip(MARK, struct.unpack_from("<IIiiI", table, 20 * i))) for i in range(len(table) // 20)]
    return {"kind": "musical_mcss", "cells": cells, "marks": marks}


def write_musical_mcss(data: dict) -> bytes:
    blob = struct.pack("<I", len(data["cells"]))
    for cell in data["cells"]:
        quad2 = cell.get("quad2", dict.fromkeys(QUAD, 0))
        blob += struct.pack("<12i", *(cell["quad"][name] for name in QUAD), *(quad2[name] for name in QUAD))
        blob += bytes(cell["marks"]).ljust(8, b"\xff")
    return blob + b"".join(struct.pack("<IIiiI", *(mark[name] for name in MARK)) for mark in data["marks"])


# The packers' sources

SUFFIXES = (".ncer", ".nanr", ".nmcr", ".nmar", ".nanr.lz11")
SUFFIX_KINDS = {".ncer": "cells", ".nanr": "animations", ".nmcr": "multi_cells", ".nmar": "multi_animations"}


def json_path(path: Path) -> Path:
    """Returns the JSON that stands in for a file: its name without its compression, then .json."""
    return path.with_name(path.name.removesuffix(".lz11") + ".json")


def file_kind(path: Path) -> str:
    """Returns the kind of the JSON a file of these kinds has, by its name."""
    name = path.name.removesuffix(".lz11")
    if name.endswith("_info.bin"):
        return "musical_mcss" if "musical" in path.parts else "mcss"
    return SUFFIX_KINDS[Path(name).suffix]


def is_cells_file(path: Path) -> bool:
    return path.name.endswith(SUFFIXES) or path.name.endswith("_info.bin")


def has_source(path: Path) -> bool:
    """Returns whether a file of these kinds has a source: itself, or its JSON."""
    return path.is_file() or json_path(path).is_file()


def packed_json(path: Path) -> bytes | None:
    """Returns the file that a file's JSON packs into, uncompressed, or None where the file has no JSON, so that a
    packer reads the file itself."""
    source = json_path(path)
    if not is_cells_file(path) or not source.is_file():
        return None
    if path.is_file():
        raise ValueError(f"{source}: {path.name} is also there; keep one of the two")
    try:
        data = json.loads(source.read_text())
        if data.get("kind") != file_kind(path):
            raise ValueError(f"its kind is {data.get('kind')!r}, but {path.name} is {file_kind(path)!r}")
        return write_file(data)
    except (ValueError, KeyError, IndexError, TypeError, AttributeError, StopIteration, struct.error) as error:
        raise ValueError(f"{source}: {error!r}") from error


# The command line


def read_file(path: Path) -> dict:
    blob = path.read_bytes()
    if path.name.endswith(".lz11"):
        blob = decompress(blob)
    if path.name.endswith("_info.bin"):
        return read_musical_mcss(blob) if "musical" in path.parts else read_mcss(blob)
    return read_nitro(blob)


def write_file(data: dict) -> bytes:
    if data["kind"] == "mcss":
        return write_mcss(data)
    return write_musical_mcss(data) if data["kind"] == "musical_mcss" else write_nitro(data)


def files(root: Path):
    for path in sorted(root.rglob("*")):
        if path.is_file() and is_cells_file(path):
            yield path


def json_files(root: Path):
    """Yields the files of these kinds under a root that a JSON stands in for."""
    for source in sorted(root.rglob("*.json")):
        path = source.with_name(source.name.removesuffix(".json"))
        if is_cells_file(path):
            yield path


def edit(path: Path):
    if not path.is_file() or not is_cells_file(path):
        raise SystemExit(f"{path}: not a cell, animation or MCSS file")
    data = read_file(path)
    if write_file(data) != (decompress(path.read_bytes()) if path.name.endswith(".lz11") else path.read_bytes()):
        raise SystemExit(f"{path}: its JSON wouldn't pack back to it")
    json_path(path).write_text(dumps(data) + "\n")
    path.unlink()


def check(roots: list[Path]) -> bool:
    checked, failed = 0, []
    for root in roots:
        for path in json_files(root):
            checked += 1
            try:
                packed_json(path)
            except ValueError as error:
                failed.append(str(error))
        for path in files(root):
            checked += 1
            blob = path.read_bytes()
            if path.name.endswith(".lz11"):
                blob = decompress(blob)
            try:
                if write_file(json.loads(json.dumps(read_file(path)))) != blob:
                    failed.append(f"{path}: differs")
            except (ValueError, KeyError, IndexError, struct.error) as error:
                failed.append(f"{path}: {error}")
    for line in failed[:50]:
        print(line)
    print(f"{checked - len(failed)} of {checked} files convert to JSON and back byte for byte or pack from their JSON")
    return not failed


def main():
    args = sys.argv[1:]
    if len(args) >= 2 and args[0] == "edit":
        for arg in args[1:]:
            edit(Path(arg))
    elif len(args) in (2, 3) and args[0] == "dump":
        text = dumps(read_file(Path(args[1])))
        if len(args) == 3:
            Path(args[2]).write_text(text + "\n")
        else:
            print(text)
    elif len(args) == 3 and args[0] == "pack":
        Path(args[2]).write_bytes(write_file(json.loads(Path(args[1]).read_text())))
    elif args[:1] == ["check"]:
        sys.exit(0 if check([Path(arg) for arg in args[1:]] or [Path("res")]) else 1)
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
