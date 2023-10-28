#pragma once

#include "Camera.h"
#include <vulkan/vulkan.h>
#include "GameObject.h"
#include "Material.h"

namespace Kaamoo {

#define MAX_LIGHT_NUM 10

    struct PointLight {
        glm::vec4 position{};
        glm::vec4 color{};
    };

    struct GlobalUbo {
        glm::mat4 viewMatrix{1.f};
        glm::mat4 inverseViewMatrix{1.f};
        glm::mat4 projectionMatrix{1.f};
        glm::vec4 ambientColor{1, 1, 1, 0.02f};
        PointLight pointLights[MAX_LIGHT_NUM];
        int lightNum;
    };

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        Camera &camera;
        GameObject::Map &gameObjects;
        Material::Map &materials;
        GlobalUbo& globalUbo;
    };
}