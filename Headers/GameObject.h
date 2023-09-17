#pragma once

#include <memory>
#include "Model.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Kaamoo {
    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::vec3 rotation;

        glm::mat4 mat4() {
            auto transform = glm::translate(glm::mat4{1.f},translation);
            
            transform=glm::rotate(transform,rotation.y,{0,1,0});
            transform=glm::rotate(transform,rotation.x,{1,0,0});
            transform=glm::rotate(transform,rotation.z,{0,0,1});
            
            transform = glm::scale(transform,scale);
            return transform;
        }
    };


    class GameObject {
    public:
        //定义一个类型别名，id即代表无符号整数
        using id_t = unsigned int;
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


    private:
        id_t id;

        GameObject(id_t id) : id{id} {};
    };
}