#version 450
layout (location = 0) in vec2 outUV;
layout (location = 0) out vec4 fragColor;

layout (set = 0, binding = 0) uniform sampler2D noisyTxt;


struct Light {
    vec4 position;
    vec4 direction;
    vec4 color;
    // 0: point lights, 1: directional lights
    int lightCategory;
};

layout (set = 0, binding = 1) uniform GlobalUbo {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    float curTime;
    int lightNum;
    Light lights[10];
} ubo;

//layout (set = 0, binding = 1) uniform GlobalUbo {
//    mat4 viewMatrix;
//    mat4 inverseViewMatrix;
//    mat4 projectionMatrix;
//    vec4 ambientLightColor;
//    Light lights[10];
//    int lightNum;
//    mat4 lightProjectionViewMatrix;
//    float curTime;
//    mat4 shadowViewMatrix[6];
//    mat4 shadowProjMatrix;
//
//} ubo;

layout (push_constant) uniform shaderInformation
{
    float aspectRatio;
}
pushc;

void main()
{
    vec2 uv = outUV;
    float gamma = 1. / 2.2;
        fragColor = pow(texture(noisyTxt, uv).rgba, vec4(gamma));
//    fragColor = vec4(0, ubo.curTime, 1.0, 1.0);
}
