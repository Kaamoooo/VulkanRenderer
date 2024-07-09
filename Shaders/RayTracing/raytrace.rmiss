#version 460
#extension GL_EXT_ray_tracing: require

#include "RayTracingGlobal.glsl"

layout(location=0) rayPayloadInEXT hitPayLoad payLoad;

void main()
{
    payLoad.hitValue += (1 - payLoad.opacity) * vec3(1, 0, 0);
}