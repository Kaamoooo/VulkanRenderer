#pragma once

#include <vulkan/vulkan.h>
#include <utility>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include "Device.hpp"
#include "Descriptor.h"
#include "Sampler.h"
#include "Image.h"
#include "Buffer.h"

namespace Kaamoo {

    enum ShaderCategory {
        vertex,
        fragment,
        tessellationControl,
        tessellationEvaluation,
        geometry
    };
    typedef struct ShaderModule {
        std::shared_ptr<VkShaderModule> shaderModule;
        ShaderCategory shaderCategory;

        ShaderModule(std::shared_ptr<VkShaderModule> shaderModule, ShaderCategory shaderCategory) {
            this->shaderModule = std::move(shaderModule);
            this->shaderCategory = shaderCategory;
        }
    } ShaderModule;
    
    class Material {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, Material>;

        Material(id_t id,
                 std::vector<std::shared_ptr<ShaderModule>> shaderModules,
                 std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayouts,
                 std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets,
                 std::vector<std::shared_ptr<Image>> imagePointers,
                 std::vector<std::shared_ptr<Sampler>> samplerPointers,
                 std::vector<std::shared_ptr<Buffer>> bufferPointers,
                 std::string pipelineCategory) :
                materialId(id),
                shaderModules(std::move(shaderModules)),
                descriptorSets(std::move(descriptorSets)),
                descriptorSetLayoutPointers(std::move(descriptorSetLayouts)),
                imagePointers(std::move(imagePointers)),
                samplerPointers(std::move(samplerPointers)),
                bufferPointers(std::move(bufferPointers)),
                pipelineCategory(pipelineCategory) {};
 

        [[nodiscard]] std::vector<std::shared_ptr<ShaderModule>> getShaderModulePointers() const {
            return shaderModules;
        }

        [[nodiscard]] std::vector<std::shared_ptr<DescriptorSetLayout>> getDescriptorSetLayoutPointers() const {
            return descriptorSetLayoutPointers;
        }

        [[nodiscard]] std::vector<std::shared_ptr<VkDescriptorSet>> getDescriptorSetPointers() const {
            return descriptorSets;
        }

        [[nodiscard]] const std::vector<std::shared_ptr<Buffer>> &getBufferPointers() const {
            return bufferPointers;
        }

        id_t getMaterialId() const {
            return materialId;
        }

        const std::string &getPipelineCategory() const {
            return pipelineCategory;
        }

    private:
        id_t materialId;
        std::vector<std::shared_ptr<ShaderModule>> shaderModules;
        std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers;
        std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;
        std::vector<std::shared_ptr<Image>> imagePointers;
        std::vector<std::shared_ptr<Sampler>> samplerPointers;
        std::vector<std::shared_ptr<Buffer>> bufferPointers;
        std::string pipelineCategory;
    };

} // Kaamoo

