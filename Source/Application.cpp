#include "../Headers/Application.hpp"

namespace Kaamoo {

    void Application::run() {
        RenderSystem renderSystem{device, renderer.getSwapChainRenderPass()};

        auto currentTime = std::chrono::high_resolution_clock::now();
        
        Camera camera{};
//        camera.setViewDirection(glm::vec3{0},glm::vec3{0.5f,0,1.f});
        camera.setViewTarget(glm::vec3{-1.f,-2.f,20.f},glm::vec3{0,0,2.5f});

        //没有模型的gameObject，用以存储相机的状态
        auto viewerObject = GameObject::createGameObject();
        KeyboardMovementController cameraController{};
        
        
        while (!myWindow.shouldClose()) {
            glfwPollEvents();
            
            //glfwPollEvents函数会阻塞程序，因此在这里定义新时间
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float,std::chrono::seconds::period>(newTime-currentTime).count();
            currentTime=newTime;
            
            cameraController.moveInPlaneXZ(myWindow.getGLFWwindow(),frameTime,viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation,viewerObject.transform.rotation); 
            
            float aspectRatio = renderer.getAspectRatio();
//            camera.setOrthographicProjection(-aspectRatio, aspectRatio, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspectRatio, 0.1f, 20.f);
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
        //世界坐标
        cubeGameObj.transform.translation = {0, 0, 2.5f};
        //vulkan的view视图，xy在-1~1，z在0~1，除此之外会被裁剪
        cubeGameObj.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back(std::move(cubeGameObj));
    }
}