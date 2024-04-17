#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include "../Pipeline.hpp"
#include "../Device.hpp"
#include "../Model.hpp"
#include "../GameObject.hpp"
#include "../FrameInfo.h"

namespace Kaamoo {
    class RenderSystem {
    public:
        RenderSystem(Device &device, VkRenderPass renderPass, Material &material);
        
        void Init();

        ~RenderSystem();

        RenderSystem(const RenderSystem &) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;

        virtual void render(FrameInfo &frameInfo);

        template<class T>
        void UpdateGlobalUboBuffer(T &globalUbo, uint32_t frameIndex) {
            material.getBufferPointers()[0]->writeToIndex(&globalUbo, frameIndex);
            material.getBufferPointers()[0]->flushIndex(frameIndex);
        };


    protected:
        virtual void createPipeline(VkRenderPass renderPass);

        virtual void createPipelineLayout();

        //手动编译Shader，此时读取编译后的文件
        //路径是从可执行文件开始的，并非从根目录
        Device &device;
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        Material &material;
        VkRenderPass renderPass;
    };


}