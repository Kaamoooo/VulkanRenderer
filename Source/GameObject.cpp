#include "GameObject.h"

namespace Kaamoo {

    void GameObject::setIterationTimes(int times) {
        iteration = times;
    }

    GameObject GameObject::makePointLight(float intensity, float radius, glm::vec3 color) {
        GameObject gameObject = GameObject::createGameObject();
        gameObject.color = color;
        gameObject.transform.scale.x = radius;
        gameObject.pointLightComponent = std::make_unique<PointLightComponent>();
        gameObject.pointLightComponent->lightIntensity = intensity;
        return gameObject;
    }

    glm::mat4 TransformComponent::mat4() {

        auto transform = glm::translate(glm::mat4{1.f}, translation);

        transform = glm::rotate(transform, rotation.y, {0, 1, 0});
        transform = glm::rotate(transform, rotation.x, {1, 0, 0});
        transform = glm::rotate(transform, rotation.z, {0, 0, 1});

        transform = glm::scale(transform, scale);
        return transform;
    }

    glm::mat3 TransformComponent::normalMatrix() {
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

}
