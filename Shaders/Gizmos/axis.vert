#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec4 worldPos;
layout (location = 2) out vec4 worldNormal;
layout (location = 3) out vec2 outUV;

struct Light{
    vec4 position;
    vec4 direction;
    vec4 color;
    // 0: point lights, 1: directional lights
    int lightCategory;
};

layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    float curTime;
    int frameIndex;
    int lightNum;
    //constant light count for now
    Light lights[10];
} ubo;

layout (push_constant) uniform PushConstantData {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    vec3 cameraWorldPos = vec3(ubo.inverseViewMatrix[3]);
    mat4 modelMatrix = push.modelMatrix;
    vec3 forward = normalize(ubo.inverseViewMatrix[2].xyz);
    float aspectRatio = ubo.projectionMatrix[1][1] / ubo.projectionMatrix[0][0];
    modelMatrix[3] = vec4(cameraWorldPos + forward * 5, 1);
    worldPos = modelMatrix * vec4(position, 1);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPos;

    worldNormal = normalize(push.normalMatrix * vec4(normal, 0));
    fragColor = color;
    outUV = uv;
}