#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include <chrono>
#include "MyWindow.hpp"
#include "Pipeline.hpp"
#include "Renderer.h"
#include "Device.hpp"
#include "Model.hpp"
#include "GameObject.hpp"
#include "RenderSystems/RenderSystem.h"
#include "RenderSystems/PointLightSystem.h"
#include "Descriptor.h"
#include "Image.h"
#include "Material.hpp"
#include "ShaderBuilder.h"
#include "Utils/JsonUtils.hpp"


#include "Sampler.h"
#include "ShaderBuilder.h"
#include "RenderSystems/ShadowSystem.h"
#include "RenderSystems/GrassSystem.h"
#include "RenderSystems/SkyBoxSystem.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/Input/CameraMovementComponent.hpp"
#include "Components/Input/ObjectMovementComponent.hpp"

namespace Kaamoo {
    class Application {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 800;

        void run();

        Application();

        Application(const Application &) = delete;

        Application &operator=(const Application &) = delete;

    private:
        void loadGameObjects();
        void loadMaterials();
        void updateLight(FrameInfo &frameInfo);
        void UpdateComponents(FrameInfo &frameInfo);
        
        MyWindow myWindow{WIDTH, HEIGHT, "VulkanTest"};
        Device device{myWindow};
        Renderer renderer{myWindow, device};

        GameObject::Map gameObjects;
        Material::Map materials;
        std::unique_ptr<DescriptorPool> globalPool;
        std::shared_ptr<VkRenderPass> shadowPass;
        std::shared_ptr<VkFramebuffer> shadowFramebuffer;
    };
}