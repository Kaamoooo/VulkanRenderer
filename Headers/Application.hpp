#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include "MyWindow.hpp"
#include "Pipeline.hpp"
#include "Renderer.h"
#include "Device.hpp"
#include "Model.hpp"
#include "GameObject.h"
#include "RenderSystem.h"

namespace Kaamoo {
    class Application {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 800;

        void run();
        Application();
        
        Application(const Application&)=delete;
        Application& operator= (const Application&) = delete;

    private:
        void loadGameObjects();
        
        MyWindow myWindow{WIDTH, HEIGHT, "Hello, Vulkan!"};
        //手动编译Shader，此时读取编译后的文件
        //路径是从可执行文件开始的，并非从根目录
        Device device{myWindow};
//        Pipeline pipeline{device, "../Shaders/MyShader.vert.spv", "../Shaders/MyShader.frag.spv",
//                          Pipeline::setDefaultPipelineConfigureInfo(WIDTH,HEIGHT)};
        std::vector<GameObject> gameObjects;
        Renderer renderer{myWindow,device};
    };


}