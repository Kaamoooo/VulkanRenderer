#version 450
#include "UBO.glsl"
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 smoothedNormal;
layout (location = 4) in vec2 uv;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec4 worldPos;
layout (location = 2) out vec4 worldNormal;
layout (location = 3) out vec2 outUV;

layout (push_constant) uniform PushConstantData {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 scaleOrigin;
} push;

void main() {
    worldPos = push.modelMatrix * vec4(position + normal * 0.01f, 1);
    worldNormal = normalize(push.normalMatrix * vec4(normal, 0));
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPos;

    fragColor = color;
    outUV = uv;
}