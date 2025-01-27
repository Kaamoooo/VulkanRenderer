#include "RenderSystem.h"

#include <utility>
#include "../StructureInfos.h"
#include "../Components/MeshRendererComponent.hpp"
#include "../Components/LightComponent.hpp"

namespace Kaamoo {

    RenderSystem::RenderSystem(Device &device, const VkRenderPass &renderPass, const std::shared_ptr<Material> material)
            : device{device}, m_material{material}, m_renderPass{renderPass} {

    }

    RenderSystem::~RenderSystem() {
        vkDestroyPipelineLayout(device.device(), m_pipelineLayout, nullptr);
    }

    void RenderSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        if (m_material->getPipelineCategory() == "Light") {
            pushConstantRange.size = sizeof(PointLightPushConstant);
        } else if (m_material->getPipelineCategory() == "Opaque")
            pushConstantRange.size = sizeof(SimplePushConstantData);
        else if (m_material->getPipelineCategory() == "Overlay") {
            pushConstantRange.size = 0;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_material->getDescriptorSetLayoutPointers().size());
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        for (auto &descriptorSetLayoutPointer: m_material->getDescriptorSetLayoutPointers()) {
            descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
        }

        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRange.size == 0 ? 0 : 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create m_pipeline layout");
        }

    }

    void RenderSystem::createPipeline(VkRenderPass renderPass) {
        PipelineConfigureInfo pipelineConfigureInfo{};
        Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);

        if (m_material->getPipelineCategory() == PipelineCategory.Light) {
            Pipeline::enableAlphaBlending(pipelineConfigureInfo);
            pipelineConfigureInfo.attributeDescriptions.clear();
            pipelineConfigureInfo.vertexBindingDescriptions.clear();
        } else if (m_material->getPipelineCategory() == "Overlay") {
            pipelineConfigureInfo.attributeDescriptions.clear();
            pipelineConfigureInfo.vertexBindingDescriptions.clear();
        } else if (m_material->getPipelineCategory() == PipelineCategory.Transparent) {
            Pipeline::enableAlphaBlending(pipelineConfigureInfo);
        }

        pipelineConfigureInfo.renderPass = renderPass;
        pipelineConfigureInfo.pipelineLayout = m_pipelineLayout;
        m_pipeline = std::make_unique<Pipeline>(
                device,
                pipelineConfigureInfo,
                m_material
        );
    }


    void RenderSystem::render(FrameInfo &frameInfo, GameObject *gameObject) {
        m_pipeline->bind(frameInfo.commandBuffer);

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

        if (m_material->getPipelineCategory() == "Overlay") {
            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        } else if (m_material->getPipelineCategory() == "Light") {
            PointLightPushConstant pointLightPushConstant{};

            LightComponent *lightComponent;
            if (!gameObject->TryGetComponent(lightComponent)) return;

            pointLightPushConstant.position = glm::vec4(gameObject->transform->GetTranslation(), 1.f);
            pointLightPushConstant.color = glm::vec4(lightComponent->getColor(), lightComponent->getLightIntensity());
            pointLightPushConstant.radius = gameObject->transform->GetScale().x;
            vkCmdPushConstants(frameInfo.commandBuffer, m_pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(PointLightPushConstant), &pointLightPushConstant);
            vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        } else if (m_material->getPipelineCategory() == "Opaque") {
            SimplePushConstantData push{};
            push.modelMatrix = gameObject->transform->mat4();
            push.normalMatrix = gameObject->transform->normalMatrix();

            vkCmdPushConstants(frameInfo.commandBuffer, m_pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(SimplePushConstantData),
                               &push);
            MeshRendererComponent *meshRendererComponent;
            if (!gameObject->TryGetComponent(meshRendererComponent)) return;
            meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
            meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
        }
    }

    void RenderSystem::Init() {
        createPipelineLayout();
        createPipeline(m_renderPass);
    }


}