#include <map>
#include "PointLightSystem.h"
#include "../FrameInfo.h"

namespace Kaamoo {

    struct PointLightPushConstant {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius;
    };

    PointLightSystem::PointLightSystem(Device &device, VkRenderPass renderPass,
                                       VkDescriptorSetLayout descriptorSetLayout)
            : device{device} {
        createPipelineLayout(descriptorSetLayout);
        createPipeline(renderPass);
    }

    PointLightSystem::~PointLightSystem() {
        vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    }

    void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PointLightPushConstant);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {globalSetLayout};

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout");
        }

    }

    void PointLightSystem::createPipeline(VkRenderPass renderPass) {
        PipelineConfigureInfo pipelineConfigureInfo{};
        Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);
        Pipeline::enableAlphaBlending(pipelineConfigureInfo);
        pipelineConfigureInfo.attributeDescriptions.clear();
        pipelineConfigureInfo.vertexBindingDescriptions.clear();
        pipelineConfigureInfo.renderPass = renderPass;
        pipelineConfigureInfo.pipelineLayout = pipelineLayout;
//        pipeline = std::make_unique<Pipeline>(
//                device,
//                material
//        );
    }

    void PointLightSystem::render(FrameInfo &frameInfo) {
        std::map<float, GameObject::id_t> sorted;
        for(auto& kv:frameInfo.gameObjects){
            auto& obj=kv.second;
            if (obj.pointLightComponent== nullptr) continue;
            
            auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
            float disSquared = glm::dot(offset,offset);
            sorted[disSquared]=obj.getId();
        }
        
        pipeline->bind(frameInfo.commandBuffer);

        for (auto it = sorted.rbegin();it!=sorted.rend();it++) {
            
            auto &obj = frameInfo.gameObjects.at(it->second);

            PointLightPushConstant pointLightPushConstant{};
            pointLightPushConstant.position = glm::vec4(obj.transform.translation, 1.f);
            pointLightPushConstant.color = glm::vec4(obj.color, obj.pointLightComponent->lightIntensity);
            pointLightPushConstant.radius = obj.transform.scale.x;

            vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(PointLightPushConstant), &pointLightPushConstant);

            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        }
    }

}