#version 450

layout(location = 0) in vec2 positions;
layout(location=1) in vec3 color;

//layout(location=0) out vec3 fragColor;

//uniform表示为全局变量，且由应用程序传入，通常与各种矩阵与光源位置等配合
layout(push_constant) uniform PushConstantData{
    mat2 transform;
    vec2 offset;
    vec3 color;
} pushConstantData;

void main(){
    gl_Position = vec4(pushConstantData.transform*(pushConstantData.offset+positions), 0.0, 1.0);
//    fragColor=color;
}