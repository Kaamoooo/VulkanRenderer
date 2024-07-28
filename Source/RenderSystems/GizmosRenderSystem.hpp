#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {

    const std::string UI_MODEL_PATH = "Gizmos/";
    const std::string UI_SHADER_PATH = "Gizmos/";
    
    const int GIZMOS_AXIS_RADIUS = 240;

    class GizmosRenderSystem : public RenderSystem {
    public:
        GizmosRenderSystem(Device &device, VkRenderPass renderPass, std::shared_ptr<Material> material)
                : RenderSystem(device, renderPass, material) {
            createPipelineLayout();
            ShaderBuilder shaderBuilder(device);
            // Scene axis
            {
                std::string axisModelName = "axis.obj";
                std::string axisVertexShaderName = "axis.vert.spv";
                std::string axisFragmentShaderName = "axis.frag.spv";
                std::shared_ptr<Model> modelFromFile = Model::createModelFromFile(*Device::getDeviceSingleton(), Model::BaseModelsPath + UI_MODEL_PATH + axisModelName);
                Model::models.emplace(axisModelName, modelFromFile);
                auto *meshRendererComponent = new MeshRendererComponent(modelFromFile, material->getMaterialId());
                m_axisObjPtr = std::make_shared<GameObject>(std::move(GameObject::createGameObject("Axis")));
                m_axisObjPtr->TryAddComponent(meshRendererComponent);
                m_axisObjPtr->transform->scale = glm::vec3(0.4f);
                m_axisMaterial = std::make_shared<Material>(*material);
                const std::string vertexShaderPath = UI_SHADER_PATH + axisVertexShaderName;
                const std::string fragmentShaderPath = UI_SHADER_PATH + axisFragmentShaderName;
                m_axisMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(vertexShaderPath), ShaderCategory::vertex));
                m_axisMaterial->getShaderModulePointers().push_back(std::make_shared<ShaderModule>(shaderBuilder.createShaderModule(fragmentShaderPath), ShaderCategory::fragment));

                createPipeline(renderPass, m_axisPipeline, m_axisMaterial);
            }
        };

        void createPipelineLayout() override {
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
            pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
            if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create m_pipeline layout");
            };
        }

        void createPipeline(VkRenderPass renderPass, std::shared_ptr<Pipeline> &pipeline, std::shared_ptr<Material> material) {
            PipelineConfigureInfo pipelineConfigureInfo{};
            Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);
            
            pipelineConfigureInfo.renderPass = renderPass;
            pipelineConfigureInfo.pipelineLayout = m_pipelineLayout;
            pipeline = std::make_shared<Pipeline>(
                    device,
                    pipelineConfigureInfo,
                    material
            );
        };

        void render(FrameInfo &frameInfo) override {

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: m_material->getDescriptorSetPointers()) {
                if (descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
                }
            }
            vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_pipelineLayout,
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

                vkCmdPushConstants(frameInfo.commandBuffer, m_pipelineLayout,
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
        };

    private:
        std::shared_ptr<GameObject> m_axisObjPtr;
        std::shared_ptr<Pipeline> m_axisPipeline;
        std::shared_ptr<Material> m_axisMaterial;
    };
}


