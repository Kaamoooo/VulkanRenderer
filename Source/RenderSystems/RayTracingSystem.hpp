#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {

    const std::string RayGenShaderName = "rayGenShader";
    const std::string RayClosestHitShaderName = "rayClosestHitShader";
    const std::string RayMissShaderName = "rayMissShader";

    class RayTracingSystem : public RenderSystem {
#ifdef RAY_TRACING
    public:
        struct PushConstant {
            int rayTracingImageIndex;
        };

        RayTracingSystem(Device &device,const VkRenderPass& renderPass, std::shared_ptr<Material> material) : RenderSystem(device, nullptr, material) {};

        void rayTrace(FrameInfo &frameInfo) {
            m_pipeline->bind(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: m_material->getDescriptorSetPointers()) {
                if (descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
                }
            }

            m_pushConstant.rayTracingImageIndex = frameInfo.frameIndex % 2;
            vkCmdPushConstants(frameInfo.commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(PushConstant), &m_pushConstant);

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
                                          SCENE_HEIGHT,
                                          SCENE_WIDTH,
                                          1
            );
        }

        void render(FrameInfo &frameInfo,GameObject* gameObject) override {
            rayTrace(frameInfo);
        }

        void Init() override {
            createPipelineLayout();
            createPipeline(m_renderPass);
        }

    private:
        PushConstant m_pushConstant{};

        void createPipelineLayout() override {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_material->getDescriptorSetLayoutPointers().size());

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            for (auto &descriptorSetLayoutPointer: m_material->getDescriptorSetLayoutPointers()) {
                descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
            }
            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

            VkPushConstantRange pushConstantRange{VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(PushConstant)};
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

