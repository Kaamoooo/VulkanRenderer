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

layout (push_constant, std430) uniform PushConstantData {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    float NormalScaleFactor = 0.015;

    worldNormal = normalize(push.normalMatrix * vec4(normalize(smoothedNormal), 0));
    worldPos = push.modelMatrix * vec4(position, 1);
    fragColor = color;
    outUV = uv;

    vec3 viewPos = vec3(ubo.viewMatrix * worldPos);
    vec3 viewNormal = normalize(ubo.viewMatrix * worldNormal).xyz;
    viewNormal.z = 0;
    viewNormal = normalize(viewNormal);
    vec3 viewNormalScaledPosition = viewPos + viewNormal * NormalScaleFactor;

    gl_Position = ubo.projectionMatrix * vec4((viewNormalScaledPosition), 1);


}