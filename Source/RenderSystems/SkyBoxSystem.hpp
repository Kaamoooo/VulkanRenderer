#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {
    class SkyBoxSystem : public RenderSystem {
    public:
        SkyBoxSystem(Device &device, VkRenderPass renderPass, Material &material)
                : RenderSystem(device, renderPass, material) {};

    private:
        void createPipelineLayout() override {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(material.getDescriptorSetLayoutPointers().size());

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            for (auto &descriptorSetLayoutPointer: material.getDescriptorSetLayoutPointers()) {
                descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
            }

            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
            pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
            if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout");
            };
        }

        void createPipeline(VkRenderPass renderPass) override {
            PipelineConfigureInfo pipelineConfigureInfo{};
            Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);

            pipelineConfigureInfo.rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
            pipelineConfigureInfo.depthStencilInfo.depthTestEnable = VK_FALSE;
            pipelineConfigureInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
            pipelineConfigureInfo.colorBlendAttachment.blendEnable = VK_FALSE;
            pipelineConfigureInfo.renderPass = renderPass;
            pipelineConfigureInfo.pipelineLayout = pipelineLayout;
            pipeline = std::make_unique<Pipeline>(
                    device,
                    pipelineConfigureInfo,
                    material
            );
        };

        void render(FrameInfo &frameInfo) override {
            pipeline->bind(frameInfo.commandBuffer);

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: material.getDescriptorSetPointers()) {
                if (descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
                }
            }
            vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    0,
                    material.getDescriptorSetLayoutPointers().size(),
                    descriptorSets.data(),
                    0,
                    nullptr
            );

            for (auto &pair: frameInfo.gameObjects) {
                auto &obj = pair.second;
                MeshRendererComponent *meshRendererComponent;
                if (!obj.TryGetComponent<MeshRendererComponent>(meshRendererComponent))continue;
                if (meshRendererComponent->GetMaterialID() != material.getMaterialId())continue;
                meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
                meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
            }
        };

    };
}


