#version 450
#include "UBO.glsl"

layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{
    mat4 modelMatrix;
    mat4 projectionViewMatrix;
} push; 

layout(set=1, binding=0) uniform ShadowUbo{
    mat4 projectionViewMatrix;
} shadowUbo;

void main(){
    outColor=vec4(0.5, 0.5, 0.5, 1);
}