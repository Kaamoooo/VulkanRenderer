#version 450

layout(location=0) in vec3 fragColor;

layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{
    mat4 transform;
    vec3 color;
    vec3 forwardDir;
} pushConstantData;



void main(){
//    outColor=vec4(pushConstantData.color, 1.0f);
    outColor=vec4(fragColor, 1.0f);
//    outColor=vec4(0 , 1.0f , 1.0f , 1.0f);
}