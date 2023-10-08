#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include "Pipeline.hpp"
#include "Device.hpp"
#include "Model.hpp"
#include "GameObject.h"
#include "Camera.h"
#include "FrameInfo.h"

namespace Kaamoo {
    class RenderSystem {
    public:

        void run();
        RenderSystem(Device &device,VkRenderPass renderPass,VkDescriptorSetLayout globalSetLayout);
        ~RenderSystem();

        RenderSystem(const RenderSystem&)=delete;
        RenderSystem& operator= (const RenderSystem&) = delete;
        void renderGameObjects(FrameInfo &frameInfo);

    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        //手动编译Shader，此时读取编译后的文件
        //路径是从可执行文件开始的，并非从根目录
        Device& device;
//        Pipeline pipeline{device, "../Shaders/MyShader.vert.spv", "../Shaders/MyShader.frag.spv",
//                          Pipeline::setDefaultPipelineConfigureInfo(WIDTH,HEIGHT)};
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    };


}