//
// Created by asus on 2023/12/23.
//

#include "GrassSystem.h"
#include "../Components/MeshRendererComponent.hpp"

namespace Kaamoo {

    //定义constant data,是暂存的且会被销毁
    //constant data对于频繁改变的数据性能较高，然而尺寸有限，同时对于模型合批处理起来比较困难
    //由于Shader中的constant data变量是按16bytes来分别存储的，即不满16字节的变量也要存储16字节，对于大型结构或数组，必须添加alignas(16)来与Shader匹配，表示内存地址是16的倍数
    struct SimplePushConstantData {
        //按对角线初始化
        glm::mat4 modelMatrix{1.f};
        glm::mat4 vaseModelMatrix{1.f};
    };
    struct PointLightPushConstant {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius;
    };

    void GrassSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
                VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
                VK_SHADER_STAGE_GEOMETRY_BIT;
        pushConstantRange.offset = 0;
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

    void GrassSystem::createPipeline(VkRenderPass renderPass) {
        PipelineConfigureInfo pipelineConfigureInfo{};
        Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);

        pipelineConfigureInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{};

        tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tessellationStateCreateInfo.patchControlPoints = 3;
        tessellationStateCreateInfo.flags = 0;
        pipelineConfigureInfo.tessellationStateCreateInfo = tessellationStateCreateInfo;

        pipelineConfigureInfo.renderPass = renderPass;
        pipelineConfigureInfo.pipelineLayout = pipelineLayout;
        pipeline = std::make_unique<Pipeline>(
                device,
                pipelineConfigureInfo,
                material
        );
    }


    void GrassSystem::render(FrameInfo &frameInfo) {
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
            SimplePushConstantData push{};
            if (moveObject == nullptr && obj.getName() == "smooth_vase.obj") {
                moveObject = &obj;
            }
            MeshRendererComponent *meshRendererComponent;
            if (!obj.TryGetComponent<MeshRendererComponent>(meshRendererComponent))continue;
            if (meshRendererComponent->GetMaterialID() != material.getMaterialId())continue;
            if (moveObject != nullptr)
                push.vaseModelMatrix = moveObject->transform->mat4();
            push.modelMatrix = pair.second.transform->mat4();
            vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                               VK_SHADER_STAGE_ALL_GRAPHICS,
                               0,
                               sizeof(SimplePushConstantData),
                               &push);
            meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
            meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
        }
    }

    GrassSystem::GrassSystem(Device &device, VkRenderPass renderPass, Material &material) : RenderSystem(device,
                                                                                                         renderPass,
                                                                                                         material) {
    }

}