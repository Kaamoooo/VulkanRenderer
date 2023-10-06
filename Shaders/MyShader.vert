#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

//uniform表示为全局变量，且由应用程序传入，通常与各种矩阵与光源位置等配合
layout(push_constant) uniform PushConstantData{
    mat4 transform;//projection * view * model
    mat4 normalMatrix;
} push;

const vec3 DIRECTION_TO_LIGHT=normalize(vec3(1, -3, -1));
const float AMBIENT=0.02;

void main(){
    gl_Position = push.transform *vec4(position, 1.0);

    vec3 worldNormal = normalize(mat3(push.normalMatrix)*normal);

    float lightIntensity =AMBIENT+ max(0, dot(worldNormal, DIRECTION_TO_LIGHT));

    fragColor=lightIntensity*color;
}