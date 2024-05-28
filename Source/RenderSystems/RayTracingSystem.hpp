#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {
    struct PushConstantRay
    {
        glm::vec4  clearColor;
        glm::vec3  lightPosition;
        float lightIntensity;
        int   lightType;
    };
    
    const std::string RayGenShaderName = "rayGenShader";
    const std::string RayClosestHitShaderName = "rayClosestHitShader";
    const std::string RayMissShaderName = "rayMissShader";
    
    class RayTracingSystem : public RenderSystem {
    public:
        RayTracingSystem(Device &device, VkRenderPass renderPass, Material &material)
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
            
            VkPushConstantRange pushConstantRange{VK_SHADER_STAGE_RAYGEN_BIT_KHR|VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR|VK_SHADER_STAGE_MISS_BIT_KHR,0,sizeof(PushConstantRay)};
            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
            
            if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) !=
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
            pipelineConfigureInfo.pipelineLayout = m_pipelineLayout;
            pipeline = std::make_unique<Pipeline>(device,pipelineConfigureInfo,material);
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
                    m_pipelineLayout,
                    0,
                    material.getDescriptorSetLayoutPointers().size(),
                    descriptorSets.data(),
                    0,
                    nullptr
            );

//            for (auto &pair: frameInfo.gameObjects) {
//                auto &obj = pair.second;
//                MeshRendererComponent *meshRendererComponent;
//                if (!obj.TryGetComponent<MeshRendererComponent>(meshRendererComponent))continue;
//                if (meshRendererComponent->GetMaterialID() != material.getMaterialId())continue;
//                meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
//                meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
//            }
        };

    };
}


