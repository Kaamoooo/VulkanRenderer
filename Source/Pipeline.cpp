﻿#include "Pipeline.hpp"
#include "Material.hpp"

namespace Kaamoo {
    Pipeline::Pipeline(Device &device, const PipelineConfigureInfo &pipelineConfigureInfo, Material &material)
            : device(device), m_material(material) {
        createPipeline(pipelineConfigureInfo);
    }


    Pipeline::~Pipeline() {
        for (auto &shaderModule: m_material.getShaderModulePointers()) {
            if (*(shaderModule->shaderModule) != nullptr) {
                vkDestroyShaderModule(device.device(), *(shaderModule->shaderModule), nullptr);
                *shaderModule->shaderModule = nullptr;
                shaderModule = nullptr;
            }
        }
        vkDestroyPipeline(device.device(), m_pipeline, nullptr);
    }

    void Pipeline::createPipeline(const PipelineConfigureInfo &pipelineConfigureInfo) {
        //Shader
        uint32_t shaderStageCount = m_material.getShaderModulePointers().size();
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo[shaderStageCount];

#ifdef RAY_TRACING
        VkRayTracingShaderGroupCreateInfoKHR group{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.generalShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;
#endif
        for (int i = 0; i < m_material.getShaderModulePointers().size(); i++) {
            shaderStageCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            auto &shaderCategory = m_material.getShaderModulePointers()[i]->shaderCategory;
            switch (shaderCategory) {
                case ShaderCategory::vertex:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
                    break;
                case ShaderCategory::fragment:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;
                case ShaderCategory::tessellationControl:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                    break;
                case ShaderCategory::tessellationEvaluation:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                    break;
                case geometry:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                    break;
#ifdef RAY_TRACING
                case rayGen:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                    group.generalShader = i;
                    m_rayTracingGroups.push_back(group);
                    break;
                case rayClosestHit:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                    group.generalShader = VK_SHADER_UNUSED_KHR;
                    group.closestHitShader = i;
                    m_rayTracingGroups.push_back(group);
                    break;
                case rayMiss:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
                    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                    group.generalShader = i;
                    m_rayTracingGroups.push_back(group);
                    break;
#endif
            }
            shaderStageCreateInfo[i].module = *m_material.getShaderModulePointers()[i]->shaderModule;
            shaderStageCreateInfo[i].pName = "main";
            shaderStageCreateInfo[i].flags = 0;
            shaderStageCreateInfo[i].pNext = nullptr;
            shaderStageCreateInfo[i].pSpecializationInfo = nullptr;
        }


#ifdef RAY_TRACING
        VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
        rayTracingPipelineCreateInfo.stageCount = shaderStageCount;
        rayTracingPipelineCreateInfo.pStages = shaderStageCreateInfo;
        rayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(m_rayTracingGroups.size());
        rayTracingPipelineCreateInfo.pGroups = m_rayTracingGroups.data();
        rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
        rayTracingPipelineCreateInfo.layout = pipelineConfigureInfo.pipelineLayout;
        Device::pfn_vkCreateRayTracingPipelinesKHR(device.device(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCreateInfo, nullptr, &m_pipeline);
#else
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
                pipelineCreateInfo.pTessellationState = &pipelineConfigureInfo.tessellationStateCreateInfo;
        
                pipelineCreateInfo.layout = pipelineConfigureInfo.pipelineLayout;
                pipelineCreateInfo.renderPass = pipelineConfigureInfo.renderPass;
                pipelineCreateInfo.subpass = pipelineConfigureInfo.subpass;
        
                pipelineCreateInfo.basePipelineIndex = -1;
                pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        
                if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
                                              &m_pipeline) != VK_SUCCESS) {
                    throw std::runtime_error("Creating graphics pipeline failed");
                }
#endif

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

        configureInfo.tessellationStateCreateInfo = {};

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
#ifdef RAY_TRACING
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);
#else
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
#endif
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