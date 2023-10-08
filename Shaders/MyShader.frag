#version 450

layout(location=0) in vec3 fragColor;
layout(location=1) in vec4 worldPos;
layout(location=2) in vec4 worldNormal;


layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

layout(set=0, binding=0) uniform GlobalUbo{
    mat4 projectionViewMatrix;
    vec4 ambientLightColor;
    vec4 pointLightPosition;
    vec4 pointLightColor;
} ubo;

void main(){
    float lightDistance = distance(worldPos.xyz,ubo.pointLightPosition.xyz);
    float attenuation = 1.0f/(lightDistance*lightDistance);

    vec3 directionToLight = normalize(ubo.pointLightPosition-worldPos).xyz;
    vec3 pointLightColor = ubo.pointLightColor.xyz*ubo.pointLightColor.w*attenuation;
    vec3 ambientLightColor = ubo.ambientLightColor.xyz*ubo.ambientLightColor.w;

    vec3 diffuse = max(0,dot(worldNormal.xyz,directionToLight))*pointLightColor;
    outColor=vec4(fragColor*(diffuse+ambientLightColor),1)*2;
}