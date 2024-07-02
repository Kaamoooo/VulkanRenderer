#version 460

#include "RayTracingGlobal.glsl"

layout (location = 0) rayPayloadInEXT vec3 hitValue;
layout (location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
    vec2 uv;
};

layout (buffer_reference, std430) buffer VerticesBuffer {Vertex vertices[];};
layout (buffer_reference, std430) buffer IndicesBuffer { uint indices[]; };

struct Model {
    uint64_t verticesAddress;
    uint64_t indicesAddress;
    ivec2 textureEntry;
};

layout (set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout (set = 1, binding = 1) buffer ModelBuffer {Model models[];} modelBuffer;
layout (set = 1, binding = 2) uniform sampler2D textureSamplers[];

void main()
{
    hitValue = vec3(0, 1, 0);
}
