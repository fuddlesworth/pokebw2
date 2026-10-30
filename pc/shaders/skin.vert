#version 450
layout(set=1,binding=0) uniform SkinRect { vec4 rect; vec4 source; } skin;
layout(location=0) out vec2 uv;
void main() {
    const vec2 corners[6] = vec2[6](vec2(0,0),vec2(1,0),vec2(1,1),vec2(0,0),vec2(1,1),vec2(0,1));
    vec2 corner = corners[gl_VertexIndex];
    vec2 point = skin.rect.xy + corner * skin.rect.zw;
    gl_Position = vec4(point.x * 2 - 1, 1 - point.y * 2, 0, 1);
    uv = skin.source.xy + corner * skin.source.zw;
}
