#include "Pipeline.hpp"
#include "Material.h"

namespace Kaamoo {
    Pipeline::Pipeline(Device &device, const PipelineConfigureInfo &pipelineConfigureInfo, Material &material)
            : device(device), material(material) {
        createPipeline(pipelineConfigureInfo);
    }


    Pipeline::~Pipeline() {
        for (auto &shaderModule: material.getShaderModulePointers()) {
            if (*shaderModule != nullptr) {
                vkDestroyShaderModule(device.device(), *shaderModule, nullptr);
                *shaderModule= nullptr;
            }
        }
        vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
    }

    void Pipeline::createPipeline(const PipelineConfigureInfo &pipelineConfigureInfo) {
        uint32_t shaderStageCount = 2;

        VkPipelineShaderStageCreateInfo shaderStageCreateInfo[shaderStageCount];

        for (int i = 0; i < material.getShaderModulePointers().size(); i++) {
            shaderStageCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            switch (i) {
                case 0:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
                    break;
                case 1:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;
                default:
                    break;
            }
            shaderStageCreateInfo[i].module = *material.getShaderModulePointers()[i];
            shaderStageCreateInfo[i].pName = "main";
            shaderStageCreateInfo[i].flags = 0;
            shaderStageCreateInfo[i].pNext = nullptr;
            shaderStageCreateInfo[i].pSpecializationInfo = nullptr;
        }

//        VkPipelineViewportStateCreateInfo viewportInfo{};
//        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//        viewportInfo.viewportCount = 1;
//        viewportInfo.pViewports = &pipelineConfigureInfo.viewport;
//        viewportInfo.scissorCount = 1;
//        viewportInfo.pScissors = &pipelineConfigureInfo.scissor;

        auto &bindingDescription = pipelineConfigureInfo.vertexBindingDescriptions;
        auto &attributeDescription = pipelineConfigureInfo.attributeDescriptions;

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();
        vertexInputStateCreateInfo.pVertexBindingDescriptions = bindingDescription.data();

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = shaderStageCount;
        pipelineCreateInfo.pStages = shaderStageCreateInfo;
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &pipelineConfigureInfo.inputAssemblyInfo;
        pipelineCreateInfo.pViewportState = &pipelineConfigureInfo.viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &pipelineConfigureInfo.rasterizationInfo;
        pipelineCreateInfo.pMultisampleState = &pipelineConfigureInfo.multisampleInfo;
        pipelineCreateInfo.pColorBlendState = &pipelineConfigureInfo.colorBlendInfo;
        pipelineCreateInfo.pDepthStencilState = &pipelineConfigureInfo.depthStencilInfo;
        //指定动态状态
        pipelineCreateInfo.pDynamicState = &pipelineConfigureInfo.dynamicStateCreateInfo;

        pipelineCreateInfo.layout = pipelineConfigureInfo.pipelineLayout;
        pipelineCreateInfo.renderPass = pipelineConfigureInfo.renderPass;
        pipelineCreateInfo.subpass = pipelineConfigureInfo.subpass;

        pipelineCreateInfo.basePipelineIndex = -1;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                      &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Creating graphics pipeline failed");
        }

    }

    void Pipeline::setDefaultPipelineConfigureInfo(PipelineConfigureInfo &configureInfo) {
        configureInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        configureInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        configureInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

//        configureInfo.viewport.x = 0.0f;
//        configureInfo.viewport.y = 0.0f;
//        configureInfo.viewport.width = static_cast<float>(width);
//        configureInfo.viewport.height = static_cast<float>(height);
//        configureInfo.viewport.minDepth = 0.0f;
//        configureInfo.viewport.maxDepth = 1.0f;
//
//        configureInfo.scissor.offset = {0, 0};
//        configureInfo.scissor.extent = {width, height};
        configureInfo.viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        configureInfo.viewportStateCreateInfo.viewportCount = 1;
        configureInfo.viewportStateCreateInfo.pViewports = nullptr;
        configureInfo.viewportStateCreateInfo.scissorCount = 1;
        configureInfo.viewportStateCreateInfo.pScissors = nullptr;


        configureInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        configureInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
        configureInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        configureInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        configureInfo.rasterizationInfo.lineWidth = 1.0f;
        configureInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        configureInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        configureInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
        configureInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
        configureInfo.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
        configureInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

        configureInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        configureInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
        configureInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        configureInfo.multisampleInfo.minSampleShading = 1.0f;           // Optional
        configureInfo.multisampleInfo.pSampleMask = nullptr;             // Optional
        configureInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
        configureInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

        configureInfo.colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        configureInfo.colorBlendAttachment.blendEnable = VK_FALSE;
        configureInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
        configureInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
        configureInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
        configureInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
        configureInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
        configureInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

        configureInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        configureInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
        configureInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
        configureInfo.colorBlendInfo.attachmentCount = 1;
        configureInfo.colorBlendInfo.pAttachments = &configureInfo.colorBlendAttachment;
        configureInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
        configureInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
        configureInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
        configureInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

        configureInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        configureInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
        configureInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
        configureInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        configureInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        configureInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
        configureInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
        configureInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
        configureInfo.depthStencilInfo.front = {};  // Optional
        configureInfo.depthStencilInfo.back = {};   // Optional

        //指定管线的动态状态
        configureInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        configureInfo.dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        configureInfo.dynamicStateCreateInfo.pDynamicStates = configureInfo.dynamicStateEnables.data();
        configureInfo.dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(configureInfo.dynamicStateEnables.size());
        configureInfo.dynamicStateCreateInfo.flags = 0;
        configureInfo.dynamicStateCreateInfo.pNext = nullptr;

        configureInfo.vertexBindingDescriptions = Model::Vertex::getBindingDescriptions();
        configureInfo.attributeDescriptions = Model::Vertex::getAttributeDescriptions();
    }

    void Pipeline::bind(VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }

    void Pipeline::enableAlphaBlending(PipelineConfigureInfo &configureInfo) {
        configureInfo.colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        configureInfo.colorBlendAttachment.blendEnable = VK_TRUE;
        configureInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        configureInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        configureInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        configureInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
        configureInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
        configureInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
    }


}