#pragma once

#include <memory>
#include "Model.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

namespace Kaamoo {
    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::vec3 rotation;

        //根据 平移 缩放 旋转 构造model to world的变换矩阵
        glm::mat4 mat4();
        glm::mat3  normalMatrix();
    };


    class GameObject {
    public:
        //定义一个类型别名，id即代表无符号整数
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t,GameObject>;
        
        std::shared_ptr<Model> model;
        glm::vec3 color;
        TransformComponent transform{};

        static GameObject createGameObject() {
            static id_t currentID = 0;
            return GameObject(currentID++);
        }

        id_t getId() { return id; }

        GameObject(GameObject &) = delete;

        //移动构造函数
        GameObject(GameObject &&) = default;

        GameObject &operator=(GameObject &) = delete;

        GameObject &operator=(GameObject &&) = default;

        glm::vec3 getForwardDir() const {
            float yaw = transform.rotation.y;
            return glm::vec3{glm::sin(yaw), 0, glm::cos(yaw)};
        };
        
        void setIterationTimes(int times);
        
        uint32_t getIterationTimes() const {
            return iteration;
        }

    private:
        id_t id;
        uint32_t iteration=0;

        GameObject(id_t id) : id{id} {};
    };
}