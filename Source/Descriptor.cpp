#include "Descriptor.h"
#include "RayTracing/TLAS.hpp"
// std
#include <cassert>
#include <stdexcept>

namespace Kaamoo {

    DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::addBinding(
            uint32_t binding,
            VkDescriptorType descriptorType,
            VkShaderStageFlags stageFlags,
            uint32_t count) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;

        bindings.push_back(layoutBinding);
        return *this;
    }

    std::shared_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build() const {
        return std::make_shared<DescriptorSetLayout>(Device, bindings);
    }

    const std::vector<VkDescriptorSetLayoutBinding> &
    DescriptorSetLayout::Builder::getBindings() const {
        return bindings;
    }

// *************** Descriptor Set Layout *********************

    DescriptorSetLayout::DescriptorSetLayout(
            class Device &Device, const std::vector<VkDescriptorSetLayoutBinding> &bindings)
            : Device{Device}, bindings{bindings} {

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(
                Device.device(),
                &descriptorSetLayoutInfo,
                nullptr,
                &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(Device.device(), descriptorSetLayout, nullptr);
    }

// *************** Descriptor Pool Builder *********************

    DescriptorPool::Builder &DescriptorPool::Builder::addPoolSize(
            VkDescriptorType descriptorType, uint32_t count) {
        poolSizes.push_back(VkDescriptorPoolSize{descriptorType, count});
        return *this;
    }

    DescriptorPool::Builder &DescriptorPool::Builder::setPoolFlags(
            VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }

    DescriptorPool::Builder &DescriptorPool::Builder::setMaxSets(uint32_t count) {
        maxSets = count;
        return *this;
    }

    std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const {
        return std::make_unique<DescriptorPool>(Device, maxSets, poolFlags, poolSizes);
    }

// *************** Descriptor Pool *********************

    DescriptorPool::DescriptorPool(
            class Device &Device,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize> &poolSizes)
            : Device{Device} {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(Device.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    DescriptorPool::~DescriptorPool() {
        vkDestroyDescriptorPool(Device.device(), descriptorPool, nullptr);
    }

    bool DescriptorPool::allocateDescriptor(
            VkDescriptorSetLayout descriptorSetLayout, std::shared_ptr<VkDescriptorSet> &descriptorPtr) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        if (vkAllocateDescriptorSets(Device.device(), &allocInfo, descriptorPtr.get()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set");
            return false;
        }
        return true;
    }

    void DescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const {
        vkFreeDescriptorSets(
                Device.device(),
                descriptorPool,
                static_cast<uint32_t>(descriptors.size()),
                descriptors.data());
    }

    void DescriptorPool::resetPool() {
        vkResetDescriptorPool(Device.device(), descriptorPool, 0);
    }

// *************** Descriptor Writer *********************

    DescriptorWriter::DescriptorWriter(std::shared_ptr<DescriptorSetLayout> setLayout, DescriptorPool &pool)
            : setLayout{setLayout}, pool{pool} {}

    DescriptorWriter &DescriptorWriter::writeBuffer(
            uint32_t binding, std::shared_ptr<VkDescriptorBufferInfo> bufferInfo) {
        auto &bindingDescription = setLayout->bindings[binding];
        auto write = std::make_shared<VkWriteDescriptorSet>();
        write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write->descriptorType = bindingDescription.descriptorType;
        write->dstBinding = binding;
        write->pBufferInfo = bufferInfo.get();
        write->descriptorCount = 1;
        m_bufferInfo = bufferInfo;
        writes.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeBuffers(uint32_t binding, std::vector<VkDescriptorBufferInfo> &bufferInfos) {
        auto &bindingDescription = setLayout->bindings[binding];
        auto write = std::make_shared<VkWriteDescriptorSet>();
        write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write->descriptorType = bindingDescription.descriptorType;
        write->dstBinding = binding;
        write->pBufferInfo = bufferInfos.data();
        write->descriptorCount = bufferInfos.size();

        writes.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeImage(
            uint32_t binding, const std::shared_ptr<VkDescriptorImageInfo> &imageInfo) {

        auto &bindingDescription = setLayout->bindings[binding];
        auto write = std::make_shared<VkWriteDescriptorSet>();
        write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write->descriptorType = bindingDescription.descriptorType;
        write->dstBinding = binding;
        write->pImageInfo = imageInfo.get();
        write->descriptorCount = 1;
        writes.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeImages(uint32_t binding, std::vector<VkDescriptorImageInfo> &imageInfos) {
        auto &bindingDescription = setLayout->bindings[binding];
        auto write = std::make_shared<VkWriteDescriptorSet>();
        write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write->descriptorType = bindingDescription.descriptorType;
        write->dstBinding = binding;
        write->pImageInfo = imageInfos.data();
        write->descriptorCount = imageInfos.size();
        writes.push_back(write);
        return *this;
    }

#ifdef RAY_TRACING

    DescriptorWriter &DescriptorWriter::writeTLAS(uint32_t binding,
                                                  std::shared_ptr<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureInfo) {
        auto &bindingDescription = setLayout->bindings[binding];
        auto write = std::make_shared<VkWriteDescriptorSet>();
        write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write->descriptorType = bindingDescription.descriptorType;
        write->dstBinding = binding;
        write->descriptorCount = 1;
        write->pNext = accelerationStructureInfo.get();
        writes.push_back(write);
        return *this;
    }

#endif

    bool DescriptorWriter::build(std::shared_ptr<VkDescriptorSet> &setPtr) {
        bool success = pool.allocateDescriptor(setLayout->getDescriptorSetLayout(), setPtr);
        if (!success) {
            return false;
        }
        overwrite(*setPtr);
        return true;
    }

    void DescriptorWriter::overwrite(VkDescriptorSet &set) {
        std::vector<VkWriteDescriptorSet> writeVector;
        for (auto &write: writes) {
            write->dstSet = set;
            writeVector.push_back(*write);
        }
        vkUpdateDescriptorSets(pool.Device.device(), writes.size(), writeVector.data(), 0, nullptr);
    }

} 