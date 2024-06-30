#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_ray_tracing: require
#extension GL_EXT_nonuniform_qualifier: enable

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


//layout(push_constant) uniform struct PushConstantRay {
//    vec4 clearColor;
//    vec3 lightPosition;
//    float lightIntensity;
//    int lightType;
//} pushConstantRay;