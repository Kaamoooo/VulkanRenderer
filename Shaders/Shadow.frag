#version 450

layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{
    mat4 modelMatrix;
    mat4 projectionViewMatrix;
} push;

struct PointLight{
    vec4 position;
    vec4 color;
};

layout(set=0, binding=0) uniform GlobalUbo{
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int lightNum;
    mat4 lightProjectionViewMatrix;
} ubo;

layout(set=1, binding=0) uniform ShadowUbo{
    mat4 projectionViewMatrix;
} shadowUbo;

void main(){
    outColor=vec4(0.5, 0.5, 0.5, 1);
}