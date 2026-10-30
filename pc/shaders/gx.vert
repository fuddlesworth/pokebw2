#version 450
layout(location=0) in vec4 clip;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 color;
layout(location=3) in uvec3 material;
layout(location=0) out vec2 texcoord;
layout(location=1) out vec4 vertexColor;
layout(location=2) flat out uvec3 attributes;
void main() {
    // SDL's GPU API already normalizes Vulkan's viewport orientation.
    gl_Position = vec4(clip.x, clip.y, (clip.z + clip.w) * 0.5, clip.w);
    texcoord = uv;
    vertexColor = color;
    attributes = material;
}
