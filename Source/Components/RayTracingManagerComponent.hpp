#pragma once

#include "Component.hpp"
#include "../RayTracing/BLAS.hpp"

namespace Kaamoo {
    class RayTracingManagerComponent : public Component {
    public:
        RayTracingManagerComponent() {
            name = "MeshRendererComponent";
        }

        explicit RayTracingManagerComponent(const rapidjson::Value &object) {
            name = "RayTracingManagerComponentComponent";
        }


        void LateUpdate(const ComponentUpdateInfo &updateInfo) override {
        };


    private:
    };
}