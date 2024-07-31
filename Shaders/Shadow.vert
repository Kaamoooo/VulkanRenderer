#version 450
#extension GL_EXT_multiview : enable
#include "UBO.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 smoothedNormal;
layout (location = 4) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec4 worldPos;
layout(location = 2) out vec4 worldNormal;
layout(location = 3) out vec2 outUV;

layout(push_constant) uniform PushConstantData{
    mat4 modelMatrix;
} push;

layout(set=1, binding=0) uniform ShadowUbo{
    mat4 projectionViewMatrix;
} shadowUbo;

void main(){
    vec4 position = vec4(position, 1);
    gl_Position = ubo.shadowProjMatrix*ubo.shadowViewMatrix[gl_ViewIndex]*push.modelMatrix*position;
}