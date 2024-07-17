#version 460
#extension GL_EXT_ray_tracing: require

#include "RayTracingGlobal.glsl"

layout(location=0) rayPayloadInEXT hitPayLoad payLoad;
layout(set = 1, binding = 3) uniform samplerCube skyboxSampler;
void main()
{
    vec3 cubeMapUV = gl_WorldRayDirectionEXT;
    cubeMapUV.y = -cubeMapUV.y;
    vec3 skyBoxColor = texture(skyboxSampler,cubeMapUV).rgb;
    payLoad.hitValue += (1 - payLoad.opacity) * skyBoxColor;
}