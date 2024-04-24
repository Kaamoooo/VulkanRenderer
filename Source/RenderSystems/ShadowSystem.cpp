#include "ShadowSystem.h"

#include <utility>
#include "../FrameInfo.h"
#include "../Application.h"
#include "../Components/MeshRendererComponent.hpp"

namespace Kaamoo {

    struct ShadowPushConstant {
        glm::mat4 modelMatrix{};
    };

    ShadowSystem::ShadowSystem(Device &device, VkRenderPass renderPass, Material &material)
            : device{device}, material{material} {
        createPipelineLayout();
        createPipeline(renderPass);
    }

    ShadowSystem::~ShadowSystem() {
        vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    }

    void ShadowSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ShadowPushConstant);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(material.getDescriptorSetLayoutPointers().size());
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        for (auto &descriptorSetLayoutPointer: material.getDescriptorSetLayoutPointers()) {
            descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
        }
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout");
        }

    }

    void ShadowSystem::createPipeline(VkRenderPass renderPass) {
        PipelineConfigureInfo pipelineConfigureInfo{};
        Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);

        VkVertexInputBindingDescription bindingDescription[1];
        VkVertexInputAttributeDescription attributeDescription[1];
        bindingDescription[0].binding = 0;
        bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription[0].stride = 2 * sizeof(glm::vec3);

        attributeDescription[0].binding = 0;
        attributeDescription[0].location = 0;
        attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[0].location = 0;

//        pipelineConfigureInfo.attributeDescriptions.clear();
//        pipelineConfigureInfo.attributeDescriptions.push_back(attributeDescription[0]);
//        pipelineConfigureInfo.vertexBindingDescriptions.clear();
//        pipelineConfigureInfo.vertexBindingDescriptions.push_back(bindingDescription[0]);
        pipelineConfigureInfo.renderPass = renderPass;
        pipelineConfigureInfo.pipelineLayout = pipelineLayout;
        pipeline = std::make_unique<Pipeline>(
                device,
                pipelineConfigureInfo,
                material
        );
    }

    ///@param[in]rotation:defined in DEGREE instead of RADIANT
    glm::mat4 calculateViewMatrixForRotation(glm::vec3 position, glm::vec3 rotation) {
        glm::mat4 mat{1.0};
        float rx = glm::radians(rotation.x);
        float ry = glm::radians(rotation.y);
        float rz = glm::radians(rotation.z);
        mat = glm::rotate(mat, -rx, glm::vec3(1, 0, 0));
        mat = glm::rotate(mat, -ry, glm::vec3(0, 1, 0));
        mat = glm::rotate(mat, -rz, glm::vec3(0, 0, 1));
        mat = glm::translate(mat, -position);
        return mat;
    }

    void ShadowSystem::renderShadow(FrameInfo &frameInfo) {
        pipeline->bind(frameInfo.commandBuffer);

        std::vector<VkDescriptorSet> descriptorSets;
        for (auto &descriptorSetPointer: material.getDescriptorSetPointers()) {
            if (*descriptorSetPointer != nullptr) {
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

            ShadowPushConstant push{};
            push.modelMatrix = obj.transform->mat4();
            vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(ShadowPushConstant),
                               &push);
            //too much draw call here, but currently I have no better ideas about how to batch all models together
            if (meshRendererComponent->GetModelPtr() != nullptr) {
                meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
                meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
//                break;
            }
        }
    }
}
