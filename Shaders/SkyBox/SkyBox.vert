#version 450
#include "../UBO.glsl"
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 outUV;

void main(){
    vec4 cameraPosition = ubo.inverseViewMatrix * vec4(0,0,0,1);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(position+cameraPosition.xyz,1);
    outUV=position;
}