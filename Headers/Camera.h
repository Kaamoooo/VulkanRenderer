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
        
        glm::mat4 getViewMatrix() const {
            return viewMatrix;
        }
        
        void setViewDirection(glm::vec3 position,glm::vec3 direction,glm::vec3 up = glm::vec3(0,-fabs(1),0));
        void setViewTarget(glm::vec3 position,glm::vec3 target,glm::vec3 up = glm::vec3(0,-fabs(1),0));
        //使用YXZ tait-bryan angles
        void setViewYXZ(glm::vec3 position,glm::vec3 rotation);

    private:
        glm::mat4 projectionMatrix{1.f};
        glm::mat4 viewMatrix{1.f};
    };
}