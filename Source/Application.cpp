#include "Application.hpp"
#include <numeric>
#include <rapidjson/document.h>
#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>

namespace Kaamoo {
    Application::~Application() {
        GUI::Destroy();
        m_gameObjects.clear();
        m_materials.clear();
    }

    Application::Application() : m_shaderBuilder(m_device) {
        m_globalPool = DescriptorPool::Builder(m_device).
                setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).
                addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).
                addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).
                addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * MATERIAL_NUMBER).build();
        loadGameObjects();
        loadMaterials();
        createRenderSystems();
        GUI::Init(m_renderer, m_window);
    }

    void Application::run() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float totalTime = 0;
        GlobalUbo ubo{};
        Awake();
        while (!m_window.shouldClose()) {
            glfwPollEvents();
            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            totalTime += frameTime;
            currentTime = newTime;

            if (auto commandBuffer = m_renderer.beginFrame()) {
                int frameIndex = m_renderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, m_gameObjects, m_materials, ubo, m_window.getCurrentExtent(), GUI::GetSelectedId(), false};
                FixedUpdateComponents(frameInfo);
                UpdateComponents(frameInfo);
                UpdateUbo(ubo, totalTime);

#ifdef RAY_TRACING
                GUI::BeginFrame(ImVec2(m_window.getCurrentExtent().width, m_window.getCurrentExtent().height));
                GUI::ShowWindow(ImVec2(m_window.getCurrentExtent().width, m_window.getCurrentExtent().height),
                                &m_gameObjects, &m_pGameObjectDescs, &m_hierarchyTree, frameInfo);
                m_pGameObjectDescBuffer->writeToBuffer(m_pGameObjectDescs.data(), m_pGameObjectDescs.size() * sizeof(GameObjectDesc));

                m_rayTracingSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                m_rayTracingSystem->rayTrace(frameInfo);

                m_renderer.setDenoiseRtxToComputeSynchronization(commandBuffer, frameIndex % 2);

                m_computeSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                m_computeSystem->render(frameInfo);

                m_renderer.setDenoiseComputeToPostSynchronization(commandBuffer, frameIndex % 2);

                m_renderer.beginSwapChainRenderPass(commandBuffer);

                m_postSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                m_postSystem->render(frameInfo);

#else
                m_renderer.beginShadowRenderPass(commandBuffer);
                m_shadowSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                m_shadowSystem->renderShadow(frameInfo);
                m_renderer.endShadowRenderPass(commandBuffer);

                m_renderer.setShadowMapSynchronization(commandBuffer);

                m_renderer.beginSwapChainRenderPass(commandBuffer);

                for (const auto &item: m_renderSystems) {
                    item->UpdateGlobalUboBuffer(ubo, frameIndex);
                    item->render(frameInfo);
                }

                GUI::BeginFrame(ImVec2(m_window.getCurrentExtent().width, m_window.getCurrentExtent().height));
                GUI::ShowWindow(ImVec2(m_window.getCurrentExtent().width, m_window.getCurrentExtent().height),
                                &m_gameObjects, &m_materials, &m_hierarchyTree, frameInfo);
#endif
                GUI::EndFrame(commandBuffer);
                m_renderer.endSwapChainRenderPass(commandBuffer);

                m_renderer.beginGizmosRenderPass(commandBuffer);
                m_gizmosRenderSystem->UpdateGlobalUboBuffer(ubo, frameIndex);
                m_gizmosRenderSystem->render(frameInfo, GizmosType::EdgeDetectionStencil);
                m_gizmosRenderSystem->render(frameInfo, GizmosType::EdgeDetection);

                m_gizmosRenderSystem->render(frameInfo, GizmosType::Axis);
                m_renderer.endGizmosRenderPass(commandBuffer);

                m_renderer.endFrame();
            }

        }
        vkDeviceWaitIdle(m_device.device());
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

        std::unordered_map<int, GameObject *> transformIdToParentGameObjMap;
        auto *componentFactory = new ComponentFactory();
        if (gameObjectsDocument.IsArray()) {
            for (rapidjson::SizeType i = 0; i < gameObjectsDocument.Size(); i++) {
                auto &gameObject = GameObject::createGameObject();
                const rapidjson::Value &object = gameObjectsDocument[i];

                const rapidjson::Value &transformJsonObj = object["transform"];

                int32_t transformId = HierarchyTree::DEFAULT_TRANSFORM_ID;
                if (transformJsonObj.HasMember("id")) {
                    transformId = transformJsonObj["id"].GetInt();
                }

                gameObject.transform->SetTransformId(transformId);

                if (transformJsonObj.HasMember("childrenIds")) {
                    const rapidjson::Value &childrenIdsArray = transformJsonObj["childrenIds"];
                    for (rapidjson::SizeType j = 0; j < childrenIdsArray.Size(); j++) {
                        const rapidjson::Value &arrayId = childrenIdsArray[j];
                        const int childrenId = arrayId.GetInt();
                        if (transformId != -1) {
                            transformIdToParentGameObjMap[childrenId] = &gameObject;
                        }
                    }
                }

                const rapidjson::Value &translationArray = transformJsonObj["translation"];
                glm::vec3 translation{translationArray[0].GetFloat(), translationArray[1].GetFloat(),
                                      translationArray[2].GetFloat()};

                const rapidjson::Value &scaleArray = transformJsonObj["scale"];
                glm::vec3 scale{scaleArray[0].GetFloat(), scaleArray[1].GetFloat(), scaleArray[2].GetFloat()};

                const rapidjson::Value &rotationArray = transformJsonObj["rotation"];
                glm::vec3 rotation{rotationArray[0].GetFloat(), rotationArray[1].GetFloat(),
                                   rotationArray[2].GetFloat()};


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
                gameObject.transform->SetTranslation(translation);
                gameObject.transform->SetScale(scale);
                gameObject.transform->SetRotation(glm::radians(rotation));

                m_gameObjects.emplace(gameObject.getId(), std::move(gameObject));
            }
        }
        
        for (auto &pair: m_gameObjects) {
            auto &gameObject = pair.second;
            if (transformIdToParentGameObjMap.find(gameObject.transform->GetTransformId()) != transformIdToParentGameObjMap.end()) {
                auto parent = transformIdToParentGameObjMap[gameObject.transform->GetTransformId()];
                parent->transform->AddChild(gameObject.transform);
                //Make sure parent node exists in the hierarchy tree before inserting child node.
                m_hierarchyTree.AddNode(parent->getId(), gameObject.getId(), &gameObject);
            } else {
                m_hierarchyTree.AddNode(HierarchyTree::ROOT_ID, gameObject.getId(), &gameObject);
            }
            gameObject.OnLoad();
        }

        for (auto &pair: m_gameObjects) {
            auto &gameObject = pair.second;
            gameObject.Loaded();
        }
        
#ifdef RAY_TRACING
        BLAS::buildBLAS(VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
#endif
    }

    void Application::loadMaterials() {

        uint32_t minUniformOffsetAlignment = std::lcm(m_device.properties.limits.minUniformBufferOffsetAlignment,
                                                      m_device.properties.limits.nonCoherentAtomSize);
        std::string materialsString = JsonUtils::ReadJsonFile(BasePath + "Materials.json");
        rapidjson::Document materialsDocument;
        materialsDocument.Parse(materialsString.c_str());

        auto globalUboBufferPtr = std::make_shared<Buffer>(
                m_device, sizeof(GlobalUbo), SwapChain::MAX_FRAMES_IN_FLIGHT,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minUniformOffsetAlignment);
        globalUboBufferPtr->map();

#ifdef RAY_TRACING
        uint32_t minStorageBufferOffsetAlignment = m_device.properties2.properties.limits.minStorageBufferOffsetAlignment;
        {
            std::vector<std::shared_ptr<ShaderModule>> shaderModulePointers{};
            std::vector<std::shared_ptr<Image>> imagePointers{m_renderer.getOffscreenImageColor(0), m_renderer.getOffscreenImageColor(1)};
            std::vector<std::shared_ptr<Sampler>> samplerPointers{};
            std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{};
            std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{};
            std::vector<std::shared_ptr<Buffer>> bufferPointers{globalUboBufferPtr};
            std::vector<VkDescriptorImageInfo> imageInfos;


            //Generate Shader
            const std::string rayGenShaderPath = "RayTracing/raytrace.rgen.spv";
            shaderModulePointers.push_back(std::make_shared<ShaderModule>(m_shaderBuilder.createShaderModule(rayGenShaderPath), ShaderCategory::rayGen));

            //Miss Shader
            const std::string rayMissShaderPath = "RayTracing/raytrace.rmiss.spv";
            shaderModulePointers.push_back(std::make_shared<ShaderModule>(m_shaderBuilder.createShaderModule(rayMissShaderPath), ShaderCategory::rayMiss));
            const std::string rayMiss2ShaderPath = "RayTracing/raytraceShadow.rmiss.spv";
            shaderModulePointers.push_back(std::make_shared<ShaderModule>(m_shaderBuilder.createShaderModule(rayMiss2ShaderPath), ShaderCategory::rayMiss2));

            //Load materials
            std::unordered_map<int, glm::vec2> textureEntries{};
            std::unordered_map<int, PBR> pbrMaterials{};
            std::unordered_map<int, int> idShaderOffsetMap{};
            int shaderGroupOffset = 0;
            if (materialsDocument.IsArray()) {
                for (rapidjson::SizeType i = 0; i < materialsDocument.Size(); i++) {
                    const rapidjson::Value &object = materialsDocument[i];
                    const int id = object["id"].GetInt();
                    const std::string rayClosestShaderName = object["rayClosestHitShader"].GetString();
                    shaderModulePointers.push_back(std::make_shared<ShaderModule>(m_shaderBuilder.createShaderModule(rayClosestShaderName), ShaderCategory::rayClosestHit));
                    if (object.HasMember("rayAnyHitShader")) {
                        const std::string rayAnyHitShaderName = object["rayAnyHitShader"].GetString();
                        shaderModulePointers.push_back(std::make_shared<ShaderModule>(m_shaderBuilder.createShaderModule(rayAnyHitShaderName), ShaderCategory::rayAnyHit));
                    }

                    idShaderOffsetMap.emplace(id, shaderGroupOffset);
                    shaderGroupOffset++;

                    auto textureNames = object["texture"].GetArray();
                    glm::i32vec2 textureEntry{};
                    textureEntry.x = imageInfos.size();
                    for (auto &textureNameGenericValue: textureNames) {
                        std::string textureName = textureNameGenericValue.GetString();
                        auto image = std::make_shared<Image>(m_device, ImageType.Default);
                        image->createTextureImage(BaseTexturePath + textureName);
                        image->createImageView();
                        auto sampler = std::make_shared<Sampler>(m_device);
                        sampler->createTextureSampler();
                        auto imageInfo = image->descriptorInfo(*sampler);
                        imagePointers.emplace_back(image);
                        samplerPointers.emplace_back(sampler);
                        imageInfos.emplace_back(*imageInfo);
                    }
                    textureEntry.y = imageInfos.size() - textureEntry.x;
                    if (object.HasMember("PBR")) {
                        auto pbr = PBRLoader::loadPBR(object["PBR"]);
                        if (PBRParametersCount - PBRLoader::getValidPropertyCount(pbr) != textureEntry.y) {
                            throw std::runtime_error("Texture count does not match with PBR properties");
                        }
                        pbrMaterials.emplace(id, pbr);
                    }
                    textureEntries.emplace(id, textureEntry);
                }
            }
            for (auto &gameObjectPair: m_gameObjects) {
                auto &gameObject = gameObjectPair.second;
                MeshRendererComponent *meshRendererComponent;
                if (gameObject.TryGetComponent(meshRendererComponent)) {
                    auto model = meshRendererComponent->GetModelPtr();
                    TLAS::createTLAS(*model, meshRendererComponent->GetTLASId(), idShaderOffsetMap[meshRendererComponent->GetMaterialID()], gameObject.transform->mat4());
                }
            }
            TLAS::buildTLAS();

            //TLAS, offscreen, GBuffer
            auto rayGenDescriptorSetLayoutPtr = DescriptorSetLayout::Builder(m_device).
                    addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR).
                    addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 2).
                    addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 2).
                    build();
            auto rayGenDescriptorSet = std::make_shared<VkDescriptorSet>();
            descriptorSetLayoutPointers.push_back(rayGenDescriptorSetLayoutPtr);
            descriptorSetPointers.push_back(rayGenDescriptorSet);

            auto accelerationStructureInfo = std::make_shared<VkWriteDescriptorSetAccelerationStructureKHR>();
            accelerationStructureInfo->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            accelerationStructureInfo->accelerationStructureCount = 1;
            accelerationStructureInfo->pAccelerationStructures = &TLAS::tlas;

            std::vector<VkDescriptorImageInfo> offscreenImageInfos;
            auto offScreenImageInfo = std::make_shared<VkDescriptorImageInfo>();
            offScreenImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            offScreenImageInfo->imageView = m_renderer.getOffscreenImageColor(0)->imageView;
            offscreenImageInfos.emplace_back(*offScreenImageInfo);
            offScreenImageInfo->imageView = m_renderer.getOffscreenImageColor(1)->imageView;
            offscreenImageInfos.emplace_back(*offScreenImageInfo);

            std::vector<VkDescriptorImageInfo> worldPosImageInfos{};
            auto worldPosImageInfo = m_renderer.getWorldPosImageColor(0)->descriptorInfo();
            worldPosImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            worldPosImageInfos.emplace_back(*worldPosImageInfo);
            worldPosImageInfo = m_renderer.getWorldPosImageColor(1)->descriptorInfo();
            worldPosImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            worldPosImageInfos.emplace_back(*worldPosImageInfo);

            DescriptorWriter(rayGenDescriptorSetLayoutPtr, *m_globalPool).
                    writeTLAS(0, accelerationStructureInfo).
                    writeImages(1, offscreenImageInfos).
                    writeImages(2, worldPosImageInfos).
                    build(rayGenDescriptorSet);

            //ObjectDesc
            int meshRendererCount = 0;
            for (auto &modelPair: m_gameObjects) {
                auto &gameObject = modelPair.second;
                MeshRendererComponent *meshRendererComponent;
                if (gameObject.TryGetComponent(meshRendererComponent)) {
                    meshRendererCount++;
                }
            }
            m_pGameObjectDescs.resize(meshRendererCount);
            for (auto &modelPair: m_gameObjects) {
                GameObjectDesc modelDesc{};
                auto &gameObject = modelPair.second;
                MeshRendererComponent *meshRendererComponent;
                if (gameObject.TryGetComponent(meshRendererComponent)) {
                    modelDesc.vertexBufferAddress = meshRendererComponent->GetModelPtr()->getVertexBuffer()->getDeviceAddress();
                    modelDesc.indexBufferAddress = meshRendererComponent->GetModelPtr()->getIndexBuffer()->getDeviceAddress();
                    auto entry = textureEntries.find(meshRendererComponent->GetMaterialID());
                    if (entry != textureEntries.end()) {
                        modelDesc.textureEntry = entry->second;
                    }
                    auto pbrEntry = pbrMaterials.find(meshRendererComponent->GetMaterialID());
                    if (pbrEntry != pbrMaterials.end()) {
                        modelDesc.pbr = pbrEntry->second;
                    }
                    m_pGameObjectDescs[meshRendererComponent->GetTLASId()] = modelDesc;
                }
            }

            m_pGameObjectDescBuffer = std::make_shared<Buffer>(m_device, sizeof(GameObjectDesc), m_pGameObjectDescs.size(),
                                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, minStorageBufferOffsetAlignment);
            m_pGameObjectDescBuffer->map(m_pGameObjectDescBuffer->getBufferSize());
            m_pGameObjectDescBuffer->writeToBuffer(m_pGameObjectDescs.data(), m_pGameObjectDescs.size() * sizeof(GameObjectDesc));
            bufferPointers.push_back(m_pGameObjectDescBuffer);

            //Skybox cube map
            auto skyBoxImage = std::make_shared<Image>(m_device, ImageType.CubeMap);
            skyBoxImage->createTextureImage(BaseTexturePath + SkyboxCubeMapName, true);
            skyBoxImage->createImageView();
            auto skyBoxSampler = std::make_shared<Sampler>(m_device);
            skyBoxSampler->createTextureSampler();
            imagePointers.emplace_back(skyBoxImage);
            samplerPointers.emplace_back(skyBoxSampler);

            auto sceneDescriptorSetLayoutPtr = DescriptorSetLayout::Builder(m_device).
                    addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                               VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR).
                    addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR).
                    addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, imageInfos.size()).
                    addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_MISS_BIT_KHR).
                    build();
            descriptorSetLayoutPointers.push_back(sceneDescriptorSetLayoutPtr);

            auto sceneDescriptorSet = std::make_shared<VkDescriptorSet>();
            DescriptorWriter(sceneDescriptorSetLayoutPtr, *m_globalPool).
                    writeBuffer(0, globalUboBufferPtr->descriptorInfo(globalUboBufferPtr->getBufferSize())).
                    writeBuffer(1, m_pGameObjectDescBuffer->descriptorInfo(m_pGameObjectDescBuffer->getBufferSize())).
                    writeImages(2, imageInfos).
                    writeImage(3, skyBoxImage->descriptorInfo(*skyBoxSampler)).
                    build(sceneDescriptorSet);
            descriptorSetPointers.push_back(sceneDescriptorSet);
            auto material = std::make_shared<Material>(Material::MaterialId::rayTracing, shaderModulePointers, descriptorSetLayoutPointers, descriptorSetPointers,
                                                       imagePointers, samplerPointers, bufferPointers, "RayTracing");
            m_materials.emplace(Material::MaterialId::rayTracing, std::move(material));

        }

        //Post
        {
            auto postSystemDescriptorSetLayoutPtr =
                    DescriptorSetLayout::Builder(m_device).
                            addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT).
                            addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2).
                            build();


            std::vector<VkDescriptorImageInfo> offscreenImageInfos{};
            auto offScreenPostImageInfo = m_renderer.getOffscreenImageColor(0)->descriptorInfo();
            offScreenPostImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            offscreenImageInfos.emplace_back(*offScreenPostImageInfo);
            offScreenPostImageInfo = m_renderer.getOffscreenImageColor(1)->descriptorInfo();
            offScreenPostImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            offscreenImageInfos.emplace_back(*offScreenPostImageInfo);

            auto postSystemDescriptorSet = std::make_shared<VkDescriptorSet>();
            auto postDescriptorSet = std::make_shared<VkDescriptorSet>();
            DescriptorWriter(postSystemDescriptorSetLayoutPtr, *m_globalPool).
                    writeBuffer(0, globalUboBufferPtr->descriptorInfo()).
                    writeImages(1, offscreenImageInfos).
                    build(postDescriptorSet);

            std::vector<std::shared_ptr<ShaderModule>> shaderModulePointers{
                    std::make_shared<ShaderModule>(m_shaderBuilder.createShaderModule(PostVertexShaderName), ShaderCategory::vertex),
                    std::make_shared<ShaderModule>(m_shaderBuilder.createShaderModule(PostFragmentShaderName), ShaderCategory::fragment),
            };
            std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{postSystemDescriptorSetLayoutPtr};
            std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{postDescriptorSet};
            std::vector<std::shared_ptr<Image>> imagePointers{};
            std::vector<std::shared_ptr<Sampler>> samplerPointers{};
            std::vector<std::shared_ptr<Buffer>> bufferPointers{globalUboBufferPtr};

            auto postMaterial = std::make_shared<Material>(Material::MaterialId::post, shaderModulePointers, descriptorSetLayoutPointers, descriptorSetPointers, imagePointers, samplerPointers,
                                                           bufferPointers,
                                                           PipelineCategory.Post);
            m_materials.emplace(Material::MaterialId::post, std::move(postMaterial));
        }

        //Compute
        {
            auto computeSystemDescriptorSetLayoutPtr =
                    DescriptorSetLayout::Builder(m_device).
                            addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT).
                            addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2).
                            addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2).
                            addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT).
                            addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2).
                            build();

            std::vector<VkDescriptorImageInfo> offscreenImageInfos{};
            auto offScreenPostImageInfo = m_renderer.getOffscreenImageColor(0)->descriptorInfo();
            offScreenPostImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            offscreenImageInfos.emplace_back(*offScreenPostImageInfo);
            offScreenPostImageInfo = m_renderer.getOffscreenImageColor(1)->descriptorInfo();
            offScreenPostImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            offscreenImageInfos.emplace_back(*offScreenPostImageInfo);

            std::vector<VkDescriptorImageInfo> worldPosImageInfos{};
            auto worldPosImageInfo = m_renderer.getWorldPosImageColor(0)->descriptorInfo();
            worldPosImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            worldPosImageInfos.emplace_back(*worldPosImageInfo);
            worldPosImageInfo = m_renderer.getWorldPosImageColor(1)->descriptorInfo();
            worldPosImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            worldPosImageInfos.emplace_back(*worldPosImageInfo);

            auto denoisingImageInfo = m_renderer.getDenoisingAccumulationImageColor()->descriptorInfo();
            denoisingImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            std::vector<VkDescriptorImageInfo> viewPosImageInfos{};
            auto viewPosImageInfo = m_renderer.getViewPosImageColor(0)->descriptorInfo();
            viewPosImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            viewPosImageInfos.emplace_back(*viewPosImageInfo);
            viewPosImageInfo = m_renderer.getViewPosImageColor(1)->descriptorInfo();
            viewPosImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            viewPosImageInfos.emplace_back(*viewPosImageInfo);

            auto postDescriptorSet = std::make_shared<VkDescriptorSet>();
            DescriptorWriter(computeSystemDescriptorSetLayoutPtr, *m_globalPool).
                    writeBuffer(0, globalUboBufferPtr->descriptorInfo()).
                    writeImages(1, offscreenImageInfos).
                    writeImages(2, worldPosImageInfos).
                    writeImage(3, denoisingImageInfo).
                    writeImages(4, viewPosImageInfos).
                    build(postDescriptorSet);

            std::vector<std::shared_ptr<ShaderModule>> shaderModulePointers{
                    std::make_shared<ShaderModule>(m_shaderBuilder.createShaderModule(RayTracingDenoiseComputeShaderName), ShaderCategory::compute),
            };
            std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{computeSystemDescriptorSetLayoutPtr};
            std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{postDescriptorSet};
            std::vector<std::shared_ptr<Image>> imagePointers{};
            std::vector<std::shared_ptr<Sampler>> samplerPointers{};
            std::vector<std::shared_ptr<Buffer>> bufferPointers{globalUboBufferPtr};

            auto computeMaterial = std::make_shared<Material>(Material::MaterialId::compute, shaderModulePointers, descriptorSetLayoutPointers, descriptorSetPointers, imagePointers, samplerPointers,
                                                              bufferPointers,
                                                              PipelineCategory.Compute);
            m_materials.emplace(Material::MaterialId::compute, std::move(computeMaterial));
        }
#else
        auto bufferInfo = globalUboBufferPtr->descriptorInfo();

        auto globalDescriptorSetLayoutPointer = DescriptorSetLayout::Builder(m_device).
                addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS).
                addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS).
                build();

        std::shared_ptr<VkDescriptorSet> globalDescriptorSetPointer = std::make_shared<VkDescriptorSet>();
        DescriptorWriter(globalDescriptorSetLayoutPointer, *m_globalPool).
                writeBuffer(0, bufferInfo).
                writeImage(1, m_renderer.getShadowImageInfo()).
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
                DescriptorSetLayout::Builder descriptorSetLayoutBuilder(m_device);
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

                DescriptorWriter descriptorWriter(materialDescriptorSetLayoutPointer, *m_globalPool);

                //Write Descriptors
                int writerBindingPoint = 0;
                std::shared_ptr<Image> image;
                std::vector<std::shared_ptr<VkDescriptorImageInfo>> imageInfos;
                for (auto &textureNameGenericValue: textureNames) {
                    std::string textureName = textureNameGenericValue.GetString();

                    if (pipelineCategoryString == PipelineCategory.SkyBox)
                        image = std::make_shared<Image>(m_device, ImageType.CubeMap);
                    else
                        image = std::make_shared<Image>(m_device, ImageType.Default);

                    image->createTextureImage(BaseTexturePath + textureName);
                    image->createImageView();
                    auto sampler = std::make_shared<Sampler>(m_device);
                    sampler->createTextureSampler();
                    auto imageInfo = image->descriptorInfo(*sampler);
                    imageInfos.emplace_back(imageInfo);
                    descriptorWriter.writeImage(writerBindingPoint++, imageInfo);
                    imagePointers.emplace_back(image);
                    samplerPointers.emplace_back(sampler);
                }

                std::vector<std::shared_ptr<Buffer>> bufferPointers{globalUboBufferPtr};
                if (pipelineCategoryString == "Shadow") {
                    auto shadowUboBuffer = std::make_shared<Buffer>(m_device, sizeof(ShadowUbo),
                                                                    SwapChain::MAX_FRAMES_IN_FLIGHT,
                                                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                                    minUniformOffsetAlignment);
                    bufferPointers.push_back(shadowUboBuffer);
                    auto shadowUboBufferInfo = shadowUboBuffer->descriptorInfo(sizeof(ShadowUbo) * SwapChain::MAX_FRAMES_IN_FLIGHT);
                    descriptorWriter.writeBuffer(writerBindingPoint++, shadowUboBufferInfo);
                } else if (pipelineCategoryString == PipelineCategory.Overlay ||
                           pipelineCategoryString == PipelineCategory.Opaque ||
                           pipelineCategoryString == PipelineCategory.TessellationGeometry ||
                           pipelineCategoryString == PipelineCategory.SkyBox) {
                    imagePointers.push_back(m_renderer.getShadowImage());
                    samplerPointers.push_back(m_renderer.getShadowSampler());
                    auto imageInfo = m_renderer.getShadowImageInfo();
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
        //Gizmos
        {
            auto uiDescriptorSetLayoutPtr = DescriptorSetLayout::Builder(m_device).
                    addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS).
                    build();

            auto uiDescriptorSet = std::make_shared<VkDescriptorSet>();
            DescriptorWriter(uiDescriptorSetLayoutPtr, *m_globalPool).
                    writeBuffer(0, globalUboBufferPtr->descriptorInfo(globalUboBufferPtr->getBufferSize())).
                    build(uiDescriptorSet);

            std::vector<std::shared_ptr<ShaderModule>> shaderModulePointers{};
            std::vector<std::shared_ptr<DescriptorSetLayout>> descriptorSetLayoutPointers{uiDescriptorSetLayoutPtr};
            std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSetPointers{uiDescriptorSet};
            std::vector<std::shared_ptr<Image>> imagePointers{};
            std::vector<std::shared_ptr<Sampler>> samplerPointers{};
            std::vector<std::shared_ptr<Buffer>> bufferPointers{globalUboBufferPtr};

            auto uiMaterial = std::make_shared<Material>(Material::MaterialId::gizmos, shaderModulePointers, descriptorSetLayoutPointers, descriptorSetPointers, imagePointers, samplerPointers,
                                                         bufferPointers,
                                                         PipelineCategory.Gizmos);
            m_materials.emplace(Material::MaterialId::gizmos, std::move(uiMaterial));
        }
    }

    void Application::Awake() {
        ComponentAwakeInfo awakeInfo{};
        for (auto &pair: m_gameObjects) {
            awakeInfo.gameObject = &pair.second;
            pair.second.Awake(awakeInfo);
        }
    }


}