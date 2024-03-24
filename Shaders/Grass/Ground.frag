#version 450
#include "../UBO.glsl"

layout(location=0) in vec3 fragColor;
layout(location=1) in vec4 worldPos;
layout(location=2) in vec4 worldNormal0;
layout(location = 3) in vec2 uv;


layout (location=0) out vec4 outColor;

layout(push_constant) uniform PushConstantData{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;


layout(set=1, binding=0) uniform sampler2D texSampler;
layout(set=1, binding=1) uniform sampler2D normalSampler;
layout(set=1, binding=2) uniform sampler2D shadowSampler;

void main(){
    vec3 texColor = texture(texSampler, uv).xyz;
    vec3 normalMapColor = texture(normalSampler, uv).xyz;
    vec3 normal = normalize(normalMapColor * 2.0 - 1.0);
    vec4 worldNormal = normalize(push.normalMatrix*vec4(normal, 0));
    
    vec3 ambientLightColor = ubo.ambientLightColor.xyz*ubo.ambientLightColor.w;
    vec3 totalDiffuse=vec3(0, 0, 0);
    vec3 totalSpecular = vec3(0, 0, 0);

    vec3 worldCameraPos = ubo.inverseViewMatrix[3].xyz;
    vec3 viewDirection = normalize(worldPos.xyz - worldCameraPos);

    for (int i=0;i<ubo.lightNum;i++){
        Light light = ubo.lights[i];
        
        float lightDistance = distance(worldPos.xyz, light.position.xyz);
        float c0=1,c1=0.2,c2=0.002;
        float d = lightDistance;
        
        //point light
        float attenuation = min(1,1/(c0+c1*d+c2*d*d));
        vec3 directionToLight = normalize(light.position-worldPos).xyz;
        
        //directrional light
        if(light.lightCategory==1){
            attenuation=1;
            directionToLight = normalize(-light.direction.xyz);
        }
        
        vec3 lightColorWithAttenuation = light.color.xyz*light.color.w*attenuation;

        vec3 diffuse = max(0, dot(worldNormal.xyz, directionToLight))*lightColorWithAttenuation;

        vec3 halfVector = normalize(viewDirection + directionToLight);
        float blinn = dot(halfVector, worldNormal.xyz);
        blinn = clamp(blinn, 0, 1);
        blinn = pow(blinn, 32.0);

        totalSpecular+=lightColorWithAttenuation*blinn;
        totalDiffuse+=diffuse;
    }


    vec4 lightFragPos = ubo.lightProjectionViewMatrix*worldPos;
    lightFragPos/=lightFragPos.w;
    float fragDepth = 0;

    vec3 tmpUV = (lightFragPos.xyz)/2+0.5;
    float depth=1;
    float shadowMask = 0;
    if (all(greaterThanEqual(tmpUV, vec3(0.0, 0,0))) && all(lessThanEqual(tmpUV, vec3(1,1.0, 1.0)))) {
        depth = texture(shadowSampler, tmpUV.xy).x;
        fragDepth=lightFragPos.z;
        shadowMask=clamp(0, 1, (fragDepth-depth)*10);
        //    outColor = vec4(1 - (1-fragDepth)*20);  
        //    outColor = vec4(depth-fragDepth);  
    }
    vec4 lightingResult=vec4(fragColor*texColor*(totalDiffuse+ambientLightColor+totalSpecular), 1);
    outColor = lightingResult*(1-shadowMask);
//    outColor = vec4(lightFragPos.z);
}