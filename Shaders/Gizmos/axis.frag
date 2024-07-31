#version 450
#include "UBO.glsl"

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec4 worldPos;
layout (location = 2) in vec4 worldNormal;
layout (location = 3) in vec2 uv;


layout (location = 0) out vec4 outColor;

layout (push_constant) uniform PushConstantData {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;



void main() {
    outColor = vec4(fragColor, 1.0);
}