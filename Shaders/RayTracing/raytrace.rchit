#version 460

#include "RayTracingGlobal.glsl"
#include "../PBR.glsl"
#include "../Utils/random.glsl"

layout (location = 0) rayPayloadInEXT hitPayLoad payLoad;
layout (location = 1) rayPayloadEXT ShadowPayload shadowPayload;

hitAttributeEXT vec3 attribs;

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
    payLoad.recursionDepth++;
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

    if (payLoad.recursionDepth == 1) {
        payLoad.closestHitWorldPos = vec4(worldPos, 1);
    }

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
                shadowRayDistance = distance;
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

        float shadowMask = 0;
        if (geometry > 0.001 || pbr.opacity < 0.99) {
            float tMin = 0.0001;
            float tMax = shadowRayDistance;
            vec3 origin = worldPos;
            vec3 direction = pixelToLight;
            uint flags = gl_RayFlagsSkipClosestHitShaderEXT;
            shadowPayload.opaqueShadowed = true;
            shadowPayload.opacity = 0;
            traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 1, origin, tMin, direction, tMax, 1);
            if (shadowPayload.opaqueShadowed) {
                shadowMask = 0;
            } else {
                shadowMask = 1 - shadowPayload.opacity;
            }
        }

        lo += radiance * brdf * geometry * shadowMask * attenuation;
    }

    if (payLoad.isBouncing) {
        lo = (lo + pbr.emissive) * pbr.opacity;
    } else {
        lo = (1 - payLoad.opacity) * (lo + pbr.emissive) * pbr.opacity;
        payLoad.opacity += (1 - payLoad.opacity) * pbr.opacity;
    }

    if (payLoad.bounceCount == 0) {
        payLoad.hitValue += lo;
    } else {
        payLoad.accumulatedDistance += distance(gl_WorldRayOriginEXT, worldPos);
        payLoad.hitValue += lo * (1.0 / pow(payLoad.accumulatedDistance + 1, 1));
    }

    //Global illumination
    if (payLoad.bounceCount < MAX_BOUNCE_COUNT)
    {
        float tMin = 0.001;
        float tMax = 1000;
        vec3 randomVector = randomHemisphereVector(worldNormal, ubo.curTime + worldPos.x + worldPos.y + worldPos.z);
        vec3 reflectVector = reflect(gl_WorldRayDirectionEXT, worldNormal) + randomVector * pbr.roughness;
        uint flags = gl_RayFlagsNoneEXT;
        payLoad.bounceCount++;
        payLoad.isBouncing = true;
        traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 0, worldPos, tMin, reflectVector, tMax, 0);
    }

    //Transparent
    if (pbr.opacity < 0.99 && payLoad.opacity < 0.99) {
        float tMin = 0.001;
        float tMax = 1000;
        vec3 origin = worldPos;
        vec3 direction = gl_WorldRayDirectionEXT;
        uint flags = gl_RayFlagsNoneEXT;
        payLoad.isBouncing = false;
        traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 0, origin, tMin, direction, tMax, 0);
    }

}   
