//
// Created by asus on 2023/10/26.
//

#include <fstream>
#include <iostream>
#include "ShaderBuilder.h"

namespace Kaamoo {
    std::vector<char> ShaderBuilder::readFile(const std::string &filepath) {
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

    std::shared_ptr<VkShaderModule> ShaderBuilder::createShaderModule(const std::string& shaderName) {
        //judge whether there exists the same shader to create
        //This is disabled in RayTracing due to SBT. The geometries have strict rules to correspond with the shaders.
#ifndef RAY_TRACING
        auto count = shaderModuleMap.count(shaderName);
        if (count > 0)return shaderModuleMap.at(shaderName);
#endif
        
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        auto code = readFile(BASE_SHADER_PATH + shaderName);
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        auto shaderModule=std::make_shared<VkShaderModule>();
        if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule.get()) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }
#ifndef RAY_TRACING
        shaderModuleMap.emplace(shaderName, shaderModule);
#endif
        return shaderModule;
    }

#ifndef RAY_TRACING
    std::shared_ptr<VkShaderModule> ShaderBuilder::getShaderModulePointer(const std::string& shaderName) {
        auto count = shaderModuleMap.count(shaderName);
        if (count <= 0) {
            createShaderModule(shaderName);
        }
        return shaderModuleMap.at(shaderName);
    }
#endif 
    
} // Kaamoo