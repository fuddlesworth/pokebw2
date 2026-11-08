#!/usr/bin/env python3
"""Reading and writing whole Nitro texture sets (BTX0 with one TEX0), as the map texture sets (a/0/1/4) lay them out, so
that a set can gain textures from another (a set's imports.json, map_texture_data.py), for a model moved from one area
to another whose set lacks some of its textures.

read_set() takes a set apart: its textures (name, image parameters and extra parameters, the image's offset into the
texel data held in its parameters), its palettes (name, offset into the palette data and flag), the texel data and the
palette data, whole, since textures and palettes share bytes. write_set() lays a set out again: BTX0 header, TEX0
header, the texture dictionary, the palette dictionary, the texel data, then the palette data. Every vanilla map
texture set reads and writes back byte for byte (`nitro_texture_writer.py check`).

add_texture() appends a texture of another set, its image and its palette copied as they are encoded there.

    nitro_texture_writer.py check [ARCHIVE]   # read and write back every set (extract/b2_us/files/a/0/1/4 by default)

Written by bjayers for Paper White 2 (github.com/bjayers/paperwhite2).
"""

import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

from nitro_dictionary import build_dictionary

HEADER = struct.Struct("<4sIIHHHHIIHHHHIIIHHHHI")
# Bits per texel by format (the parameters' bits 26 to 28): A3I5, 2-bit, 4-bit, 8-bit, 4x4, A5I3, direct
BITS = {1: 8, 2: 2, 3: 4, 4: 8, 5: 2, 6: 8, 7: 16}
# Palette colors a format uses, for copying a palette: 4-bit and 2-bit index few
COLORS = {1: 32, 2: 4, 3: 16, 4: 256, 6: 8}


@dataclass
class TextureSet:
    textures: list = field(default_factory=list)  # [name, params, extra]
    palettes: list = field(default_factory=list)  # [name, offset in bytes, flag]
    texels: bytes = b""
    palette_data: bytes = b""
    # The TEX0 header's fields the layout doesn't decide, kept as read
    keys: tuple = (0, 0, 0, 0)
    flags: tuple = (0, 0, 0)


def _dictionary(data, offset, size):
    count = data[offset + 1]
    header = offset + 12 + 4 * count
    if struct.unpack_from("<H", data, header)[0] != size:
        raise ValueError("unexpected Nitro dictionary record size")
    records = [data[header + 4 + size * i:header + 4 + size * (i + 1)] for i in range(count)]
    names_at = header + 4 + size * count
    names = [data[names_at + 16 * i:names_at + 16 * (i + 1)].split(b"\0")[0].decode("ascii") for i in range(count)]
    return names, records


def read_set(blob):
    if blob[:4] != b"BTX0" or struct.unpack_from("<H", blob, 14)[0] != 1:
        raise ValueError("expected a BTX0 file with one block")
    base = struct.unpack_from("<I", blob, 16)[0]
    (magic, size, key, tex_size, tex_dict, flag, _, tex_at, key4, size4, dict4, flag4, _, at4, index4, keyp, pal_size,
     flagp, pal_dict, _, pal_at) = HEADER.unpack_from(blob, base)
    if magic != b"TEX0" or size4:
        raise ValueError("expected a TEX0 block without 4x4 textures")
    t = TextureSet(keys=(key, key4, keyp, dict4), flags=(flag, flag4, flagp))
    names, records = _dictionary(blob, base + tex_dict, 8)
    t.textures = [[n, *struct.unpack("<II", r)] for n, r in zip(names, records)]
    names, records = _dictionary(blob, base + pal_dict, 4)
    t.palettes = [[n, struct.unpack("<H", r[:2])[0] << 3, struct.unpack("<H", r[2:])[0]]
                  for n, r in zip(names, records)]
    t.texels = bytes(blob[base + tex_at:base + tex_at + (tex_size << 3)])
    t.palette_data = bytes(blob[base + pal_at:base + pal_at + (pal_size << 3)])
    return t


def write_set(t):
    textures = build_dictionary([n for n, _, _ in t.textures], [struct.pack("<II", p, e) for _, p, e in t.textures])
    palettes = build_dictionary([n for n, _, _ in t.palettes],
                                [struct.pack("<HH", o >> 3, f) for _, o, f in t.palettes])
    texels = t.texels + b"\0" * (-len(t.texels) % 8)
    palette_data = t.palette_data + b"\0" * (-len(t.palette_data) % 8)
    tex_dict = HEADER.size
    pal_dict = tex_dict + len(textures)
    tex_at = pal_dict + len(palettes)
    pal_at = tex_at + len(texels)
    size = pal_at + len(palette_data)
    key, key4, keyp, dict4 = t.keys
    flag, flag4, flagp = t.flags
    header = HEADER.pack(b"TEX0", size, key, len(texels) >> 3, tex_dict, flag, 0, tex_at, key4, 0, dict4, flag4, 0,
                         pal_at, pal_at, keyp, len(palette_data) >> 3, flagp, pal_dict, 0, pal_at)
    block = header + textures + palettes + texels + palette_data
    return b"BTX0" + struct.pack("<HHIHHI", 0xFEFF, 1, 20 + len(block), 16, 1, 20) + block


def texture_bytes(params):
    """Where a texture's image lies in the texel data, from its parameters."""
    width, height = 8 << ((params >> 20) & 7), 8 << ((params >> 23) & 7)
    kind = (params >> 26) & 7
    if kind == 5:
        raise ValueError("4x4 textures can't be copied yet")
    return (params & 0xffff) << 3, width * height * BITS[kind] // 8


def add_texture(target, source, name, palette_name):
    """Appends the source set's texture name, and its palette as palette_name, to the target set: the image and the
    palette's colors as they are encoded there. A palette the target has by that name already must hold the same
    colors."""
    if any(n == name for n, _, _ in target.textures):
        raise ValueError(f"the set has a texture {name} already")
    found = [t for t in source.textures if t[0] == name]
    if not found:
        raise ValueError(f"the source set has no texture {name}")
    _, params, extra = found[0]
    start, length = texture_bytes(params)
    image = source.texels[start:start + length]
    texels = target.texels + b"\0" * (-len(target.texels) % 8)
    if len(texels) + length > 0xffff << 3:
        raise ValueError("the set's texel data would be too large")
    target.texels = texels + image
    target.textures.append([name, (params & ~0xffff) | (len(texels) >> 3), extra])
    kind = (params >> 26) & 7
    if kind == 7:
        return
    palettes = [p for p in source.palettes if p[0] == palette_name]
    if not palettes:
        raise ValueError(f"the source set has no palette {palette_name}")
    _, offset, flag = palettes[0]
    colors = source.palette_data[offset:offset + 2 * COLORS[kind]]
    have = [p for p in target.palettes if p[0] == palette_name]
    if have:
        if target.palette_data[have[0][1]:have[0][1] + len(colors)] != colors:
            raise ValueError(f"the set has a palette {palette_name} with other colors")
        return
    data = target.palette_data + b"\0" * (-len(target.palette_data) % 8)
    target.palette_data = data + colors
    target.palettes.append([palette_name, len(data), flag])


def check(archive_path):
    from narc import read_narc
    archive = read_narc(Path(archive_path).read_bytes())
    failures = 0
    for index, blob in enumerate(archive):
        if write_set(read_set(blob)) != blob:
            failures += 1
            if failures <= 5:
                print(f"set {index}: differs")
    print(f"{len(archive)} texture sets, {len(archive) - failures} written back byte for byte")
    return failures == 0


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "check":
        root = Path(__file__).resolve().parents[2]
        sys.exit(0 if check(sys.argv[2] if len(sys.argv) > 2 else root / "extract/b2_us/files/a/0/1/4") else 1)
    sys.exit(__doc__)
