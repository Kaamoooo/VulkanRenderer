#include "../Headers/Application.hpp"

namespace Kaamoo {

    void Application::run() {
        RenderSystem renderSystem{device, renderer.getSwapChainRenderPass()};

        Camera camera{};

        while (!myWindow.shouldClose()) {
            glfwPollEvents();
            float aspectRatio = renderer.getAspectRatio();
//            camera.setOrthographicProjection(-aspectRatio, aspectRatio, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspectRatio, 0.1f, 10.f);
            if (auto commandBuffer = renderer.beginFrame()) {
                //Q:为什么没有将beginFrame与beginSwapChainRenderPass结合在一起？
                //A:为了在这里添加一些其他的pass，例如offscreen shadow pass
                renderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }
        vkDeviceWaitIdle(device.device());
    }

    Application::Application() {
        loadGameObjects();
    }

    // temporary helper function, creates a 1x1x1 cube centered at offset
    std::unique_ptr<Model> createCubeModel(Device &device, glm::vec3 offset) {
        std::vector<Model::Vertex> vertices{

                // left face (white)
                {{-.5f, -.5f, -.5f},  {.9f, .9f, .9f}},
                {{-.5f, .5f,  .5f},   {.9f, .9f, .9f}},
                {{-.5f, -.5f, .5f},   {.9f, .9f, .9f}},
                {{-.5f, -.5f, -.5f},  {.9f, .9f, .9f}},
                {{-.5f, .5f,  -.5f},  {.9f, .9f, .9f}},
                {{-.5f, .5f,  .5f},   {.9f, .9f, .9f}},

                // right face (yellow)
                {{.5f,  -.5f, -.5f},  {.8f, .8f, .1f}},
                {{.5f,  .5f,  .5f},   {.8f, .8f, .1f}},
                {{.5f,  -.5f, .5f},   {.8f, .8f, .1f}},
                {{.5f,  -.5f, -.5f},  {.8f, .8f, .1f}},
                {{.5f,  .5f,  -.5f},  {.8f, .8f, .1f}},
                {{.5f,  .5f,  .5f},   {.8f, .8f, .1f}},

                // top face (orange, remember y-axis points down)
                {{-.5f, -.5f, -.5f},  {.9f, .6f, .1f}},
                {{.5f,  -.5f, .5f},   {.9f, .6f, .1f}},
                {{-.5f, -.5f, .5f},   {.9f, .6f, .1f}},
                {{-.5f, -.5f, -.5f},  {.9f, .6f, .1f}},
                {{.5f,  -.5f, -.5f},  {.9f, .6f, .1f}},
                {{.5f,  -.5f, .5f},   {.9f, .6f, .1f}},

                // bottom face (red)
                {{-.5f, .5f,  -.5f},  {.8f, .1f, .1f}},
                {{.5f,  .5f,  .5f},   {.8f, .1f, .1f}},
                {{-.5f, .5f,  .5f},   {.8f, .1f, .1f}},
                {{-.5f, .5f,  -.5f},  {.8f, .1f, .1f}},
                {{.5f,  .5f,  -.5f},  {.8f, .1f, .1f}},
                {{.5f,  .5f,  .5f},   {.8f, .1f, .1f}},

                // nose face (blue)
                {{-.5f, -.5f, 0.5f},  {.1f, .1f, .8f}},
                {{.5f,  .5f,  0.5f},  {.1f, .1f, .8f}},
                {{-.5f, .5f,  0.5f},  {.1f, .1f, .8f}},
                {{-.5f, -.5f, 0.5f},  {.1f, .1f, .8f}},
                {{.5f,  -.5f, 0.5f},  {.1f, .1f, .8f}},
                {{.5f,  .5f,  0.5f},  {.1f, .1f, .8f}},

                // tail face (green)
                {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
                {{.5f,  .5f,  -0.5f}, {.1f, .8f, .1f}},
                {{-.5f, .5f,  -0.5f}, {.1f, .8f, .1f}},
                {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
                {{.5f,  -.5f, -0.5f}, {.1f, .8f, .1f}},
                {{.5f,  .5f,  -0.5f}, {.1f, .8f, .1f}},

        };
        for (auto &v: vertices) {
            v.position += offset;
        }
        return std::make_unique<Model>(device, vertices);
    }

    void Application::loadGameObjects() {
//        std::vector<Model::Vertex> vertices{
//                {{0.5f,  0.5f},  {1, 1, 1}},
//                {{0.5f,  -0.5f}, {1, 0, 0}},
//                {{-0.5f, 0.5f},  {0, 0, 1}},
//        };
//        auto model = std::make_shared<Model>(device, vertices);
//
//        GameObject gameObject = GameObject::createGameObject();
//        gameObject.model = model;
//        gameObject.transform.translation = {-0.3f, 0.2f};
//        gameObject.color = {.1f, .8f, .1f};
//        gameObject.transform.scale = {2.f, 0.4f};
//        gameObject.transform.rotation = .25f * glm::two_pi<float>();

        //push_back是复制传递，因为我们不允许复制gameObject以防内存泄露，这里要使用move
//        gameObjects.push_back(std::move(gameObject));

        std::shared_ptr<Model> cubeModel = createCubeModel(device, {0, 0, 0});
        auto cubeGameObj = GameObject::createGameObject();
        cubeGameObj.model = cubeModel;
        cubeGameObj.transform.translation = {0, 0, 2.5f};
        //vulkan的view视图，xy在-1~1，z在0~1，除此之外会被裁剪
        cubeGameObj.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back(std::move(cubeGameObj));
    }
}