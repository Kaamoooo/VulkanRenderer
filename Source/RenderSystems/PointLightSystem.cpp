#include <map>
#include "PointLightSystem.h"
#include "../StructureInfos.h"
#include "../Components/LightComponent.hpp"

//deprecated, I combined pointLightSystem into regular render system in that they are based on game objects.
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
            throw std::runtime_error("failed to create m_pipeline layout");
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
//        m_pipeline = std::make_unique<Pipeline>(
//                m_device,
//                m_material
//        );
    }

    void PointLightSystem::render(FrameInfo &frameInfo) {
        std::map<float, id_t> sorted;
        for(auto& kv:frameInfo.gameObjects){
            auto& obj=kv.second;
            
            LightComponent* lightComponent;
            if (!obj.TryGetComponent(lightComponent)) continue;
            
//            auto offset = frameInfo.cameraComponent->getPosition() - obj.transform->translation;
//            float disSquared = glm::dot(offset,offset);
//            sorted[disSquared]=obj.getId();
        }
        
        pipeline->bind(frameInfo.commandBuffer);

        for (auto it = sorted.rbegin();it!=sorted.rend();it++) {
            
            auto &obj = frameInfo.gameObjects.at(it->second);

            LightComponent* lightComponent;
            if (!obj.TryGetComponent(lightComponent)) continue;

            PointLightPushConstant pointLightPushConstant{};
            pointLightPushConstant.position = glm::vec4(obj.transform->translation, 1.f);
            pointLightPushConstant.color = glm::vec4(lightComponent->getColor(), lightComponent->getLightIntensity());
            pointLightPushConstant.radius = obj.transform->scale.x;

            vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(PointLightPushConstant), &pointLightPushConstant);

            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        }
    }

}