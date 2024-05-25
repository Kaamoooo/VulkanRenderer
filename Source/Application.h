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
//#include "Components/CameraComponent.hpp"
//#include "Components/Input/CameraMovementComponent.hpp"
//#include "Components/Input/ObjectMovementComponent.hpp"

namespace Kaamoo {
    class Application {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 800;

#ifdef RAY_TRACING
        inline const static std::string ConfigPath = "RayTracing/";
#else
        inline const static std::string ConfigPath = "Rasterization/";
#endif
        inline const static std::string BasePath = "../Configurations/" + ConfigPath;
        inline const static std::string BaseTexturePath = "../Textures/";
        inline const static std::string GameObjectsFileName = "GameObjects.json";
        inline const static std::string MaterialsFileName = "Materials.json";
        inline const static std::string ComponentsFileName = "Components.json";
        const int MATERIAL_NUMBER = 16;

        void run();

        Application();

        ~Application();

        Application(const Application &) = delete;

        Application &operator=(const Application &) = delete;

        static const Device &getDevice() { return device; }

        static const Renderer &getRenderer() { return renderer; }

        static const MyWindow &getWindow() { return myWindow; }

    private:
        void loadGameObjects();

        void loadMaterials();

        void UpdateComponents(FrameInfo &frameInfo);

        inline static MyWindow myWindow{WIDTH, HEIGHT, "VulkanTest"};
        inline static Device device{myWindow};
        inline static Renderer renderer{myWindow, device};

        GameObject::Map gameObjects;
        Material::Map materials;
        std::unique_ptr<DescriptorPool> globalPool;
        std::shared_ptr<VkRenderPass> shadowPass;
        std::shared_ptr<VkFramebuffer> shadowFramebuffer;

        void UpdateUbo(GlobalUbo &ubo, float totalTime, std::shared_ptr<ShadowSystem> shadowSystem);

        void Awake();
    };
}