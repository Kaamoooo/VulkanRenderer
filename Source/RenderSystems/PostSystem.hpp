﻿#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {
    class PostSystem : public RenderSystem {
    public:
        struct PushConstant {
            int rayTracingImageIndex;
            bool firstFrame = true;
            alignas(16)glm::mat4 viewMatrix[2];
        };

        PostSystem(Device &device, const VkRenderPass& renderPass, std::shared_ptr<Material> material) :
                RenderSystem(device, renderPass, material) {};

        PostSystem(const RenderSystem &) = delete;


        void render(FrameInfo &frameInfo, GameObject *gameObject = nullptr) override {
            m_pipeline->bind(frameInfo.commandBuffer);

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: m_material->getDescriptorSetPointers()) {
                if (descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
                }
            }

            m_pushConstant.rayTracingImageIndex = frameInfo.frameIndex % 2;
            m_pushConstant.viewMatrix[m_pushConstant.rayTracingImageIndex] = frameInfo.globalUbo.viewMatrix;
            vkCmdPushConstants(frameInfo.commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstant), &m_pushConstant);

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
            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);

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
            pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
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

            //No input bindings
            pipelineConfigureInfo.vertexBindingDescriptions.clear();
            pipelineConfigureInfo.attributeDescriptions.clear();

            pipelineConfigureInfo.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
            pipelineConfigureInfo.depthStencilInfo.depthTestEnable = VK_FALSE;
            pipelineConfigureInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
            pipelineConfigureInfo.colorBlendAttachment.blendEnable = VK_FALSE;
            pipelineConfigureInfo.renderPass = renderPass;
            pipelineConfigureInfo.pipelineLayout = m_pipelineLayout;
            m_pipeline = std::make_unique<Pipeline>(
                    device,
                    pipelineConfigureInfo,
                    m_material
            );
        }

    };
}


