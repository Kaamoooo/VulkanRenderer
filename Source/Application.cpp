#include "Application.h"
#include "ComponentFactory.hpp"
#include <numeric>
#include <rapidjson/document.h>
#include <glm/ext/matrix_clip_space.hpp>

namespace Kaamoo {
    Application::~Application() {
        gameObjects.clear();
        m_materials.clear();
    }

    Application::Application() : m_shaderBuilder(device) {
        globalPool = DescriptorPool::Builder(device).
                setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).
                addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).
                addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).build();
        loadGameObjects();
        loadMaterials();
        createRenderSystems();
    }

    void Application::run() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float totalTime = 0;
        GlobalUbo ubo{};
        Awake();
        while (!myWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            totalTime += frameTime;
            currentTime = newTime;

            if (auto commandBuffer = renderer.beginFrame()) {
                int frameIndex = renderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, gameObjects, m_materials, ubo, myWindow.getExtent()};

                UpdateComponents(frameInfo);
                UpdateUbo(ubo, totalTime);

#ifdef RAY_TRACING
                m_rayTracingSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                m_rayTracingSystem->rayTrace(frameInfo);
                
                renderer.beginSwapChainRenderPass(commandBuffer);
                m_postSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                m_postSystem->render(frameInfo);
                renderer.endSwapChainRenderPass(commandBuffer);
#else
                renderer.beginShadowRenderPass(commandBuffer);
                m_shadowSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                m_shadowSystem->renderShadow(frameInfo);
                renderer.endShadowRenderPass(commandBuffer);
                
                renderer.setShadowMapSynchronization(commandBuffer);

                renderer.beginSwapChainRenderPass(commandBuffer);
                for (const auto &item: m_renderSystems) {
                    item->UpdateGlobalUboBuffer(ubo, frameIndex);
                    item->render(frameInfo);
                }
                renderer.endSwapChainRenderPass(commandBuffer);
#endif

                renderer.endFrame();
            }

        }
        vkDeviceWaitIdle(device.device());
    }

    void Application::loadGameObjects() {
        std::string gameObjectsJsonString = JsonUtils::ReadJsonFile(BasePath + GameObjectsFileName);
        std::string componentsJsonString = JsonUtils::ReadJsonFile(BasePath + ComponentsFileName);

        rapidjson::Document gameObjectsDocument;
        rapidjson::Document componentsDocument;
        gameObjectsDocument.Parse(gameObjectsJsonString.c_str());
        componentsDocument.Parse(componentsJsonString.c_str());

        std::unordered_map<int, rapidjson::Value> componentsMap;
        if (componentsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < componentsDocument.Size(); i++) {
                rapidjson::Value componentValue = std::move(componentsDocument[i]);
                componentsMap[componentValue["id"].GetInt()] = componentValue;
            }
        }

        auto *componentFactory = new ComponentFactory();
        if (gameObjectsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < gameObjectsDocument.Size(); i++) {
                const rapidjson::Value &object = gameObjectsDocument[i];

                const rapidjson::Value &transformObject = object["transform"];

                const rapidjson::Value &translationArray = transformObject["translation"];
                glm::vec3 translation{translationArray[0].GetFloat(), translationArray[1].GetFloat(),
                                      translationArray[2].GetFloat()};

                const rapidjson::Value &scaleArray = transformObject["scale"];
                glm::vec3 scale{scaleArray[0].GetFloat(), scaleArray[1].GetFloat(), scaleArray[2].GetFloat()};

                const rapidjson::Value &rotationArray = transformObject["rotation"];
                glm::vec3 rotation{rotationArray[0].GetFloat(), rotationArray[1].GetFloat(),
                                   rotationArray[2].GetFloat()};

                auto &gameObject = GameObject::createGameObject();

                if (object.HasMember("componentIds")) {
                    auto componentIdsArray = object["componentIds"].GetArray();
                    for (rapidjson::SizeType j = 0; j < componentIdsArray.Size(); j++) {
                        const rapidjson::Value &arrayId = componentIdsArray[j];
                        const int componentId = arrayId.GetInt();
                        auto componentPtr = componentFactory->CreateComponent(componentsMap, componentId);
                        if (componentPtr) {
                            gameObject.TryAddComponent(componentPtr);
                        }
                    }
                }
                gameObject.setName(object["name"].GetString());
                gameObject.transform->translation = translation;
                gameObject.transform->scale = scale;
                gameObject.transform->rotation = rotation;

                gameObjects.emplace(gameObject.getId(), std::move(gameObject));
            }
        }

        for (auto &pair: gameObjects) {
            auto &gameObject = pair.second;
            gameObject.OnLoad();
        }

        for (auto &pair: gameObjects) {
            auto &gameObject = pair.second;
            gameObject.Loaded();
        }

    }

    void Application::loadMaterials() {

        uint32_t minOffsetAlignment = std::lcm(device.properties.limits.minUniformBufferOffsetAlignment,
                                               device.properties.limits.nonCoherentAtomSize);
        std::string materialsString = JsonUtils::ReadJsonFile(BasePath + "Materials.json");
        rapidjson::Document materialsDocument;
        materialsDocument.Parse(materialsString.c_str());

        auto globalUboBufferPtr = std::make_shared<Buffer>(
                device, sizeof(GlobalUbo), SwapChain::MAX_FRAMES_IN_FLIGHT,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minOffsetAlignment);
        globalUboBufferPtr->map();

#ifdef RAY_TRACING
        //TLAS, offscreen
        auto rayGenDescriptorSetLayoutPtr = DescriptorSetLayout::Builder(device).
                addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR).
                addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR).
                build();

        auto rayGenDescriptorSet = std::make_shared<VkDescriptorSet>();

        auto accelerationStructureInfo = std::make_shared<VkWriteDescriptorSetAccelerationStructureKHR>();
        accelerationStructureInfo->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureInfo->accelerationStructureCount = 1;
        accelerationStructureInfo->pAccelerationStructures = &TLAS::tlas;
        auto offScreenImageInfo = std::make_shared<VkDescriptorImageInfo>();
        offScreenImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        offScreenImageInfo->imageView = renderer.getOffscreenImageColor()->imageView;

        DescriptorWriter(rayGenDescriptorSetLayoutPtr, *globalPool).
                writeTLAS(0, accelerationStructureInfo).
                writeImage(1, offScreenImageInfo).
                build(rayGenDescriptorSet);

        //Global, Obj, Textures
        auto sceneDescriptorSetLayoutPtr = DescriptorSetLayout::Builder(device).
                addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR).
                addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR).
                build();
        
        //Obj
        std::vector<ModelDesc> modelDescs;
        for (auto &modelPair: Model::models) {
            ModelDesc modelDesc{};
            auto model = modelPair.second;
            modelDesc.indexReference = model->getIndexReference();
            modelDesc.vertexBufferAddress = model->getVertexBuffer()->getDeviceAddress();
            modelDesc.indexBufferAddress = model->getIndexBuffer()->getDeviceAddress();
            modelDescs.push_back(modelDesc);
        }
        auto objBufferPtr = std::make_shared<Buffer>(device, sizeof(ModelDesc), modelDescs.size(),
                                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minOffsetAlignment);
        objBufferPtr->map();

        //Textures, for now we ignore

        //Build Descriptor Set
        auto sceneDescriptorSet = std::make_shared<VkDescriptorSet>();
        DescriptorWriter(sceneDescriptorSetLayoutPtr, *globalPool).
                writeBuffer(0, globalUboBufferPtr->descriptorInfo(globalUboBufferPtr->getBufferSize())).
                writeBuffer(1, objBufferPtr->descriptorInfo(objBufferPtr->getBufferSize())).
                build(sceneDescriptorSet);

        //Load from Json
        if (materialsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < materialsDocument.Size(); i++) {
                const rapidjson::Value &object = materialsDocument[i];

                const int id = object["id"].GetInt();
                const std::string pipelineCategoryString = object["pipelineCategory"].GetString();

                if (pipelineCategoryString == PipelineCategory.RayTracing) {

                    const std::string rayGenShaderName = object["rayGenShader"].GetString();
                    const std::string rayMissShaderName = object["rayMissShader"].GetString();
                    const std::string rayClosestShaderName = object["rayClosestHitShader"].GetString();

                    m_shaderBuilder.createShaderModule(rayGenShaderName);
                    m_shaderBuilder.createShaderModule(rayMissShaderName);
                    m_shaderBuilder.createShaderModule(rayClosestShaderName);

                    std::vector<std::shared_ptr<ShaderModule>> shaderModulePointers{
                            std::make_shared<ShaderModule>(m_shaderBuilder.getShaderModulePointer(rayGenShaderName), ShaderCategory::rayGen),
                            std::make_shared<ShaderModule>(m_shaderBuilder.getShaderModulePointer(rayMissShaderName), ShaderCategory::rayMiss),
                            std::make_shared<ShaderModule>(m_shaderBuilder.getShaderModulePointer(rayClosestShaderName), ShaderCategory::rayClosestHit)
                    };

//                const bool tessEnabled = object.HasMember("tessellationControlShader");

                    std::vector<std::shared_ptr<Image>> imagePointers{renderer.getOffscreenImageColor()};
                    std::vector<std::shared_ptr<Sampler>> samplerPointers{};
                    std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{rayGenDescriptorSet, sceneDescriptorSet};
                    std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{rayGenDescriptorSetLayoutPtr, sceneDescriptorSetLayoutPtr};
                    std::vector<std::shared_ptr<Buffer>> bufferPointers{globalUboBufferPtr, objBufferPtr};

//                auto textureNames = object["texture"].GetArray();
//                for (auto &textureNameGenericValue: textureNames) {
//                    descriptorSetLayoutBuilder.addBinding(layoutBindingPoint++,
//                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//                                                          VK_SHADER_STAGE_ALL_GRAPHICS);
//                }

                    //Todo: Currently only one ray tracing m_material
                    auto material = std::make_shared<Material>(id, shaderModulePointers, descriptorSetLayoutPointers, descriptorSetPointers,
                                      imagePointers, samplerPointers, bufferPointers, pipelineCategoryString);
                    m_materials.emplace(id, std::move(material));
                }
            }
        }

        //Post
        {
            auto postSystemDescriptorSetLayoutPtr =
                    DescriptorSetLayout::Builder(device).
                            addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT).
                            addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT).
                            build();


            auto offScreenPostImageInfo = renderer.getOffscreenImageColor()->descriptorInfo();
            offScreenPostImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            auto postDescriptorSet = std::make_shared<VkDescriptorSet>();
            DescriptorWriter(postSystemDescriptorSetLayoutPtr, *globalPool).
                    writeImage(0, offScreenPostImageInfo).
                    writeBuffer(1, globalUboBufferPtr->descriptorInfo()).
                    build(postDescriptorSet);

            m_shaderBuilder.createShaderModule(PostVertexShaderName);
            m_shaderBuilder.createShaderModule(PostFragmentShaderName);
            std::vector<std::shared_ptr<ShaderModule>> shaderModulePointers{
                    std::make_shared<ShaderModule>(m_shaderBuilder.getShaderModulePointer(PostVertexShaderName), ShaderCategory::vertex),
                    std::make_shared<ShaderModule>(m_shaderBuilder.getShaderModulePointer(PostFragmentShaderName), ShaderCategory::fragment),
            };
            std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{postSystemDescriptorSetLayoutPtr};
            std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{postDescriptorSet};
            std::vector<std::shared_ptr<Image>> imagePointers{renderer.getOffscreenImageColor()};
            std::vector<std::shared_ptr<Sampler>> samplerPointers{};
            std::vector<std::shared_ptr<Buffer>> bufferPointers{globalUboBufferPtr};

            auto postMaterial = std::make_shared<Material>(-1, shaderModulePointers, descriptorSetLayoutPointers, descriptorSetPointers, imagePointers, samplerPointers, bufferPointers, PipelineCategory.Post);
            m_materials.emplace(-1, std::move(postMaterial));
        }
#else
        auto bufferInfo = globalUboBufferPtr->descriptorInfo();

        auto globalDescriptorSetLayoutPointer = DescriptorSetLayout::Builder(device).
                addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS).
                addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS).
                build();

        std::shared_ptr<VkDescriptorSet> globalDescriptorSetPointer = std::make_shared<VkDescriptorSet>();
        DescriptorWriter(globalDescriptorSetLayoutPointer, *globalPool).
                writeBuffer(0, bufferInfo).
                writeImage(1, renderer.getShadowImageInfo()).
                build(globalDescriptorSetPointer);

        if (materialsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < materialsDocument.Size(); i++) {
                const rapidjson::Value &object = materialsDocument[i];

                const int id = object["id"].GetInt();
                const std::string pipelineCategoryString = object["pipelineCategory"].GetString();

                const std::string vertexShaderName = object["vertexShader"].GetString();
                const std::string fragmentShaderName = object["fragmentShader"].GetString();

                m_shaderBuilder.createShaderModule(vertexShaderName);
                m_shaderBuilder.createShaderModule(fragmentShaderName);
                std::vector<std::shared_ptr<ShaderModule>> shaderModulePointers{
                        std::make_shared<ShaderModule>(m_shaderBuilder.getShaderModulePointer(vertexShaderName),
                                                       ShaderCategory::vertex),
                        std::make_shared<ShaderModule>(m_shaderBuilder.getShaderModulePointer(fragmentShaderName),
                                                       ShaderCategory::fragment)};

                const bool tessEnabled = object.HasMember("tessellationControlShader");
                if (tessEnabled) {
                    const std::string tessellationControlShaderName = object["tessellationControlShader"].GetString();
                    const std::string tessellationEvaluationShaderName = object["tessellationEvaluationShader"].GetString();
                    shaderModulePointers.emplace_back(std::make_shared<ShaderModule>(
                            m_shaderBuilder.getShaderModulePointer(tessellationControlShaderName),
                            ShaderCategory::tessellationControl));
                    shaderModulePointers.emplace_back(std::make_shared<ShaderModule>(
                            m_shaderBuilder.getShaderModulePointer(tessellationEvaluationShaderName),
                            ShaderCategory::tessellationEvaluation));
                }

                const bool geomEnabled = object.HasMember("geometryShader");
                if (geomEnabled) {
                    const std::string geometryShaderName = object["geometryShader"].GetString();
                    shaderModulePointers.emplace_back(
                            std::make_shared<ShaderModule>(m_shaderBuilder.getShaderModulePointer(geometryShaderName),
                                                           ShaderCategory::geometry));
                }

                auto textureNames = object["texture"].GetArray();

                std::vector<std::shared_ptr<Image>> imagePointers{};
                std::vector<std::shared_ptr<Sampler>> samplerPointers{};
                std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{globalDescriptorSetPointer};
                std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{globalDescriptorSetLayoutPointer};

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


                auto m_material = std::make_shared<Material>(id, shaderModulePointers, descriptorSetLayoutPointers, descriptorSetPointers,
                                    imagePointers, samplerPointers, bufferPointers, pipelineCategoryString);

                m_materials.emplace(id, std::move(m_material));
            }
        }
#endif
    }

    void Application::Awake() {
        ComponentAwakeInfo awakeInfo{};
        for (auto &pair: gameObjects) {
            awakeInfo.gameObject = &pair.second;
            pair.second.Awake(awakeInfo);
        }
    }

    void Application::UpdateComponents(FrameInfo &frameInfo) {
        ComponentUpdateInfo updateInfo{};
        RendererInfo rendererInfo{renderer.getAspectRatio()};
        updateInfo.frameInfo = &frameInfo;
        updateInfo.rendererInfo = &rendererInfo;

        static bool firstFrame = true;
        if (firstFrame) {
            for (auto &pair: gameObjects) {
                updateInfo.gameObject = &pair.second;
                pair.second.Start(updateInfo);
            }
            firstFrame = false;
        }

        for (auto &pair: gameObjects) {
            updateInfo.gameObject = &pair.second;
            pair.second.Update(updateInfo);
        }

        for (auto &pair: gameObjects) {
            updateInfo.gameObject = &pair.second;
            pair.second.LateUpdate(updateInfo);
        }
    }

    void Application::UpdateUbo(GlobalUbo &ubo, float totalTime) noexcept {
        ubo.lightNum = LightComponent::lightNum;
        ubo.curTime = totalTime;
#ifndef RAY_TRACING
        ubo.shadowViewMatrix[0] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(0, 90, 180));
        ubo.shadowViewMatrix[1] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(0, -90, 180));
        ubo.shadowViewMatrix[2] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(-90, 0, 0));
        ubo.shadowViewMatrix[3] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(90, 0, 0));
        ubo.shadowViewMatrix[4] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(180, 0, 0));
        ubo.shadowViewMatrix[5] = m_shadowSystem->calculateViewMatrixForRotation(ubo.lights[0].position, glm::vec3(0, 0, 180));
        ubo.shadowProjMatrix = CameraComponent::CorrectionMatrix * glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 5.0f);
        ubo.lightProjectionViewMatrix = ubo.shadowProjMatrix * ubo.shadowViewMatrix[0];
#endif
    }

    void Application::createRenderSystems() {

        for (auto &material: m_materials) {
            auto pipelineCategory = material.second->getPipelineCategory();
#ifdef RAY_TRACING
            if (pipelineCategory == PipelineCategory.RayTracing) {
                m_rayTracingSystem = std::make_shared<RayTracingSystem>(device, renderer.getOffscreenRenderPass(), material.second);
                m_rayTracingSystem->Init();
            } else
#else
                if (pipelineCategory == PipelineCategory.Shadow) {
                    m_shadowSystem = std::make_shared<ShadowSystem>(device, renderer.getShadowRenderPass(), material.second);
                    continue;
                } else
#endif
            if (pipelineCategory == PipelineCategory.TessellationGeometry) {
                auto renderSystem = std::make_shared<GrassSystem>(device, renderer.getSwapChainRenderPass(), material.second);
                renderSystem->Init();
                m_renderSystems.push_back(std::dynamic_pointer_cast<RenderSystem>(renderSystem));
                continue;
            } else if (pipelineCategory == PipelineCategory.SkyBox) {
                auto renderSystem = std::make_shared<SkyBoxSystem>(device, renderer.getSwapChainRenderPass(), material.second);
                renderSystem->Init();
                m_renderSystems.push_back(std::dynamic_pointer_cast<RenderSystem>(renderSystem));
                continue;
            } else if (pipelineCategory == PipelineCategory.Opaque) {
                auto renderSystem = std::make_shared<RenderSystem>(device, renderer.getSwapChainRenderPass(), material.second);
                renderSystem->Init();
                m_renderSystems.push_back(renderSystem);
            } else if (pipelineCategory == PipelineCategory.Post) {
                m_postSystem = std::make_shared<PostSystem>(device, renderer.getSwapChainRenderPass(), material.second);
                m_postSystem->Init();
            }
        }
    }


}