﻿#include "RenderSystem.h"

#include <utility>
#include "../FrameInfo.h"

namespace Kaamoo {

    //定义constant data,是暂存的且会被销毁
    //constant data对于频繁改变的数据性能较高，然而尺寸有限，同时对于模型合批处理起来比较困难
    //由于Shader中的constant data变量是按16bytes来分别存储的，即不满16字节的变量也要存储16字节，对于大型结构或数组，必须添加alignas(16)来与Shader匹配，表示内存地址是16的倍数
    struct SimplePushConstantData {
        //按对角线初始化
        glm::mat4 modelMatrix{1.f};
        glm::mat4 normalMatrix{1.f};
    };
    struct PointLightPushConstant {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius;
    };

    RenderSystem::RenderSystem(Device &device, VkRenderPass renderPass, Material &material)
            : device{device}, material{material} {
        createPipelineLayout();
        createPipeline(renderPass);
    }

    RenderSystem::~RenderSystem() {
        vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    }

    void RenderSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        if (material.getPipelineCategory() == "Light") {
            pushConstantRange.size = sizeof(PointLightPushConstant);
        } else if (material.getPipelineCategory() == "Opaque")
            pushConstantRange.size = sizeof(SimplePushConstantData);

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

    void RenderSystem::createPipeline(VkRenderPass renderPass) {
        PipelineConfigureInfo pipelineConfigureInfo{};
        Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);
        if (material.getPipelineCategory() == "Light") {
            Pipeline::enableAlphaBlending(pipelineConfigureInfo);
            pipelineConfigureInfo.attributeDescriptions.clear();
            pipelineConfigureInfo.vertexBindingDescriptions.clear();
        }
        pipelineConfigureInfo.renderPass = renderPass;
        pipelineConfigureInfo.pipelineLayout = pipelineLayout;
        pipeline = std::make_unique<Pipeline>(
                device,
                pipelineConfigureInfo,
                material
        );
    }

    void updateLight(FrameInfo &frameInfo, GameObject &obj) {
        int lightIndex = obj.pointLightComponent->lightIndex;

        assert(lightIndex < MAX_LIGHT_NUM && "点光源数目过多");

        auto rotateLight = glm::rotate(glm::mat4{1.f}, frameInfo.frameTime, glm::vec3(0, -1.f, 0));
        obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1));

        PointLight pointLight{};
        pointLight.color = glm::vec4(obj.color, obj.pointLightComponent->lightIntensity);
        pointLight.position = glm::vec4(obj.transform.translation, 1.f);
        frameInfo.globalUbo.pointLights[lightIndex] = pointLight;

    }

    void RenderSystem::render(FrameInfo &frameInfo) {
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
            if (obj.getMaterialId() != material.getMaterialId())continue;
            if (obj.pointLightComponent != nullptr) {
                updateLight(frameInfo, obj);
                PointLightPushConstant pointLightPushConstant{};
                pointLightPushConstant.position = glm::vec4(obj.transform.translation, 1.f);
                pointLightPushConstant.color = glm::vec4(obj.color, obj.pointLightComponent->lightIntensity);
                pointLightPushConstant.radius = obj.transform.scale.x;
                vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   sizeof(PointLightPushConstant), &pointLightPushConstant);
                vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
            } else {
                SimplePushConstantData push{};
                push.modelMatrix = pair.second.transform.mat4();
                push.normalMatrix = pair.second.transform.normalMatrix();

                vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0,
                                   sizeof(SimplePushConstantData),
                                   &push);
                pair.second.model->bind(frameInfo.commandBuffer);
                pair.second.model->draw(frameInfo.commandBuffer);
            }
        }
    }

}