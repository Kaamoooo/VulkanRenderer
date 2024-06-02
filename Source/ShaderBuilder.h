#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Device.hpp"

namespace Kaamoo {

    class ShaderBuilder {
    public:
        ShaderBuilder() = delete;
        
        explicit ShaderBuilder(Device &device): device(device){};

        std::shared_ptr<VkShaderModule> createShaderModule(const std::string& shaderName);

        std::shared_ptr<VkShaderModule> getShaderModulePointer(const std::string& shaderName);

        std::vector<char> readFile(const std::string &filepath);

    private:
        const std::string BASE_SHADER_PATH = "../Shaders/";
        
        std::unordered_map<std::string, std::shared_ptr<VkShaderModule>> shaderModuleMap;
        
        Device& device;
    };

} // Kaamoo

