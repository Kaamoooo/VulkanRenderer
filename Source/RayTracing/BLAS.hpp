#pragma once

#include "../Model.hpp"
#ifdef RAY_TRACING

namespace Kaamoo {
    struct BLASInput {
        std::vector<VkAccelerationStructureGeometryKHR> asGeometryArray;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildRangeInfoArray;
        VkBuildAccelerationStructureFlagsKHR flags;
        uint32_t modelIndexReference;
    };

    struct BLASBuildInfo {
        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
        VkAccelerationStructureBuildRangeInfoKHR *pBuildRangeInfo;
        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo;
        VkAccelerationStructureKHR as;
    };

    class BLAS {
    public:

        inline static std::unordered_map<uint32_t, std::shared_ptr<BLASBuildInfo>> blasBuildInfoMap = {};
        static void release() {
            for (auto &buffer: buffers) {
                delete buffer;
            }
            for (auto &blas: blasToDestroy) {
                Device::pfn_vkDestroyAccelerationStructureKHR(Device::getDeviceSingleton()->device(), blas, nullptr);
            }
            
            for(auto& blasBuildInfo:blasBuildInfoMap){
                Device::pfn_vkDestroyAccelerationStructureKHR(Device::getDeviceSingleton()->device(), blasBuildInfo.second->as, nullptr);
            }
            
            blasInputs.clear();
            blasBuildInfoMap.clear();
        }
        static auto modelToBLASInput(const std::shared_ptr<Model> &model) {
            VkDeviceAddress vertexBufferDeviceAddress = model->getVertexBuffer()->getDeviceAddress();
            VkDeviceAddress indexBufferDeviceAddress = model->getIndexBuffer()->getDeviceAddress();

            uint32_t primitiveCount = model->getPrimitiveCount();

            VkAccelerationStructureGeometryTrianglesDataKHR asGeometryTrianglesData{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
            asGeometryTrianglesData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            asGeometryTrianglesData.vertexData.deviceAddress = vertexBufferDeviceAddress;
            asGeometryTrianglesData.vertexStride = sizeof(Model::Vertex);
            asGeometryTrianglesData.indexType = VK_INDEX_TYPE_UINT32;
            asGeometryTrianglesData.indexData.deviceAddress = indexBufferDeviceAddress;
            asGeometryTrianglesData.maxVertex = model->getVertexCount() - 1;

            VkAccelerationStructureGeometryKHR asGeometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
            asGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            asGeometry.geometry.triangles = asGeometryTrianglesData;
            asGeometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

            VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo{};
            asBuildRangeInfo.primitiveCount = primitiveCount;
            asBuildRangeInfo.primitiveOffset = 0;
            asBuildRangeInfo.firstVertex = 0;
            asBuildRangeInfo.transformOffset = 0;

            BLASInput blasInput{
                    .asGeometryArray = {asGeometry},
                    .asBuildRangeInfoArray = {asBuildRangeInfo},
                    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                             VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR,
                    .modelIndexReference=model->getIndexReference()
            };
            blasInputs.emplace_back(blasInput);
        };
        static void buildBLAS(VkBuildAccelerationStructureFlagsKHR flags) {
            uint32_t blasCount = blasInputs.size();
            VkDeviceSize blasTotalSize = 0;
            uint32_t compactionCount = 0;
            VkDeviceSize maxScratchSize = 0;

            std::vector<std::shared_ptr<BLASBuildInfo>> buildInfos(blasCount);
            for (int i = 0; i < blasCount; i++) {
                buildInfos[i] = std::make_shared<BLASBuildInfo>();
                blasBuildInfoMap.emplace(blasInputs[i].modelIndexReference, buildInfos[i]);
                buildInfos[i]->buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
                buildInfos[i]->buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                buildInfos[i]->buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                buildInfos[i]->buildGeometryInfo.flags = flags | blasInputs[i].flags;
                //Currently 1 geometry per BLAS
                buildInfos[i]->buildGeometryInfo.geometryCount = static_cast<uint32_t>(blasInputs[i].asGeometryArray.size());
                buildInfos[i]->buildGeometryInfo.pGeometries = blasInputs[i].asGeometryArray.data();

                buildInfos[i]->pBuildRangeInfo = blasInputs[i].asBuildRangeInfoArray.data();

                std::vector<uint32_t> maxPrimCount(blasInputs[i].asBuildRangeInfoArray.size());
                for (int j = 0; j < blasInputs[i].asBuildRangeInfoArray.size(); ++j) {
                    maxPrimCount[j] = blasInputs[i].asBuildRangeInfoArray[j].primitiveCount;
                }

                buildInfos[i]->buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
                Device::pfn_vkGetAccelerationStructureBuildSizesKHR(
                        Device::getDeviceSingleton()->device(),
                        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                        &buildInfos[i]->buildGeometryInfo,
                        maxPrimCount.data(),
                        &buildInfos[i]->buildSizesInfo
                );

                blasTotalSize += buildInfos[i]->buildSizesInfo.accelerationStructureSize;
                maxScratchSize += std::max(maxScratchSize, buildInfos[i]->buildSizesInfo.buildScratchSize);
                compactionCount +=
                        buildInfos[i]->buildGeometryInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR
                        ? 1 : 0;
            }
            auto scratchBuffer = new Buffer(
                    *Device::getDeviceSingleton(),
                    maxScratchSize,
                    1,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
            buffers.emplace_back(scratchBuffer);
            VkDeviceAddress scratchBufferDeviceAddress = scratchBuffer->getDeviceAddress();

            VkQueryPool queryPool;
            if (compactionCount > 0) {
                VkQueryPoolCreateInfo queryPoolCreateInfo{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
                queryPoolCreateInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
                queryPoolCreateInfo.queryCount = compactionCount;
                vkCreateQueryPool(Device::getDeviceSingleton()->device(), &queryPoolCreateInfo, nullptr, &queryPool);
            }

            std::vector<uint32_t> indicesToCreate;
            VkDeviceSize batchSize = 0;
            VkDeviceSize batchSizeLimit = 256 * 1024 * 1024;

            for (int i = 0; i < blasCount; i++) {
                indicesToCreate.push_back(i);
                batchSize += buildInfos[i]->buildSizesInfo.accelerationStructureSize;
                if (batchSize > batchSizeLimit || i == blasCount - 1) {
                    VkCommandBuffer commandBuffer = Device::getDeviceSingleton()->beginSingleTimeCommands();
                    cmdCreateBLAS(commandBuffer, indicesToCreate, buildInfos, scratchBufferDeviceAddress, queryPool);
                    Device::getDeviceSingleton()->endSingleTimeCommands(commandBuffer);
                    
                    if (queryPool) {
                        commandBuffer = Device::getDeviceSingleton()->beginSingleTimeCommands();
                        cmdCompactBLAS(commandBuffer, indicesToCreate, buildInfos, queryPool);
                        Device::getDeviceSingleton()->endSingleTimeCommands(commandBuffer);
                    }
                    indicesToCreate.clear();
                    batchSize = 0;
                }
            }
            vkDestroyQueryPool(Device::getDeviceSingleton()->device(), queryPool, nullptr);
        }

    private:
        inline static std::vector<BLASInput> blasInputs{};
        inline static std::vector<Buffer*> buffers{};
        inline static std::vector<VkAccelerationStructureKHR> blasToDestroy{};

        static void cmdCreateBLAS(VkCommandBuffer &commandBuffer,
                                  const std::vector<uint32_t> &indices,
                                  std::vector<std::shared_ptr<BLASBuildInfo>> &buildInfos,
                                  VkDeviceAddress scratchBufferDeviceAddress,
                                  VkQueryPool queryPool) {
            if (queryPool) {
                vkResetQueryPool(Device::getDeviceSingleton()->device(), queryPool, 0, indices.size());
            }
            uint32_t queryCount = 0;
            for (const auto &idx: indices) {
                VkAccelerationStructureCreateInfoKHR asCreateInfo{
                        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
                asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                asCreateInfo.size = buildInfos[idx]->buildSizesInfo.accelerationStructureSize;
                auto asBuffer = new Buffer(
                        *Device::getDeviceSingleton(),
                        asCreateInfo.size,
                        1,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                );
                buffers.emplace_back(asBuffer);
                asCreateInfo.buffer = asBuffer->getBuffer();
                asCreateInfo.offset = 0;
                Device::pfn_vkCreateAccelerationStructureKHR(
                        Device::getDeviceSingleton()->device(),
                        &asCreateInfo,
                        nullptr,
                        &buildInfos[idx]->as
                );

                buildInfos[idx]->buildGeometryInfo.dstAccelerationStructure = buildInfos[idx]->as;
                buildInfos[idx]->buildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

                Device::pfn_vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfos[idx]->buildGeometryInfo,
                                                                &buildInfos[idx]->pBuildRangeInfo);

                VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
                barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                     VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr,
                                     0, nullptr);

                if (queryPool) {
                    Device::pfn_vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, 1, &buildInfos[idx]->as,
                                                                              VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
                                                                              queryPool,queryCount);
                    queryCount++;
                }
            }
        }

        static void cmdCompactBLAS(VkCommandBuffer &commandBuffer,
                                   const std::vector<uint32_t> &indices,
                                   std::vector<std::shared_ptr<BLASBuildInfo>> &buildInfos,
                                   VkQueryPool &queryPool) {
            uint32_t queryCount = 0;
            std::vector<VkAccelerationStructureKHR> cleanUpBLAS(blasInputs.size());
            std::vector<VkDeviceSize> compactSizes(indices.size());
            vkGetQueryPoolResults(
                    Device::getDeviceSingleton()->device(), queryPool,
                    0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
                    compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT
            );
            for (auto &idx: indices) {
                cleanUpBLAS[idx] = buildInfos[idx]->as;
                buildInfos[idx]->buildSizesInfo.accelerationStructureSize = compactSizes[queryCount++];

                VkAccelerationStructureCreateInfoKHR compactAsCreateInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
                compactAsCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                compactAsCreateInfo.size = buildInfos[idx]->buildSizesInfo.accelerationStructureSize;
                auto asBuffer = new Buffer(
                        *Device::getDeviceSingleton(),
                        compactAsCreateInfo.size,
                        1,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                );
                buffers.emplace_back(asBuffer);
                compactAsCreateInfo.buffer = asBuffer->getBuffer();
                Device::pfn_vkCreateAccelerationStructureKHR(
                        Device::getDeviceSingleton()->device(),
                        &compactAsCreateInfo,
                        nullptr,
                        &buildInfos[idx]->as
                );

                VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
                copyInfo.dst = buildInfos[idx]->as;
                copyInfo.src = cleanUpBLAS[idx];
                copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                Device::pfn_vkCmdCopyAccelerationStructureKHR(commandBuffer, &copyInfo);
                blasToDestroy.push_back(cleanUpBLAS[idx]);
            }
        }
    };


}
#endif