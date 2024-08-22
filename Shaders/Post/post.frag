#version 450
layout (location = 0) in vec2 outUV;
layout (location = 0) out vec4 fragColor;


struct Light {
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
    int lightNum;
    Light lights[10];
} ubo;

layout (set = 0, binding = 1) uniform sampler2D sampledImage[2];

layout (push_constant, std430) uniform PushConstant {
    int rayTracingImageIndex;
    bool firstFrame;
    mat4 viewMatrix[2];
} pushConstant;

void main()
{
    vec2 uv = outUV;
    vec4 centerColor = texture(sampledImage[pushConstant.rayTracingImageIndex], uv);
    fragColor = centerColor;
}
