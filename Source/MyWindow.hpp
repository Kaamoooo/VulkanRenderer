#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>

namespace Kaamoo {
    class MyWindow {
    public:
        MyWindow(int w, int h, std::string name);

        ~MyWindow();

        bool shouldClose() {
            return glfwWindowShouldClose(window);
        };

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

        VkExtent2D getExtent() {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        };
        bool isWindowResized(){
            return isFrameBufferResized;
        }
        void resetWindowResizedFlag(){
            isFrameBufferResized= false;
        }

        //禁用窗口的赋值
        MyWindow(const MyWindow &) = delete;
        MyWindow operator=(const MyWindow &) = delete;
        
        static GLFWwindow* getGLFWwindow() {
            return window;
        }

    private:
        static void frameBufferResizedCallback(GLFWwindow* myWindow, int width, int height);
        
        void initWindow();

        inline static GLFWwindow *window;
        std::string windowName;

        int width;
        int height;
        bool isFrameBufferResized = false;
        
        bool frameBufferResized = false;
    };


}