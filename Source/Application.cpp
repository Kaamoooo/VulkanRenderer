#include "Application.hpp"
#include <numeric>

namespace Kaamoo {

    struct GlobalUbo {
        glm::mat4 projectionView{1.f};
        glm::vec3 lightDirection = glm::normalize(glm::vec3(1, -3, -1));
    };

    void Application::run() {
        uint32_t minOffsetAlignment = std::lcm(device.properties.limits.minUniformBufferOffsetAlignment,
                                               device.properties.limits.nonCoherentAtomSize);

        Buffer globalUboBuffer(
                device, sizeof(GlobalUbo), SwapChain::MAX_FRAMES_IN_FLIGHT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
                minOffsetAlignment
        );
        globalUboBuffer.map();

        RenderSystem renderSystem{device, renderer.getSwapChainRenderPass()};

        auto currentTime = std::chrono::high_resolution_clock::now();

        Camera camera{};
//        camera.setViewDirection(glm::vec3{0},glm::vec3{0.5f,0,1.f});
        camera.setViewTarget(glm::vec3{-1.f, -2.f, 20.f}, glm::vec3{0, 0, 2.5f});

        //没有模型的gameObject，用以存储相机的状态
        auto viewerObject = GameObject::createGameObject();
        KeyboardController cameraController{};

        while (!myWindow.shouldClose()) {
            glfwPollEvents();

            //glfwPollEvents函数会阻塞程序，因此在这里定义新时间
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ(myWindow.getGLFWwindow(), frameTime, viewerObject);
            cameraController.changeIteration(myWindow.getGLFWwindow(), gameObjects);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspectRatio = renderer.getAspectRatio();
//            camera.setOrthographicProjection(-aspectRatio, aspectRatio, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(50.f), aspectRatio, 0.1f, 20.f);

            if (auto commandBuffer = renderer.beginFrame()) {
                int frameIndex = renderer.getFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera
                };
                
                GlobalUbo ubo{};
                ubo.projectionView=camera.getProjectionMatrix()*camera.getViewMatrix();
                globalUboBuffer.writeToIndex(&ubo,frameIndex);
                globalUboBuffer.flushIndex(frameIndex);
                
                //Q:为什么没有将beginFrame与beginSwapChainRenderPass结合在一起？
                //A:为了在这里添加一些其他的pass，例如offscreen shadow pass
                renderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(frameInfo, gameObjects);
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

        std::shared_ptr<Model> cubeModel = Model::createModelFromFile(device, "../Models/smooth_vase.obj");
        auto cubeGameObj = GameObject::createGameObject();
        cubeGameObj.model = cubeModel;
        //世界坐标
        cubeGameObj.transform.translation = {0, 0, 2.5f};
        //vulkan的view视图，xy在-1~1，z在0~1，除此之外会被裁剪
        cubeGameObj.transform.scale = {2.5f, 2.5f, 2.5f};
        gameObjects.push_back(std::move(cubeGameObj));
    }
}