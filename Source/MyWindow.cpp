﻿#include "../Headers/MyWindow.hpp"

namespace Kaamoo {
    MyWindow::MyWindow(int w, int h, std::string name) : width(w), height(h), windowName(name) {
        initWindow();
    }

    MyWindow::~MyWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void MyWindow::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window,this);
        glfwSetFramebufferSizeCallback(window,frameBufferResizedCallback);
    }

    void MyWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface");
        }

    }

    void MyWindow::frameBufferResizedCallback(GLFWwindow *myWindow, int width, int height) {
        //reinterpret_cast，强制指针类型转换而不进行类型检查
        auto window = reinterpret_cast<MyWindow *>(glfwGetWindowUserPointer(myWindow));
        window->frameBufferResized = true;
        window->width = width;
        window->height = height;
    }
}