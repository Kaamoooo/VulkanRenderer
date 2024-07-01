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
    Model model = modelBuffer.models[gl_InstanceCustomIndexEXT];
    VerticesBuffer verticesBuffer = VerticesBuffer(model.verticesAddress);
    IndicesBuffer indicesBuffer = IndicesBuffer(model.indicesAddress);

    uint i0 = indicesBuffer.indices[gl_PrimitiveID * 3];
    uint i1 = indicesBuffer.indices[gl_PrimitiveID * 3 + 1];
    uint i2 = indicesBuffer.indices[gl_PrimitiveID * 3 + 2];
    Vertex v0 = verticesBuffer.vertices[i0];
    Vertex v1 = verticesBuffer.vertices[i1];
    Vertex v2 = verticesBuffer.vertices[i2];

    vec3 barycentric = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos = barycentric.x * v0.position + barycentric.y * v1.position + barycentric.z * v2.position;
    vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));

    vec3 normal = normalize(barycentric.x * v0.normal + barycentric.y * v1.normal + barycentric.z * v2.normal);
    vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

    vec2 uv = barycentric.x * v0.uv + barycentric.y * v1.uv + barycentric.z * v2.uv;

    //Temporarily difined here
    vec3 lightDir = normalize(vec3(0, 1, 1));
    float lightIntensity = max(dot(worldNormal, -lightDir), 0.0);
    if (lightIntensity > 0.001) {
        float tMin = 0.0001;
        //Todo: This is directional light hence the tMax is set to a large value
        float tMax = 10000;
        vec3 origin = worldPos + 0.001 * worldNormal;
        vec3 direction = -lightDir;
        uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        isShadowed = true;
        traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 1, origin, tMin, direction, tMax, 1);
    }
    float attenuation = 1;
    if (isShadowed) {
        attenuation = 0.1;
    }

    vec3 texColor = texture(textureSamplers[model.textureEntry.x], uv).xyz;
    hitValue = lightIntensity * attenuation * vec3(1, 1, 1) * texColor;
    //    hitValue = vec3(0, 1, 0);
}
