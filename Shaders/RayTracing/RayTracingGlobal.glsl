struct hitPayLoad {
    vec3 hitValue;
};

struct Light {
    vec4 position;
    vec4 direction;
    vec4 color;
    // 0: point lights, 1: directional lights
    int lightCategory;
};

layout (set = 1, binding = 0) uniform GlobalUbo {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    float curTime;
    int lightNum;
    Light lights[10];
} ubo;

layout (set = 0, binding = 2) uniform GlobalUbo1 {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    float curTime;
    int lightNum;
    Light lights[10];
} ubo1;
//
//layout(push_constant) uniform struct PushConstantRay {
//    vec4 clearColor;
//    vec3 lightPosition;
//    float lightIntensity;
//    int lightType;
//} pushConstantRay;