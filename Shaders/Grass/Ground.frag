#version 450
#include "../UBO.glsl"

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec4 worldPos;
layout (location = 2) in vec4 worldNormal0;
layout (location = 3) in vec2 uv;


layout (location = 0) out vec4 outColor;

layout (push_constant) uniform PushConstantData {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;


layout (set = 1, binding = 0) uniform sampler2D texSampler;
layout (set = 1, binding = 1) uniform sampler2D normalSampler;
layout (set = 1, binding = 2) uniform samplerCube shadowSampler;

void main() {
    vec3 texColor = texture(texSampler, uv).xyz;
    vec3 normalMapColor = texture(normalSampler, uv).xyz;
    vec3 normal = normalize(normalMapColor * 2.0 - 1.0);
    //Hard coded here, not constructing TBN, I am not planning to optimize rasterization pipeline for now
    vec4 worldNormal = normalize(vec4(normal.x,-normal.z,normal.y, 0));

    vec3 ambientLightColor = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 totalDiffuse = vec3(0, 0, 0);
    vec3 totalSpecular = vec3(0, 0, 0);

    vec3 worldCameraPos = ubo.inverseViewMatrix[3].xyz;
    vec3 viewDirection = normalize(-worldPos.xyz + worldCameraPos);

    for (int i = 0;i < ubo.lightNum; i++) {
        Light light = ubo.lights[i];

        if (light.lightCategory == -1) { continue; }

        float lightDistance = distance(worldPos.xyz, light.position.xyz);
        float c0 = 1, c1 = 0.002;
        float d = lightDistance;

        //point light
        float attenuation = min(1, 1 / (c0 + c1 * d));
        vec3 directionToLight = normalize(light.position.xyz - worldPos.xyz);

        //directional light
        if (light.lightCategory == 1) {
            attenuation = 1;
            directionToLight = normalize(-light.direction.xyz);
        }

        vec3 lightColorWithAttenuation = light.color.xyz * light.color.w * attenuation;

        vec3 diffuse = max(0, dot(worldNormal.xyz, directionToLight)) * lightColorWithAttenuation;
        vec3 halfVector = normalize(viewDirection + directionToLight);
        float blinn = dot(halfVector, worldNormal.xyz);
        blinn = clamp(blinn, 0, 1);
        blinn = pow(blinn, 32.0);

        totalSpecular += lightColorWithAttenuation * blinn;
        totalDiffuse += diffuse;
    }

    vec3 cubeMapDirection = worldPos.xyz - ubo.lights[0].position.xyz;
    cubeMapDirection.y = -cubeMapDirection.y;
    int cubeMapIndex = getCubeMapIndex(cubeMapDirection);
    vec4 lightFragPos = ubo.shadowProjMatrix * ubo.shadowViewMatrix[cubeMapIndex] * worldPos;
    float depth = texture(shadowSampler, cubeMapDirection).x;
    float fragDepth = lightFragPos.z / lightFragPos.w;
    float shadowMask = clamp(0, 1, (fragDepth - depth));
    vec4 lightingResult = vec4(fragColor * texColor * (totalDiffuse + ambientLightColor + totalSpecular), 1);
    outColor = lightingResult * (1 - shadowMask);
}