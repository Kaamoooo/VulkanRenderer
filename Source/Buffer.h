#pragma once

#include <memory>
#include "Device.hpp"

namespace Kaamoo {

    class Buffer {
    public:
        Buffer(
                Device &device,
                VkDeviceSize instanceSize,
                uint32_t instanceCount,
                VkBufferUsageFlags usageFlags,
                VkMemoryPropertyFlags memoryPropertyFlags,
                VkDeviceSize minOffsetAlignment = 1);

        ~Buffer();

        Buffer(const Buffer &) = delete;

        Buffer &operator=(const Buffer &) = delete;

        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void unmap();

        void writeToBuffer(void *data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        std::shared_ptr<VkDescriptorBufferInfo> descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void writeToIndex(void *data, int index);

        VkResult flushIndex(int index);

        std::shared_ptr<VkDescriptorBufferInfo> descriptorInfoForIndex(int index);

        VkResult invalidateIndex(int index);

        VkBuffer getBuffer() const { return buffer; }

        void *getMappedMemory() const { return mapped; }

        uint32_t getInstanceCount() const { return instanceCount; }

        VkDeviceSize getInstanceSize() const { return instanceSize; }

        VkDeviceSize getAlignmentSize() const { return instanceSize; }

        VkBufferUsageFlags getUsageFlags() const { return usageFlags; }

        VkMemoryPropertyFlags getMemoryPropertyFlags() const { return memoryPropertyFlags; }

        VkDeviceSize getBufferSize() const { return bufferSize; }
        
        VkDeviceAddress getDeviceAddress() const{
            VkBufferDeviceAddressInfo bufferDeviceAddressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
            bufferDeviceAddressInfo.buffer = buffer;
            return vkGetBufferDeviceAddress(Device.device(), &bufferDeviceAddressInfo);
        };
        
        VkDeviceMemory getMemory() const { return memory; }

    private:
        static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

        Device &Device;
        void *mapped = nullptr;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkDeviceSize bufferSize;
        uint32_t instanceCount;
        VkDeviceSize instanceSize;
        VkDeviceSize alignmentSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
    };

}  // namespace 