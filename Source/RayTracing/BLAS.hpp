#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include "../Model.hpp"

namespace Kaamoo {
    struct BLASInput {
        std::vector<VkAccelerationStructureGeometryKHR> asGeometryArray;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildRangeInfoArray;
    };

    class BLAS {
    public:
        static auto modelToBLASInput(std::shared_ptr<Model> model, glm::mat4 translation = glm::mat4{1.f}) {
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
            asGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

            VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo{};
            asBuildRangeInfo.primitiveCount = primitiveCount;
            asBuildRangeInfo.primitiveOffset = 0;
            asBuildRangeInfo.firstVertex = 0;
            asBuildRangeInfo.transformOffset = 0;

            BLASInput blasInput{
                    .asGeometryArray = {asGeometry},
                    .asBuildRangeInfoArray = {asBuildRangeInfo}
            };
            blasInputs.emplace_back(blasInput);
        };

        static void buildBLAS() {
            
        }

    private:
        static void createBLAS(VkCommandBuffer commandBuffer,VkBuildAccelerationStructureFlagsKHR flags){

        }

        inline static std::vector<BLASInput> blasInputs{};
    };


}