#!/usr/bin/env python3
"""Reading and writing whole Nitro map models (BMD0 with one MDL0), laid out as the game's map chunks lay theirs out.

map_model_data.py edits a chunk's original model in place, keeping its materials and draw calls. This module writes a
model from its parts instead, so a model can gain materials and draw calls, or be new: map_model_data.py writes a map
model whose data.json has "written" whole from its GLB.

read_model() takes a model apart into a Model; write_model() lays one out. Every vanilla map model reads and writes
back byte for byte (`nitro_model_writer.py check`), which is what shows the layout is the game's:

- BMD0: magic, BOM 0xFEFF, version 2, file size, header size 16, one block at 20;
- MDL0: magic, size, the model-set dictionary (one model, its record its offset from the block), the model;
- the model: size and the offsets of its render commands, materials, shapes and envelope matrices (none: the model's
  end), its info (44 bytes), the node dictionary and each node's data, the render commands padded to 4 bytes;
- the material block: the offsets of its texture and palette dictionaries, the material dictionary (each record the
  material's offset from the block), the texture and palette dictionaries (each record the offset of its list of
  material numbers, their count and a flag), the lists, padding to 4 bytes, then the materials;
- the shape block: the shape dictionary, 16-byte shape records, then their display lists one after another.

draw_model() makes the parts of a new model from materials and triangles.

    nitro_model_writer.py check [ARCHIVE]   # read and write back every model of a/0/0/8 (extract/b2_us by default)

Written by bjayers for Paper White 2 (github.com/bjayers/paperwhite2).
"""

import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

from nitro_dictionary import build_dictionary

INFO = struct.Struct("<BBBBBBBBiiHHHHhhhhhhii")
SHAPE = struct.Struct("<HHIII")
NAME_LIMIT = 16


@dataclass
class Model:
    name: str
    # sbcType, scalingRule, texMtxMode, numNode, numMat, numShp, firstUnusedMtxStackID, dummy, posScale,
    # invPosScale, numVertex, numPolygon, numTriangle, numQuad, box x, y, z, w, h, d, boxPosScale, boxInvPosScale
    info: list
    nodes: list = field(default_factory=list)  # (name, data)
    sbc: bytes = b""
    materials: list = field(default_factory=list)  # (name, record)
    textures: list = field(default_factory=list)  # (name, [material numbers], flag)
    palettes: list = field(default_factory=list)
    shapes: list = field(default_factory=list)  # (name, item tag, flag, display list)


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def read_dictionary(data, offset):
    """A dictionary's names and records."""
    if data[offset] != 0:
        raise ValueError("unexpected Nitro dictionary header")
    count = data[offset + 1]
    header = offset + 12 + 4 * count
    size = u16(data, header)
    records = [data[header + 4 + size * i:header + 4 + size * (i + 1)] for i in range(count)]
    names_at = header + 4 + size * count
    names = [data[names_at + 16 * i:names_at + 16 * (i + 1)].split(b"\0")[0].decode("ascii") for i in range(count)]
    return names, records


def dictionary(names, records):
    """A dictionary with the game's search tree."""
    for name in names:
        if len(name.encode("ascii")) > NAME_LIMIT:
            raise ValueError(f"{name}: Nitro names are at most {NAME_LIMIT} characters")
    if len(names) > 255:
        raise ValueError("a Nitro dictionary holds at most 255 names")
    return build_dictionary(names, records)


def read_model(bmd):
    """A BMD0 file's one model, taken apart."""
    if bmd[:4] != b"BMD0" or u16(bmd, 4) != 0xFEFF or u16(bmd, 14) != 1:
        raise ValueError("expected a BMD0 file with one block")
    block = u32(bmd, 16)
    if bmd[block:block + 4] != b"MDL0":
        raise ValueError("expected an MDL0 block")
    names, records = read_dictionary(bmd, block + 8)
    if len(names) != 1:
        raise ValueError("expected one model in the MDL0 block")
    start = block + u32(records[0], 0)
    data = bmd[start:start + u32(bmd, start)]
    size, sbc_at, mat_at, shp_at, evp_at = struct.unpack_from("<5I", data)
    if evp_at != size:
        raise ValueError("models with envelope matrices aren't supported")
    model = Model(names[0], list(INFO.unpack_from(data, 20)))

    node_names, node_records = read_dictionary(data, 64)
    offsets = [u32(r, 0) for r in node_records]
    ends = offsets[1:] + [sbc_at - 64]
    model.nodes = [(n, bytes(data[64 + a:64 + b])) for n, a, b in zip(node_names, offsets, ends)]
    model.sbc = bytes(data[sbc_at:mat_at])

    texture_at, palette_at = struct.unpack_from("<HH", data, mat_at)
    mat_names, mat_records = read_dictionary(data, mat_at + 4)
    for at, target in ((texture_at, model.textures), (palette_at, model.palettes)):
        names_, records_ = read_dictionary(data, mat_at + at)
        for name, record in zip(names_, records_):
            offset, count, flag = struct.unpack("<HBB", record)
            target.append((name, list(data[mat_at + offset:mat_at + offset + count]), flag))
    mat_offsets = [u32(r, 0) for r in mat_records]
    mat_ends = mat_offsets[1:] + [shp_at - mat_at]
    model.materials = [(n, bytes(data[mat_at + a:mat_at + b])) for n, a, b in zip(mat_names, mat_offsets, mat_ends)]

    shp_names, shp_records = read_dictionary(data, shp_at)
    for name, record in zip(shp_names, shp_records):
        at = shp_at + u32(record, 0)
        tag, record_size, flag, dl_at, dl_size = SHAPE.unpack_from(data, at)
        if record_size != SHAPE.size:
            raise ValueError("unexpected shape record size")
        model.shapes.append((name, tag, flag, bytes(data[at + dl_at:at + dl_at + dl_size])))
    return model


def _pad(data, alignment=4):
    return data + b"\0" * (-len(data) % alignment)


def write_model(model):
    """The BMD0 file of a model."""
    # The nodes
    node_dict_size = len(dictionary([n for n, _ in model.nodes], [b"\0" * 4] * len(model.nodes)))
    node_offsets = []
    node_data = b""
    for _, data in model.nodes:
        node_offsets.append(node_dict_size + len(node_data))
        node_data += data
    nodes = dictionary([n for n, _ in model.nodes], [struct.pack("<I", o) for o in node_offsets])
    head = 64 + len(nodes) + len(node_data)
    sbc = _pad(model.sbc)

    # The material block
    mat_names = [n for n, _ in model.materials]
    mat_dict_size = 16 + 24 * len(mat_names)
    pair_sizes = [16 + 24 * len(model.textures), 16 + 24 * len(model.palettes)]
    lists_at = 4 + mat_dict_size + sum(pair_sizes)
    lists = b""
    pair_records = []
    for pairs in (model.textures, model.palettes):
        records = []
        for _, numbers, flag in pairs:
            records.append(struct.pack("<HBB", lists_at + len(lists), len(numbers), flag))
            lists += bytes(numbers)
        pair_records.append(records)
    materials_at = lists_at + len(_pad(lists))
    mat_offsets = []
    mat_data = b""
    for _, record in model.materials:
        mat_offsets.append(materials_at + len(mat_data))
        mat_data += record
    texture_dict = dictionary([n for n, _, _ in model.textures], pair_records[0])
    palette_dict = dictionary([n for n, _, _ in model.palettes], pair_records[1])
    mat_block = (struct.pack("<HH", 4 + mat_dict_size, 4 + mat_dict_size + pair_sizes[0])
                 + dictionary(mat_names, [struct.pack("<I", o) for o in mat_offsets])
                 + texture_dict + palette_dict + _pad(lists) + mat_data)

    # The shape block
    shp_dict_size = 16 + 24 * len(model.shapes)
    records = b""
    lists_ = b""
    first_dl = shp_dict_size + SHAPE.size * len(model.shapes)
    for i, (_, tag, flag, dl) in enumerate(model.shapes):
        at = shp_dict_size + SHAPE.size * i
        records += SHAPE.pack(tag, SHAPE.size, flag, first_dl + len(lists_) - at, len(dl))
        lists_ += dl
    shp_block = (dictionary([n for n, *_ in model.shapes],
                            [struct.pack("<I", shp_dict_size + SHAPE.size * i) for i in range(len(model.shapes))])
                 + records + lists_)

    sbc_at = head
    mat_at = sbc_at + len(sbc)
    shp_at = mat_at + len(mat_block)
    size = shp_at + len(shp_block)
    data = (struct.pack("<5I", size, sbc_at, mat_at, shp_at, size) + INFO.pack(*model.info) + nodes + node_data
            + sbc + mat_block + shp_block)
    model_set = dictionary([model.name], [struct.pack("<I", 8 + 16 + 24)])
    block = b"MDL0" + struct.pack("<I", 8 + len(model_set) + len(data)) + model_set + data
    return b"BMD0" + struct.pack("<HHIHHI", 0xFEFF, 2, 20 + len(block), 16, 1, 20) + block


# The GPU commands of map display lists, by opcode: their parameter words
GPU_PARAMS = {0x00: 0, 0x14: 1, 0x1b: 3, 0x20: 1, 0x21: 1, 0x22: 1, 0x23: 2, 0x24: 1, 0x25: 1, 0x26: 1, 0x27: 1,
              0x28: 1, 0x40: 1, 0x41: 0}
SHAPE_NORMAL, SHAPE_COLOR, SHAPE_TEXCOORD = 1, 2, 4
# The material most vanilla map materials are: front faces, fog, opaque, repeating its texture both ways, without a
# texture matrix; no light on (byte 12, the polygon attributes' low byte: lights 0 to 3 in bits 0 to 3), which a
# material whose shapes have normals turns on (POLYGON_LIGHT0), as vanilla's are lit by light 0's diffuse and ambient
POLYGON_LIGHT0 = 0x01
DEFAULT_MATERIAL = bytes.fromhex("00002c0039e7ff7f0000000080801f00fff81f3f00000300ffffffff0000ce1f0800080000100000"
                                 "00100000")


@dataclass
class Vertex:
    # fx32-scaled position as the display list stores it (s3.12 at the model's scale), texel coordinates in sixteenths,
    # RGB555, and the normal packed in 10-bit thirds; None where the vertex has no command for it
    position: tuple
    texcoord: int | None = None
    color: int | None = None
    normal: int | None = None


def signed(value, bits):
    return value - (1 << bits) if value & 1 << (bits - 1) else value


def decode_display_list(dl):
    """A display list's triangles, each three Vertex, with the commands' state carried as the GPU does. Raises on the
    matrix commands, which map lists need only rarely."""
    words = struct.unpack(f"<{len(dl) // 4}I", dl)
    i = 0
    state = {"texcoord": None, "color": None, "normal": None}
    position = [0, 0, 0]
    kind = None
    strip = []
    triangles = []

    def emit(vertex):
        strip.append(vertex)
        n = len(strip)
        if kind == 0 and n % 3 == 0:
            triangles.append(tuple(strip[-3:]))
        elif kind == 1 and n % 4 == 0:
            a, b, c, d = strip[-4:]
            triangles.extend([(a, b, c), (a, c, d)])
        elif kind == 2 and n >= 3:
            a, b, c = strip[-3:]
            triangles.append((a, b, c) if n % 2 else (b, a, c))
        elif kind == 3 and n >= 4 and n % 2 == 0:
            a, b, c, d = strip[-4:]
            triangles.extend([(a, b, d), (a, d, c)])

    while i < len(words):
        ops = [(words[i] >> shift) & 0xff for shift in (0, 8, 16, 24)]
        i += 1
        for op in ops:
            if op not in GPU_PARAMS:
                raise ValueError(f"unknown GPU command {op:#x}")
            params = words[i:i + GPU_PARAMS[op]]
            i += GPU_PARAMS[op]
            if op == 0x40:
                kind, strip = params[0] & 3, []
            elif op == 0x20:
                state["color"] = params[0] & 0x7fff
            elif op == 0x21:
                state["normal"] = params[0] & 0x3fffffff
            elif op == 0x22:
                state["texcoord"] = params[0]
            elif op in (0x14, 0x1b):
                raise ValueError("display lists with matrix commands aren't supported")
            elif 0x23 <= op <= 0x28:
                p = params[0] if params else 0
                if op == 0x23:
                    position = [signed(p & 0xffff, 16), signed(p >> 16, 16), signed(params[1] & 0xffff, 16)]
                elif op == 0x24:
                    position = [signed(p >> s & 0x3ff, 10) << 6 for s in (0, 10, 20)]
                elif op == 0x25:
                    position[0], position[1] = signed(p & 0xffff, 16), signed(p >> 16, 16)
                elif op == 0x26:
                    position[0], position[2] = signed(p & 0xffff, 16), signed(p >> 16, 16)
                elif op == 0x27:
                    position[1], position[2] = signed(p & 0xffff, 16), signed(p >> 16, 16)
                else:
                    position = [signed((position[k] + signed(p >> 10 * k & 0x3ff, 10)) & 0xffff, 16)
                                for k in range(3)]
                emit(Vertex(tuple(position), state["texcoord"], state["color"], state["normal"]))
    return triangles


def _cross(a, b, c):
    u = [b[k] - a[k] for k in range(3)]
    v = [c[k] - a[k] for k in range(3)]
    return (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])


def _flat_convex(a, b, c, d):
    """Whether the quad a, b, c, d is flat and convex, so the GPU draws it as its two triangles would be drawn."""
    n1, n2 = _cross(a, b, c), _cross(a, c, d)
    l1, l2 = sum(x * x for x in n1) ** 0.5, sum(x * x for x in n2) ** 0.5
    if not l1 or not l2 or sum(x * y for x, y in zip(n1, n2)) < 0.9999 * l1 * l2:
        return False
    # b and d lie either side of a-c by construction; a and c must lie either side of b-d
    m1, m2 = _cross(b, d, a), _cross(b, d, c)
    return sum(x * y for x, y in zip(m1, m2)) < 0


def quads(triangles):
    """The triangles as runs of quads and of triangles: two triangles (a, b, c) and (a, c, d) one after the other
    that make a flat, convex quad are one quad. Each run is (kind, vertices): kind 1 quads, 0 triangles."""
    runs = []
    i = 0
    while i < len(triangles):
        t = triangles[i]
        polygon = t
        if i + 1 < len(triangles):
            u = triangles[i + 1]
            if u[0] == t[0] and u[1] == t[2] and _flat_convex(t[0].position, t[1].position, t[2].position,
                                                              u[2].position):
                polygon = (t[0], t[1], t[2], u[2])
        i += 2 if len(polygon) == 4 else 1
        kind = 1 if len(polygon) == 4 else 0
        if runs and runs[-1][0] == kind:
            runs[-1][1].extend(polygon)
        else:
            runs.append((kind, list(polygon)))
    return runs


def _vertex_command(position, last):
    """The shortest command that sets a vertex at position after one at last."""
    x, y, z = position
    if last is not None:
        if z == last[2]:
            return 0x25, [(x & 0xffff) | (y & 0xffff) << 16]
        if y == last[1]:
            return 0x26, [(x & 0xffff) | (z & 0xffff) << 16]
        if x == last[0]:
            return 0x27, [(y & 0xffff) | (z & 0xffff) << 16]
        delta = [position[k] - last[k] for k in range(3)]
        if all(-512 <= d <= 511 for d in delta):
            return 0x28, [sum((d & 0x3ff) << 10 * k for k, d in enumerate(delta))]
    if all(c % 64 == 0 and -32768 <= c <= 32704 for c in position):
        return 0x24, [sum(((c >> 6) & 0x3ff) << 10 * k for k, c in enumerate(position))]
    return 0x23, [(x & 0xffff) | (y & 0xffff) << 16, z & 0xffff]


def _vertex_key(v):
    return (v.position, v.texcoord, v.color, v.normal)


def strips(triangles):
    """The triangles as the runs a display list draws: strips, each triangle sharing an edge (and its two vertices'
    every attribute) with the one before, as the GPU winds them (a strip's odd triangles turn its first two corners
    round), and the triangles that join none, as one run of separate triangles. Each run is (kind, vertices): kind 2
    a strip, 0 separate triangles."""
    by_edge = {}
    for t, triangle in enumerate(triangles):
        keys = [_vertex_key(v) for v in triangle]
        for i in range(3):
            by_edge.setdefault((keys[i], keys[(i + 1) % 3]), []).append((t, i))
    used = [False] * len(triangles)
    runs = []
    singles = []
    for t, triangle in enumerate(triangles):
        if used[t]:
            continue
        used[t] = True
        strip = list(triangle)
        while True:
            k = len(strip) - 2
            first, second = (strip[-2], strip[-1]) if k % 2 == 0 else (strip[-1], strip[-2])
            found = None
            for u, i in by_edge.get((_vertex_key(first), _vertex_key(second)), []):
                if not used[u]:
                    found = (u, i)
                    break
            if found is None:
                break
            u, i = found
            used[u] = True
            strip.append(triangles[u][(i + 2) % 3])
        if len(strip) == 3:
            singles += strip
        else:
            runs.append((2, strip))
    if singles:
        runs.append((0, singles))
    return runs


def encode_display_list(triangles):
    """A display list drawing triangles of Vertex: as strips (where triangles share edges) or as quads (where two
    triangles in a row make one), whichever is shorter, with a color, normal or texture coordinate only when it
    changes, each vertex with the shortest command that reaches it exactly, and the commands packed four to a word."""
    return min((_encode_runs(strips(triangles)), _encode_runs(quads(triangles))), key=len)


def _encode_runs(runs):
    commands = []
    last = {"texcoord": None, "color": None, "normal": None}
    position = None
    for kind, vertices in runs:
        commands.append((0x40, [kind]))
        for vertex in vertices:
            for op, name in ((0x20, "color"), (0x21, "normal"), (0x22, "texcoord")):
                value = getattr(vertex, name)
                if value is not None and value != last[name]:
                    commands.append((op, [value]))
                    last[name] = value
            commands.append(_vertex_command(vertex.position, position))
            position = vertex.position
        commands.append((0x41, []))
    out = bytearray()
    for start in range(0, len(commands), 4):
        group = commands[start:start + 4]
        ops = [op for op, _ in group] + [0] * (4 - len(group))
        out += struct.pack("<I", ops[0] | ops[1] << 8 | ops[2] << 16 | ops[3] << 24)
        for _, params in group:
            out += struct.pack(f"<{len(params)}I", *params)
    return bytes(out)


def _counts(dl):
    """A display list's vertex commands and quads, for the model's info."""
    words = struct.unpack(f"<{len(dl) // 4}I", dl)
    i = vertex_ops = quads_drawn = 0
    kind = None
    run = 0
    while i < len(words):
        ops = [(words[i] >> shift) & 0xff for shift in (0, 8, 16, 24)]
        i += 1
        for op in ops:
            params = words[i:i + GPU_PARAMS[op]]
            i += GPU_PARAMS[op]
            if op == 0x40:
                kind, run = params[0] & 3, 0
            elif op == 0x41 and kind == 1:
                quads_drawn += run // 4
            elif 0x23 <= op <= 0x28:
                vertex_ops += 1
                run += 1
    return vertex_ops, quads_drawn


def box(points, scale=4096):
    """The model box of points (display-list units), in those units divided by scale / 4096, the smallest power of
    two that fits, and that scale."""
    while True:
        low = [min(p[k] for p in points) * 4096 // scale for k in range(3)]
        size = [-(-max(p[k] for p in points) * 4096 // scale) - low[k] for k in range(3)]
        if all(-32768 <= v <= 32767 for v in (*low, *size)):
            return low + size, scale
        scale *= 2


def draw_model(name, materials, draws, pos_scale=64):
    """A new model: materials as (name, texture name or None, palette name or None, record), each record a material
    record (DEFAULT_MATERIAL with the texture's width and height at bytes 32 to 36, or a vanilla material's); draws as
    (material number, triangles of Vertex), each a draw call with a shape of its own. Positions are in display-list
    units: a game unit is 4096 / pos_scale of them."""
    model = Model(name, [])
    model.nodes = [("polySurface1", bytes.fromhex("07f80010"))]
    model.materials = [(m[0], m[3]) for m in materials]
    for target, index in ((model.textures, 1), (model.palettes, 2)):
        named = {}
        for number, material in enumerate(materials):
            if material[index]:
                named.setdefault(material[index], []).append(number)
        target.extend((n, numbers, 0) for n, numbers in named.items())
    sbc = bytearray(bytes.fromhex("26000000000200010b"))
    vertices = triangle_count = quads_total = 0
    points = []
    for shape, (material, triangles) in enumerate(draws):
        if not triangles:
            raise ValueError(f"{name}: draw call {shape} has no triangles")
        first = triangles[0][0]
        flag = ((SHAPE_NORMAL if first.normal is not None else 0) | (SHAPE_COLOR if first.color is not None else 0)
                | (SHAPE_TEXCOORD if first.texcoord is not None else 0))
        model.shapes.append((f"polygon{shape}", 0, flag, encode_display_list(triangles)))
        sbc += bytes((0x04, material, 0x05, shape))
        dl = model.shapes[-1][3]
        vertex_ops, quad_count = _counts(dl)
        vertices += vertex_ops
        quads_total += quad_count
        triangle_count += len(triangles) - 2 * quad_count
        points += [v.position for t in triangles for v in t]
    sbc += b"\x2b\x01"
    model.sbc = bytes(sbc)
    if len(materials) > 255 or len(draws) > 255:
        raise ValueError(f"{name}: a model has at most 255 materials and 255 draw calls")
    if vertices > 0xffff:
        raise ValueError(f"{name}: too many vertices for one model")
    box_values, box_scale = box(points)
    # The box is in the display list's units, halved while it doesn't fit, so its scale is the model's position scale
    # times that; the game tests the box before drawing the chunk, and one too small hides it
    box_pos_scale = pos_scale * box_scale
    model.info = [0, 0, 0, 1, len(materials), len(draws), 1, 0, pos_scale * 4096, 4096 // pos_scale,
                  vertices, triangle_count + quads_total, triangle_count, quads_total, *box_values, box_pos_scale,
                  4096 * 4096 // box_pos_scale]
    return model


def canonical(triangles):
    """Triangles as a sorted list, each from its least corner, so that the same triangles wound the same way compare
    equal whatever order and corner they start from."""
    out = []
    for t in triangles:
        keys = [_vertex_key(v) for v in t]
        out.append(min(tuple(keys[i:] + keys[:i]) for i in range(3)))
    return sorted(out, key=repr)


def check_drawing(archive_path):
    """Draws every vanilla shape again from its decoded triangles and checks it decodes to the same triangles."""
    from map_model_data import model_bytes
    from narc import read_narc
    archive = read_narc(Path(archive_path).read_bytes())
    shapes = failures = skipped = 0
    for index, chunk in enumerate(archive):
        model = read_model(model_bytes(chunk))
        for name, _, flag, dl in model.shapes:
            try:
                triangles = decode_display_list(dl)
            except ValueError:
                skipped += 1
                continue
            shapes += 1
            again = decode_display_list(encode_display_list(triangles))
            # Strips may start a triangle from another corner; the triangles and their winding are what must stay
            if canonical(again) != canonical(triangles):
                failures += 1
                if failures <= 5:
                    print(f"model {index} ({model.name}) {name}: draws differently")
    print(f"{shapes} shapes drawn again, {shapes - failures} the same; {skipped} with matrix commands skipped")
    # The model box draw_model gives a vanilla model's own triangles must hold every vertex, as the game reads it with
    # its scale; the game's own boxes are much the same (made from the models' floats, a few at a coarser scale, a few
    # holding other things)
    boxes = differ = 0
    for index, chunk in enumerate(archive):
        model = read_model(model_bytes(chunk))
        draws = [(0, decode_display_list(dl)) for _, _, _, dl in model.shapes]
        drawn = draw_model(model.name, [("m", None, None, DEFAULT_MATERIAL)], draws, model.info[8] // 4096)
        x, y, z, w, h, d, scale, _ = drawn.info[14:22]
        step = scale / model.info[8]
        holds = all(x * step <= p[0] <= (x + w) * step and y * step <= p[1] <= (y + h) * step
                    and z * step <= p[2] <= (z + d) * step
                    for _, t in draws for tri in t for v in tri for p in [v.position])
        if not holds:
            boxes += 1
            if boxes <= 5:
                print(f"model {index} ({model.name}): box {drawn.info[14:22]}, the game's {model.info[14:22]}")
        differ += drawn.info[14:20] != model.info[14:20]
    print(f"{len(archive)} models: {len(archive) - boxes} boxes holding every vertex "
          f"({differ} written a little differently from the game's)")
    return failures == 0 and boxes == 0


def check(archive_path):
    from map_model_data import model_bytes
    from narc import read_narc
    archive = read_narc(Path(archive_path).read_bytes())
    failures = 0
    for index, chunk in enumerate(archive):
        bmd = model_bytes(chunk)
        model = read_model(bmd)
        written = write_model(model)
        if written != bmd:
            failures += 1
            if failures <= 5:
                first = (next(i for i, (a, b) in enumerate(zip(written, bmd)) if a != b)
                         if len(written) == len(bmd) else -1)
                print(f"model {index} ({model.name}): differs (sizes {len(written)} and {len(bmd)}, "
                      f"first byte {first})")
    print(f"{len(archive)} models, {len(archive) - failures} written back byte for byte")
    return failures == 0


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "check":
        root = Path(__file__).resolve().parents[2]
        archive = sys.argv[2] if len(sys.argv) > 2 else root / "extract/b2_us/files/a/0/0/8"
        ok = check(archive) & check_drawing(archive)
        sys.exit(0 if ok else 1)
    sys.exit(__doc__)
