﻿#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include "../Pipeline.hpp"
#include "../Device.hpp"
#include "../Model.hpp"
#include "../GameObject.hpp"
#include "../StructureInfos.h"

namespace Kaamoo {
    class PointLightSystem {
    public:

        void run();
        PointLightSystem(Device &device,VkRenderPass renderPass,VkDescriptorSetLayout globalSetLayout);
        ~PointLightSystem();

        PointLightSystem(const PointLightSystem&)=delete;
        PointLightSystem& operator= (const PointLightSystem&) = delete;
        
        void update(FrameInfo &frameInfo,GlobalUbo &ubo); 
        void render(FrameInfo &frameInfo);

    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        //手动编译Shader，此时读取编译后的文件
        //路径是从可执行文件开始的，并非从根目录
        Device& device;
//        Pipeline m_pipeline{m_device, "../ShaderBuilder/axis.vert.spv", "../ShaderBuilder/axis.frag.spv",
//                          Pipeline::setDefaultPipelineConfigureInfo(SCENE_WIDTH,SCENE_HEIGHT)};
        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
    };


}