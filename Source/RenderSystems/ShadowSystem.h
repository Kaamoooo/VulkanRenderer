#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include "../Pipeline.hpp"
#include "../Device.hpp"
#include "../Model.hpp"
#include "../GameObject.hpp"
#include "../StructureInfos.h"

namespace Kaamoo {
    class ShadowSystem {
    public:


        ShadowSystem(Device &device, VkRenderPass renderPass, std::shared_ptr<Material> material);

        ~ShadowSystem();

        ShadowSystem(const ShadowSystem &) = delete;

        ShadowSystem &operator=(const ShadowSystem &) = delete;

        void renderShadow(FrameInfo &frameInfo);

        
        template<class T>
        void UpdateGlobalUboBuffer(T &globalUbo, uint32_t frameIndex) {
            if (material->getBufferPointers().empty()) {
                return;
            }
            material->getBufferPointers()[0]->writeToIndex(&globalUbo, frameIndex);
            material->getBufferPointers()[0]->flushIndex(frameIndex);
        };
        
        glm::mat4 calculateViewMatrixForRotation(glm::vec3 position, glm::vec3 rotation) {
            glm::mat4 mat{1.0};
            float rx = glm::radians(rotation.x);
//            float rx = direction.x;
            float ry = glm::radians(rotation.y);
//            float ry = direction.y;
            float rz = glm::radians(rotation.z);
//            float rz = direction.z;
            mat = glm::rotate(mat, -rx, glm::vec3(1, 0, 0));
            mat = glm::rotate(mat, -ry, glm::vec3(0, 1, 0));
            mat = glm::rotate(mat, -rz, glm::vec3(0, 0, 1));
            mat = glm::translate(mat, -position);
            return mat;
        }

    private:
        void createPipelineLayout();

        void createPipeline(VkRenderPass renderPass);
        
        //手动编译Shader，此时读取编译后的文件
        //路径是从可执行文件开始的，并非从根目录
        Device &device;
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        std::shared_ptr<Material> material;
    };


}