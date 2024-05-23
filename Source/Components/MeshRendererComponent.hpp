#pragma once

#include "Component.hpp"
#include "../RayTracing/BLAS.hpp"
#include "../RayTracing/TLAS.hpp"

namespace Kaamoo {
    class MeshRendererComponent : public Component {
    public:
        ~MeshRendererComponent() override {
            models.clear();
        };

        MeshRendererComponent(std::shared_ptr<Model> model, id_t materialID) : model(model), materialId(materialID) {
            name = "MeshRendererComponent";
        }

        explicit MeshRendererComponent(const rapidjson::Value &object) {
            if (!object.HasMember("model")) {
                this->model = nullptr;
            } else {
                const std::string modelName = object["model"].GetString();
                if (models.count(modelName) > 0) {
                    this->model = models.at(modelName);
                } else {
                    std::shared_ptr<Model> modelFromFile = Model::createModelFromFile(*Device::getDeviceSingleton(),
                                                                                      Model::BaseModelsPath +
                                                                                      modelName);
                    this->model = modelFromFile;
                    models.emplace(modelName, modelFromFile);
                    BLAS::modelToBLASInput(model);
                }
            }
            this->materialId = object["materialId"].GetInt();
            name = "MeshRendererComponent";
        }

        id_t GetMaterialID() const { return materialId; }

        std::shared_ptr<Model> GetModelPtr() { return model; }

        void Awake(const ComponentAwakeInfo &awakeInfo) override {
            if (model == nullptr) {
                return;
            }
            TLAS::createTLAS(*model, awakeInfo.gameObject->transform->mat4());
        }

        void Update(const ComponentUpdateInfo &updateInfo) override {
            if (model == nullptr) {
                return;
            }
            if (lastTransform != updateInfo.gameObject->transform->mat4()) {
                lastTransform = updateInfo.gameObject->transform->mat4();
//                TLAS::createTLAS(*model, lastTransform);
            }
        };


    private:
        inline static std::unordered_map<std::string, std::shared_ptr<Model>> models{};

        id_t materialId;
        std::shared_ptr<Model> model = nullptr;
        glm::mat4 lastTransform = glm::mat4{-10000000.f};
    };
}