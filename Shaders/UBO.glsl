struct Light{
    vec4 position;
    vec4 direction;
    vec4 color;
// 0: point lights, 1: directional lights
    int lightCategory;
};

layout(set=0, binding=0) uniform GlobalUbo{
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    vec4 ambientLightColor;
    Light lights[10];
    int lightNum;    
    mat4 lightProjectionViewMatrix;
    float curTime;
} ubo;
