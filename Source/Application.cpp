#include "../Headers/Application.hpp"

namespace Kaamoo {

    void Application::run() {
        RenderSystem renderSystem{device, renderer.getSwapChainRenderPass()};

        while (!myWindow.shouldClose()) {
            glfwPollEvents();

            if (auto commandBuffer = renderer.beginFrame()) {
                //Q:为什么没有将beginFrame与beginSwapChainRenderPass结合在一起？
                //A:为了在这里添加一些其他的pass，例如offscreen shadow pass
                renderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(commandBuffer, gameObjects);
                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }
        vkDeviceWaitIdle(device.device());
    }

    Application::Application() {
        loadGameObjects();
    }

    void Application::loadGameObjects() {
        std::vector<Model::Vertex> vertices{
                {{0.5f,  0.5f},  {1, 1, 1}},
                {{0.5f,  -0.5f}, {1, 0, 0}},
                {{-0.5f, 0.5f},  {0, 0, 1}},
        };
        auto model = std::make_shared<Model>(device, vertices);

        GameObject gameObject = GameObject::createGameObject();
        gameObject.model = model;
        gameObject.transform2d.translation = {-0.3f, 0.2f};
        gameObject.color = {.1f, .8f, .1f};
        gameObject.transform2d.scale = {2.f, 0.4f};
        gameObject.transform2d.rotation = .25f * glm::two_pi<float>();

        //push_back是复制传递，因为我们不允许复制gameObject以防内存泄露，这里要使用move
        gameObjects.push_back(std::move(gameObject));
    }
}