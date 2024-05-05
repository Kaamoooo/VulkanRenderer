#version 450
#include "../UBO.glsl"

layout(location = 0) in vec3 inUV;
layout (location=0) out vec4 outColor;

layout(set=1, binding=1) uniform samplerCube shadowMapTexture;
layout(set=1, binding=0) uniform samplerCube cubeMapTexture;

void main(){
    vec3 cubeMapUV = inUV;
    cubeMapUV.y*=-1;
    outColor = texture(cubeMapTexture, cubeMapUV);
    outColor = vec4(cubeMapUV, 1.0);
}