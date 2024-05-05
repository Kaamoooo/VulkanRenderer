#ifndef STRUCTURE_INFOS_INCLUDED
#define STRUCTURE_INFOS_INCLUDED

#include <vulkan/vulkan.h>
#include "GameObject.hpp"
#include "Material.hpp"

namespace Kaamoo {

#define MAX_LIGHT_NUM 10
#define MAX_SHADOW_NUM 3
    class GameObject;

    enum LightCategory {
        POINT_LIGHT = 0,
        DIRECTIONAL_LIGHT = 1
    };
    
    struct Light {
        glm::vec4 position{};
        glm::vec4 rotation{};
        glm::vec4 color{};
        // 0: point lights, 1: directional lights
        alignas(16) LightCategory lightCategory;
    };

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

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        std::unordered_map<id_t, GameObject> &gameObjects;
        Material::Map &materials;
        GlobalUbo& globalUbo;
    };
    
    struct RendererInfo{
        float aspectRatio;
    };
    
    struct ShadowUbo{
        glm::mat4 viewProjectionMatrix;
    };
}

#endif