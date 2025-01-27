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
#include "ResourceManager.hpp"
#include "RenderManager.hpp"

namespace Kaamoo {
    class Application {
    public:
        Application() {
            m_resourceManager = std::make_shared<ResourceManager>();
            m_renderManager = std::make_unique<RenderManager>(m_resourceManager);
        }

        ~Application() {
            GUI::Destroy();
        }
        
        void run(){
            auto currentTime = std::chrono::high_resolution_clock::now();
            float totalTime = 0;
            Awake();
            auto& _window = m_resourceManager->GetWindow();
            auto& _renderer = m_resourceManager->GetRenderer();
            auto& _gameObjects = m_resourceManager->GetGameObjects();
            auto& _materials = m_resourceManager->GetMaterials();
            auto& _device = m_resourceManager->GetDevice();
            while (!_window.shouldClose()) {
                glfwPollEvents();
                auto newTime = std::chrono::high_resolution_clock::now();
                float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
                totalTime += frameTime;
                currentTime = newTime;

                if (auto commandBuffer = _renderer.beginFrame()) {
                    int frameIndex = _renderer.getFrameIndex();
                    FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, _gameObjects, _materials, m_ubo, _window.getCurrentExtent(), GUI::GetSelectedId(), false};
                    UpdateComponents(frameInfo);
                    UpdateUbo(totalTime);
                    UpdateRendering(frameInfo);
                }

            }
            vkDeviceWaitIdle(_device.device());
        };

        Application(const Application &) = delete;

        Application &operator=(const Application &) = delete;

    private:
        GlobalUbo m_ubo{};
        std::shared_ptr<VkFramebuffer> m_shadowFramebuffer;

        std::unique_ptr<RenderManager> m_renderManager;
        std::shared_ptr<ResourceManager> m_resourceManager;

        void UpdateUbo(float totalTime) {
            m_renderManager->UpdateUbo(m_ubo, totalTime);
        }

        void Awake(){
            auto& _gameObjects = m_resourceManager->GetGameObjects();

            ComponentAwakeInfo awakeInfo{};
            for (auto &pair: _gameObjects) {
                awakeInfo.gameObject = &pair.second;
                pair.second.Awake(awakeInfo);
            }
        }

        void UpdateComponents(FrameInfo &frameInfo) {
            ComponentUpdateInfo updateInfo{};
            auto& _renderer = m_resourceManager->GetRenderer();
            auto& _gameObjects = m_resourceManager->GetGameObjects();
            RendererInfo rendererInfo{_renderer.getAspectRatio(), _renderer.FOV_Y, _renderer.NEAR_CLIP, _renderer.FAR_CLIP};
            updateInfo.frameInfo = &frameInfo;
            updateInfo.rendererInfo = &rendererInfo;

            static bool firstFrame = true;
            if (firstFrame) {
                for (auto &pair: _gameObjects) {
                    updateInfo.gameObject = &pair.second;
                    pair.second.Start(updateInfo);
                }
                firstFrame = false;
            }

            FixedUpdateComponents(frameInfo);

            for (auto &pair: _gameObjects) {
                updateInfo.gameObject = &pair.second;
                pair.second.Update(updateInfo);
            }

            for (auto &pair: _gameObjects) {
                updateInfo.gameObject = &pair.second;
                pair.second.LateUpdate(updateInfo);
            }
        }

        void FixedUpdateComponents(FrameInfo &frameInfo) {
            auto& _renderer = m_resourceManager->GetRenderer();
            auto& _gameObjects = m_resourceManager->GetGameObjects();
            static float reservedFrameTime = 0;
            float frameTime = frameInfo.frameTime + reservedFrameTime;
            
            while (frameTime >= FIXED_UPDATE_INTERVAL) {
                frameTime -= FIXED_UPDATE_INTERVAL;

                ComponentUpdateInfo updateInfo{};
                RendererInfo rendererInfo{_renderer.getAspectRatio()};
                updateInfo.frameInfo = &frameInfo;
                updateInfo.rendererInfo = &rendererInfo;
                for (auto &pair: _gameObjects) {
                    updateInfo.gameObject = &pair.second;
                    pair.second.FixedUpdate(updateInfo);
                }
                for (auto &pair: _gameObjects) {
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
            auto& _renderer = m_resourceManager->GetRenderer();
            auto& _gameObjects = m_resourceManager->GetGameObjects();
            auto& _hierarchyTree = m_resourceManager->GetHierarchyTree();
#ifdef RAY_TRACING
            auto& _pGameObjectDescBuffer = m_resourceManager->GetGameObjectDescBuffer();
            auto& _pGameObjectDescs = m_resourceManager->GetGameObjectDescs();
            frameInfo.pGameObjectDescBuffer = _pGameObjectDescBuffer;
            frameInfo.pGameObjectDescs = _pGameObjectDescs;
#endif
            m_renderManager->UpdateRendering(_renderer, frameInfo, _hierarchyTree);
        }
    };
}