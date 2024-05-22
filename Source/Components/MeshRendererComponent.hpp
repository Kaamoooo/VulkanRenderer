#pragma once

#include "Component.hpp"
#include "../RayTracing/BLAS.hpp"

namespace Kaamoo {
    class MeshRendererComponent : public Component {
    public:
        MeshRendererComponent(std::shared_ptr<Model> model, id_t materialID) : model(model), materialId(materialID) {
            name = "MeshRendererComponent";
        }

        explicit MeshRendererComponent(const rapidjson::Value &object) {
            if (!object.HasMember("model")) {
                this->model = nullptr;
            } else {
                const std::string modelName = object["model"].GetString();
                std::shared_ptr<Model> modelFromFile = Model::createModelFromFile(*Device::getDeviceSingleton(),
                                                                                  Model::BaseModelsPath + modelName);
                this->model = modelFromFile;
                BLAS::modelToBLASInput(model);
            }
            this->materialId = object["materialId"].GetInt();
            name = "MeshRendererComponent";
        }

        id_t GetMaterialID() const { return materialId; }

        std::shared_ptr<Model> GetModelPtr() { return model; }

        void Update(const ComponentUpdateInfo &updateInfo) override {
            if (model == nullptr) {
                return;
            }
            if(lastTransform!=updateInfo.gameObject->transform->mat4()){
                lastTransform = updateInfo.gameObject->transform->mat4();
                //TODO: update the object which is transformed
                
                BLAS::modelToBLASInput(model);
//                TLAS::(, lastTransform)
            }
        };


    private:
        id_t materialId;
        std::shared_ptr<Model> model = nullptr;
        glm::mat4 lastTransform = glm::mat4{-10000000.f};
    };
}