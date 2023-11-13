#version 450

layout(location=0) in vec3 fragColor;
layout(location=1) in vec4 worldPos;
layout(location=2) in vec4 worldNormal;
layout(location = 3) in vec2 uv;


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
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int lightNum;
    mat4 lightProjectionViewMatrix;
} ubo;


layout(set=1, binding=0) uniform sampler2D texSampler;
layout(set=1, binding=1) uniform sampler2D shadowSampler;

void main(){
    vec3 texColor = texture(texSampler, uv).xyz;

    vec3 ambientLightColor = ubo.ambientLightColor.xyz*ubo.ambientLightColor.w;
    vec3 totalDiffuse=vec3(0, 0, 0);
    vec3 totalSpecular = vec3(0, 0, 0);

    vec3 worldCameraPos = ubo.inverseViewMatrix[3].xyz;
    vec3 viewDirection = normalize(worldPos.xyz - worldCameraPos);

    for (int i=0;i<ubo.lightNum;i++){
        PointLight pointLight = ubo.pointLights[i];
        float lightDistance = distance(worldPos.xyz, pointLight.position.xyz);
        float attenuation = 1.0f/(lightDistance*lightDistance);
        vec3 directionToLight = normalize(pointLight.position-worldPos).xyz;
        vec3 pointLightColorWithAttenuation = pointLight.color.xyz*pointLight.color.w*attenuation;

        vec3 diffuse = max(0, dot(worldNormal.xyz, directionToLight))*pointLightColorWithAttenuation;

        vec3 halfVector = normalize(viewDirection + directionToLight);
        float blinn = dot(halfVector, worldNormal.xyz);
        blinn = clamp(blinn, 0, 1);
        blinn = pow(blinn, 32.0);

        totalSpecular+=pointLightColorWithAttenuation*blinn;
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