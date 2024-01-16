#version 450

layout (vertices=3) out;

//In parameters
layout(location = 0) in vec3 inPosition[];
layout(location = 1) in vec3 inColor[];
layout(location = 2) in vec3 inNormal[];
layout(location = 3) in vec2 inUv[];

layout(location = 0) out vec3 outPosition[];
layout(location = 1) out vec3 outColor[];
layout(location = 2) out vec3 outNormal[];
layout(location = 3) out vec2 outUv[];

void main(){
    outPosition[gl_InvocationID] = inPosition[gl_InvocationID];
    outColor[gl_InvocationID]=inColor[gl_InvocationID];
    outNormal[gl_InvocationID]=inNormal[gl_InvocationID];
    outUv[gl_InvocationID]=inUv[gl_InvocationID];

    //Because the tess parameters need setting once per primitive, by declaring this we can save much performance consuming.
    if (gl_InvocationID==0){
        gl_TessLevelInner[0] = 8;
        gl_TessLevelOuter[0] = 6;
        gl_TessLevelOuter[1] = 6;
        gl_TessLevelOuter[2] = 6;
    }
}