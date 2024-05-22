#pragma once

#include "../Utils/Utils.hpp"
//#include "BLAS.hpp"

namespace Kaamoo {
    class TLAS {
    /*public:
        static auto createTLAS(Model &model, glm::mat4 translation = glm::mat4{1.f}) {
            VkAccelerationStructureInstanceKHR instance{};
            //Todo: May encounter a problem here
            instance.transform = Utils::GlmMatrixToVulkanMatrix(translation);
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.instanceCustomIndex = model.getIndexReference();
            instance.accelerationStructureReference = Device::getDeviceSingleton()->
                    getAccelerationStructureAddressKHR(BLAS::blasBuildInfoMap[model.getIndexReference()].as);
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instances.emplace_back(instance);
        }

    private:
        static std::vector<VkAccelerationStructureInstanceKHR> instances;*/
    };


}