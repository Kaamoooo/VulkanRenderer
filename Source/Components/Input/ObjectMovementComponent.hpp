#pragma once

#include "InputControllerComponent.hpp"

namespace Kaamoo {
    class ObjectMovementComponent : public InputControllerComponent {
    public:
        ObjectMovementComponent(GLFWwindow *window) : InputControllerComponent(window) {
            name = "ObjectMovementComponent";
        }

        void Update(const ComponentUpdateInfo &updateInfo) override {
            auto moveObject = updateInfo.gameObject;
            if (moveObject != nullptr) {
                if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) {
                    moveObject->transform->Translate({-m_gameObjectMovementSpeed, 0, 0});
                }
                if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) {
                    moveObject->transform->Translate({m_gameObjectMovementSpeed, 0, 0});
                }
                if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) {
                    moveObject->transform->Translate({0, 0, m_gameObjectMovementSpeed});
                }
                if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) {
                    moveObject->transform->Translate({0, 0, -m_gameObjectMovementSpeed});
                }
            }
        }

    private:
        float m_gameObjectMovementSpeed{0.003f};
    };
}