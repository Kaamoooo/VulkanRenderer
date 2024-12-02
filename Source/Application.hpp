#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include <chrono>
#include <glm/ext/matrix_clip_space.hpp>
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
#include "RenderSystems/GizmosRenderSystem.hpp"
#include "RenderSystems/ComputeSystem.hpp"

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
        inline const static std::string SkyboxCubeMapName = "Cubemap/SwedishRoyalCastle";
        const int MATERIAL_NUMBER = 16;

        void run();

        Application();

        ~Application();

        Application(const Application &) = delete;

        Application &operator=(const Application &) = delete;

    private:

        MyWindow m_window{SCENE_WIDTH + UI_LEFT_WIDTH + UI_LEFT_WIDTH_2, SCENE_HEIGHT, "Tiny Vulkan Renderer"};
        Device m_device{m_window};
        Renderer m_renderer{m_window, m_device};

        std::shared_ptr<DescriptorPool> m_globalPool;
        GameObject::Map m_gameObjects;
        HierarchyTree m_hierarchyTree;
        ShaderBuilder m_shaderBuilder;
        Material::Map m_materials;
        std::shared_ptr<VkRenderPass> m_shadowPass;
        std::shared_ptr<VkFramebuffer> m_shadowFramebuffer;

        std::vector<std::shared_ptr<RenderSystem>> m_renderSystems;
        std::shared_ptr<PostSystem> m_postSystem;
        std::shared_ptr<GizmosRenderSystem> m_gizmosRenderSystem;
        std::shared_ptr<ComputeSystem> m_computeSystem;
        std::shared_ptr<Buffer> m_pGameObjectDescBuffer;

#ifdef RAY_TRACING
        std::shared_ptr<RayTracingSystem> m_rayTracingSystem;
        std::vector<GameObjectDesc> m_pGameObjectDescs;
#else
        std::shared_ptr<ShadowSystem> m_shadowSystem;
#endif

        void loadGameObjects();

        void loadMaterials();

        void createRenderSystems() {
            for (auto &material: m_materials) {
                auto pipelineCategory = material.second->getPipelineCategory();
#ifdef RAY_TRACING
                if (pipelineCategory == PipelineCategory.RayTracing) {
                    m_rayTracingSystem = std::make_shared<RayTracingSystem>(m_device, nullptr, material.second);
                    m_rayTracingSystem->Init();
                }
                if (pipelineCategory == PipelineCategory.Post) {
                    m_postSystem = std::make_shared<PostSystem>(m_device, m_renderer.getSwapChainRenderPass(), material.second);
                    m_postSystem->Init();
                }
                if (pipelineCategory == PipelineCategory.Compute) {
                    m_computeSystem = std::make_shared<ComputeSystem>(m_device, nullptr, material.second);
                    m_computeSystem->Init();
                }
#else
                if (pipelineCategory == PipelineCategory.Shadow) {
                m_shadowSystem = std::make_shared<ShadowSystem>(m_device, m_renderer.getShadowRenderPass(), material.second);
                continue;
            } else if (pipelineCategory == PipelineCategory.TessellationGeometry) {
                auto renderSystem = std::make_shared<GrassSystem>(m_device, m_renderer.getSwapChainRenderPass(), material.second);
                renderSystem->Init();
                m_renderSystems.push_back(std::dynamic_pointer_cast<RenderSystem>(renderSystem));
                continue;
            } else if (pipelineCategory == PipelineCategory.SkyBox) {
                auto renderSystem = std::make_shared<SkyBoxSystem>(m_device, m_renderer.getSwapChainRenderPass(), material.second);
                renderSystem->Init();
                m_renderSystems.push_back(std::dynamic_pointer_cast<RenderSystem>(renderSystem));
                continue;
            } else if (pipelineCategory == PipelineCategory.Opaque) {
                auto renderSystem = std::make_shared<RenderSystem>(m_device, m_renderer.getSwapChainRenderPass(), material.second);
                renderSystem->Init();
                m_renderSystems.push_back(renderSystem);
            }
#endif
                if (pipelineCategory == PipelineCategory.Gizmos) {
                    m_gizmosRenderSystem = std::make_shared<GizmosRenderSystem>(m_device, m_renderer.getSwapChainRenderPass(), material.second);
//          This render system contains multiple pipelines, so I initialize it in the constructor.
//          m_gizmosRenderSystem->Init();   
                }
            }
        }

        void UpdateUbo(GlobalUbo &ubo, float totalTime) {
            ubo.lightNum = LightComponent::GetLightNum();
            ubo.curTime = totalTime;
#ifndef RAY_TRACING
            ubo.shadowViewMatrix[0] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(0, 90, 180));
        ubo.shadowViewMatrix[1] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(0, -90, 180));
        ubo.shadowViewMatrix[2] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(-90, 0, 0));
        ubo.shadowViewMatrix[3] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(90, 0, 0));
        ubo.shadowViewMatrix[4] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(180, 0, 0));
        ubo.shadowViewMatrix[5] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(0, 0, 180));
        ubo.shadowProjMatrix = CameraComponent::CorrectionMatrix * glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 5.0f);
        ubo.lightProjectionViewMatrix = ubo.shadowProjMatrix * ubo.shadowViewMatrix[0];
#endif

        }

        void Awake();

        void UpdateComponents(FrameInfo &frameInfo) {
            ComponentUpdateInfo updateInfo{};
            RendererInfo rendererInfo{m_renderer.getAspectRatio()};
            updateInfo.frameInfo = &frameInfo;
            updateInfo.rendererInfo = &rendererInfo;

            static bool firstFrame = true;
            if (firstFrame) {
                for (auto &pair: m_gameObjects) {
                    updateInfo.gameObject = &pair.second;
                    pair.second.Start(updateInfo);
                }
                firstFrame = false;
            }
            
            FixedUpdateComponents(frameInfo);

            for (auto &pair: m_gameObjects) {
                updateInfo.gameObject = &pair.second;
                pair.second.Update(updateInfo);
            }

            for (auto &pair: m_gameObjects) {
                updateInfo.gameObject = &pair.second;
                pair.second.LateUpdate(updateInfo);
            }
        }

        void FixedUpdateComponents(FrameInfo &frameInfo) {
            static float reservedFrameTime = 0;
            float frameTime = frameInfo.frameTime + reservedFrameTime;
//            int times = 0;
            while (frameTime >= FIXED_UPDATE_INTERVAL) {
//                times++;
                frameTime -= FIXED_UPDATE_INTERVAL;

                ComponentUpdateInfo updateInfo{};
                RendererInfo rendererInfo{m_renderer.getAspectRatio()};
                updateInfo.frameInfo = &frameInfo;
                updateInfo.rendererInfo = &rendererInfo;
                for (auto &pair: m_gameObjects) {
                    updateInfo.gameObject = &pair.second;
                    pair.second.FixedUpdate(updateInfo);
                }
            }
            reservedFrameTime = frameTime;
//            static int frameIndex = 0;
//            std::cout << "Frame " << frameIndex << " updated " << times << " times: " << frameInfo.frameTime << std::endl;
//            frameIndex++;
        }
    };
}