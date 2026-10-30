#version 450
layout(set=2,binding=0,std430) readonly buffer TextureMemory { uint texels[]; };
layout(set=2,binding=1,std430) readonly buffer PaletteMemory { uint palettes[]; };
struct BackgroundState {
    uint vram[131072];
    uint palette[128];
    uint extendedPalette[8192];
    uint object[65536];
    uint objectPalette[128];
    uint objectExtendedPalette[2048];
    uint oam[256];
    uint registers[32];
    uint display;
};
layout(set=2,binding=2,std430) readonly buffer BackgroundMemory { BackgroundState backgrounds[2]; };
struct RearPixel { uint color; float depth; };
layout(set=2,binding=3,std430) readonly buffer RearPlaneMemory { RearPixel rearPixels[]; };
layout(location=0) in vec2 texcoord;
layout(location=1) in vec4 vertexColor;
layout(location=2) flat in uvec3 attributes;
layout(location=0) out vec4 outColor;
layout(set=3, binding=0) uniform CaptureState { vec4 sceneRect; vec4 sceneDepth; uint captureMode; uint spriteFilter; };
uint byteAt(uint address) { return (texels[(address & 524287u) >> 2] >> ((address & 3u) * 8u)) & 255u; }
uint halfAt(uint address) { return byteAt(address) | (byteAt(address + 1u) << 8u); }
vec3 rgb(uint value) { return vec3(value & 31u, (value >> 5u) & 31u, (value >> 10u) & 31u) / 31.0; }
vec3 palette(uint address) {
    if (address >= 98304u) return vec3(1,0,1);
    return rgb((palettes[address >> 2u] >> ((address & 2u) * 8u)) & 65535u);
}
uint objectByte(uint engine, uint address) {
    address &= engine == 0u ? 262143u : 131071u;
    return (backgrounds[engine].object[address >> 2u] >> ((address & 3u) * 8u)) & 255u;
}
int objectMatrix(uint engine, uint entry) {
    return int(backgrounds[engine].oam[entry * 2u + 1u]) >> 16;
}
vec4 objectPixel(uint engine, uint descriptor, vec2 uv) {
    uint entry = descriptor & 127u;
    uint a01 = backgrounds[engine].oam[entry * 2u], a2 = backgrounds[engine].oam[entry * 2u + 1u] & 65535u;
    uint display = backgrounds[engine].display, mode = (a01 >> 10u) & 3u;
    int width = int((descriptor >> 8u) & 127u), height = int((descriptor >> 16u) & 127u);
    ivec2 p = ivec2(floor(uv));
    if ((a01 & 256u) != 0u) {
        uint matrix = ((a01 >> 25u) & 31u) * 4u;
        ivec2 center = ivec2(width, height) / ((a01 & 512u) != 0u ? 1 : 2);
        p -= center;
        p = ivec2((objectMatrix(engine, matrix) * p.x + objectMatrix(engine, matrix + 1u) * p.y) >> 8,
                  (objectMatrix(engine, matrix + 2u) * p.x + objectMatrix(engine, matrix + 3u) * p.y) >> 8);
        p += ivec2(width, height) / 2;
        if (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height) return vec4(0);
    } else {
        if ((a01 & 268435456u) != 0u) p.x = width - 1 - p.x;
        if ((a01 & 536870912u) != 0u) p.y = height - 1 - p.y;
    }
    if (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height) return vec4(0);
    uint name = a2 & 1023u, bank = a2 >> 12u;
    if (mode == 3u) {
        uint address;
        if ((display & 64u) != 0u) address = (name << (7u + ((display >> 22u) & 1u))) + uint(p.y * width + p.x) * 2u;
        else {
            uint stride = (display & 32u) != 0u ? 256u : 128u;
            address = ((name / (stride / 8u) * 8u + uint(p.y)) * stride + name % (stride / 8u) * 8u + uint(p.x)) * 2u;
        }
        uint value = objectByte(engine, address) | (objectByte(engine, address + 1u) << 8u);
        return vec4(rgb(value), (value & 32768u) != 0u && bank != 0u ? float(bank + 1u) / 16.0 : 0);
    }
    bool eightBit = (a01 & 8192u) != 0u;
    uint tileBytes = eightBit ? 64u : 32u;
    uint address;
    if ((display & 16u) != 0u)
        address = (name << (5u + ((display >> 20u) & 3u))) + uint(p.y / 8 * (width / 8) + p.x / 8) * tileBytes;
    else address = (eightBit ? name & ~1u : name) * 32u + uint(p.y / 8) * 1024u + uint(p.x / 8) * tileBytes;
    uint pixel = uint((p.y & 7) * 8 + (p.x & 7));
    uint value = objectByte(engine, address + (eightBit ? pixel : pixel / 2u));
    uint index = eightBit ? value : (value >> ((pixel & 1u) * 4u)) & 15u;
    if (index == 0u) return vec4(0);
    bool extended = eightBit && (display & 2147483648u) != 0u;
    uint paletteIndex = index + (extended ? bank * 256u : eightBit ? 0u : bank * 16u);
    uint color = extended ? backgrounds[engine].objectExtendedPalette[paletteIndex / 2u] : backgrounds[engine].objectPalette[paletteIndex / 2u];
    return vec4(rgb(color >> ((paletteIndex & 1u) * 16u)), 1);
}
int coordinate(float value, int size, bool repeat, bool mirror) {
    int coord = int(floor(value));
    if (!repeat) return clamp(coord, 0, size - 1);
    int wrapped = coord & (size - 1);
    return mirror && (coord & size) != 0 ? size - 1 - wrapped : wrapped;
}
uint bgByte(uint engine, uint address) {
    address &= engine == 0u ? 524287u : 131071u;
    return (backgrounds[engine].vram[address >> 2u] >> ((address & 3u) * 8u)) & 255u;
}
uint bgHalf(uint engine, uint address) { return bgByte(engine, address) | (bgByte(engine, address + 1u) << 8u); }
vec4 backgroundPixel(uint engine, uint layer) {
    if (layer == 4u) return vec4(rgb(backgrounds[engine].palette[0] & 65535u), 1);
    uint display = backgrounds[engine].display;
    uint control = (backgrounds[engine].registers[2u + layer / 2u] >> ((layer & 1u) * 16u)) & 65535u;
    uint mode = display & 7u, size = control >> 14u;
    bool affine = layer == 2u ? mode == 2u || mode == 4u : layer == 3u && (mode == 1u || mode == 2u);
    bool extended = layer == 2u ? mode == 5u : layer == 3u && mode >= 3u && mode <= 5u;
    bool bitmap = extended && (control & 128u) != 0u;
    uint charBase = ((control >> 2u) & 15u) * 16384u;
    uint screenBase = ((control >> 8u) & 31u) * (bitmap ? 16384u : 2048u);
    if (engine == 0u && !bitmap) {
        charBase += ((display >> 24u) & 7u) * 65536u;
        screenBase += ((display >> 27u) & 7u) * 65536u;
    }
    ivec2 p = ivec2(floor(texcoord));
    int width, height;
    if (affine || extended) {
        uint base = layer == 2u ? 8u : 12u;
        uint ab = backgrounds[engine].registers[base], cd = backgrounds[engine].registers[base + 1u];
        int a = int(ab << 16u) >> 16, b = int(ab) >> 16;
        int c = int(cd << 16u) >> 16, d = int(cd) >> 16;
        int x = int(backgrounds[engine].registers[base + 2u] << 4u) >> 4;
        int y = int(backgrounds[engine].registers[base + 3u] << 4u) >> 4;
        p = ivec2((x + a * p.x + b * p.y) >> 8, (y + c * p.x + d * p.y) >> 8);
        width = bitmap ? (size == 0u ? 128 : size == 3u ? 512 : 256) : 128 << size;
        height = bitmap ? (size == 0u ? 128 : size >= 2u ? 512 : 256) : width;
        if ((control & 8192u) == 0u && (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height)) return vec4(0);
    } else {
        uint scroll = backgrounds[engine].registers[4u + layer];
        p += ivec2(scroll & 511u, (scroll >> 16u) & 511u);
        width = (size & 1u) != 0u ? 512 : 256;
        height = (size & 2u) != 0u ? 512 : 256;
    }
    p &= ivec2(width - 1, height - 1);
    uint index, paletteBank = 0u;
    bool extendedPalette = !affine && !bitmap && (control & 128u) != 0u && (display & 1073741824u) != 0u;
    if (bitmap) {
        uint offset = uint(p.y * width + p.x);
        if ((control & 4u) != 0u) {
            uint value = bgHalf(engine, screenBase + offset * 2u);
            return vec4(rgb(value), (value & 32768u) != 0u ? 1 : 0);
        }
        index = bgByte(engine, screenBase + offset);
    } else if (affine) {
        uint tile = bgByte(engine, screenBase + uint((p.y / 8) * (width / 8) + p.x / 8));
        index = bgByte(engine, charBase + tile * 64u + uint((p.y & 7) * 8 + (p.x & 7)));
    } else {
        uint mapIndex = extended ? uint((p.y / 8) * (width / 8) + p.x / 8) :
            uint(((p.y / 256) * (width / 256) + p.x / 256) * 1024 + ((p.y / 8) & 31) * 32 + ((p.x / 8) & 31));
        uint tile = bgHalf(engine, screenBase + mapIndex * 2u);
        ivec2 pixel = p & 7;
        if ((tile & 1024u) != 0u) pixel.x = 7 - pixel.x;
        if ((tile & 2048u) != 0u) pixel.y = 7 - pixel.y;
        bool eightBit = extended || (control & 128u) != 0u;
        uint offset = uint(pixel.y * 8 + pixel.x);
        uint value = bgByte(engine, charBase + (tile & 1023u) * (eightBit ? 64u : 32u) + (eightBit ? offset : offset / 2u));
        index = eightBit ? value : (value >> ((offset & 1u) * 4u)) & 15u;
        paletteBank = eightBit ? (extendedPalette ? (tile >> 12u) * 256u : 0u) : (tile >> 12u) * 16u;
    }
    if (index == 0u) return vec4(0);
    uint entry = paletteBank + index, color;
    if (extendedPalette) {
        uint slot = layer < 2u && (control & 8192u) != 0u ? layer + 2u : layer;
        entry += slot * 4096u;
        color = backgrounds[engine].extendedPalette[entry / 2u] >> ((entry & 1u) * 16u);
    } else color = backgrounds[engine].palette[entry / 2u] >> ((entry & 1u) * 16u);
    return vec4(rgb(color), 1);
}
vec3 applyBrightness(vec3 color, uint engine, uint plane) {
    if (captureMode != 0u) return color;
    uint blend = backgrounds[engine].registers[20] & 65535u;
    if ((blend & (1u << plane)) != 0u) {
        float factor = float(min(backgrounds[engine].registers[21] & 31u, 16u)) / 16.0;
        uint effect = (blend >> 6u) & 3u;
        if (effect == 2u) color = mix(color, vec3(1), factor);
        if (effect == 3u) color *= 1.0 - factor;
    }
    uint brightness = backgrounds[engine].registers[27];
    float amount = float(min(brightness & 31u, 16u)) / 16.0;
    if ((brightness >> 14u) == 1u) return mix(color, vec3(1), amount);
    if ((brightness >> 14u) == 2u) return color * (1.0 - amount);
    return color;
}
void main() {
    uint param = attributes.x, poly = attributes.z, format = (param >> 26u) & 7u;
    if ((poly & 1073741824u) != 0u) {
        if (captureMode == 2u) discard;
        outColor = objectPixel(attributes.y, param, texcoord);
        if (spriteFilter != 0u && captureMode == 0u) {
            vec2 origin = floor(texcoord - 0.5) + 0.5, weight = fract(texcoord - 0.5);
            vec4 a = objectPixel(attributes.y, param, origin);
            vec4 b = objectPixel(attributes.y, param, origin + vec2(1, 0));
            vec4 c = objectPixel(attributes.y, param, origin + vec2(0, 1));
            vec4 d = objectPixel(attributes.y, param, origin + vec2(1, 1));
            a.rgb *= a.a; b.rgb *= b.a; c.rgb *= c.a; d.rgb *= d.a;
            outColor = mix(mix(a, b, weight.x), mix(c, d, weight.x), weight.y);
            if (outColor.a > 0) outColor.rgb /= outColor.a;
        }
        if (outColor.a == 0) discard;
        outColor.rgb = applyBrightness(outColor.rgb, attributes.y, 4u);
        return;
    }
    if ((poly & 2147483648u) != 0u) {
        if (captureMode == 2u) discard;
        outColor = backgroundPixel(attributes.y, param);
        if (outColor.a == 0) discard;
        outColor.rgb = applyBrightness(outColor.rgb, attributes.y, param == 4u ? 5u : param);
        return;
    }
    if (sceneDepth.z != 0 && (poly & 8388608u) == 0u) {
        ivec2 pixel = clamp(ivec2((gl_FragCoord.xy - sceneRect.xy) / sceneRect.zw * vec2(256, 192)),
                           ivec2(0), ivec2(255, 191));
        RearPixel rear = rearPixels[pixel.y * 256 + pixel.x];
        if ((poly & 4194304u) != 0u) {
            outColor = vec4(applyBrightness(rgb(rear.color), 0u, 0u), float((rear.color >> 16u) & 31u) / 31.0);
            return;
        }
        float limit = mix(sceneDepth.x, sceneDepth.y, rear.depth);
        if (gl_FragCoord.z > limit) discard;
    }
    // The DS cull bits enable the back and front faces independently.
    if ((poly & (gl_FrontFacing ? 128u : 64u)) == 0u) discard;
    int width = 8 << ((param >> 20u) & 7u), height = 8 << ((param >> 23u) & 7u);
    int x = coordinate(texcoord.x, width, (param & 65536u) != 0u, (param & 262144u) != 0u);
    int y = coordinate(texcoord.y, height, (param & 131072u) != 0u, (param & 524288u) != 0u);
    uint offset = uint(y * width + x), base = (param & 65535u) * 8u;
    uint palbase = attributes.y * (format == 2u ? 8u : 16u), index = 0u;
    vec4 tex = vec4(1);
    if (format == 1u || format == 6u) {
        uint value = byteAt(base + offset);
        index = value & (format == 1u ? 31u : 7u);
        tex = vec4(palette(palbase + index * 2u), format == 1u ? float(value >> 5u) / 7.0 : float(value >> 3u) / 31.0);
    } else if (format == 2u || format == 3u || format == 4u) {
        uint bits = format == 2u ? 2u : format == 3u ? 4u : 8u;
        index = (byteAt(base + offset * bits / 8u) >> ((offset * bits) & 7u)) & ((1u << bits) - 1u);
        tex = vec4(palette(palbase + index * 2u), index == 0u && (param & 536870912u) != 0u ? 0 : 1);
    } else if (format == 5u) {
        uint block = uint((y / 4) * (width / 4) + x / 4), address = base + block * 4u;
        index = (byteAt(address + uint(y & 3)) >> (uint(x & 3) * 2u)) & 3u;
        uint control = halfAt(131072u + ((address & 131071u) >> 1u) + ((address & 262144u) >> 2u));
        uint mode = control >> 14u, colors = palbase + (control & 16383u) * 4u;
        vec3 c0 = palette(colors), c1 = palette(colors + 2u);
        tex.rgb = palette(colors + index * 2u);
        if (mode == 1u && index == 2u) tex.rgb = (c0 + c1) * 0.5;
        if (mode == 3u && index >= 2u) tex.rgb = index == 2u ? (c0 * 5 + c1 * 3) / 8 : (c0 * 3 + c1 * 5) / 8;
        if (mode < 2u && index == 3u) tex.a = 0;
    } else if (format == 7u) {
        uint value = halfAt(base + offset * 2u);
        tex = vec4(rgb(value), (value & 32768u) != 0u ? 1 : 0);
    }
    uint mode = (poly >> 4u) & 3u;
    outColor = mode == 1u ? vec4(mix(vertexColor.rgb, tex.rgb, tex.a), vertexColor.a) : tex * vertexColor;
    if (outColor.a <= 0) discard;
    // Unused hardware bit 23 marks native UI; bits 24-29 remain the original polygon ID.
    if ((poly & 8388608u) == 0u)
        outColor.rgb = applyBrightness(outColor.rgb, 0u, 0u);
}
