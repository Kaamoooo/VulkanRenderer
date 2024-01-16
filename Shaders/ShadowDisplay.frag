#version 450
#include "UBO.glsl"

layout(location=0) in vec2 uv;

layout (location=0) out vec4 outColor;
 

layout(set=0, binding=1) uniform sampler2D image;

void main(){
    float depth=texture(image, uv).r;
//    outColor=vec4(depth,depth,depth,1);
    outColor=vec4(1 - (1-depth)*20);
//    outColor=vec4(0.8,1,0.5,1);
}