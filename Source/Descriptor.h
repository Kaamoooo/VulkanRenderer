#pragma once

#include "Device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace Kaamoo {

    class DescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(Device &Device) : Device{Device} {}

            Builder &addBinding(
                    uint32_t binding,
                    VkDescriptorType descriptorType,
                    VkShaderStageFlags stageFlags,
                    uint32_t count = 1);

            std::shared_ptr<DescriptorSetLayout> build( ) const;

        private:
            Device &Device;
//            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
            std::vector<VkDescriptorSetLayoutBinding> bindings{};
        public:
            const std::vector<VkDescriptorSetLayoutBinding> &getBindings() const;
        };

        DescriptorSetLayout(
                Device &Device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);

        ~DescriptorSetLayout();

        DescriptorSetLayout(const DescriptorSetLayout &) = delete;

        DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        Device &Device;
        VkDescriptorSetLayout descriptorSetLayout;
        std::vector< VkDescriptorSetLayoutBinding> bindings;

        friend class DescriptorWriter;
    };

    class DescriptorPool {
    public:
        class Builder {
        public:
            Builder(Device &Device) : Device{Device} {}

            Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);

            Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);

            Builder &setMaxSets(uint32_t count);

            std::unique_ptr<DescriptorPool> build() const;

        private:
            Device &Device;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        DescriptorPool(
                Device &Device,
                uint32_t maxSets,
                VkDescriptorPoolCreateFlags poolFlags,
                const std::vector<VkDescriptorPoolSize> &poolSizes);

        ~DescriptorPool();

        DescriptorPool(const DescriptorPool &) = delete;

        DescriptorPool &operator=(const DescriptorPool &) = delete;

        bool allocateDescriptor(
                const VkDescriptorSetLayout descriptorSetLayout, std::shared_ptr<VkDescriptorSet> &descriptorPtr) const;

        void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

        void resetPool();

    private:
        Device &Device;
        VkDescriptorPool descriptorPool;

        friend class DescriptorWriter;
    };

    class DescriptorWriter {
    public:
        DescriptorWriter(std::shared_ptr<DescriptorSetLayout> setLayout, DescriptorPool &pool);

        DescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo bufferInfo);

        DescriptorWriter &writeImage(uint32_t binding, std::shared_ptr<VkDescriptorImageInfo> imageInfo);

        bool build(std::shared_ptr<VkDescriptorSet> &setPtr);

        void overwrite(VkDescriptorSet &set);

    private:
        std::shared_ptr<DescriptorSetLayout> setLayout;
        DescriptorPool &pool;
        std::vector<std::shared_ptr<VkWriteDescriptorSet>> writes;
    };

}  // namespace 