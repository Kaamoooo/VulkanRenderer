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
#include "Descriptor.h"
#include "Image.h"
#include "Material.hpp"
#include "ShaderBuilder.h"
#include "Utils/JsonUtils.hpp"

#include "Sampler.h"
#include "RenderSystems/RenderSystem.h"
#include "RenderSystems/PointLightSystem.h"
#include "RenderSystems/ShadowSystem.h"
#include "RenderSystems/GrassSystem.h"
#include "RenderSystems/SkyBoxSystem.hpp"
#include "RenderSystems/RayTracingSystem.hpp"
#include "RenderSystems/PostSystem.hpp"

#include "ComponentFactory.hpp"
#include "GUI.hpp"

namespace Kaamoo {
    class Application {
    public:

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

    private:

        MyWindow m_window{SCENE_WIDTH + UI_LEFT_WIDTH + UI_LEFT_WIDTH_2, SCENE_HEIGHT, "VulkanTest"};
        Device m_device{m_window};
        Renderer m_renderer{m_window, m_device};

        std::shared_ptr<DescriptorPool> m_globalPool;
        GameObject::Map m_gameObjects;
        ShaderBuilder m_shaderBuilder;
        Material::Map m_materials;
        std::shared_ptr<VkRenderPass> m_shadowPass;
        std::shared_ptr<VkFramebuffer> m_shadowFramebuffer;

        std::vector<std::shared_ptr<RenderSystem>> m_renderSystems;
        std::shared_ptr<PostSystem> m_postSystem;
        std::shared_ptr<Buffer> m_pGameObjectDescBuffer; 

#ifdef RAY_TRACING
        std::shared_ptr<RayTracingSystem> m_rayTracingSystem;
        std::vector<GameObjectDesc> m_pGameObjectDescs;
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