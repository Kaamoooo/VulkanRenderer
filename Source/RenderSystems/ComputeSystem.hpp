#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {
    const std::string RayTracingDenoiseComputeShaderName = "Compute/RayTracingDenoise.comp.spv";

    class ComputeSystem : public RenderSystem {
    public:
        struct PushConstant {
            int rayTracingImageIndex;
            bool firstFrame = true;
            alignas(16)glm::mat4 viewMatrix[2];
        };

        ComputeSystem(Device &device, VkRenderPass renderPass, std::shared_ptr<Material> material) :
                RenderSystem(device, renderPass, material) {};

        ComputeSystem(const RenderSystem &) = delete;


        void render(FrameInfo &frameInfo) override {
            m_pipeline->bind(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: m_material->getDescriptorSetPointers()) {
                if (descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
                }
            }

            m_pushConstant.rayTracingImageIndex = frameInfo.frameIndex % 2;
            m_pushConstant.viewMatrix[m_pushConstant.rayTracingImageIndex] = frameInfo.globalUbo.viewMatrix;
            vkCmdPushConstants(frameInfo.commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstant), &m_pushConstant);
            vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    m_pipelineLayout,
                    0,
                    m_material->getDescriptorSetLayoutPointers().size(),
                    descriptorSets.data(),
                    0,
                    nullptr
            );

            uint32_t groupCountX = (SCENE_WIDTH + 15) / 16;
            uint32_t groupCountY = (SCENE_HEIGHT + 15) / 16;
            vkCmdDispatch(frameInfo.commandBuffer, groupCountX, groupCountY, 1);

            m_pushConstant.firstFrame = false;
        }

    private:
        PushConstant m_pushConstant{};

        void createPipelineLayout() override {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = 1;

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            for (auto &descriptorSetLayoutPointer: m_material->getDescriptorSetLayoutPointers()) {
                descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
            }
            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

            VkPushConstantRange pushConstantRange = {};
            pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(PushConstant);
            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
            if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create m_pipeline layout");
            };
        }

        void createPipeline(VkRenderPass renderPass) override {
            PipelineConfigureInfo pipelineConfigureInfo{};
            Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);
            pipelineConfigureInfo.pipelineLayout = m_pipelineLayout;
            m_pipeline = std::make_unique<Pipeline>(device, pipelineConfigureInfo, m_material);
        };;

    };
}


