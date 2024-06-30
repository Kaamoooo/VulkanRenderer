#version 460

#include "RayTracingGlobal.glsl"

layout (location = 0) rayPayloadInEXT vec3 hitValue;
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
};

layout (set = 1, binding = 1) buffer ModelBuffer {Model models[];} modelBuffer;

//Vertex unpack(int index){
//    
//}

void main()
{
    Model model = modelBuffer.models[gl_InstanceCustomIndexEXT];
    VerticesBuffer verticesBuffer = VerticesBuffer(model.verticesAddress);
    IndicesBuffer indicesBuffer = IndicesBuffer(model.indicesAddress);
    
    uint i0 = indicesBuffer.indices[gl_PrimitiveID*3];
    uint i1 = indicesBuffer.indices[gl_PrimitiveID*3 + 1];
    uint i2 = indicesBuffer.indices[gl_PrimitiveID*3 + 2];
    Vertex v0 = verticesBuffer.vertices[i0];
    Vertex v1 = verticesBuffer.vertices[i1];
    Vertex v2 = verticesBuffer.vertices[i2];
    
    vec3 barycentric = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos = barycentric.x * v0.position + barycentric.y * v1.position + barycentric.z * v2.position;
    vec3 worldPos = vec3(gl_ObjectToWorldEXT*vec4(pos, 1.0));
    
    vec3 normal = normalize(barycentric.x * v0.normal + barycentric.y * v1.normal + barycentric.z * v2.normal);
    vec3 worldNormal = normalize(vec3(normal*gl_WorldToObjectEXT));

    //Temporarily difined here
    vec3 lightDir = normalize(vec3(0, 1, 1));
    float lightIntensity = max(dot(worldNormal, -lightDir), 0.0);
    hitValue = lightIntensity*vec3(1, 1, 1);

//    hitValue = vec3(0, 1, 0);
}
