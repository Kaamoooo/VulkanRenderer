#pragma once

#include <vulkan/vulkan.h>
#include "GameObject.hpp"
#include "Material.hpp"

namespace Kaamoo {

#define MAX_LIGHT_NUM 10
#define MAX_SHADOW_NUM 3
    class GameObject;

    enum LightCategory {
        NONE = -1,
        POINT_LIGHT = 0,
        DIRECTIONAL_LIGHT = 1
    };
    
    struct Light {
        glm::vec4 position{};
        glm::vec4 direction{};
        glm::vec4 color{};
        // 0: point lights, 1: directional lights
        alignas(16) LightCategory lightCategory;
    };
    
#ifdef RAY_TRACING
    struct GlobalUbo {
        glm::mat4 viewMatrix{1.f};
        glm::mat4 inverseViewMatrix{1.f};
        glm::mat4 projectionMatrix{1.f};
        glm::mat4 inverseProjectionMatrix{1.f};
        float curTime;
        int lightNum;
        alignas(16)Light lights[MAX_LIGHT_NUM];
    };

    struct GameObjectDesc {
        uint64_t vertexBufferAddress;//0~8
        uint64_t indexBufferAddress; //8~16
        PBR pbr; //16~64
        glm::i32vec2 textureEntry; //64~72
    };

#else
    struct GlobalUbo {
        glm::mat4 viewMatrix{1.f};
        glm::mat4 inverseViewMatrix{1.f};
        glm::mat4 projectionMatrix{1.f};
        glm::vec4 ambientColor{1, 1, 1, 0.005f};
        Light lights[MAX_LIGHT_NUM];
        alignas(16) int lightNum;
        alignas(16) glm::mat4 lightProjectionViewMatrix;
        alignas(16) float curTime;
        alignas(16) glm::mat4 shadowViewMatrix[6];
        alignas(16) glm::mat4 shadowProjMatrix;
    };
#endif

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        std::unordered_map<id_t, GameObject> &gameObjects;
        Material::Map &materials;
        GlobalUbo& globalUbo;
        VkExtent2D extent;
        id_t selectedGameObjectId;
        bool sceneUpdated;
    };
    
    struct RendererInfo{
        float aspectRatio;
    };
    
    struct ShadowUbo{
        glm::mat4 viewProjectionMatrix;
    };
}