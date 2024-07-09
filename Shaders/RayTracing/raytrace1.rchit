#version 460

#include "RayTracingGlobal.glsl"

layout (location = 0) rayPayloadInEXT vec3 hitValue;
layout (location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

layout (set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout (set = 1, binding = 2) uniform sampler2D textureSamplers[];

void main()
{
    hitValue = vec3(0, 1, 0);
}
