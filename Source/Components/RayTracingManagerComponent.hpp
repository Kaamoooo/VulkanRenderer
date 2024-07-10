#pragma once

#include "Component.hpp"
#include "../RayTracing/BLAS.hpp"

//Deprecated
#ifdef RAY_TRACING
namespace Kaamoo {
    class RayTracingManagerComponent : public Component {
    public:
        ~RayTracingManagerComponent() override {
            BLAS::release();
            TLAS::release();
        };

        RayTracingManagerComponent() {
            name = "RayTracingManagerComponent";
        }

        explicit RayTracingManagerComponent(const rapidjson::Value &object) {
            name = "RayTracingManagerComponentComponent";
        }

        void OnLoad(GameObject *gameObject) override {
        }

        void Loaded(GameObject *gameObject) override {
        };

        void LateUpdate(const ComponentUpdateInfo &updateInfo) override {
            if (TLAS::shouldUpdate) {
                TLAS::buildTLAS(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, true);
                TLAS::shouldUpdate= false;
            }
        };


    private:
    };
}

#endif