#version 450
#include "../UBO.glsl"

layout(location=0) in vec3 fragColor;
layout(location=1) in vec4 worldPos;
layout(location=2) in vec4 worldNormal;
layout(location = 3) in vec2 uv;


layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{
    mat4 modelMatrix;
    mat4 vaseModelMatrix;
} push;

layout(set=1, binding=0) uniform sampler2D texSampler;
layout(set=1, binding=1) uniform samplerCube shadowSampler;

void main(){
    vec3 texColor = fragColor;

    vec3 ambientLightColor = ubo.ambientLightColor.xyz*ubo.ambientLightColor.w;
    vec3 totalDiffuse=vec3(0, 0, 0);
    vec3 totalSpecular = vec3(0, 0, 0);

    vec3 worldCameraPos = ubo.inverseViewMatrix[3].xyz;
    vec3 viewDirection = normalize(worldPos.xyz - worldCameraPos);

    for (int i=0;i<ubo.lightNum;i++){
        Light light = ubo.lights[i];

        float lightDistance = distance(worldPos.xyz, light.position.xyz);
        float c0=1, c1=0.2, c2=0.2;
        float d = lightDistance;

        //point light
        float attenuation = min(1, 1/(c0+c1*d+c2*d*d))-0.3;
        vec3 directionToLight = normalize(light.position-worldPos).xyz;

        //directional light
        if (light.lightCategory==1){
            attenuation=1;
            directionToLight = normalize(-light.direction.xyz);
        }

        vec3 lightColorWithAttenuation = light.color.xyz*light.color.w*attenuation;

        vec3 tempNormal = normalize(vec3(0, -2, 0));
        tempNormal = worldNormal.xyz;

        float diffuseWeight0 = max(0, dot(tempNormal.xyz, directionToLight));
        float diffuseWeight1 = max(0, dot(-tempNormal.xyz, directionToLight));
        vec3 diffuse =max(diffuseWeight0, diffuseWeight1) *lightColorWithAttenuation;

        vec3 halfVector = normalize(viewDirection + directionToLight);
        float blinn0 = dot(halfVector, tempNormal.xyz)*0.5;
        float blinn1 = dot(halfVector, -tempNormal.xyz)*0.5;
        float blinn = max(blinn0, blinn1);
        blinn = clamp(blinn, 0, 1);
        blinn = pow(blinn, 32.0);

        totalSpecular+=lightColorWithAttenuation*blinn;
        totalDiffuse+=diffuse;
    }

    vec3 cubeMapDirection = worldPos.xyz-ubo.lights[0].position.xyz;
    cubeMapDirection.y = -cubeMapDirection.y;
    int cubeMapIndex = getCubeMapIndex(cubeMapDirection);

    vec4 lightFragPos = ubo.shadowProjMatrix*ubo.shadowViewMatrix[3]*worldPos;
    float depth = texture(shadowSampler, cubeMapDirection).x;
    float fragDepth = lightFragPos.z/lightFragPos.w;
    float shadowMask =clamp(0, 1, (fragDepth-depth)*10);
    vec4 lightingResult=vec4(fragColor*texColor*(totalDiffuse+ambientLightColor+totalSpecular), 1);
    outColor = lightingResult*(1-shadowMask);

}