#pragma once

#include <vulkan/vulkan.h>
#include <utility>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <rapidjson/document.h>
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
        geometry,
        rayGen,
        rayClosestHit,
        rayMiss,
        rayMiss2
    };

    struct alignas(16) PBR{
        glm::vec3 albedo;//0~12
        alignas(16) glm::vec3 normal;//16~28
        alignas(4)  float metallic; //28~32
        float roughness; //32~36
        float opacity;  //36~40
        float AO;   //40~44
        alignas(16)glm::vec3 emissive; //48~60
    };

    typedef struct ShaderModule {
        std::shared_ptr<VkShaderModule> shaderModule;
        ShaderCategory shaderCategory;

        ShaderModule(std::shared_ptr<VkShaderModule> shaderModule, ShaderCategory shaderCategory) {
            this->shaderModule = std::move(shaderModule);
            this->shaderCategory = shaderCategory;
        }
    } ShaderModule;

    const int PBRParametersCount = 7;

    class PBRLoader {
    public:
        static PBR loadPBR(const rapidjson::Value &value) {
            PBR pbr;
            if (value.HasMember("albedo")) {
                auto &albedo = value["albedo"];
                pbr.albedo = glm::vec3(albedo[0].GetFloat(), albedo[1].GetFloat(), albedo[2].GetFloat());
            } else {
                pbr.albedo = glm::vec3(-1.f);
            }

            if (value.HasMember("normal")) {
                pbr.normal = glm::vec3(0);
            } else {
                pbr.normal = glm::vec3(-1.f);
            }

            if (value.HasMember("metallic")) {
                pbr.metallic = value["metallic"].GetFloat();
            } else {
                pbr.metallic = -1.f;
            }

            if (value.HasMember("roughness")) {
                pbr.roughness = value["roughness"].GetFloat();
            } else {
                pbr.roughness = -1.f;
            }

            if (value.HasMember("opacity")) {
                pbr.opacity = value["opacity"].GetFloat();
            } else {
                pbr.opacity = -1.f;
            }

            if (value.HasMember("ao")) {
                pbr.AO = 0;
            } else {
                pbr.AO = -1;
            }
            
            if (value.HasMember("emissive")) {
                auto &emissive = value["emissive"];
                pbr.emissive = glm::vec3(emissive[0].GetFloat(), emissive[1].GetFloat(), emissive[2].GetFloat());
            } else {
                pbr.emissive = glm::vec3(-1, -1, -1);
            }
            return pbr;
        }

        static int getValidPropertyCount(const PBR &pbr) {
            int count = 0;
            if (pbr.albedo != glm::vec3(-1.f)) {
                count++;
            }
            if (pbr.normal != glm::vec3(-1.f)) {
                count++;
            }
            if (pbr.metallic != -1.f) {
                count++;
            }
            if (pbr.roughness != -1.f) {
                count++;
            }
            if (pbr.opacity != -1.f) {
                count++;
            }
            if (pbr.AO!= -1.f) {
                count++;
            }
            if (pbr.emissive != glm::vec3(-1, -1, -1)) {
                count++;
            }
            return count;
        }
    };

    class Material {
    public:
        using id_t = signed int;
        using Map = std::unordered_map<id_t, std::shared_ptr<Material>>;

        ~Material() {
            auto device = Device::getDeviceSingleton()->device();
            for (auto &shaderModule: shaderModules) {
                vkDestroyShaderModule(device, *shaderModule->shaderModule, nullptr);
            }

        };

        Material(id_t id,
                 std::vector<std::shared_ptr<ShaderModule>> &shaderModules,
                 std::vector<std::shared_ptr<DescriptorSetLayout>> &descriptorSetLayouts,
                 std::vector<std::shared_ptr<VkDescriptorSet>> &descriptorSets,
                 std::vector<std::shared_ptr<Image>> &imagePointers,
                 std::vector<std::shared_ptr<Sampler>> &samplerPointers,
                 std::vector<std::shared_ptr<Buffer>> &bufferPointers,
                 std::string pipelineCategory) :
                materialId(id),
                shaderModules(std::move(shaderModules)),
                descriptorSets(std::move(descriptorSets)),
                descriptorSetLayoutPointers(std::move(descriptorSetLayouts)),
                imagePointers(std::move(imagePointers)),
                samplerPointers(std::move(samplerPointers)),
                bufferPointers(std::move(bufferPointers)),
                pipelineCategory(pipelineCategory) {};

        [[nodiscard]] std::vector<std::shared_ptr<Image>> getImagePointers() const {
            return imagePointers;
        }

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