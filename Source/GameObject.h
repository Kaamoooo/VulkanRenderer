#pragma once

#include <memory>
#include "Model.hpp"
#include "Material.h"

#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <utility>

namespace Kaamoo {
    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::vec3 rotation;

        //根据 平移 缩放 旋转 构造model to world的变换矩阵
        glm::mat4 mat4() const;

        glm::mat3 normalMatrix();
    };

    struct LightComponent {
        int lightIndex = 0;
        float lightIntensity = 1.0f;
        int lightCategory = 0;
    };


    class GameObject {
    public:
        //定义一个类型别名，id即代表无符号整数
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, GameObject>;

        TransformComponent transform{};
        glm::vec3 color{};

        //可选组件
        std::shared_ptr<Model> model = nullptr;
        std::unique_ptr<LightComponent> lightComponent = nullptr;

        static GameObject createGameObject(id_t materialId = -1) {
            static id_t currentID = 0;
            return {currentID++, materialId};
        }

        static GameObject makeLight(float intensity, float radius, glm::vec3 color, int lightCategory);

        id_t getId() { return id; }

        id_t getMaterialId() const { return materialId; }

        //移动构造函数
        GameObject(GameObject &&) = default;

        GameObject &operator=(GameObject &) = delete;

        GameObject &operator=(GameObject &&) = default;

        glm::vec3 getForwardDir() const {
            float yaw = transform.rotation.y;
            return glm::vec3{glm::sin(yaw), 0, glm::cos(yaw)};
        };

        void Translate(glm::vec3 translation);
        
        [[nodiscard]] std::string getName() const { return name; }
        
        void setName(std::string name) { GameObject::name = std::move(name); }

    private:
    public:
        void setMaterialId(id_t materialId);

    private:
        id_t id;
        id_t materialId;
        
        std::string name;

//        Material& material; 
        GameObject(id_t id, id_t materialId = -1,std::string name = "GameObject") : id{id}, materialId{materialId},name{std::move(name)} {};
    };
}