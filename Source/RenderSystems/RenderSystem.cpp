#include "RenderSystem.h"

#include <utility>
#include "../FrameInfo.h"
#include "../Components/MeshRendererComponent.hpp"
#include "../Components/LightComponent.hpp"

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
            : device{device}, material{material},renderPass{renderPass} {

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
        else if (material.getPipelineCategory() == "Overlay"){
            pushConstantRange.size = 0;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(material.getDescriptorSetLayoutPointers().size());
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        for (auto &descriptorSetLayoutPointer: material.getDescriptorSetLayoutPointers()) {
            descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
        }
        
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRange.size == 0 ? 0 : 1;
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
        } else if (material.getPipelineCategory() == "Overlay") {
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

    
    void RenderSystem::render(FrameInfo &frameInfo) {
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
            MeshRendererComponent* meshRendererComponent;
            if(!obj.TryGetComponent<MeshRendererComponent>(meshRendererComponent))continue;
            if (meshRendererComponent->GetMaterialID() != material.getMaterialId())continue;
            
            if (material.getPipelineCategory()=="Overlay"){
                vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
            }else if (material.getPipelineCategory()=="Light") {
                PointLightPushConstant pointLightPushConstant{};
                
                LightComponent* lightComponent;
                if (!obj.TryGetComponent(lightComponent)) continue;
                
                pointLightPushConstant.position = glm::vec4(obj.transform->translation, 1.f);
                pointLightPushConstant.color = glm::vec4(lightComponent->color, lightComponent->lightIntensity);
                pointLightPushConstant.radius = obj.transform->scale.x;
                vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   sizeof(PointLightPushConstant), &pointLightPushConstant);
                vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
            } else if (material.getPipelineCategory()=="Opaque") {
                SimplePushConstantData push{};
                push.modelMatrix = pair.second.transform->mat4();
                push.normalMatrix = pair.second.transform->normalMatrix();

                vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0,
                                   sizeof(SimplePushConstantData),
                                   &push);
                meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
                meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
            }
        }
    }

    void RenderSystem::Init() {
        createPipelineLayout();
        createPipeline(renderPass);
    }

}