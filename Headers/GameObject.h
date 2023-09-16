#pragma once

#include <memory>
#include "Model.hpp"

namespace Kaamoo {
    struct Transform2dComponent {
        glm::vec2 translation{};
        glm::vec2 scale{1.f, 1.f};
        float rotation;

        glm::mat2 mat2() {
            const float s = glm::sin(rotation);
            const float c = glm::cos(rotation);

            //glm的矩阵是列主序！！
            glm::mat2 rotationMatrix = {
                    {c, s}, {-s, c}
            };
            glm::mat2 scaleMatrix = {{scale.x, 0},
                                     {0,scale.y}};
            return rotationMatrix*scaleMatrix;
        }
    };


    class GameObject {
    public:
        //定义一个类型别名，id即代表无符号整数
        using id_t = unsigned int;
        std::shared_ptr<Model> model;
        glm::vec3 color;
        Transform2dComponent transform2d;

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