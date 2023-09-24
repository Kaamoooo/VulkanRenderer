﻿#pragma once

#include "GameObject.h"
#include "MyWindow.hpp"

namespace Kaamoo{
    class KeyboardController{
    public:
        struct KeyMappings{
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
        };
        
        KeyMappings keys{};
        
        float moveSpeed{3.f};
        float lookSpeed{1.5f};
        
        void moveInPlaneXZ(GLFWwindow* glfWwindow,float dt,GameObject& gameObject);
        
        void changeIteration(GLFWwindow* glfWwindow,std::vector<GameObject>&);
    };
}