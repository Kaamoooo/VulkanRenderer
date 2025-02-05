#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include "../Pipeline.hpp"
#include "../Device.hpp"
#include "../Model.hpp"
#include "../GameObject.hpp"
#include "../StructureInfos.h"
#include "../Components/MeshRendererComponent.hpp"

namespace Kaamoo {
    class ShadowSystem {
    public:
        ShadowSystem(Device &device, const VkRenderPass& renderPass, const std::shared_ptr<Material>& material) : device{device}, material{material} {
            createPipelineLayout();
            createPipeline(renderPass);
        }

        ~ShadowSystem() {
            vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
        }

        ShadowSystem(const ShadowSystem &) = delete;

        ShadowSystem &operator=(const ShadowSystem &) = delete;

        void renderShadow(FrameInfo &frameInfo) {
            pipeline->bind(frameInfo.commandBuffer);

            std::vector<VkDescriptorSet> descriptorSets;
            for (auto &descriptorSetPointer: material->getDescriptorSetPointers()) {
                if (*descriptorSetPointer != nullptr) {
                    descriptorSets.push_back(*descriptorSetPointer);
                }
            }
            vkCmdBindDescriptorSets(
                    frameInfo.commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    0,
                    material->getDescriptorSetLayoutPointers().size(),
                    descriptorSets.data(),
                    0,
                    nullptr
            );

            for (auto &pair: frameInfo.gameObjects) {
                auto &obj = pair.second;

                MeshRendererComponent *meshRendererComponent;
                if (!obj.TryGetComponent<MeshRendererComponent>(meshRendererComponent))continue;

                auto material = frameInfo.materials.at(meshRendererComponent->GetMaterialID());
                if (material->getPipelineCategory() == PipelineCategory.SkyBox)continue;
                ShadowPushConstant push{};
                push.modelMatrix = obj.transform->mat4();
                vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0,
                                   sizeof(ShadowPushConstant),
                                   &push);
                //too much draw call here, but currently I have no better ideas about how to batch all models together
                if (meshRendererComponent->GetModelPtr() != nullptr && obj.IsActive()) {
                    meshRendererComponent->GetModelPtr()->bind(frameInfo.commandBuffer);
                    meshRendererComponent->GetModelPtr()->draw(frameInfo.commandBuffer);
//                break;
                }
            }
        }


        template<class T>
        void UpdateGlobalUboBuffer(T &globalUbo, uint32_t frameIndex) {
            if (material->getBufferPointers().empty()) {
                return;
            }
            material->getBufferPointers()[0]->writeToIndex(&globalUbo, frameIndex);
            material->getBufferPointers()[0]->flushIndex(frameIndex);
        };

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

    private:
        struct ShadowPushConstant {
            glm::mat4 modelMatrix{};
        };

        void createPipelineLayout() {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags =
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(ShadowPushConstant);

            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(material->getDescriptorSetLayoutPointers().size());
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            for (auto &descriptorSetLayoutPointer: material->getDescriptorSetLayoutPointers()) {
                descriptorSetLayouts.push_back(descriptorSetLayoutPointer->getDescriptorSetLayout());
            }
            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
            if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create m_pipeline layout");
            }

        }

        void createPipeline(VkRenderPass renderPass) {
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

        //手动编译Shader，此时读取编译后的文件
        //路径是从可执行文件开始的，并非从根目录
        Device &device;
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        std::shared_ptr<Material> material;
    };


}