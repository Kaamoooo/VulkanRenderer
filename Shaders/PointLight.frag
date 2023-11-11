#version 450

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;

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


layout(push_constant) uniform Push{
    vec4 position;
    vec4 color;
    float radius;
}push;

const float PI=3.141592653;

void main() {
    float distance = sqrt(dot(fragOffset, fragOffset));
    if (distance>=1.0){
        discard;
    }

    outColor = vec4(push.color.xyz+0.2, 0.5*(cos(distance*PI)+0.1));
}
