#version 460

#include "RayTracingGlobal.glsl"
#include "../PBR.glsl"

layout (location = 0) rayPayloadInEXT hitPayLoad payLoad;
layout (location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
    vec2 uv;
};

struct PBR {
    vec3 albedo;//0~12
    int padding0;//12~16
    vec3 normal;//16~28
    float metallic;//28~32
    float roughness;//32~36
    float opacity;//36~40
    float AO;//40~44
    int padding;//44~48
    vec3 emissive;//48~60
};

struct GameObjectDesc {
    uint64_t verticesAddress;//0~8
    uint64_t indicesAddress;//8~16
    PBR pbr;//16~64
    ivec2 textureEntry;//64~72
    //    int padding[2];//72~80 causes error in NSight
    int padding0;//72~76
    int padding1;//76~80
};


layout (buffer_reference, std430) buffer VerticesBuffer {Vertex vertices[];};
layout (buffer_reference, std430) buffer IndicesBuffer { uint indices[]; };
layout (set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout (set = 1, binding = 1, std430) readonly buffer GameObjectDescBuffer {GameObjectDesc gameObjectDescs[];} gameObjectDescBuffer;
layout (set = 1, binding = 2) uniform sampler2D textureSamplers[];

PBR reloadPBR(PBR rawPBR, ivec2 textureEntry, vec2 uv, vec3 normal, mat3 TBN) {
    PBR pbr;
    int textureIndex = textureEntry.x;

    if (rawPBR.albedo == vec3(-1, -1, -1)) {
        pbr.albedo = texture(textureSamplers[textureIndex], uv).xyz;
        textureIndex++;
    } else {
        pbr.albedo = rawPBR.albedo;
    }

    if (rawPBR.normal == vec3(-1, -1, -1)) {
        vec3 texNormal = texture(textureSamplers[textureIndex], uv).xyz;
        pbr.normal = texNormal * 2.0 - 1.0;
        pbr.normal = TBN * pbr.normal;
        textureIndex++;
    } else {
        pbr.normal = normal;
    }

    if (rawPBR.metallic == -1) {
        pbr.metallic = texture(textureSamplers[textureIndex], uv).x;
        textureIndex++;
    } else {
        pbr.metallic = rawPBR.metallic;
    }

    if (rawPBR.roughness == -1) {
        pbr.roughness = texture(textureSamplers[textureIndex], uv).x;
        pbr.roughness = 0.5;
        textureIndex++;
    } else {
        pbr.roughness = rawPBR.roughness;
        pbr.roughness = 0.5;
    }

    if (rawPBR.opacity == -1) {
        pbr.opacity = texture(textureSamplers[textureIndex], uv).x;
        textureIndex++;
    } else {
        pbr.opacity = rawPBR.opacity;
    }

    if (rawPBR.AO == -1) {
        pbr.AO = texture(textureSamplers[textureIndex], uv).x;
    } else {
        pbr.AO = 1;
    }

    if (rawPBR.emissive == vec3(-1, -1, -1)) {
        pbr.emissive = texture(textureSamplers[textureIndex], uv).xyz;
    } else {
        pbr.emissive = rawPBR.emissive;
    }

    return pbr;
}

void main()
{
    GameObjectDesc gameObjectDesc = gameObjectDescBuffer.gameObjectDescs[gl_InstanceCustomIndexEXT];
    VerticesBuffer verticesBuffer = VerticesBuffer(gameObjectDesc.verticesAddress);
    IndicesBuffer indicesBuffer = IndicesBuffer(gameObjectDesc.indicesAddress);

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
    vec2 uv = barycentric.x * v0.uv + barycentric.y * v1.uv + barycentric.z * v2.uv;

    vec3 deltaPos1 = v1.position - v0.position;
    vec3 deltaPos2 = v2.position - v0.position;
    vec2 deltaUV1 = v1.uv - v0.uv;
    vec2 deltaUV2 = v2.uv - v0.uv;
    float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    vec3 tangent = normalize((deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r);
    vec3 bitangent = normalize((deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r);

    mat3 TBN = mat3(tangent, bitangent, normalize(cross(tangent, bitangent)));

    PBR pbr = reloadPBR(gameObjectDesc.pbr, gameObjectDesc.textureEntry, uv, normal, TBN);
    vec3 worldNormal = normalize(vec3(pbr.normal * gl_WorldToObjectEXT));

    vec3 lo = vec3(0, 0, 0);
    for (int i = 0; i < ubo.lightNum; i++) {
        //Directional Light for now
        Light light = ubo.lights[i];
        vec3 pixelToLight;
        float attenuation = 1;
        float shadowRayDistance = 10000;
        switch (light.lightCategory) {
            case 0:
                float distance = length(light.position.xyz - worldPos);
                shadowRayDistance = 100;
                attenuation = min(1, 1.0f / (distance * distance));
                pixelToLight = normalize(light.position.xyz - worldPos);
                break;
            case 1:
                pixelToLight = normalize(-light.direction.xyz);
                break;
            case 2:
                pixelToLight = normalize(light.position.xyz);
                break;
        }
        const vec3 pixelToView = -gl_WorldRayDirectionEXT;
        const vec3 brdf = cookTorrenceBRDF(worldNormal, pixelToView, pixelToLight, pbr.albedo, pbr.roughness, pbr.metallic);
        const vec3 radiance = light.color.xyz * light.color.w;
        const float geometry = clamp(dot(pixelToLight, worldNormal), 0, 1);

        float shadowMask = 1;
        if (geometry > 0.001) {
            float tMin = 0.0001;
            float tMax = shadowRayDistance;
            vec3 origin = worldPos + 0.0001 * worldNormal;
            vec3 direction = pixelToLight;
            uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
            isShadowed = true;
            traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 1, origin, tMin, direction, tMax, 1);
        }
        if (isShadowed) {
            shadowMask = 0.1;
        }

        lo += pbr.emissive + radiance * brdf * geometry * shadowMask * attenuation;
    }

    payLoad.hitValue = lo;
    //    payLoad.hitValue = viewDirection;
    //    hitValue = vec3(uv.xy, 0);
    //    hitValue = worldNormal;
    //    hitValue = pbr.normal;
    //    hitValue = vec3(pbr.albedo);
    //    hitValue = vec3(pbr.roughness);
    //    hitValue = vec3(pbr.opacity);
    //    hitValue = vec3(gameObjectDesc.pbr.metallic);
    //    hitValue = vec3(gameObjectDesc.pbr.opacity);
    //    hitValue = worldPos;
    //    hitValue = pos;
}   
