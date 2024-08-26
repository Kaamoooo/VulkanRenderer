#pragma once

#include "Component.hpp"
#include "../RayTracing/BLAS.hpp"
#include "../RayTracing/TLAS.hpp"

namespace Kaamoo {
    class MeshRendererComponent : public Component {
    public:

        ~MeshRendererComponent() override {
            Model::models.clear();
        };

        MeshRendererComponent(std::shared_ptr<Model> model, id_t materialID) : model(model), materialId(materialID) {
            name = "MeshRendererComponent";
        }

        explicit MeshRendererComponent(const rapidjson::Value &object) {
            if (!object.HasMember("model")) {
                this->model = nullptr;
            } else {
                const std::string modelName = object["model"].GetString();
#ifdef RAY_TRACING
                std::shared_ptr<Model> modelFromFile = Model::createModelFromFile(*Device::getDeviceSingleton(), Model::BaseModelsPath + modelName);
                this->model = modelFromFile;
                this->model->SetName(modelName);
                Model::models.emplace(modelName, modelFromFile);
                static int tlasIdStatic = 0;
                tlasId = tlasIdStatic++;
                BLAS::modelToBLASInput(model);
#else
                if (Model::models.count(modelName) > 0) {
                    this->model = Model::models.at(modelName);
                } else {
                    std::shared_ptr<Model> modelFromFile = Model::createModelFromFile(*Device::getDeviceSingleton(), Model::BaseModelsPath + modelName);
                    this->model = modelFromFile;
                    Model::models.emplace(modelName, modelFromFile);
                }
                this->model->SetName(modelName);
#endif
            }
            this->materialId = object["materialId"].GetInt();
            name = "MeshRendererComponent";
        }

        id_t GetMaterialID() const { return materialId; }

        std::shared_ptr<Model> GetModelPtr() { return model; }

        void Loaded(GameObject *gameObject) override {
            if (model == nullptr) {
                return;
            }
        }

        void Update(const ComponentUpdateInfo &updateInfo) override {
            if (model == nullptr) {
                return;
            }
#ifdef RAY_TRACING
            if (lastTransform != updateInfo.gameObject->transform->mat4()) {
                lastTransform = updateInfo.gameObject->transform->mat4();
                TLAS::updateTLAS(tlasId, lastTransform);
                updateInfo.frameInfo->sceneUpdated = true;
            }
#endif
        };

#ifdef RAY_TRACING

        id_t GetTLASId() const {
            return tlasId;
        }

        void SetUI(std::vector<GameObjectDesc> *gameObjectDesc, FrameInfo &frameInfo) override {
            if (model == nullptr) {
                return;
            }
            ImGui::Text("Model:       %s", model->GetName().c_str());
            if (ImGui::TreeNode("PBR")) {
                GameObjectDesc &desc = gameObjectDesc->at(tlasId);
                auto validProperty = PBRLoader::GetValidProperty(desc.pbr);
                for (const auto &item: validProperty) {
                    switch (item) {
                        case 0:
                            ImGui::Text("Albedo:");
                            ImGui::SameLine(120);
                            ImGui::SetNextItemWidth(140);
                            if (ImGui::InputFloat3("##Albedo", &desc.pbr.albedo.x)) {
                                frameInfo.sceneUpdated = true;
                            }
                            Utils::ClampVec3(desc.pbr.albedo, 0, 1);
                            break;
                        case 2:
                            ImGui::Text("Metallic:");
                            ImGui::SameLine(120);
                            ImGui::SetNextItemWidth(140);
                            if (ImGui::InputFloat("##Metallic", &desc.pbr.metallic)) {
                                frameInfo.sceneUpdated = true;
                            }
                            Utils::ClampFloat(desc.pbr.metallic, 0, 1);
                            break;
                        case 3:
                            ImGui::Text("Roughness:");
                            ImGui::SameLine(120);
                            ImGui::SetNextItemWidth(140);
                            if (ImGui::InputFloat("##Roughness", &desc.pbr.roughness)) {
                                frameInfo.sceneUpdated = true;
                            }
                            Utils::ClampFloat(desc.pbr.roughness, 0, 1);
                            break;
                        case 4:
                            ImGui::Text("Opacity:");
                            ImGui::SameLine(120);
                            ImGui::SetNextItemWidth(140);
                            if (ImGui::InputFloat("##Opacity", &desc.pbr.opacity)) {
                                frameInfo.sceneUpdated = true;
                            }
                            Utils::ClampFloat(desc.pbr.opacity, 0, 1);
                            break;
                        case 6:
                            ImGui::Text("Emissive:");
                            ImGui::SameLine(120);
                            ImGui::SetNextItemWidth(140);
                            if (ImGui::InputFloat3("##Emissive", &desc.pbr.emissive.x)) {
                                frameInfo.sceneUpdated = true;
                            }
                            Utils::ClampVec3(desc.pbr.emissive, 0, 1);
                            break;
                        default:
                            break;
                    }
                }
                ImGui::TreePop();
            }
        }

#else

        void SetUI(Material::Map *materialMap, FrameInfo &frameInfo) override {
            if (model == nullptr) {
                return;
            }
            ImGui::Text("Model:       %s", model->GetName().c_str());
            auto material = materialMap->at(materialId);
            ImGui::Text("Category:    %s", material->getPipelineCategory().c_str());
        }

#endif

    private:
        id_t tlasId;
        id_t materialId;
        std::shared_ptr<Model> model = nullptr;
        glm::mat4 lastTransform = glm::mat4{-10000000.f};
    };
}