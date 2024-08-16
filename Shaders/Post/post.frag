#version 450
layout (location = 0) in vec2 outUV;
layout (location = 0) out vec4 fragColor;

layout (set = 0, binding = 0) uniform sampler2D sampledImage[2];
layout (set = 0, binding = 1, rgba32f) uniform image2D storageImage[2];
struct Light {
    vec4 position;
    vec4 direction;
    vec4 color;
    // 0: point lights, 1: directional lights
    int lightCategory;
};

layout (set = 0, binding = 2) uniform GlobalUbo {
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    float curTime;
    int lightNum;
    Light lights[10];
} ubo;
layout (set = 0, binding = 3) uniform sampler2D worldPosImage;


layout (push_constant, std430) uniform PushConstant {
    int rayTracingImageIndex;
    bool firstFrame;
    mat4 viewMatrix[2];
} pushConstant;

const int filterSize = 5;
const float sigma_d = 10.0;
const float sigma_r = 0.1;

void main()
{
    vec2 uv = outUV;
    vec4 centerColor = texture(sampledImage[pushConstant.rayTracingImageIndex], uv);
    vec4 sum = vec4(0.0);
    float w_sum = 0.0;

    for (int x = -filterSize; x <= filterSize; x++)
    {
        for (int y = -filterSize; y <= filterSize; y++)
        {
            vec2 offset = vec2(x, y) / vec2(textureSize(sampledImage[pushConstant.rayTracingImageIndex], 0));
            vec4 neighborColor = texture(sampledImage[pushConstant.rayTracingImageIndex], uv + offset);
            float w = exp(-0.5 * (dot(offset, offset) / (sigma_d * sigma_d) + pow(length(centerColor - neighborColor), 2.0) / (sigma_r * sigma_r)));
            sum += neighborColor * w;
            w_sum += w;
        }
    }
//    vec4 denoisedColor = sum / w_sum;
    vec4 denoisedColor = centerColor;

    int lastFrameIndex = 1 - pushConstant.rayTracingImageIndex;
    mat4 lastFrameViewMatrix = pushConstant.viewMatrix[lastFrameIndex];
    vec3 worldPos = texture(worldPosImage, uv).xyz;
    vec4 clipPos = (ubo.projectionMatrix * lastFrameViewMatrix * vec4(worldPos, 1.0));
    clipPos /= clipPos.w;
    vec2 lastFrameUV = clipPos.xy * 0.5 + 0.5;
    vec4 lastFrameColor = texture(sampledImage[lastFrameIndex], lastFrameUV);
    vec4 outColor = lastFrameColor * 0.8 + denoisedColor * 0.2;
    if (pushConstant.firstFrame) {
        outColor = denoisedColor;
    }

    fragColor = outColor;
    ivec2 pixelCoords = ivec2(uv * vec2(textureSize(sampledImage[0], 0)));
    imageStore(storageImage[pushConstant.rayTracingImageIndex], pixelCoords, outColor);
}
