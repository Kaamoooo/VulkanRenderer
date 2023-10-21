#include "Application.hpp"
#include <numeric>

namespace Kaamoo {


    void Application::run() {
        
#pragma region 创建ubo, description set并将ubo写入到descriptor
        uint32_t minOffsetAlignment = std::lcm(device.properties.limits.minUniformBufferOffsetAlignment,
                                               device.properties.limits.nonCoherentAtomSize);
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
                        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                        .build();

        VkDescriptorSet globalDescriptorSet{};
        auto bufferInfo = globalUboBuffer.descriptorInfo();
        DescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSet);
#pragma endregion

#pragma region 创建normal, pointLight的render system用以渲染不同类别的物体(管线不同)
        RenderSystem renderSystem{
                device,
                renderer.getSwapChainRenderPass(),
                globalSetLayout->getDescriptorSetLayout()};

        PointLightSystem pointLightSystem{
                device,
                renderer.getSwapChainRenderPass(),
                globalSetLayout->getDescriptorSetLayout()
        };
#pragma endregion
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        
#pragma region 设置camera相关参数
        Camera camera{};
        camera.setViewTarget(glm::vec3{-1.f, -2.f, 20.f}, glm::vec3{0, 0, 2.5f});

        auto viewerObject = GameObject::createGameObject();
        viewerObject.transform.translation.z = -2.5f;
        viewerObject.transform.translation.y = -0.5f;
        KeyboardController cameraController{};
#pragma endregion

#pragma region 加载纹理
        std::unique_ptr<Image> image=std::make_unique<Image>(device);
        image->createTextureImage("../Textures/texture1.jpg");
        
#pragma endregion
        
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
                ubo.viewMatrix = camera.getViewMatrix();
                ubo.projectionMatrix = camera.getProjectionMatrix();
                pointLightSystem.update(frameInfo, ubo);


                globalUboBuffer.writeToIndex(&ubo, frameIndex);
                globalUboBuffer.flushIndex(frameIndex);


                //Q:为什么没有将beginFrame与beginSwapChainRenderPass结合在一起？
                //A:为了在这里添加一些其他的pass，例如offscreen shadow pass
                renderer.beginSwapChainRenderPass(commandBuffer);

                renderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);

                renderer.endSwapChainRenderPass(commandBuffer);
                renderer.endFrame();
            }
        }
        vkDeviceWaitIdle(device.device());
    }

    Application::Application() {
        globalPool = DescriptorPool::Builder(device)
                .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();

        loadGameObjects();
    }

    void Application::loadGameObjects() {

        std::shared_ptr<Model> vaseModel = Model::createModelFromFile(device, "../Models/smooth_vase.obj");
        auto vaseObj = GameObject::createGameObject();
        vaseObj.model = vaseModel;
        vaseObj.transform.translation = {0, 0, 0};
        vaseObj.transform.scale = {2.5f, 2.5f, 2.5f};
        gameObjects.emplace(vaseObj.getId(), std::move(vaseObj));

        std::shared_ptr<Model> floorModel = Model::createModelFromFile(device, "../Models/quad.obj");
        auto floorObj = GameObject::createGameObject();
        floorObj.model = floorModel;
        floorObj.transform.translation = {0, 0, 0};
        floorObj.transform.scale = {2.5f, 1, 2.5f};
        gameObjects.emplace(floorObj.getId(), std::move(floorObj));

        std::vector<glm::vec3> lightColors{
                {1.f, .1f, .1f},
                {.1f, .1f, 1.f},
                {.1f, 1.f, .1f},
                {1.f, 1.f, .1f},
                {.1f, 1.f, 1.f},
                {1.f, 1.f, 1.f}
        };

        for (int i = 0; i < lightColors.size(); ++i) {
            auto pointLight = GameObject::makePointLight(0.2f, 0.1f, lightColors[i]);
            pointLight.transform.translation = {0.5f, -0.5f, -1};
            auto rotateLight = glm::rotate(glm::mat4{1.f},
                                           (float) i * glm::two_pi<float>() / static_cast<float>(lightColors.size()),
                                           glm::vec3(0, -1.f, 0));
            pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1, -1, -1, 1));
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }
    }
}