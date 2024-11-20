#ifndef TRANSFORM_COMPONENT_INCLUDED
#define TRANSFORM_COMPONENT_INCLUDED

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
        
        void Rotate(glm::vec3 r,glm::vec3 rotateCenter = glm::vec3(0.f)) {
            rotation += r;
        }

        void AddChild(TransformComponent *child) {
            childrenNodes.push_back(child);
            child->parentNode = this;
        }

        glm::mat3 normalMatrix() {
            glm::mat3 invScaleMatrix = glm::mat4{1.f};
            const glm::vec3 invScale = 1.0f / scale;
            invScaleMatrix[0][0] = invScale.x;
            invScaleMatrix[1][1] = invScale.y;
            invScaleMatrix[2][2] = invScale.z;

            auto rotationMatrix = GetRotationMatrix();

            return rotationMatrix * invScaleMatrix;
        }

        glm::mat4 mat4() const {

            auto transform = glm::translate(glm::mat4{1.f}, GetTranslation());

            auto worldRotation = GetRotation();
            transform = glm::rotate(transform, worldRotation.y, {0, 1, 0});
            transform = glm::rotate(transform, worldRotation.x, {1, 0, 0});
            transform = glm::rotate(transform, worldRotation.z, {0, 0, 1});

            transform = glm::scale(transform, GetScale());
            return transform;
        }

        void SetTransformId(int32_t id) {
            transformId = id;
        }

        int32_t GetTransformId() const {
            return transformId;
        }

        void SetTranslation(glm::vec3 t) {
            translation = t;
        }

        glm::vec3 GetTranslation() const {
            if (parentNode != nullptr && transformId != -1)
                return translation + parentNode->GetTranslation();
            return translation;
        }
        
        glm::vec3 GetRelativeTranslation() const {
            return translation;
        }

        void SetScale(glm::vec3 s) {
            scale = s;
        }

        glm::vec3 GetScale() const {
            if (parentNode != nullptr && transformId != -1)
                return scale * parentNode->GetScale();
            return scale;
        }
        
        glm::vec3 GetRelativeScale() const {
            return scale;
        }

        void SetRotation(glm::vec3 r) {
            rotation = r;
        }

        glm::vec3 GetRotation() const {
            if (parentNode != nullptr && transformId != -1)
                return rotation + parentNode->GetRotation();
            return rotation;
        }
        
        glm::vec3 GetRelativeRotation() const {
            return rotation;
        }
        
        glm::mat3 GetRotationMatrix() const {
            auto rotationMatrix = glm::mat4(1.0f);
            rotationMatrix = glm::rotate(rotationMatrix, rotation.y, {0, 1, 0});
            rotationMatrix = glm::rotate(rotationMatrix, rotation.x, {1, 0, 0});
            rotationMatrix = glm::rotate(rotationMatrix, rotation.z, {0, 0, 1});
            return glm::mat3(rotationMatrix);
        }

    private:

        int32_t transformId = -1;
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::vec3 rotation{};
        TransformComponent *parentNode{nullptr};
        std::vector<TransformComponent *> childrenNodes{};
    };
}

#endif