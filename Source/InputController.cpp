//
// Created by asus on 2023/9/19.
//
#include <iostream>
#include "InputController.h"

namespace Kaamoo {
    bool mousePressed{false};
    bool mouseMoved{false};
    int width, height;
    glm::vec2 lastPos{0};
    glm::vec2 curPos{0};
    glm::vec2 deltaPos{0};

    void cursor_position_callback(GLFWwindow *, double xpos, double ypos) {
        if (mousePressed) {
            mouseMoved= true;
            lastPos = curPos;
            curPos = glm::vec2{xpos / width, ypos / height};
            deltaPos = curPos - lastPos;

            std::cout << "deltaPos: " << deltaPos.x << ", " << deltaPos.y << std::endl;
        }
    }

    void mouse_button_callback(GLFWwindow *, int button, int action, int mods) {
        if (action == GLFW_PRESS) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                mousePressed = true;
                if (glm::dot(deltaPos, deltaPos) > std::numeric_limits<float>::epsilon()) {
                    deltaPos = glm::normalize(curPos - lastPos);
                    mouseMoved = true;
                } else {
                    mouseMoved = false;
                }
            }
        } else if (action == GLFW_RELEASE) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                mousePressed = false;
                lastPos = glm::vec2{0};
                curPos = glm::vec2{0};
                deltaPos = glm::vec2{0};
            }
        }
    }

    void InputController::moveCamera(float dt, GameObject &gameObject) {
        glm::vec3 rotation{0};

        if (moveObject!= nullptr){
            if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) moveObject->transform->Translate({-mouseSensitivity, 0, 0});
            if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) moveObject->transform->Translate({mouseSensitivity, 0, 0});
            if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) moveObject->transform->Translate({0, 0,mouseSensitivity});
            if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) moveObject->transform->Translate({0, 0,-mouseSensitivity});
        }
        
//        if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotation.y += 1;
//        if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotation.y -= 1;
//        if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotation.x -= 1;
//        if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotation.x += 1;

        if (mousePressed && mouseMoved) {
            rotation.x += deltaPos.y;
            rotation.y -= deltaPos.x;
            mouseMoved = false;
        }

        //防止没有按下按键时，对0归一化导致错误   
        if (glm::dot(rotation, rotation) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform->rotation += glm::normalize(rotation) * lookSpeed * dt;
        }

        float yaw = gameObject.transform->rotation.y;
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
            gameObject.transform->translation += glm::normalize(moveDir) * moveSpeed * dt;
        }
    }

    InputController::InputController(GLFWwindow *window) {
        this->window = window;
        glfwGetWindowSize(window, &width, &height);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwMakeContextCurrent(window);
    }


}

