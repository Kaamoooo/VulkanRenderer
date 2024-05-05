﻿#include "Application.h"
#include <numeric>
#include <rapidjson/document.h>
#include <mmcobj.h>

namespace Kaamoo {
    const std::string BaseConfigurationPath = "../Configurations/";
    const std::string BaseModelsPath = "../Models/";
    const std::string BaseTexturePath = "../Textures/";
    const int MATERIAL_NUMBER = 16;
    int LightNum = 0;

    Application::Application() {
        globalPool = DescriptorPool::Builder(device).setMaxSets(
                SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                               SwapChain::MAX_FRAMES_IN_FLIGHT *
                                                                               MATERIAL_NUMBER).addPoolSize(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).build();
        loadGameObjects();
        loadMaterials();
    }

    void Application::run() {

        std::vector<std::shared_ptr<RenderSystem>> renderSystems;
        std::shared_ptr<ShadowSystem> shadowSystem;

        for (auto &material: materials) {
            auto pipelineCategory = material.second.getPipelineCategory();
            if (pipelineCategory == "Shadow") {
                shadowSystem = std::make_shared<ShadowSystem>(device, renderer.getShadowRenderPass(), material.second);
                continue;
            } else if (pipelineCategory == "TessellationGeometry") {
                auto renderSystem = std::make_shared<GrassSystem>(device, renderer.getSwapChainRenderPass(),
                                                                  material.second);
                renderSystem->Init();
                renderSystems.push_back(std::dynamic_pointer_cast<RenderSystem>(renderSystem));
                continue;
            } else if (pipelineCategory == PipelineCategory.SkyBox) {
                auto renderSystem = std::make_shared<SkyBoxSystem>(device, renderer.getSwapChainRenderPass(),
                                                                   material.second);
                renderSystem->Init();
                renderSystems.push_back(std::dynamic_pointer_cast<RenderSystem>(renderSystem));
                continue;
            }
            auto renderSystem = std::make_shared<RenderSystem>(device, renderer.getSwapChainRenderPass(),
                                                               material.second);
            renderSystem->Init();
            renderSystems.push_back(renderSystem);
        }

//        InputController inputController{myWindow.getGLFWwindow()};

        auto currentTime = std::chrono::high_resolution_clock::now();
        float totalTime = 0;
        GlobalUbo ubo{};
        ubo.lightNum = LightNum;
        while (!myWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            totalTime += frameTime;
            currentTime = newTime;

            if (auto commandBuffer = renderer.beginFrame()) {
                int frameIndex = renderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, gameObjects, materials, ubo};

                ubo.curTime = totalTime;

                UpdateComponents(frameInfo);
//                updateCamera(frameInfo, inputController);
//                updateGameObjectMovement(frameInfo, inputController);
                updateLight(frameInfo);

                ubo.shadowViewMatrix[0] = shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position,
                                                                                       glm::vec3(0, 90, 180));
                ubo.shadowViewMatrix[1] = shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position,
                                                                                       glm::vec3(0, -90, 180));
                ubo.shadowViewMatrix[2] = shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position,
                                                                                       glm::vec3(-90, 0, 0));
                ubo.shadowViewMatrix[3] = shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position,
                                                                                       glm::vec3(90, 0, 0));
                ubo.shadowViewMatrix[4] = shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position,
                                                                                       glm::vec3(180, 0, 0));
                ubo.shadowViewMatrix[5] = shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position,
                                                                                       glm::vec3(0, 0, 180));
                ubo.shadowProjMatrix = CameraComponent::CorrectionMatrix * glm::perspective(glm::radians(90.0f),
                                                                                            (float) Application::WIDTH /
                                                                                            (float) Application::HEIGHT,
                                                                                            0.1f, 5.0f);
                ubo.lightProjectionViewMatrix = ubo.shadowProjMatrix * ubo.shadowViewMatrix[0];
//                auto rotateLight = glm::rotate(glm::mat4{1.f}, frameInfo.frameTime, glm::vec3(0, -1.f, 0));


                renderer.beginShadowRenderPass(commandBuffer);
                shadowSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                shadowSystem->renderShadow(frameInfo);
                renderer.endShadowRenderPass(commandBuffer);

                renderer.setShadowMapSynchronization(commandBuffer);

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

    void Application::loadGameObjects() {
        std::string gameObjectsJsonString = JsonUtils::ReadJsonFile(BaseConfigurationPath + "GameObjects.json");

        rapidjson::Document gameObjectsDocument;
        gameObjectsDocument.Parse(gameObjectsJsonString.c_str());

        if (gameObjectsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < gameObjectsDocument.Size(); i++) {
                const rapidjson::Value &object = gameObjectsDocument[i];

                const std::string modelName = object["model"].GetString();

                const rapidjson::Value &transformObject = object["transform"];

                const rapidjson::Value &translationArray = transformObject["translation"];
                glm::vec3 translation{translationArray[0].GetFloat(), translationArray[1].GetFloat(),
                                      translationArray[2].GetFloat()};

                const rapidjson::Value &scaleArray = transformObject["scale"];
                glm::vec3 scale{scaleArray[0].GetFloat(), scaleArray[1].GetFloat(), scaleArray[2].GetFloat()};

                const rapidjson::Value &rotationArray = transformObject["rotation"];
                glm::vec3 rotation{rotationArray[0].GetFloat(), rotationArray[1].GetFloat(),
                                   rotationArray[2].GetFloat()};

                //Todo:In the configuration file all objects have material IDs and models, which may not be appropriate.
                const int materialId = object["materialId"].GetInt();


                auto &gameObject = GameObject::createGameObject();

                //Todo: Create component from a json file
                //Todo: Make camera, light into json file. Not all game objects have models.
                if (modelName=="smooth_vase.obj"){
                    gameObject.TryAddComponent(new ObjectMovementComponent(myWindow.getGLFWwindow()));
                }
                
                std::shared_ptr<Model> model = Model::createModelFromFile(device, BaseModelsPath + modelName);

                auto *meshRendererComponent = new MeshRendererComponent(model, materialId);
                gameObject.TryAddComponent(meshRendererComponent);
                gameObject.setName(modelName);
                gameObject.transform->translation = translation;
                gameObject.transform->scale = scale;
                gameObject.transform->rotation = rotation;

                gameObjects.emplace(gameObject.getId(), std::move(gameObject));
            }
        }


        //Load lights as GameObjects too
        std::vector<glm::vec3> pointLightColors{{.2f, .2f, .2f}};

        for (auto pointLightColor: pointLightColors) {
            LightNum++;
            auto pointLight = GameObject::makeLight(1, 0.1f, pointLightColor, 0);
//            auto rotateLight = glm::rotate(glm::mat4{1.f},(float) i * glm::two_pi<float>() / static_cast<float>(pointLightColors.size()),glm::vec3(0, -1.f, 0));
//            pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(0, -1, 1, 1));
            pointLight.transform->translation = glm::vec4(0, -1.5f, 1, 1);
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }

        //create Directional Light
        LightNum++;
        auto directionalLight = GameObject::makeLight(0.2f, 0.1f, glm::vec3{1.f, 1.f, 1.f}, 1);
        directionalLight.transform->translation = glm::vec4(-1, -2, 0, 1);
        directionalLight.transform->rotation = glm::vec4(1, 2, 0.5, 1);
        gameObjects.emplace(directionalLight.getId(), std::move(directionalLight));

        //Create Camera
        auto &cameraObject = GameObject::createGameObject("Camera");
        auto cameraComponent = new CameraComponent();
        auto cameraMovementComponent = new CameraMovementComponent(myWindow.getGLFWwindow());
        cameraObject.TryAddComponent(cameraComponent);
        cameraObject.TryAddComponent(cameraMovementComponent);
        cameraObject.transform->translation.z = -4;
        cameraObject.transform->translation.y = -1;
//        cameraComponent->setViewTarget(cameraObject.transform->translation, glm::vec3{1, 0, 1}, glm::vec3{0, 1, 0});
        gameObjects.emplace(cameraObject.getId(), std::move(cameraObject));
    }

    void Application::loadMaterials() {

        uint32_t minOffsetAlignment = std::lcm(device.properties.limits.minUniformBufferOffsetAlignment,
                                               device.properties.limits.nonCoherentAtomSize);
        auto globalUboBufferPtr = std::make_shared<Buffer>(
                device, sizeof(GlobalUbo), SwapChain::MAX_FRAMES_IN_FLIGHT,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minOffsetAlignment);
        globalUboBufferPtr->map();

        auto globalDescriptorSetLayoutPointer = DescriptorSetLayout::Builder(device).
                addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS).
                addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS).
                build();

        std::shared_ptr<VkDescriptorSet> globalDescriptorSetPointer = std::make_shared<VkDescriptorSet>();
        auto bufferInfo = globalUboBufferPtr->descriptorInfo();
        DescriptorWriter(globalDescriptorSetLayoutPointer, *globalPool).
                writeBuffer(0, bufferInfo).
                writeImage(1, renderer.getShadowImageInfo()).
                build(globalDescriptorSetPointer);

        std::string materialsString = JsonUtils::ReadJsonFile(BaseConfigurationPath + "Materials.json");
        rapidjson::Document materialsDocument;
        materialsDocument.Parse(materialsString.c_str());
        ShaderBuilder shaderBuilder(device);

        if (materialsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < materialsDocument.Size(); i++) {
                const rapidjson::Value &object = materialsDocument[i];

                const int id = object["id"].GetInt();
                const std::string pipelineCategoryString = object["pipelineCategory"].GetString();

                const std::string vertexShaderName = object["vertexShader"].GetString();
                const std::string fragmentShaderName = object["fragmentShader"].GetString();

                shaderBuilder.createShaderModule(vertexShaderName);
                shaderBuilder.createShaderModule(fragmentShaderName);
                std::vector<std::shared_ptr<ShaderModule>> shaderModulePointers{
                        std::make_shared<ShaderModule>(shaderBuilder.getShaderModulePointer(vertexShaderName),
                                                       ShaderCategory::vertex),
                        std::make_shared<ShaderModule>(shaderBuilder.getShaderModulePointer(fragmentShaderName),
                                                       ShaderCategory::fragment)};

                const bool tessEnabled = object.HasMember("tessellationControlShader");
                if (tessEnabled) {
                    const std::string tessellationControlShaderName = object["tessellationControlShader"].GetString();
                    const std::string tessellationEvaluationShaderName = object["tessellationEvaluationShader"].GetString();
                    shaderModulePointers.emplace_back(std::make_shared<ShaderModule>(
                            shaderBuilder.getShaderModulePointer(tessellationControlShaderName),
                            ShaderCategory::tessellationControl));
                    shaderModulePointers.emplace_back(std::make_shared<ShaderModule>(
                            shaderBuilder.getShaderModulePointer(tessellationEvaluationShaderName),
                            ShaderCategory::tessellationEvaluation));
                }

                const bool geomEnabled = object.HasMember("geometryShader");
                if (geomEnabled) {
                    const std::string geometryShaderName = object["geometryShader"].GetString();
                    shaderModulePointers.emplace_back(
                            std::make_shared<ShaderModule>(shaderBuilder.getShaderModulePointer(geometryShaderName),
                                                           ShaderCategory::geometry));
                }

                auto textureNames = object["texture"].GetArray();

                std::vector<std::shared_ptr<Image>> imagePointers{};
                std::vector<std::shared_ptr<Sampler>> samplerPointers{};
                std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{globalDescriptorSetPointer};
                std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{
                        globalDescriptorSetLayoutPointer};

                //Bind Descriptors
                //binding points: textures, uniform buffers
                DescriptorSetLayout::Builder descriptorSetLayoutBuilder(device);
                int layoutBindingPoint = 0;
                for (auto &textureNameGenericValue: textureNames) {
                    descriptorSetLayoutBuilder.addBinding(layoutBindingPoint++,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                          VK_SHADER_STAGE_ALL_GRAPHICS);
                }

                if (pipelineCategoryString == PipelineCategory.Shadow) {
                    descriptorSetLayoutBuilder.addBinding(layoutBindingPoint++,
                                                          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//                                                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                                          VK_SHADER_STAGE_ALL_GRAPHICS);
                } else if (pipelineCategoryString == PipelineCategory.Overlay ||
                           pipelineCategoryString == PipelineCategory.Opaque ||
                           pipelineCategoryString == PipelineCategory.TessellationGeometry ||
                           pipelineCategoryString == PipelineCategory.SkyBox) {
                    descriptorSetLayoutBuilder.addBinding(layoutBindingPoint++,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                          VK_SHADER_STAGE_ALL_GRAPHICS);
                }

                auto materialDescriptorSetLayoutPointer = descriptorSetLayoutBuilder.build();

                DescriptorWriter descriptorWriter(materialDescriptorSetLayoutPointer, *globalPool);

                //Write Descriptors
                int writerBindingPoint = 0;
                std::shared_ptr<Image> image;
                std::vector<std::shared_ptr<VkDescriptorImageInfo>> imageInfos;
                for (auto &textureNameGenericValue: textureNames) {
                    std::string textureName = textureNameGenericValue.GetString();

                    if (pipelineCategoryString == PipelineCategory.SkyBox)
                        image = std::make_shared<Image>(device, ImageType.CubeMap);
                    else
                        image = std::make_shared<Image>(device, ImageType.Default);

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

                std::vector<std::shared_ptr<Buffer>> bufferPointers{globalUboBufferPtr};
                if (pipelineCategoryString == "Shadow") {
                    auto shadowUboBuffer = std::make_shared<Buffer>(device, sizeof(ShadowUbo),
                                                                    SwapChain::MAX_FRAMES_IN_FLIGHT,
                                                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                                    minOffsetAlignment);
                    bufferPointers.push_back(shadowUboBuffer);
                    auto shadowUboBufferInfo = shadowUboBuffer->descriptorInfo();
                    descriptorWriter.writeBuffer(writerBindingPoint++, shadowUboBufferInfo);
                } else if (pipelineCategoryString == PipelineCategory.Overlay ||
                           pipelineCategoryString == PipelineCategory.Opaque ||
                           pipelineCategoryString == PipelineCategory.TessellationGeometry ||
                           pipelineCategoryString == PipelineCategory.SkyBox) {
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
        //Todo:Optimization required
        for (auto &gameObjPair: gameObjects) {
            auto &obj = gameObjPair.second;
            LightComponent *lightComponent;
            if (obj.TryGetComponent(lightComponent)) {
                int lightIndex = lightComponent->lightIndex;

                assert(lightIndex < MAX_LIGHT_NUM && "光源数目过多");

                Light light{};

                if (lightComponent->lightCategory == LightCategory::POINT_LIGHT) {
                    auto rotateLight = glm::rotate(glm::mat4{1.f}, frameInfo.frameTime, glm::vec3(0, 1.f, 0));
                    obj.transform->translation = glm::vec3(rotateLight * glm::vec4(obj.transform->translation, 1));
                    light.lightCategory = LightCategory::POINT_LIGHT;
                } else {
                    light.lightCategory = LightCategory::DIRECTIONAL_LIGHT;
                }

                light.position = glm::vec4(obj.transform->translation, 1.f);
                light.rotation = glm::vec4(obj.transform->rotation, 1.f);
                light.color = glm::vec4(lightComponent->color, lightComponent->lightIntensity);

                frameInfo.globalUbo.lights[lightIndex] = light;
            }
        }
    }
    

    void Application::UpdateComponents(FrameInfo &frameInfo) {
        ComponentUpdateInfo updateInfo{};
        updateInfo.frameInfo = &frameInfo;
        RendererInfo rendererInfo{renderer.getAspectRatio()};
        updateInfo.rendererInfo = &rendererInfo;
        for (auto &pair: gameObjects) {
            updateInfo.gameObject = &pair.second;
            pair.second.Update(updateInfo);
        }
    }
}