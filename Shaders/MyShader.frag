#version 450

//layout(location=0) in vec3 fragColor;

layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{
    mat2 transform;
    vec2 offset;
    vec3 color;
} pushConstantData;

void main(){
    //	outColor = vec4(0,1,1,1);
    //	outColor=vec4(fragColor,1.0);
    outColor=vec4(pushConstantData.color, 1.0f);
}