#include <utility>

#include "RenderSystems/RenderSystem.h"
#include "RenderSystems/ShadowSystem.hpp"
#include "RenderSystems/GrassSystem.hpp"
#include "RenderSystems/SkyBoxSystem.hpp"
#include "RenderSystems/RayTracingSystem.hpp"
#include "RenderSystems/PostSystem.hpp"
#include "RenderSystems/GizmosRenderSystem.hpp"
#include "RenderSystems/ComputeSystem.hpp"

namespace Kaamoo {
    class RenderManager {
    public:
        RenderManager(std::shared_ptr<ResourceManager> resourceManager) {
            m_resourceManager = std::move(resourceManager);
            CreateRenderSystems(m_resourceManager->GetMaterials(), m_resourceManager->GetDevice(), m_resourceManager->GetRenderer());
        };

        ~RenderManager() {};

        RenderManager(const RenderManager &) = delete;

        RenderManager &operator=(const RenderManager &) = delete;

        void CreateRenderSystems(Material::Map &materials, Device &device, Renderer &renderer) {
            for (auto &materialPair: materials) {
                auto _material = materialPair.second;
                auto pipelineCategory = _material->getPipelineCategory();

                if (pipelineCategory == PipelineCategory.Gizmos) {
                    m_gizmosRenderSystem = std::make_shared<GizmosRenderSystem>(device, renderer.getSwapChainRenderPass(), _material);
                    continue;
//          This render system contains multiple pipelines, so I initialize it in the constructor.
//          m_gizmosRenderSystem->Init();   
                }

#ifdef RAY_TRACING
                if (pipelineCategory == PipelineCategory.RayTracing) {
                    m_rayTracingSystem = std::make_shared<RayTracingSystem>(device, nullptr, materialPair.second);
                    m_rayTracingSystem->Init();
                }
                if (pipelineCategory == PipelineCategory.Post) {
                    m_postSystem = std::make_shared<PostSystem>(device, renderer.getSwapChainRenderPass(), materialPair.second);
                    m_postSystem->Init();
                }
                if (pipelineCategory == PipelineCategory.Compute) {
                    m_computeSystem = std::make_shared<ComputeSystem>(device, nullptr, materialPair.second);
                    m_computeSystem->Init();
                }
#else

                if (m_renderSystemMap.find(_material->getMaterialId()) != m_renderSystemMap.end()) continue;

                std::shared_ptr<RenderSystem> _renderSystem;
                if (pipelineCategory == PipelineCategory.Shadow) {
                    m_shadowSystem = std::make_shared<ShadowSystem>(device, renderer.getShadowRenderPass(), _material);
                    continue;
                }

                if (pipelineCategory == PipelineCategory.TessellationGeometry) {
                    _renderSystem = std::make_shared<GrassSystem>(device, renderer.getSwapChainRenderPass(), _material);
                } else if (pipelineCategory == PipelineCategory.SkyBox) {
                    _renderSystem = std::make_shared<SkyBoxSystem>(device, renderer.getSwapChainRenderPass(), _material);
                } else if (pipelineCategory == PipelineCategory.Opaque || pipelineCategory == PipelineCategory.Overlay
                           || pipelineCategory == PipelineCategory.Light || pipelineCategory == PipelineCategory.Transparent) {
                    _renderSystem = std::make_shared<RenderSystem>(device, renderer.getSwapChainRenderPass(), _material);
                }

                if (_renderSystem != nullptr) {
                    _renderSystem->Init();
                    m_renderSystemMap[_material->getMaterialId()] = _renderSystem;
                }
#endif

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

        void UpdateRendering(Renderer &renderer, FrameInfo &frameInfo, HierarchyTree &hierarchyTree) {
            auto _frameIndex = frameInfo.frameIndex;
#ifdef RAY_TRACING
            GUI::BeginFrame(ImVec2(frameInfo.extent.width, frameInfo.extent.height));
            GUI::ShowWindow(ImVec2(frameInfo.extent.width, frameInfo.extent.height),
                            &frameInfo.gameObjects, &frameInfo.pGameObjectDescs, &hierarchyTree, frameInfo);
            frameInfo.pGameObjectDescBuffer->writeToBuffer(frameInfo.pGameObjectDescs.data(), frameInfo.pGameObjectDescs.size() * sizeof(GameObjectDesc));
            m_rayTracingSystem->UpdateGlobalUboBuffer(frameInfo.globalUbo, _frameIndex);
            m_rayTracingSystem->rayTrace(frameInfo);

            renderer.setDenoiseRtxToComputeSynchronization(frameInfo.commandBuffer, _frameIndex % 2);

            m_computeSystem->UpdateGlobalUboBuffer(frameInfo.globalUbo, _frameIndex);
            m_computeSystem->render(frameInfo);

            renderer.setDenoiseComputeToPostSynchronization(frameInfo.commandBuffer, _frameIndex % 2);

            renderer.beginSwapChainRenderPass(frameInfo.commandBuffer);

            m_postSystem->UpdateGlobalUboBuffer(frameInfo.globalUbo, _frameIndex);
            m_postSystem->render(frameInfo);
#else

            renderer.beginShadowRenderPass(frameInfo.commandBuffer);
            m_shadowSystem->UpdateGlobalUboBuffer(frameInfo.globalUbo, _frameIndex);
            m_shadowSystem->renderShadow(frameInfo);
            renderer.endShadowRenderPass(frameInfo.commandBuffer);

            renderer.setShadowMapSynchronization(frameInfo.commandBuffer);

            renderer.beginSwapChainRenderPass(frameInfo.commandBuffer);

            //Todo: SceneManager
            std::vector<std::pair<std::shared_ptr<RenderSystem>, GameObject *>> _renderQueue;
            for (auto &item: frameInfo.gameObjects) {
                auto &_gameObject = item.second;
                MeshRendererComponent *_meshRendererComponent;
                if (_gameObject.TryGetComponent(_meshRendererComponent)) {
                    _renderQueue.push_back(std::make_pair(m_renderSystemMap[_meshRendererComponent->GetMaterialID()], &_gameObject));
                    auto _renderSystem = m_renderSystemMap[_meshRendererComponent->GetMaterialID()];
                }
            }

            std::sort(_renderQueue.begin(), _renderQueue.end(), [](const auto &a, const auto &b) {
                return a.first->GetRenderQueue() < b.first->GetRenderQueue();
            });

            for (auto &item: _renderQueue) {
                auto _renderSystem = item.first;
                auto &_gameObject = *item.second;
                _renderSystem->UpdateGlobalUboBuffer(frameInfo.globalUbo, _frameIndex);
                _renderSystem->render(frameInfo, &_gameObject);
            }

            GUI::BeginFrame(ImVec2(frameInfo.extent.width, frameInfo.extent.height));
            GUI::ShowWindow(ImVec2(frameInfo.extent.width, frameInfo.extent.height),
                            &frameInfo.gameObjects, &frameInfo.materials, &hierarchyTree, frameInfo);
#endif
            GUI::EndFrame(frameInfo.commandBuffer);
            renderer.endSwapChainRenderPass(frameInfo.commandBuffer);

            renderer.beginGizmosRenderPass(frameInfo.commandBuffer);
            m_gizmosRenderSystem->UpdateGlobalUboBuffer(frameInfo.globalUbo, frameInfo.frameIndex);
            m_gizmosRenderSystem->render(frameInfo, GizmosType::EdgeDetectionStencil);
            m_gizmosRenderSystem->render(frameInfo, GizmosType::EdgeDetection);
            m_gizmosRenderSystem->render(frameInfo, GizmosType::Axis);
            renderer.endGizmosRenderPass(frameInfo.commandBuffer);

            renderer.endFrame();
        }

    private:
        std::shared_ptr<ResourceManager> m_resourceManager;

        std::unordered_map<id_t, std::shared_ptr<RenderSystem>> m_renderSystemMap;
        std::shared_ptr<PostSystem> m_postSystem;
        std::shared_ptr<GizmosRenderSystem> m_gizmosRenderSystem;
        std::shared_ptr<ComputeSystem> m_computeSystem;

#ifdef RAY_TRACING
        std::shared_ptr<RayTracingSystem> m_rayTracingSystem;
#else
        std::shared_ptr<ShadowSystem> m_shadowSystem;
#endif

    };
}