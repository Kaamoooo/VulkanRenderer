#version 450

layout(location=0) in vec3 fragColor;
layout(location=1) in vec4 worldPos;
layout(location=2) in vec4 worldNormal;


layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

struct PointLight{
    vec4 position;
    vec4 color;
};

layout(set=0, binding=0) uniform GlobalUbo{
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int lightNum;
} ubo;

void main(){
    vec3 ambientLightColor = ubo.ambientLightColor.xyz*ubo.ambientLightColor.w;
    vec3 totalDiffuse=vec3(0,0,0);
    
    for (int i=0;i<ubo.lightNum;i++){
        PointLight pointLight = ubo.pointLights[i];
        float lightDistance = distance(worldPos.xyz, pointLight.position.xyz);
        float attenuation = 1.0f/(lightDistance*lightDistance);
        vec3 directionToLight = normalize(pointLight.position-worldPos).xyz;
        vec3 pointLightColor = pointLight.color.xyz*pointLight.color.w*attenuation;
        vec3 diffuse = max(0, dot(worldNormal.xyz, directionToLight))*pointLightColor;
        totalDiffuse+=diffuse;
    }

    outColor=vec4(fragColor*(totalDiffuse+ambientLightColor), 1);
}