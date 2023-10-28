#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include "MyWindow.hpp"
#include "Pipeline.hpp"
#include "Renderer.h"
#include "Device.hpp"
#include "Model.hpp"
#include "GameObject.h"
#include "Systems/RenderSystem.h"
#include "Systems/PointLightSystem.h"
#include "KeyboardMovementController.h"
#include <chrono>
#include "Descriptor.h"
#include "Image.h"
#include "Material.h"
#include "Shaders.h"

namespace Kaamoo {
    class Application {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 800;

        void run();

        Application();

        Application(const Application &) = delete;

        Application &operator=(const Application &) = delete;

    private:
        void loadGameObjects();
        void loadMaterials();
        std::string readJsonFile(const std::string &path);
        MyWindow myWindow{WIDTH, HEIGHT, "VulkanTest"};
        Device device{myWindow};
        Renderer renderer{myWindow, device};

        GameObject::Map gameObjects;
        Material::Map materials;
        std::unique_ptr<DescriptorPool> globalPool;
    };
}