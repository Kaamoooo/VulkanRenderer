#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {
    struct PushConstantRay {
        glm::vec4 clearColor;
        glm::vec3 lightPosition;
        float lightIntensity;
        int lightType;
    };

    const std::string RayGenShaderName = "rayGenShader";
    const std::string RayClosestHitShaderName = "rayClosestHitShader";
    const std::string RayMissShaderName = "rayMissShader";

    class RayTracingSystem : public RenderSystem {
#ifdef RAY_TRACING
    public:
        
        RayTracingSystem(Device &device, VkRenderPass renderPass, std::shared_ptr<Material> material) : RenderSystem(device, renderPass, material) {};

        void rayTrace(FrameInfo &frameInfo) {
            m_pipeline->bind(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);

            //Todo: Temporarily hard code the push constant
            m_pushConstantRay.clearColor = {0.0f, 0.1f, 0.1f, 1.0f};
            m_pushConstantRay.lightPosition = {0.0f, 0.0f, 0.0f};
            m_pushConstantRay.lightIntensity = 1.0f;
            m_pushConstantRay.lightType = 0;

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: m_material->getDescriptorSetPointers()) {
                if (descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
                }
            }

//                vkCmdPushConstants(
//                        frameInfo.commandBuffer,
//                        m_pipelineLayout,
//                        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
//                        0,
//                        sizeof(PushConstantRay),
//                        &m_pushConstantRay
//                );
            vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                    m_pipelineLayout,
                    0,
                    descriptorSets.size(),
                    descriptorSets.data(),
                    0,
                    nullptr
            );

            Device::pfn_vkCmdTraceRaysKHR(frameInfo.commandBuffer,
                                          &m_pipeline->getGenRegion(),
                                          &m_pipeline->getMissRegion(),
                                          &m_pipeline->getHitRegion(),
                                          &m_pipeline->getCallableRegion(),
                                          frameInfo.extent.width,
                                          frameInfo.extent.height,
                                          1
            );
        }

        void render(FrameInfo &frameInfo) override {
            rayTrace(frameInfo);
        }

    private:
        PushConstantRay m_pushConstantRay{};

        void createPipelineLayout() override {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_material->getDescriptorSetLayoutPointers().size());

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            for (auto &descriptorSetLayoutPointer: m_material->getDescriptorSetLayoutPointers()) {
                descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
            }
            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

            VkPushConstantRange pushConstantRange{VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(PushConstantRay)};
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
        };

#endif
    };
}

