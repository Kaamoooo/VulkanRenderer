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
#include "Managers/ResourceManager.hpp"
#include "Managers/LogicManager.hpp"
#include "Managers/RenderManager.hpp"

namespace Kaamoo {
    class Application {
    public:
        Application() {
            m_resourceManager = std::make_shared<ResourceManager>();
            m_renderManager = std::make_unique<RenderManager>(m_resourceManager);
            m_logicManager = std::make_unique<LogicManager>(m_resourceManager);
        }

        ~Application() {
            GUI::Destroy();
        }

        void run() {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float totalTime = 0;
            Awake();
            auto &_window = m_resourceManager->GetWindow();
            auto &_renderer = m_resourceManager->GetRenderer();
            auto &_gameObjects = m_resourceManager->GetGameObjects();
            auto &_materials = m_resourceManager->GetMaterials();
            auto &_device = m_resourceManager->GetDevice();
            while (!_window.shouldClose()) {
                glfwPollEvents();
                auto newTime = std::chrono::high_resolution_clock::now();
                float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
                totalTime += frameTime;
                currentTime = newTime;

                if (auto commandBuffer = _renderer.beginFrame()) {
                    int frameIndex = _renderer.getFrameIndex();
                    FrameInfo frameInfo{frameIndex, frameTime, totalTime, commandBuffer, _gameObjects, _materials, m_ubo, _window.getCurrentExtent(), GUI::GetSelectedId(), false};
                    UpdateComponents(frameInfo);
                    UpdateRendering(frameInfo);
                }

            }
            vkDeviceWaitIdle(_device.device());
        };

        Application(const Application &) = delete;

        Application &operator=(const Application &) = delete;

    private:
        GlobalUbo m_ubo{};

        std::shared_ptr<ResourceManager> m_resourceManager;
        std::unique_ptr<RenderManager> m_renderManager;
        std::unique_ptr<LogicManager> m_logicManager;

        void Awake() {
            auto &_gameObjects = m_resourceManager->GetGameObjects();

            ComponentAwakeInfo awakeInfo{};
            for (auto &pair: _gameObjects) {
                awakeInfo.gameObject = &pair.second;
                pair.second.Awake(awakeInfo);
            }
        }

        void UpdateComponents(FrameInfo &frameInfo) {
            m_logicManager->UpdateComponents(frameInfo);
        }

        void UpdateRendering(FrameInfo &frameInfo) {
            auto &_renderer = m_resourceManager->GetRenderer();
            auto &_gameObjects = m_resourceManager->GetGameObjects();
            auto &_hierarchyTree = m_resourceManager->GetHierarchyTree();
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