#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>

namespace Kaamoo {
    const int UI_LEFT_WIDTH = 250;
    const int UI_LEFT_WIDTH_2 = 280;
    const int SCENE_WIDTH = 800;
    const int SCENE_HEIGHT = 800;

    class MyWindow {
    public:
        MyWindow(int w, int h, std::string name);

        ~MyWindow();

        bool shouldClose() const{
            return glfwWindowShouldClose(window);
        };

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

        VkExtent2D getCurrentExtent() const{
            return {static_cast<uint32_t>(m_windowWidth), static_cast<uint32_t>(m_windowHeight)};
        };
        
        
        VkExtent2D getCurrentSceneExtent() {
            return {static_cast<uint32_t>(m_windowWidth - UI_LEFT_WIDTH - UI_LEFT_WIDTH_2), static_cast<uint32_t>(m_windowHeight)};
        };

        bool isWindowResized() {
            return isFrameBufferResized;
        }

        void resetWindowResizedFlag() {
            isFrameBufferResized = false;
        }

        //禁用窗口的赋值
        MyWindow(const MyWindow &) = delete;

        MyWindow operator=(const MyWindow &) = delete;

        static GLFWwindow *getGLFWwindow() {
            return window;
        }

    private:
        static void frameBufferResizedCallback(GLFWwindow *myWindow, int width, int height);

        void initWindow();

        inline static GLFWwindow *window;
        std::string windowName;

        int m_windowWidth;
        int m_windowHeight;
        bool isFrameBufferResized = false;
        bool frameBufferResized = false;
    };


}