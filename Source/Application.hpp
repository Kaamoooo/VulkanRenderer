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

#include "ComponentFactory.hpp"
#include "GUI.hpp"
#include "RenderManager.hpp"

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
        GlobalUbo m_ubo{};
        GameObject::Map m_gameObjects;
        HierarchyTree m_hierarchyTree;
        ShaderBuilder m_shaderBuilder;
        Material::Map m_materials;
        std::shared_ptr<VkRenderPass> m_shadowPass;
        std::shared_ptr<VkFramebuffer> m_shadowFramebuffer;

        std::unique_ptr<RenderManager> m_renderManager;

#ifdef RAY_TRACING
        std::shared_ptr<Buffer> m_pGameObjectDescBuffer;
        std::vector<GameObjectDesc> m_pGameObjectDescs;
#endif

        void loadGameObjects();

        void loadMaterials();

        void createRenderSystems() {
            m_renderManager->CreateRenderSystems(m_materials, m_device, m_renderer);
        }

        void UpdateUbo(float totalTime) {
            m_renderManager->UpdateUbo(m_ubo, totalTime);
        }

        void Awake();

        void UpdateComponents(FrameInfo &frameInfo) {
            ComponentUpdateInfo updateInfo{};
            RendererInfo rendererInfo{m_renderer.getAspectRatio(), m_renderer.FOV_Y, m_renderer.NEAR_CLIP, m_renderer.FAR_CLIP};
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
                for (auto &pair: m_gameObjects) {
                    updateInfo.gameObject = &pair.second;
                    pair.second.LateFixedUpdate(updateInfo);
                }
            }
            reservedFrameTime = frameTime;
//            static int frameIndex = 0;
//            std::cout << "Frame " << frameIndex << " updated " << times << " times: " << frameInfo.frameTime << std::endl;
//            frameIndex++;
        }

        void UpdateRendering(FrameInfo &frameInfo) {
#ifdef RAY_TRACING
            frameInfo.pGameObjectDescBuffer = m_pGameObjectDescBuffer;
            frameInfo.pGameObjectDescs = m_pGameObjectDescs;
#endif
            m_renderManager->UpdateRendering(m_renderer, frameInfo, m_hierarchyTree);
        }
    };
}