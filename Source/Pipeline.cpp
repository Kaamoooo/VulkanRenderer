#include "Pipeline.hpp"
#include "Material.hpp"

namespace Kaamoo {
    Pipeline::Pipeline(Device &device, const PipelineConfigureInfo &pipelineConfigureInfo, std::shared_ptr<Material> material)
            : device(device), m_material(material) {
        if (material->getPipelineCategory() == PipelineCategory.RayTracing) {
#ifdef RAY_TRACING
            createRayTracingPipeline(pipelineConfigureInfo);
#endif
        } else {
            createGraphicsPipeline(pipelineConfigureInfo);
        }
    }


    Pipeline::~Pipeline() {
        
        vkDestroyPipeline(device.device(), m_pipeline, nullptr);
    }

#ifdef RAY_TRACING

    void Pipeline::createRayTracingPipeline(const PipelineConfigureInfo &pipelineConfigureInfo) {
        //Shader
        uint32_t shaderStageCount = m_material->getShaderModulePointers().size();
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo[shaderStageCount];
        
        for (int i = 0; i < m_material->getShaderModulePointers().size(); i++) {
            VkRayTracingShaderGroupCreateInfoKHR group{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
            group.anyHitShader = VK_SHADER_UNUSED_KHR;
            group.closestHitShader = VK_SHADER_UNUSED_KHR;
            group.generalShader = VK_SHADER_UNUSED_KHR;
            group.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderStageCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            auto &shaderCategory = m_material->getShaderModulePointers()[i]->shaderCategory;
            switch (shaderCategory) {
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
                case rayMiss2:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
                    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                    group.generalShader = i;
                    m_rayTracingGroups.push_back(group);
                    break;
                case rayAnyHit:
                    shaderStageCreateInfo[i].stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                    m_rayTracingGroups.back().anyHitShader = i;
                    break;
            }
            shaderStageCreateInfo[i].module = *m_material->getShaderModulePointers()[i]->shaderModule;
            shaderStageCreateInfo[i].pName = "main";
            shaderStageCreateInfo[i].flags = 0;
            shaderStageCreateInfo[i].pNext = nullptr;
            shaderStageCreateInfo[i].pSpecializationInfo = nullptr;
        }

        VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
        rayTracingPipelineCreateInfo.stageCount = shaderStageCount;
        rayTracingPipelineCreateInfo.pStages = shaderStageCreateInfo;
        rayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(m_rayTracingGroups.size());
        rayTracingPipelineCreateInfo.pGroups = m_rayTracingGroups.data();
        rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 16;
        rayTracingPipelineCreateInfo.layout = pipelineConfigureInfo.pipelineLayout;
        Device::pfn_vkCreateRayTracingPipelinesKHR(device.device(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCreateInfo, nullptr, &m_pipeline);

        createShaderBindingTable();
    }
    
    void Pipeline::createShaderBindingTable() {
        uint32_t hitCount = m_rayTracingGroups.size() - GenShaderCount - MissShaderCount;
        uint32_t handleCount = GenShaderCount + MissShaderCount + hitCount;
        uint32_t handleSize = device.rayTracingPipelineProperties.shaderGroupHandleSize;
        uint32_t handleSizeAligned = Utils::alighUp(handleSize, device.rayTracingPipelineProperties.shaderGroupHandleAlignment);

        m_genRegion.stride = Utils::alighUp(handleSizeAligned, device.rayTracingPipelineProperties.shaderGroupBaseAlignment);
        m_genRegion.size = m_genRegion.stride;
        m_missRegion.stride = handleSizeAligned;
        m_missRegion.size = Utils::alighUp(handleSizeAligned * MissShaderCount, device.rayTracingPipelineProperties.shaderGroupBaseAlignment);
        m_hitRegion.stride = handleSizeAligned;
        m_hitRegion.size = Utils::alighUp(handleSizeAligned * hitCount, device.rayTracingPipelineProperties.shaderGroupBaseAlignment);

        uint32_t dataSize = handleCount * handleSize;
        std::vector<uint8_t> shaderHandles(dataSize);
        Device::pfn_vkGetRayTracingShaderGroupHandlesKHR(device.device(), m_pipeline, 0, handleCount, dataSize, shaderHandles.data());

        VkDeviceSize sbtSize = m_genRegion.size + m_missRegion.size + m_hitRegion.size + m_callableRegion.size;
        m_shaderBindingTableBuffer =std::make_shared<Buffer>(device, sbtSize, 1,
                                                             VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_shaderBindingTableBuffer->map();

        VkDeviceAddress sbtAddress = m_shaderBindingTableBuffer->getDeviceAddress();
        m_genRegion.deviceAddress = sbtAddress;
        m_missRegion.deviceAddress = sbtAddress + m_genRegion.size;
        m_hitRegion.deviceAddress = m_missRegion.deviceAddress + m_missRegion.size;

        auto getHandle = [&](uint32_t groupIndex) -> const uint8_t * { return shaderHandles.data() + groupIndex * handleSize; };

        auto *pSbtBufferData = static_cast<uint8_t *>(m_shaderBindingTableBuffer->getMappedMemory());
        uint8_t *pData = nullptr;
        uint32_t handleIndex = 0;

        {
            //Ray Gen
            pData = pSbtBufferData;
            memcpy(pData, getHandle(handleIndex++), handleSize);

            //Miss
            pData = pSbtBufferData + m_genRegion.size;
            for (uint32_t i = 0; i < MissShaderCount; i++) {
                memcpy(pData, getHandle(handleIndex++), handleSize);
                pData += m_missRegion.stride;
            }

            //Hit
            pData = pSbtBufferData + m_genRegion.size + m_missRegion.size;
            for (uint32_t i = 0; i < hitCount; i++) {
                memcpy(pData, getHandle(handleIndex++), handleSize);
                pData += m_hitRegion.stride;
            }
        }
    }
#endif

    void Pipeline::createGraphicsPipeline(const PipelineConfigureInfo &pipelineConfigureInfo) {
        //Shader
        uint32_t shaderStageCount = m_material->getShaderModulePointers().size();
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo[shaderStageCount];

        for (int i = 0; i < m_material->getShaderModulePointers().size(); i++) {
            shaderStageCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            auto &shaderCategory = m_material->getShaderModulePointers()[i]->shaderCategory;
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
            }
            shaderStageCreateInfo[i].module = *m_material->getShaderModulePointers()[i]->shaderModule;
            shaderStageCreateInfo[i].pName = "main";
            shaderStageCreateInfo[i].flags = 0;
            shaderStageCreateInfo[i].pNext = nullptr;
            shaderStageCreateInfo[i].pSpecializationInfo = nullptr;
        }

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
            throw std::runtime_error("Creating graphics m_pipeline failed");
        }

    }

    void Pipeline::setDefaultPipelineConfigureInfo(PipelineConfigureInfo &configureInfo) {
        configureInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        configureInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        configureInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

//        configureInfo.viewport.x = 0.0f;
//        configureInfo.viewport.y = 0.0f;
//        configureInfo.viewport.m_windowWidth = static_cast<float>(m_windowWidth);
//        configureInfo.viewport.m_windowHeight = static_cast<float>(m_windowHeight);
//        configureInfo.viewport.minDepth = 0.0f;
//        configureInfo.viewport.maxDepth = 1.0f;
//
//        configureInfo.scissor.offset = {0, 0};
//        configureInfo.scissor.extent = {m_windowWidth, m_windowHeight};
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

    void Pipeline::bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint) {
        vkCmdBindPipeline(commandBuffer, bindPoint, m_pipeline);
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