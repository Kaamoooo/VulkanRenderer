#pragma once

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


            auto transformRotation = updateInfo.gameObject->transform->rotation;
            auto forwardDirMatrix = Utils::GetRotateDirectionMatrix(transformRotation);
            const glm::vec3 forwardDir = forwardDirMatrix * glm::vec4{0, 0, 1, 1};
            forwardDirMatrix = Utils::GetRotateDirectionMatrix({transformRotation.x + 1, transformRotation.y, transformRotation.z});
            const glm::vec3 forwardDirWithOffset = forwardDirMatrix * glm::vec4{0, 0, 1, 1};
            const glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, forwardDirWithOffset));
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4{1.f}, glm::radians(90.f), rightDir);
            const glm::vec3 upDir = rotationMatrix * glm::vec4{forwardDir, 1};

            glm::vec3 moveDir{0};
            if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
            if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
            if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
            if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
            if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
            if (glfwGetKey(window, keys.moveBack) == GLFW_PRESS) moveDir -= forwardDir;

            if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
                updateInfo.gameObject->transform->translation += glm::normalize(moveDir) * moveSpeed * updateInfo.frameInfo->frameTime;
            }
        }

    };
}
