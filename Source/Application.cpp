#include "Application.hpp"
#include "Sampler.h"
#include "Shaders.h"
#include "Systems/ShadowSystem.h"
#include <numeric>
#include <rapidjson/document.h>
#include <mmcobj.h>

namespace Kaamoo {
    const std::string BaseConfigurationPath = "../Configurations/";
    const std::string BaseModelsPath = "../Models/";
    const std::string BaseTexturePath = "../Textures/";
    const int MATERIAL_NUMBER = 16;
    int LightNum = 0;

    void Application::run() {

        std::vector<std::shared_ptr<RenderSystem>> renderSystems;
        std::shared_ptr<ShadowSystem> shadowSystem;

        for (auto &material: materials) {
            if (material.second.getPipelineCategory() == "Shadow") {
                shadowSystem = std::make_shared<ShadowSystem>(device, renderer.getShadowRenderPass(),
                                                              material.second);
                continue;
            }
            auto renderSystem =
                    std::make_shared<RenderSystem>(device, renderer.getSwapChainRenderPass(), material.second);
            renderSystems.push_back(renderSystem);
        }


#pragma region 设置camera相关参数
        Camera camera{};
        camera.setViewTarget(glm::vec3{-1.f, -2.f, 20.f}, glm::vec3{0, 0, 2.5f});

        auto viewerObject = GameObject::createGameObject();
        viewerObject.transform.translation.z = -2.5f;
        viewerObject.transform.translation.y = -0.5f;
        KeyboardController cameraController{};
#pragma endregion


        auto currentTime = std::chrono::high_resolution_clock::now();
        GlobalUbo ubo{};
        ubo.lightNum = LightNum;
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
                        gameObjects,
                        materials,
                        ubo
                };

                ubo.viewMatrix = camera.getViewMatrix();
                ubo.projectionMatrix = camera.getProjectionMatrix();

                updateLight(frameInfo);
                glm::mat4 lightProjectionMatrix =
                        shadowSystem->getClipMatrix() *
                        glm::perspective(glm::radians(90.0f), (float) Application::WIDTH / (float) Application::HEIGHT,
                                         0.1f,
                                         20.0f);
                ubo.lightProjectionViewMatrix = lightProjectionMatrix *
                                                shadowSystem->calculateViewMatrixForRotation(
                                                        ubo.pointLights[0].position, glm::vec3(30, 0, 0));

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = renderer.getShadowImage()->getImage();

                VkImageSubresourceRange subresourceRange{};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                subresourceRange.baseMipLevel = 0;
                subresourceRange.levelCount = 1;
                subresourceRange.baseArrayLayer = 0;
                subresourceRange.layerCount = 1;
                barrier.subresourceRange = subresourceRange;

                VkPipelineStageFlagBits srcStage, dstStage;

                barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                srcStage = static_cast<VkPipelineStageFlagBits>(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                                                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                renderer.beginShadowRenderPass(commandBuffer);
                shadowSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                shadowSystem->renderShadow(frameInfo);
                renderer.endShadowRenderPass(commandBuffer);

                vkCmdPipelineBarrier(commandBuffer,
                                     srcStage, dstStage,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                //Q:为什么没有将beginFrame与beginSwapChainRenderPass结合在一起？
                //A:为了在这里添加一些其他的pass，例如offscreen shadow pass
                renderer.beginSwapChainRenderPass(commandBuffer);
                for (const auto &item: renderSystems) {
                    item->UpdateGlobalUboBuffer(ubo, frameIndex);
                    item->render(frameInfo);
                }
                renderer.endSwapChainRenderPass(commandBuffer);


                renderer.endFrame();
            }

        }
        vkDeviceWaitIdle(device.device());
    }

    Application::Application() {
        globalPool = DescriptorPool::Builder(device)
                .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                             SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                             SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER)
                .build();
        loadGameObjects();
        loadMaterials();
    }

    std::string Application::readJsonFile(const std::string &path) {
        std::ifstream jsonFile(path);
        if (!jsonFile.is_open()) {
            throw std::runtime_error("Failed to open: " + path);
        }
        jsonFile.seekg(0, std::ios::end);
        std::streampos length = jsonFile.tellg();
        jsonFile.seekg(0, std::ios::beg);

        std::string jsonString;
        jsonString.resize(static_cast<size_t>(length));
        jsonFile.read(&jsonString[0], length);
        jsonFile.close();
        return jsonString;
    }

    void Application::loadGameObjects() {
        std::string gameObjectsJsonString = readJsonFile(BaseConfigurationPath + "GameObjects.json");

        rapidjson::Document gameObjectsDocument;
        gameObjectsDocument.Parse(gameObjectsJsonString.c_str());

        if (gameObjectsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < gameObjectsDocument.Size(); i++) {
                const rapidjson::Value &object = gameObjectsDocument[i];

                const std::string modelName = object["model"].GetString();

                const rapidjson::Value &transformObject = object["transform"];

                const rapidjson::Value &translationArray = transformObject["translation"];
                glm::vec3 translation{translationArray[0].GetFloat(),
                                      translationArray[1].GetFloat(),
                                      translationArray[2].GetFloat()};

                const rapidjson::Value &scaleArray = transformObject["scale"];
                glm::vec3 scale{scaleArray[0].GetFloat(),
                                scaleArray[1].GetFloat(),
                                scaleArray[2].GetFloat()};

                const rapidjson::Value &rotationArray = transformObject["rotation"];
                glm::vec3 rotation{rotationArray[0].GetFloat(),
                                   rotationArray[1].GetFloat(),
                                   rotationArray[2].GetFloat()};

                const int materialId = object["materialId"].GetInt();

                auto gameObject = GameObject::createGameObject(materialId);
                std::shared_ptr<Model> model = Model::createModelFromFile(device, BaseModelsPath + modelName);
                gameObject.model = model;
                gameObject.transform.translation = translation;
                gameObject.transform.scale = scale;
                gameObject.transform.rotation = rotation;

                gameObjects.emplace(gameObject.getId(), std::move(gameObject));
            }
        }


        std::vector<glm::vec3> lightColors{
                {1.f, .1f, .1f},
//                {.1f, .1f, 1.f},
//                {.1f, 1.f, .1f},
//                {1.f, 1.f, .1f},
//                {.1f, 1.f, 1.f},
//                {1.f, 1.f, 1.f}
        };

        for (int i = 0; i < lightColors.size(); ++i) {
            LightNum++;
            auto pointLight = GameObject::makePointLight(2, 0.1f, lightColors[i]);
            pointLight.setMaterialId(0);
            pointLight.transform.translation = {0.5f, -0.5f, -1};
            auto rotateLight = glm::rotate(glm::mat4{1.f},
                                           (float) i * glm::two_pi<float>() / static_cast<float>(lightColors.size()),
                                           glm::vec3(0, -1.f, 0));
            pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-0.5, -1, -0.5, 1));
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }

        //shadow display sub window
        auto shadowDisplayGameObj = GameObject::createGameObject(4);
        gameObjects.emplace(shadowDisplayGameObj.getId(), std::move(shadowDisplayGameObj));
    }

    void Application::loadMaterials() {

        //create global ubo
        uint32_t minOffsetAlignment = std::lcm(device.properties.limits.minUniformBufferOffsetAlignment,
                                               device.properties.limits.nonCoherentAtomSize);
        auto globalUboBufferPtr = std::make_shared<Buffer>(
                device,
                sizeof(GlobalUbo),
                SwapChain::MAX_FRAMES_IN_FLIGHT,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                minOffsetAlignment
        );
        globalUboBufferPtr->map();

        auto globalDescriptorSetLayoutPointer =
                DescriptorSetLayout::Builder(device)
                        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                        .addBinding(1,
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                    VK_SHADER_STAGE_ALL_GRAPHICS)
                        .build();

        std::shared_ptr<VkDescriptorSet> globalDescriptorSetPointer = std::make_shared<VkDescriptorSet>();
        auto bufferInfo = globalUboBufferPtr->descriptorInfo();
        DescriptorWriter(globalDescriptorSetLayoutPointer, *globalPool)
                .writeBuffer(0, bufferInfo)
                .writeImage(1, renderer.getShadowImageInfo())
                .build(globalDescriptorSetPointer);

        //read json
        std::string materialsString = readJsonFile(BaseConfigurationPath + "Materials.json");

        rapidjson::Document materialsDocument;
        materialsDocument.Parse(materialsString.c_str());

        Shaders shaders(device);
        if (materialsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < materialsDocument.Size(); i++) {
                const rapidjson::Value &object = materialsDocument[i];

                const int id = object["id"].GetInt();

                const std::string pipelineCategoryString = object["pipelineCategory"].GetString();
                const std::string vertexShaderName = object["vertexShader"].GetString();
                const std::string fragmentShaderName = object["fragmentShader"].GetString();
                auto textureNames = object["texture"].GetArray();

                shaders.createShaderModule(vertexShaderName);
                shaders.createShaderModule(fragmentShaderName);
                std::vector<std::shared_ptr<VkShaderModule>> shaderModulePointers{
                        shaders.getShaderModulePointer(vertexShaderName),
                        shaders.getShaderModulePointer(fragmentShaderName)
                };


                std::vector<std::shared_ptr<Image>> imagePointers{};
                std::vector<std::shared_ptr<Sampler>> samplerPointers{};
                std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{
                        globalDescriptorSetPointer
                };
                std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{
                        globalDescriptorSetLayoutPointer,
                };

                //Bind Descriptors
                //binding points: textures, uniform buffers
                DescriptorSetLayout::Builder descriptorSetLayoutBuilder(device);
                int layoutBindingPoint = 0;
                for (auto &textureNameGenericValue: textureNames) {
                    descriptorSetLayoutBuilder.addBinding(layoutBindingPoint++,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                          VK_SHADER_STAGE_ALL_GRAPHICS);
                }

                if (pipelineCategoryString == "Shadow") {
                    descriptorSetLayoutBuilder.addBinding(layoutBindingPoint++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                          VK_SHADER_STAGE_ALL_GRAPHICS);
                } else if (pipelineCategoryString == "Overlay" || pipelineCategoryString == "Opaque") {
                    descriptorSetLayoutBuilder.addBinding(layoutBindingPoint++,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                          VK_SHADER_STAGE_ALL_GRAPHICS);
                }

                auto materialDescriptorSetLayoutPointer = descriptorSetLayoutBuilder.build();

                DescriptorWriter descriptorWriter(materialDescriptorSetLayoutPointer, *globalPool);

                //Write Descriptors
                int writerBindingPoint = 0;
                std::vector<std::shared_ptr<VkDescriptorImageInfo>> imageInfos;
                for (auto &textureNameGenericValue: textureNames) {
                    std::string textureName = textureNameGenericValue.GetString();
                    auto image = std::make_shared<Image>(device);
                    image->createTextureImage(BaseTexturePath + textureName);
                    image->createImageView();
                    auto sampler = std::make_shared<Sampler>(device);
                    sampler->createTextureSampler();
                    auto imageInfo = image->descriptorInfo(*sampler);
                    imageInfos.emplace_back(imageInfo);
                    descriptorWriter.writeImage(writerBindingPoint++, imageInfo);
                    imagePointers.emplace_back(image);
                    samplerPointers.emplace_back(sampler);
                }

                std::vector<std::shared_ptr<Buffer>> bufferPointers{
                        globalUboBufferPtr
                };
                if (pipelineCategoryString == "Shadow") {
                    auto shadowUboBuffer = std::make_shared<Buffer>(
                            device,
                            sizeof(ShadowUbo),
                            SwapChain::MAX_FRAMES_IN_FLIGHT,
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                            minOffsetAlignment
                    );
                    bufferPointers.push_back(shadowUboBuffer);
                    auto shadowUboBufferInfo = shadowUboBuffer->descriptorInfo();
                    descriptorWriter.writeBuffer(writerBindingPoint++, shadowUboBufferInfo);
                } else if (pipelineCategoryString == "Overlay" || pipelineCategoryString == "Opaque") {
                    imagePointers.push_back(renderer.getShadowImage());
                    samplerPointers.push_back(renderer.getShadowSampler());
                    auto imageInfo = renderer.getShadowImageInfo();
                    imageInfos.emplace_back(imageInfo);
                    descriptorWriter.writeImage(writerBindingPoint++, imageInfo);
                }

                std::shared_ptr<VkDescriptorSet> materialDescriptorSetPointer = std::make_shared<VkDescriptorSet>();
                descriptorWriter.build(materialDescriptorSetPointer);

                descriptorSetLayoutPointers.push_back(materialDescriptorSetLayoutPointer);
                descriptorSetPointers.push_back(materialDescriptorSetPointer);


                Material material(id, shaderModulePointers, descriptorSetLayoutPointers, descriptorSetPointers,
                                  imagePointers, samplerPointers, bufferPointers, pipelineCategoryString);
                materials.emplace(id, std::move(material));
            }
        }
    }

    void Application::updateLight(FrameInfo &frameInfo) {
        for (auto &gameObjPair: gameObjects) {
            auto &obj = gameObjPair.second;
            if (obj.pointLightComponent != nullptr) {
                int lightIndex = obj.pointLightComponent->lightIndex;

                assert(lightIndex < MAX_LIGHT_NUM && "点光源数目过多");

                auto rotateLight = glm::rotate(glm::mat4{1.f}, frameInfo.frameTime, glm::vec3(0, -1.f, 0));
                obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1));

                PointLight pointLight{};
                pointLight.color = glm::vec4(obj.color, obj.pointLightComponent->lightIntensity);
                pointLight.position = glm::vec4(obj.transform.translation, 1.f);
                frameInfo.globalUbo.pointLights[lightIndex] = pointLight;
            }
        }
    }

}