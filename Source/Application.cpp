#include "Application.hpp"
#include <numeric>

namespace Kaamoo {

    struct GlobalUbo {
        glm::mat4 projectionViewMatrix{1.f};
        glm::vec4 ambientColor{1,1,1,0.02f};
        glm::vec4 pointLightPosition{-1};
        glm::vec4 pointLightColor{1.f};
    };

    void Application::run() {
        uint32_t minOffsetAlignment = std::lcm(device.properties.limits.minUniformBufferOffsetAlignment,device.properties.limits.nonCoherentAtomSize);
        Buffer globalUboBuffer(
                device,
                sizeof(GlobalUbo),
                SwapChain::MAX_FRAMES_IN_FLIGHT,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                minOffsetAlignment
        );
        globalUboBuffer.map();
        
        auto globalSetLayout = 
                DescriptorSetLayout::Builder(device)
                .addBinding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_ALL_GRAPHICS)
                .build();
        
        VkDescriptorSet globalDescriptorSet{};
        auto bufferInfo = globalUboBuffer.descriptorInfo();
        DescriptorWriter(*globalSetLayout,*globalPool)
        .writeBuffer(0,&bufferInfo)
        .build(globalDescriptorSet);

        RenderSystem renderSystem{device, renderer.getSwapChainRenderPass(),globalSetLayout->getDescriptorSetLayout()};

        auto currentTime = std::chrono::high_resolution_clock::now();

        Camera camera{};
        camera.setViewTarget(glm::vec3{-1.f, -2.f, 20.f}, glm::vec3{0, 0, 2.5f});

        auto viewerObject = GameObject::createGameObject();
        viewerObject.transform.translation.z=-2.5f;
        viewerObject.transform.translation.y=-0.5f;
        KeyboardController cameraController{};

        while (!myWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ(myWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspectRatio = renderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.f), aspectRatio, 0.1f, 20.f);

            if (auto commandBuffer = renderer.beginFrame()) {
                int frameIndex = renderer.getFrameIndex();
                FrameInfo frameInfo{
                        frameIndex,
                        frameTime,
                        commandBuffer,
                        camera,
                        globalDescriptorSet,
                        gameObjects
                };

                GlobalUbo ubo{};
                ubo.projectionViewMatrix = camera.getProjectionMatrix() * camera.getViewMatrix();
                globalUboBuffer.writeToIndex(&ubo, frameIndex);
                globalUboBuffer.flushIndex(frameIndex);

                //Q:为什么没有将beginFrame与beginSwapChainRenderPass结合在一起？
                //A:为了在这里添加一些其他的pass，例如offscreen shadow pass
                renderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(frameInfo);
                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }
        vkDeviceWaitIdle(device.device());
    }

    Application::Application() {
        globalPool=DescriptorPool::Builder(device)
                .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,SwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();
        
        loadGameObjects();
    }

    void Application::loadGameObjects() {

        std::shared_ptr<Model> vaseModel = Model::createModelFromFile(device, "../Models/smooth_vase.obj");
        auto vaseObj = GameObject::createGameObject();
        vaseObj.model = vaseModel;
        vaseObj.transform.translation = {0, 0, 0};
        vaseObj.transform.scale = {2.5f, 2.5f, 2.5f};
        gameObjects.emplace(vaseObj.getId(),std::move(vaseObj));
        
        std ::shared_ptr<Model> floorModel = Model::createModelFromFile(device, "../Models/quad.obj");
        auto floorObj = GameObject::createGameObject();
        floorObj.model = floorModel;
        floorObj.transform.translation = {0, 0, 0};
        floorObj.transform.scale = {2.5f, 1, 2.5f};
        gameObjects.emplace(floorObj.getId(),std::move(floorObj));
    }
}