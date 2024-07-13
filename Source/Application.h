#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include <chrono>
#include "Device.hpp"
#include "MyWindow.hpp"
#include "Pipeline.hpp"
#include "Renderer.h"
#include "Model.hpp"
#include "GameObject.hpp"
#include "RenderSystems/RenderSystem.h"
#include "RenderSystems/PointLightSystem.h"
#include "Descriptor.h"
#include "Image.h"
#include "Material.hpp"
#include "ShaderBuilder.h"
#include "Utils/JsonUtils.hpp"

#include <Imgui/imgui.h>
#include <Imgui/imgui_impl_vulkan.h>
#include <Imgui/imgui_impl_glfw.h>

#include "Sampler.h"
#include "ShaderBuilder.h"
#include "RenderSystems/ShadowSystem.h"
#include "RenderSystems/GrassSystem.h"
#include "RenderSystems/SkyBoxSystem.hpp"
#include "RenderSystems/RayTracingSystem.hpp"
#include "RenderSystems/PostSystem.hpp"

#include "GUI.hpp"

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

        inline static MyWindow myWindow{WIDTH, HEIGHT, "VulkanTest"};
        inline static Device device{myWindow};
        inline static Renderer renderer{myWindow, device};

        std::shared_ptr<DescriptorPool> m_globalPool;
        GameObject::Map m_gameObjects;
        ShaderBuilder m_shaderBuilder;
        Material::Map m_materials;
        std::shared_ptr<VkRenderPass> m_shadowPass;
        std::shared_ptr<VkFramebuffer> m_shadowFramebuffer;
        
        std::vector<std::shared_ptr<RenderSystem>> m_renderSystems;
        std::shared_ptr<PostSystem> m_postSystem;
        
#ifdef RAY_TRACING
        std::shared_ptr<RayTracingSystem> m_rayTracingSystem;
        std::vector<GameObjectDesc> m_gameObjectDescs;
#else
        std::shared_ptr<ShadowSystem> m_shadowSystem;
#endif

        void loadGameObjects();

        void loadMaterials();

        void createRenderSystems();
        
        void UpdateComponents(FrameInfo &frameInfo);
        
        void UpdateUbo(GlobalUbo &ubo, float totalTime) noexcept;

        void Awake();
    };
}