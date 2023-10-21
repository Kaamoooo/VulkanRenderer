#include "Pipeline.hpp"

namespace Kaamoo {
    std::vector<char> Pipeline::readFile(const std::string &filepath) {
        std::ifstream inputFileStream{filepath, std::ios::ate | std::ios::binary};

        if (!inputFileStream.is_open()) {
            throw std::runtime_error("Failed to open file: " + filepath);
        }

        size_t fileSize = static_cast<size_t>(inputFileStream.tellg());

        std::vector<char> buffer(fileSize);

        inputFileStream.seekg(0);
        inputFileStream.read(buffer.data(), fileSize);

        inputFileStream.close();
        return buffer;
    }

    Pipeline::Pipeline(Device &device, const PipelineConfigureInfo &pipelineConfigureInfo,
                       const std::string &vertShaderPath, const std::string &fragShaderPath,
                       const std::string &geoShaderPath
    ) : device(device) {
        createPipeline(pipelineConfigureInfo, vertShaderPath, fragShaderPath, geoShaderPath);
    }


    Pipeline::~Pipeline() {
        vkDestroyShaderModule(device.device(), vertShaderModule, nullptr);
        vkDestroyShaderModule(device.device(), fragShaderModule, nullptr);
        vkDestroyShaderModule(device.device(), geoShaderModule, nullptr);
        vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
    }

    void Pipeline::createPipeline(const PipelineConfigureInfo &pipelineConfigureInfo, const std::string &vertShaderPath,
                                  const std::string &fragShaderPath, const std::string &geoShaderPath
    ) {
        auto vertShaderCode = readFile(vertShaderPath);
        auto fragShaderCode = readFile(fragShaderPath);
        std::vector<char> geoShaderCode;
        if (!geoShaderPath.empty()) geoShaderCode = readFile(geoShaderPath);

        std::cout << "Vertex Shader Code Size: " << vertShaderCode.size() << std::endl;
        std::cout << "Fragment Shader Code Size: " << fragShaderCode.size() << std::endl;


        createShaderModule(vertShaderCode, &vertShaderModule);
        createShaderModule(fragShaderCode, &fragShaderModule);

        uint32_t shaderStageCount = 2;

        if (enableGeometryShader) {
            std::cout << "Geometry Shader Code Size: " << geoShaderCode.size() << std::endl;
            createShaderModule(geoShaderCode, &geoShaderModule);
            shaderStageCount++;
        }


        VkPipelineShaderStageCreateInfo shaderStageCreateInfo[shaderStageCount];

        shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageCreateInfo[0].module = vertShaderModule;
        shaderStageCreateInfo[0].pName = "main";
        shaderStageCreateInfo[0].flags = 0;
        shaderStageCreateInfo[0].pNext = nullptr;
        shaderStageCreateInfo[0].pSpecializationInfo = nullptr;
        
        shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageCreateInfo[1].module = fragShaderModule;
        shaderStageCreateInfo[1].pName = "main";
        shaderStageCreateInfo[1].flags = 0;
        shaderStageCreateInfo[1].pNext = nullptr;
        shaderStageCreateInfo[1].pSpecializationInfo = nullptr;

        if (enableGeometryShader) {
            shaderStageCreateInfo[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageCreateInfo[2].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
            shaderStageCreateInfo[2].module = geoShaderModule;
            shaderStageCreateInfo[2].pName = "main";
            shaderStageCreateInfo[2].flags = 0;
            shaderStageCreateInfo[2].pNext = nullptr;
            shaderStageCreateInfo[2].pSpecializationInfo = nullptr;
        }

//        VkPipelineViewportStateCreateInfo viewportInfo{};
//        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//        viewportInfo.viewportCount = 1;
//        viewportInfo.pViewports = &pipelineConfigureInfo.viewport;
//        viewportInfo.scissorCount = 1;
//        viewportInfo.pScissors = &pipelineConfigureInfo.scissor;

        auto& bindingDescription = pipelineConfigureInfo.bindingDescriptions;
        auto& attributeDescription = pipelineConfigureInfo.attributeDescriptions;

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
        
        configureInfo.bindingDescriptions = Model::Vertex::getBindingDescriptions();
        configureInfo.attributeDescriptions = Model::Vertex::getAttributeDescriptions();
    }

    void Pipeline::createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        //reinterpret_cast是更底层的转换，直接在二进制上进行类型转换，不进行类型检查
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
        if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }
    }

    void Pipeline::bind(VkCommandBuffer commandBuffer) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }

    void Pipeline::enableAlphaBlending(PipelineConfigureInfo & configureInfo) {
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