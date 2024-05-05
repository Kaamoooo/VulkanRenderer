#ifndef CAMERA_MOVEMENT_COMPONENT_INCLUDED
#define CAMERA_MOVEMENT_COMPONENT_INCLUDED

#include "InputControllerComponent.hpp"

namespace Kaamoo {
    class CameraMovementComponent : public InputControllerComponent {
    public:
        CameraMovementComponent(GLFWwindow *window) : InputControllerComponent(window) {
            name = "CameraMovementComponent";
        }

        void Update(const ComponentUpdateInfo &updateInfo) override {
            glm::vec3 rotation{0};

            if (mousePressed && mouseMoved) {
                rotation.x += deltaPos.y;
                rotation.y -= deltaPos.x;
                mouseMoved = false;
            }

            //防止没有按下按键时，对0归一化导致错误   
            if (glm::dot(rotation, rotation) > std::numeric_limits<float>::epsilon())
                updateInfo.gameObject->transform->rotation +=
                        glm::normalize(rotation) * lookSpeed * updateInfo.frameInfo->frameTime;


            float yaw = updateInfo.gameObject->transform->rotation.y;
            const glm::vec3 forwardDir{glm::sin(yaw), 0, glm::cos(yaw)};
            const glm::vec3 rightDir{forwardDir.z, 0, -forwardDir.x};
            const glm::vec3 upDir{0, -1, 0};

            glm::vec3 moveDir{0};
            if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
            if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
            if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
            if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
            if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
            if (glfwGetKey(window, keys.moveBack) == GLFW_PRESS) moveDir -= forwardDir;

            if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
                updateInfo.gameObject->transform->translation +=
                        glm::normalize(moveDir) * moveSpeed * updateInfo.frameInfo->frameTime;
            }
        }

    };
}

#endif