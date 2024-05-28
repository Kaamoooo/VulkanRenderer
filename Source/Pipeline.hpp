#pragma once

#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>

#include "Device.hpp"
#include "Model.hpp"
#include "Material.hpp"

namespace Kaamoo {
    const struct PipelineCategory {
        std::string Opaque = "Opaque";
        std::string TessellationGeometry = "TessellationGeometry";
        std::string SkyBox = "SkyBox";
        std::string Transparent = "Transparent";
        std::string Shadow = "Shadow";
        std::string Overlay = "Overlay";
    } PipelineCategory;

    struct PipelineConfigureInfo {
        PipelineConfigureInfo() = default;

        PipelineConfigureInfo(const PipelineConfigureInfo &) = delete;

        PipelineConfigureInfo &operator=(const PipelineConfigureInfo) = delete;

        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        VkPipelineRasterizationStateCreateInfo rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo multisampleInfo;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineColorBlendStateCreateInfo colorBlendInfo;
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
        VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo;

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
        Pipeline(Device &device, const PipelineConfigureInfo &pipelineConfigureInfo, Material &material);

        ~Pipeline();

        Pipeline(const Pipeline &) = delete;

        void operator=(const Pipeline &) = delete;

        static void setDefaultPipelineConfigureInfo(PipelineConfigureInfo &);

        static void enableAlphaBlending(PipelineConfigureInfo &);

        void bind(VkCommandBuffer commandBuffer);

    private:
        Device &device;
     
        VkPipeline m_pipeline;
     
        Material &m_material;

#ifdef RAY_TRACING
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rayTracingGroups{};
#endif

        void createPipeline(const PipelineConfigureInfo &pipelineConfigureInfo);


    };


}