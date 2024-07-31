#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {

    const std::string GIZMOS_MODEL_PATH = "Gizmos/";
    const std::string GIZMOS_SHADER_PATH = "Gizmos/";

    const int GIZMOS_AXIS_RADIUS = 240;

    enum GizmosType {
        Axis,
        EdgeDetectionStencil,
        EdgeDetection
    };

    class GizmosRenderSystem : public RenderSystem {
    public:
        ~GizmosRenderSystem() {
            vkDestroyPipelineLayout(device.device(), m_axisPipelineLayout, nullptr);
            vkDestroyPipelineLayout(device.device(), m_edgeDetectionPipelineLayout, nullptr);

        };

        GizmosRenderSystem(Device &device, VkRenderPass renderPass, std::shared_ptr<Material> material)
                : RenderSystem(device, renderPass, material) {
            ShaderBuilder shaderBuilder(device);
            m_pipelineLayout = VK_NULL_HANDLE;
            // Axis
            {
                std::string axisModelName = "axis.obj";
                std::string axisVertexShaderName = "axis.vert.spv";
                std::string axisFragmentShaderName = "axis.frag.spv";
                std::shared_ptr<Model> modelFromFile = Model::createModelFromFile(*Device::getDeviceSingleton(), Model::BaseModelsPath + GIZMOS_MODEL_PATH + axisModelName);
                Model::models.emplace(axisModelName, modelFromFile);
                auto *meshRendererComponent = new MeshRendererComponent(modelFromFile, material->getMaterialId());
                m_axisObjPtr = std::make_shared<GameObject>(std::move(GameObject::createGameObject("Axis")));
                m_axisObjPtr->TryAddComponent(meshRendererComponent);
                m_axisObjPtr->transform->scale = glm::vec3(0.4f);
                m_axisMaterial = std::make_shared<Material>(*material);
                const std::string vertexShaderPath = GIZMOS_SHADER_PATH + axisVertexShaderName;
                const std::string fragmentShaderPath = GIZMOS_SHADER_PATH + axisFragmentShaderName;
                m_axisMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(vertexShaderPath), ShaderCategory::vertex));
                m_axisMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(fragmentShaderPath), ShaderCategory::fragment));
                createPipelineLayout(GizmosType::Axis);
                createPipeline(renderPass, m_axisPipeline, m_axisMaterial, m_axisPipelineLayout, GizmosType::Axis);
            }

            //Edge detection
            {
                std::string vertexShaderName = "edgeDetection.vert.spv";
//                std::string geometryShaderName = "edgeDetection.geom.spv";
                std::string fragmentShaderName = "edgeDetection.frag.spv";
                m_edgeDetectionMaterial = std::make_shared<Material>(*material);
                m_edgeDetectionStencilMaterial = std::make_shared<Material>(*material);
                const std::string vertexShaderPath = GIZMOS_SHADER_PATH + vertexShaderName;
//                const std::string geometryShaderPath = GIZMOS_SHADER_PATH + geometryShaderName;
                const std::string fragmentShaderPath = GIZMOS_SHADER_PATH + fragmentShaderName;
                m_edgeDetectionMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(vertexShaderPath), ShaderCategory::vertex));
//                m_edgeDetectionMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(geometryShaderPath), ShaderCategory::geometry));
                m_edgeDetectionMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(fragmentShaderPath), ShaderCategory::fragment));

                std::string stencilVertexShaderName = "MyShader.vert.spv";
                const std::string stencilVertexShaderPath = stencilVertexShaderName;
                m_edgeDetectionStencilMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(stencilVertexShaderPath), ShaderCategory::vertex));
                m_edgeDetectionStencilMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(fragmentShaderPath), ShaderCategory::fragment));

                createPipelineLayout(GizmosType::EdgeDetection);

                createPipeline(renderPass, m_edgeDetectionPipeline, m_edgeDetectionMaterial, m_edgeDetectionPipelineLayout, GizmosType::EdgeDetection);
                createPipeline(renderPass, m_edgeDetectionStencilPipeline, m_edgeDetectionStencilMaterial, m_edgeDetectionPipelineLayout, GizmosType::EdgeDetectionStencil);
            }
        };

        void createPipelineLayout(GizmosType gizmosType) {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_material->getDescriptorSetLayoutPointers().size());

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            for (auto &descriptorSetLayoutPointer: m_material->getDescriptorSetLayoutPointers()) {
                descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
            }

            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.size = sizeof(SimplePushConstantData);
            switch (gizmosType) {
                case GizmosType::Axis:
                    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
                    if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &m_axisPipelineLayout) !=
                        VK_SUCCESS) {
                        throw std::runtime_error("failed to create Gizmos Axis layout");
                    };
                    break;
                case GizmosType::EdgeDetectionStencil:
                case GizmosType::EdgeDetection:
                    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
                    pushConstantRange.size = sizeof(SimplePushConstantData);
                    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
                    if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &m_edgeDetectionPipelineLayout) !=
                        VK_SUCCESS) {
                        throw std::runtime_error("failed to create Gizmos Edge Detection layout");
                    };
                    break;
            }
        }

        void createPipeline(VkRenderPass renderPass, std::shared_ptr<Pipeline> &pipeline, std::shared_ptr<Material> material, VkPipelineLayout &pipelineLayout, GizmosType gizmosType) {
            PipelineConfigureInfo pipelineConfigureInfo{};
            Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);
            switch (gizmosType) {
                case GizmosType::Axis:
                    break;
                case GizmosType::EdgeDetection: {
                    VkStencilOpState front{};
                    front.failOp = VK_STENCIL_OP_KEEP;
                    front.passOp = VK_STENCIL_OP_KEEP;
                    front.compareOp = VK_COMPARE_OP_NOT_EQUAL;
                    front.compareMask = edgeDetectionStencilMask;
                    front.writeMask = edgeDetectionStencilMask;
                    front.reference = 1;
                    pipelineConfigureInfo.depthStencilInfo.front = front;
                    pipelineConfigureInfo.depthStencilInfo.back = front;
                    pipelineConfigureInfo.depthStencilInfo.stencilTestEnable = VK_TRUE;
                    pipelineConfigureInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
                    pipelineConfigureInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
//                    pipelineConfigureInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
                    break;
                }
                case GizmosType::EdgeDetectionStencil: {
                    pipelineConfigureInfo.colorBlendAttachment.colorWriteMask = 0;
                    VkStencilOpState front{};
                    front.failOp = VK_STENCIL_OP_ZERO;
                    front.passOp = VK_STENCIL_OP_REPLACE;
                    front.compareOp = VK_COMPARE_OP_ALWAYS;
                    front.compareMask = edgeDetectionStencilMask;
                    front.writeMask = edgeDetectionStencilMask;
                    front.reference = 1;
                    pipelineConfigureInfo.depthStencilInfo.front = front;
                    pipelineConfigureInfo.depthStencilInfo.back = front;
                    pipelineConfigureInfo.depthStencilInfo.stencilTestEnable = VK_TRUE;
                    pipelineConfigureInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
                    pipelineConfigureInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
                    break;
                }
            }
            pipelineConfigureInfo.renderPass = renderPass;
            pipelineConfigureInfo.pipelineLayout = pipelineLayout;
            pipeline = std::make_shared<Pipeline>(
                    device,
                    pipelineConfigureInfo,
                    material
            );
        };

        void render(FrameInfo &frameInfo, GizmosType gizmosType) {

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: m_material->getDescriptorSetPointers()) {
                if (descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
                }
            }
            switch (gizmosType) {
                case GizmosType::Axis: {
                    vkCmdBindDescriptorSets(
                            frameInfo.commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_axisPipelineLayout,
                            0,
                            m_material->getDescriptorSetLayoutPointers().size(),
                            descriptorSets.data(),
                            0,
                            nullptr
                    );
                    MeshRendererComponent *meshRendererComponent;
                    if (m_axisObjPtr->TryGetComponent<MeshRendererComponent>(meshRendererComponent)) {
                        m_axisPipeline->bind(frameInfo.commandBuffer);
                        SimplePushConstantData push{};
                        push.modelMatrix = m_axisObjPtr->transform->mat4();
                        push.normalMatrix = m_axisObjPtr->transform->normalMatrix();

                        vkCmdPushConstants(frameInfo.commandBuffer, m_axisPipelineLayout,
                                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                           0,
                                           sizeof(SimplePushConstantData),
                                           &push);
                        VkViewport viewport{};
                        viewport.x = frameInfo.extent.width - GIZMOS_AXIS_RADIUS;
                        viewport.y = 0;
                        viewport.width = GIZMOS_AXIS_RADIUS;
                        viewport.height = GIZMOS_AXIS_RADIUS;
                        viewport.minDepth = 0.0f;
                        viewport.maxDepth = 1.0f;
                        vkCmdSetViewport(frameInfo.commandBuffer, 0, 1, &viewport);
                        meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
                        meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
                    };
                    break;
                }

                case GizmosType::EdgeDetectionStencil: {
                    vkCmdBindDescriptorSets(
                            frameInfo.commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_edgeDetectionPipelineLayout,
                            0,
                            m_edgeDetectionStencilMaterial->getDescriptorSetLayoutPointers().size(),
                            descriptorSets.data(),
                            0,
                            nullptr
                    );
                    if (frameInfo.gameObjects.find(frameInfo.selectedGameObjectId) != frameInfo.gameObjects.end()) {
                        auto &selectedGameObject = frameInfo.gameObjects.at(frameInfo.selectedGameObjectId);
                        MeshRendererComponent *meshRendererComponent;
                        if (selectedGameObject.TryGetComponent<MeshRendererComponent>(meshRendererComponent) && meshRendererComponent->GetModelPtr()) {
                            m_edgeDetectionStencilPipeline->bind(frameInfo.commandBuffer);
                            SimplePushConstantData push{};
                            push.modelMatrix = selectedGameObject.transform->mat4();
                            push.normalMatrix = selectedGameObject.transform->normalMatrix();

                            vkCmdPushConstants(frameInfo.commandBuffer, m_edgeDetectionPipelineLayout,
                                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT,
                                               0,
                                               sizeof(SimplePushConstantData),
                                               &push);
                            meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
                            meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
                        };
                    }
                    break;
                }

                case GizmosType::EdgeDetection: {
                    vkCmdBindDescriptorSets(
                            frameInfo.commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_edgeDetectionPipelineLayout,
                            0,
                            m_edgeDetectionMaterial->getDescriptorSetLayoutPointers().size(),
                            descriptorSets.data(),
                            0,
                            nullptr
                    );
                    if (frameInfo.gameObjects.find(frameInfo.selectedGameObjectId) != frameInfo.gameObjects.end()) {
                        auto &selectedGameObject = frameInfo.gameObjects.at(frameInfo.selectedGameObjectId);
                        MeshRendererComponent *meshRendererComponent;
                        if (selectedGameObject.TryGetComponent<MeshRendererComponent>(meshRendererComponent) && meshRendererComponent->GetModelPtr()) {
                            m_edgeDetectionPipeline->bind(frameInfo.commandBuffer);
                            SimplePushConstantData push{};
                            push.modelMatrix = selectedGameObject.transform->mat4();
                            push.normalMatrix = selectedGameObject.transform->normalMatrix();
                            vkCmdPushConstants(frameInfo.commandBuffer, m_edgeDetectionPipelineLayout,
                                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT,
                                               0,
                                               sizeof(SimplePushConstantData),
                                               &push);
                            meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
                            meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
                        };
                    }
                    break;
                }
            }


        };

    private:

        std::shared_ptr<GameObject> m_axisObjPtr;
        VkPipelineLayout m_axisPipelineLayout;
        std::shared_ptr<Pipeline> m_axisPipeline;
        std::shared_ptr<Material> m_axisMaterial;

        VkPipelineLayout m_edgeDetectionPipelineLayout;
        uint32_t edgeDetectionStencilMask = 0x01;
        std::shared_ptr<Pipeline> m_edgeDetectionPipeline;
        std::shared_ptr<Pipeline> m_edgeDetectionStencilPipeline;
        std::shared_ptr<Material> m_edgeDetectionMaterial;
        std::shared_ptr<Material> m_edgeDetectionStencilMaterial;
        const float m_scaleFactor = 1.1f;
    };
}


