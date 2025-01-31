#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {
    class GrassSystem : public RenderSystem {
    public:
        GrassSystem(Device &device,const VkRenderPass& renderPass,std::shared_ptr<Material>& material) : RenderSystem(device, renderPass, material) {
        }

    private:
        struct GrassPushConstant {
            glm::mat4 modelMatrix;
            glm::mat4 vaseModelMatrix;
        };

        void createPipelineLayout() override {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags =
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
                    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                    VK_SHADER_STAGE_GEOMETRY_BIT;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(GrassPushConstant);

            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_material->getDescriptorSetLayoutPointers().size());

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            for (auto &descriptorSetLayoutPointer: m_material->getDescriptorSetLayoutPointers()) {
                descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
            }

            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
            if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create m_pipeline layout");
            }
        }

        void createPipeline(VkRenderPass renderPass) override {
            PipelineConfigureInfo pipelineConfigureInfo{};
            Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);

            pipelineConfigureInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
            VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{};

            tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
            tessellationStateCreateInfo.patchControlPoints = 3;
            tessellationStateCreateInfo.flags = 0;
            pipelineConfigureInfo.tessellationStateCreateInfo = tessellationStateCreateInfo;

            pipelineConfigureInfo.renderPass = renderPass;
            pipelineConfigureInfo.pipelineLayout = m_pipelineLayout;
            m_pipeline = std::make_unique<Pipeline>(
                    device,
                    pipelineConfigureInfo,
                    m_material
            );
        }

        void render(FrameInfo &frameInfo, GameObject *gameObject) override {
            m_pipeline->bind(frameInfo.commandBuffer);

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: m_material->getDescriptorSetPointers())
                if (descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
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

            GrassPushConstant push{};
            //Todo: Huh?
            if (moveObject == nullptr && gameObject->GetName() == "Vase") {
                moveObject = gameObject;
            }
            if (moveObject != nullptr)
                push.vaseModelMatrix = moveObject->transform->mat4();
            push.modelMatrix = gameObject->transform->mat4();
            vkCmdPushConstants(frameInfo.commandBuffer, m_pipelineLayout,
                               VK_SHADER_STAGE_ALL_GRAPHICS,
                               0,
                               sizeof(GrassPushConstant),
                               &push);
            MeshRendererComponent *meshRendererComponent;
            if (!gameObject->TryGetComponent(meshRendererComponent)) return;
            meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
            meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
        }

        GameObject *moveObject = nullptr;
    };
}


