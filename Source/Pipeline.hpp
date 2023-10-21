#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include "Device.hpp"
#include "Model.hpp"

namespace Kaamoo {

    struct PipelineConfigureInfo {
        PipelineConfigureInfo()=default;
        PipelineConfigureInfo(const PipelineConfigureInfo &) = delete;
        PipelineConfigureInfo &operator=(const PipelineConfigureInfo) = delete;

        std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

        //由于修改窗口的时候需要改变管线相关的数据，每次修改都创建一个新的管线太浪费，因此我们使用动态的viewport和scissor
        std::vector<VkDynamicState> dynamicStateEnables;
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
//        VkViewport viewport;
//        VkRect2D scissor;


        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };

    class Pipeline {
    public:
        Pipeline(Device &device, const PipelineConfigureInfo &pipelineConfigureInfo, const std::string &vertShaderPath,
                 const std::string &fragShaderPath, const std::string &geoShaderPath= ""
        );

        ~Pipeline();

        Pipeline(const Pipeline &) = delete;

        void operator=(const Pipeline &) = delete;

        static void setDefaultPipelineConfigureInfo(PipelineConfigureInfo &);
        static void enableAlphaBlending(PipelineConfigureInfo&);

        void bind(VkCommandBuffer commandBuffer);

        bool isGeometryShaderEnabled() const { return enableGeometryShader; }
        

    private:
        Device &device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        VkShaderModule geoShaderModule = VK_NULL_HANDLE;

        bool enableGeometryShader = false;

        static std::vector<char> readFile(const std::string &filepath);

        void
        createPipeline(const PipelineConfigureInfo &pipelineConfigureInfo, const std::string &vertShaderPath,
                       const std::string &fragShaderPath, const std::string &geoShaderPath);

        void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);
    };


}