#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64: require
#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_ray_tracing: require
#extension GL_EXT_nonuniform_qualifier: enable

int MAX_BOUNCE_COUNT = 5;

struct hitPayLoad {
    vec3 hitValue;
    float opacity;
    float accumulatedDistance;
    int bounceCount;
    bool isBouncing;
    vec4 closestHitWorldPos;
    int recursionDepth;
};

struct ShadowPayload{
    bool opaqueShadowed;
    float opacity;
};

struct Light {
    vec4 position;
    vec4 direction;
    vec4 color;
    // 0: point lights, 1: directional lights
    int lightCategory;
};

struct PBR {
    vec3 albedo;//0~12
    int padding0;//12~16
    vec3 normal;//16~28
    float metallic;//28~32
    float roughness;//32~36
    float opacity;//36~40
    float AO;//40~44
    int padding;//44~48
    vec3 emissive;//48~60
};

struct GameObjectDesc {
    uint64_t verticesAddress;//0~8
    uint64_t indicesAddress;//8~16
    PBR pbr;//16~64
    ivec2 textureEntry;//64~72
    //    int padding[2];//72~80 causes error in NSight
    int padding0;//72~76
    int padding1;//76~80
};

layout (set = 1, binding = 0) uniform GlobalUbo {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    float curTime;
    int lightNum;
    //constant light count for now
    Light lights[10];
} ubo;

struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
    vec3 smoothedNormal;
    vec2 uv;
};

layout (buffer_reference, std430) buffer VerticesBuffer {Vertex vertices[];};

layout (buffer_reference, std430) buffer IndicesBuffer { uint indices[]; };
//layout(push_constant) uniform struct PushConstantRay {
//    vec4 clearColor;
//    vec3 lightPosition;
//    float lightIntensity;
//    int lightType;
//} pushConstantRay;