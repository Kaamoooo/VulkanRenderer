#pragma once

#include "Component.hpp"
#include "../RayTracing/BLAS.hpp"


namespace Kaamoo {
    class RayTracingManagerComponent : public Component {
    public:
        ~RayTracingManagerComponent() override {
            BLAS::release();
        };
        
        RayTracingManagerComponent() {
            name = "MeshRendererComponent";
        }

        explicit RayTracingManagerComponent(const rapidjson::Value &object) {
            name = "RayTracingManagerComponentComponent";
        }
        
        void Loaded(GameObject*gameObject) override {
            BLAS::buildBLAS(VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR
                            | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
        }

        void Start(const ComponentUpdateInfo &updateInfo) override {
            TLAS::buildTLAS(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
        };

        void LateUpdate(const ComponentUpdateInfo &updateInfo) override {
        };


    private:
    };
}