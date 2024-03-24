#include <string>
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <glm/detail/type_mat3x3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Component.hpp"

namespace Kaamoo {
    class TransformComponent : public Component {
    public:
        TransformComponent() {
            name = "TransformComponent";
        }
        
        glm::vec3 getForwardDir() const {
            float yaw = rotation.y;
            return glm::vec3{glm::sin(yaw), 0, glm::cos(yaw)};
        };

        void Translate(glm::vec3 t) {
            translation += t;
        }
        
        glm::mat3 normalMatrix() {
            glm::mat3 invScaleMatrix = glm::mat4{1.f};
            const glm::vec3 invScale = 1.0f / scale;
            invScaleMatrix[0][0] = invScale.x;
            invScaleMatrix[1][1] = invScale.y;
            invScaleMatrix[2][2] = invScale.z;

            auto rotationMatrix = glm::mat4(1.0f);
            rotationMatrix = glm::rotate(rotationMatrix, rotation.y, {0, 1, 0});
            rotationMatrix = glm::rotate(rotationMatrix, rotation.x, {1, 0, 0});
            rotationMatrix = glm::rotate(rotationMatrix, rotation.z, {0, 0, 1});

            return glm::mat3(rotationMatrix) * invScaleMatrix;
        }

        glm::mat4 mat4() const {

            auto transform = glm::translate(glm::mat4{1.f}, translation);

            transform = glm::rotate(transform, rotation.y, {0, 1, 0});
            transform = glm::rotate(transform, rotation.x, {1, 0, 0});
            transform = glm::rotate(transform, rotation.z, {0, 0, 1});

            transform = glm::scale(transform, scale);
            return transform;
        }

        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::vec3 rotation{};
    };
}