#version 450

layout(location=0) in vec2 uv;

layout (location=0) out vec4 outColor;

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

layout(set=0, binding=1) uniform sampler2D image;

void main(){
    float depth=texture(image, uv).r;
//    outColor=vec4(depth,depth,depth,1);
    outColor=vec4(1 - (1-depth)*20);
//    outColor=vec4(0.8,1,0.5,1);
}