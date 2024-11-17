#pragma once

#include <string>
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <glm/detail/type_mat3x3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>
#include <utility>
#include "../Component.hpp"
#include <GLFW/glfw3.h>

namespace Kaamoo {
    class InputControllerComponent : public Component {
    public:
        InputControllerComponent(GLFWwindow *window) {
            deltaPos = glm::vec2{0};
            curPos = glm::vec2{0};
            lastPos = glm::vec2{0};
            leftMousePressed = false;
            width = 0;
            height = 0;
            name = "InputControllerComponent";
            this->window = window;
            glfwGetWindowSize(window, &width, &height);
            glfwSetCursorPosCallback(window, cursor_position_callback);
            glfwSetMouseButtonCallback(window, mouse_button_callback);
            glfwMakeContextCurrent(window);
        }

        struct KeyMappings {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBack = GLFW_KEY_S;
            int moveUp = GLFW_KEY_E;
            int moveDown = GLFW_KEY_Q;

            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;

            int zero = GLFW_KEY_0;
            int one = GLFW_KEY_1;
            int two = GLFW_KEY_2;
            int three = GLFW_KEY_3;
            int four = GLFW_KEY_4;
        } keys;


    protected:
        GLFWwindow *window;
        
        inline static bool leftMousePressed;
        inline static bool rightMousePressed;
        inline static int width, height;

        inline static glm::vec2 lastPos;
        inline static glm::vec2 curPos;
        inline static glm::vec2 deltaPos;


    private:
        static void cursor_position_callback(GLFWwindow *, double xpos, double ypos) {
            if (rightMousePressed) {
                lastPos = curPos;
                curPos = glm::vec2{xpos / width, ypos / height};
                deltaPos = curPos - lastPos;
            }
        }

        static void mouse_button_callback(GLFWwindow *, int button, int action, int mods) {
            if (action == GLFW_PRESS) {
                if (button == GLFW_MOUSE_BUTTON_LEFT) {
                    leftMousePressed = true;
                }
                if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                    rightMousePressed = true;
                }
            }
            
            if (action == GLFW_RELEASE) {
                if (button == GLFW_MOUSE_BUTTON_LEFT) {
                    leftMousePressed = false;
                }
                if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                    rightMousePressed = false;
                    lastPos = glm::vec2{0};
                    curPos = glm::vec2{0};
                    deltaPos = glm::vec2{0};
                }
            }
        }
    };
}