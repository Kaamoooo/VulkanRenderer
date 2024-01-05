//
// Created by asus on 2023/9/19.
//
#include <iostream>
#include "KeyboardMovementController.h"

namespace Kaamoo {
    void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
        // 在这里处理鼠标拖动事件
//        std::cout << "Mouse moved to: (" << xpos << ", " << ypos << ")" << std::endl;
    }

    void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
        if (action == GLFW_PRESS) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                std::cout << "Left mouse button pressed" << std::endl;
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                std::cout << "Right mouse button pressed" << std::endl;
            }
            // Add more conditions for other mouse buttons as needed
        } else if (action == GLFW_RELEASE) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                std::cout << "Left mouse button released" << std::endl;
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                std::cout << "Right mouse button released" << std::endl;
            }
            // Add more conditions for other mouse buttons as needed
        }
    }

    void KeyboardController::moveCamera(float dt, GameObject &gameObject) {
        glm::vec3 rotation{0};
        if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotation.y += 1;
        if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotation.y -= 1;
        if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotation.x -= 1;
        if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotation.x += 1;

        //防止没有按下按键时，对0归一化导致错误 
        if (glm::dot(rotation, rotation) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.rotation += glm::normalize(rotation) * lookSpeed * dt;
        }

        float yaw = gameObject.transform.rotation.y;
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
            gameObject.transform.translation += glm::normalize(moveDir) * moveSpeed * dt;
        }
    }

    KeyboardController::KeyboardController(GLFWwindow *window) {
        this->window = window;
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwMakeContextCurrent(window);
    }


}

