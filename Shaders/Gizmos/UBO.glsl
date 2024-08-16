#define RAY_TRACING 

struct Light {
    vec4 position;
    vec4 direction;
    vec4 color;
    // 0: point lights, 1: directional lights
    int lightCategory;
};

#ifdef RAY_TRACING
layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    float curTime;
    int lightNum;
    //constant light count for now
    Light lights[10];
} ubo;
#else
layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    Light lights[10];
    int lightNum;
    mat4 lightProjectionViewMatrix;
    float curTime;
    mat4 shadowViewMatrix[6];
    mat4 shadowProjMatrix;

} ubo;
#endif
