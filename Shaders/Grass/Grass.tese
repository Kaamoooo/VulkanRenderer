#version 450
#include "../UBO.glsl"
layout (triangles, equal_spacing, ccw) in;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in vec3 inColor[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in vec2 inUv[];

layout(location=0) out vec3 fragColor;
layout(location=1) out vec4 worldPos;
layout(location=2) out vec4 worldNormal;
layout(location = 3) out vec2 uv;


layout(push_constant) uniform PushConstantData{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

vec3 barycentricInterpolation(vec3 x,vec3 y,vec3 z){
    return gl_TessCoord.x*x+gl_TessCoord.y*y+gl_TessCoord.z*z;
}

vec2 barycentricInterpolation2(vec2 x,vec2 y,vec2 z){
    return gl_TessCoord.x*x+gl_TessCoord.y*y+gl_TessCoord.z*z;
}


void main(){
    
    vec3 position= barycentricInterpolation(inPosition[0],inPosition[1],inPosition[2]);
    worldPos = push.modelMatrix*vec4(position, 1);
    vec3 normal = barycentricInterpolation(inNormal[0],inNormal[1],inNormal[2]);
    worldNormal = normalize(push.normalMatrix*vec4(normal, 0));

    fragColor = barycentricInterpolation(inColor[0],inColor[1],inColor[2]);
    uv=barycentricInterpolation2(inUv[0],inUv[1],inUv[2]);
}