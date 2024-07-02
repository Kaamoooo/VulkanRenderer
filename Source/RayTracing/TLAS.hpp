#pragma once

#include "../Utils/Utils.hpp"
#include "BLAS.hpp"

#ifdef RAY_TRACING
namespace Kaamoo {
    class TLAS {
    public:
        inline static VkAccelerationStructureKHR tlas{};

        static void release() {
            Device::pfn_vkDestroyAccelerationStructureKHR(Device::getDeviceSingleton()->device(), tlas, nullptr);
            for (auto buffer: buffers) {
                delete buffer;
            }
        }

        static auto createTLAS(Model &model, id_t tlasId,glm::mat4 translation = glm::mat4{1.f}) {
            VkAccelerationStructureInstanceKHR instance{};
            //Todo: May encounter a problem here
            instance.transform = Utils::GlmMatrixToVulkanMatrix(translation);
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.instanceCustomIndex = tlasId;
            instance.accelerationStructureReference = Device::getDeviceSingleton()->
                    getAccelerationStructureAddressKHR(BLAS::blasBuildInfoMap[model.getIndexReference()]->as);
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instances.emplace_back(instance);
        }

        //Motion for motion blur, not be used for now
        static void buildTLAS(
                VkBuildAccelerationStructureFlagBitsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                bool update = false, bool motion = false) {
            uint32_t instanceCount = static_cast<uint32_t>(instances.size());

            auto device = Device::getDeviceSingleton();

            auto commandBuffer = device->beginSingleTimeCommands();

            auto instanceBuffer = new Buffer(
                    *device, sizeof(VkAccelerationStructureInstanceKHR) * instanceCount, 1,
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            instanceBuffer->map();
            instanceBuffer->writeToBuffer(instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * instanceCount);
            buffers.emplace_back(instanceBuffer);

            VkBufferDeviceAddressInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
            bufferInfo.buffer = instanceBuffer->getBuffer();
            VkDeviceAddress instanceAddress = vkGetBufferDeviceAddress(device->device(), &bufferInfo);

            VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    0,
                    1, &barrier,
                    0, nullptr,
                    0, nullptr
            );

            cmdCreateTLAS(commandBuffer, instanceAddress, flags, update, motion);

            device->endSingleTimeCommands(commandBuffer);
        }

    private:
        inline static std::vector<VkAccelerationStructureInstanceKHR> instances{};
        inline static std::vector<Buffer *> buffers{};

        static void cmdCreateTLAS(VkCommandBuffer &commandBuffer, VkDeviceAddress instanceBufferDeviceAddress,
                                  VkBuildAccelerationStructureFlagsKHR flags, bool update, bool motion) {
            VkAccelerationStructureGeometryInstancesDataKHR instancesData{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
            instancesData.data.deviceAddress = instanceBufferDeviceAddress;

            VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
            geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            geometry.geometry.instances = instancesData;

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
            buildInfo.flags = flags;
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = &geometry;
            buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
                                    : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
            auto instancesCount = static_cast<uint32_t>(instances.size());
            Device::pfn_vkGetAccelerationStructureBuildSizesKHR(
                    Device::getDeviceSingleton()->device(),
                    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                    &buildInfo,
                    &instancesCount,
                    &sizeInfo
            );

            if (!update) {
                VkAccelerationStructureCreateInfoKHR createInfo{
                        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
                createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
                createInfo.size = sizeInfo.accelerationStructureSize;
                auto tlasBuffer = new Buffer(
                        *Device::getDeviceSingleton(),
                        sizeInfo.accelerationStructureSize,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
                );
                buffers.emplace_back(tlasBuffer);
                createInfo.buffer = tlasBuffer->getBuffer();
                Device::pfn_vkCreateAccelerationStructureKHR(
                        Device::getDeviceSingleton()->device(),
                        &createInfo,
                        nullptr,
                        &tlas
                );
            }

            auto scratchBuffer = new Buffer(
                    *Device::getDeviceSingleton(),
                    sizeInfo.buildScratchSize,
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
            );
            buffers.emplace_back(scratchBuffer);
            VkBufferDeviceAddressInfo bufferDeviceAddressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
            bufferDeviceAddressInfo.buffer = scratchBuffer->getBuffer();
            VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddress(
                    Device::getDeviceSingleton()->device(), &bufferDeviceAddressInfo);

            buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
            buildInfo.dstAccelerationStructure = tlas;
            buildInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

            VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{instancesCount, 0, 0, 0};
            VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfo = &buildRangeInfo;

            Device::pfn_vkCmdBuildAccelerationStructuresKHR(
                    commandBuffer,
                    1,
                    &buildInfo,
                    &pBuildRangeInfo
            );
        }
    };


}

#endif