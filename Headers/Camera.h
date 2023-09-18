#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>

namespace Kaamoo {
    class Camera {
    public:
        void setOrthographicProjection(
                float left, float right, float top, float bottom, float near, float far
        );

        void setPerspectiveProjection(
                float fov, float aspect, float near, float far
        );

        glm::mat4 getProjectionMatrix() const {
            return projectionMatrix;
        }

    private:
        glm::mat4 projectionMatrix{1.f};
    };
}