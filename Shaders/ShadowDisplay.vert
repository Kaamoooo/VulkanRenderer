#version 450
#include "UBO.glsl"

layout(location = 0) out vec2 outUv;
 

const vec2 OFFSETS[6] = vec2[](
vec2(-1, -1),
vec2(-1, 1),
vec2(1, -1),
vec2(1, -1),
vec2(-1, 1),
vec2(1, 1)
);


vec2 pivot=vec2(0.1, 0.1);
float radius=0.4;

void main(){
    vec2 offset = OFFSETS[gl_VertexIndex];
    vec3 worldCameraRight = vec3(ubo.viewMatrix[0][0], ubo.viewMatrix[1][0], ubo.viewMatrix[2][0]);
    vec3 worldCameraUp = vec3(ubo.viewMatrix[0][1], ubo.viewMatrix[1][1], ubo.viewMatrix[2][1]);

    vec3 worldPos = vec3(1, -1, 1) +
    radius*offset.x*worldCameraRight.xyz+
    radius*offset.y*worldCameraUp.xyz;

    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(worldPos, 1);
    vec2 tmpUv = (offset+1)/2;
    outUv = vec2(tmpUv.x,1-tmpUv.y);
    //    vec2 clipPos = (offset*radius+pivot);
    //    gl_Position = vec4(clipPos, 1, 1);
}