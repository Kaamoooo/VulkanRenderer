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
#ifdef RAY_TRACING
            TLAS::createTLAS(*model, GetTLASId(), gameObject->transform->mat4());
#endif
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

        id_t GetTLASId() const {
            return tlasId;
        }

    private:
        id_t tlasId;
        id_t materialId;
        std::shared_ptr<Model> model = nullptr;
        glm::mat4 lastTransform = glm::mat4{-10000000.f};
    };
}