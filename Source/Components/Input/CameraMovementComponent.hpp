#pragma once

//Todo: Circular include
#include "../MeshRendererComponent.hpp"
#include "InputControllerComponent.hpp"

namespace Kaamoo {
    class CameraMovementComponent : public InputControllerComponent {
    public:
        CameraMovementComponent(GLFWwindow *window) : InputControllerComponent(window) {
            name = "CameraMovementComponent";
        }

        void Update(const ComponentUpdateInfo &updateInfo) override {
            MoveCamera(updateInfo);

            FocusOnObject(updateInfo);
        }

    private:
        float m_focusMoveTime = 1;
        float m_lookSpeed{2.f};
        float moveSpeed{3.f};
        glm::vec2 m_deltaPos;


        void MoveCamera(const ComponentUpdateInfo &updateInfo) {
            glm::vec3 rotation{0};

            static glm::vec2 _lastFrameCurPos = glm::vec2{std::numeric_limits<float>::max()};
            if (rightMousePressed && glm::length(_lastFrameCurPos - curPos) > std::numeric_limits<float>::epsilon()) {
                m_deltaPos = curPos - lastPos;
                _lastFrameCurPos = curPos;
                rotation.x -= m_deltaPos.y;
                rotation.y += m_deltaPos.x;
            }

            //防止没有按下按键时，对0归一化导致错误   
            if (glm::dot(rotation, rotation) > std::numeric_limits<float>::epsilon()) {
                updateInfo.gameObject->transform->SetRotation(rotation * m_lookSpeed + updateInfo.gameObject->transform->GetRotation());
            }

            auto transformRotation = updateInfo.gameObject->transform->GetRotation();
            auto forwardDirMatrix = Utils::GetRotateDirectionMatrix(transformRotation);
            const glm::vec3 forwardDir = forwardDirMatrix * glm::vec4{0, 0, 1, 1};
            forwardDirMatrix = Utils::GetRotateDirectionMatrix({transformRotation.x + 1, transformRotation.y, transformRotation.z});
            const glm::vec3 forwardDirWithOffset = forwardDirMatrix * glm::vec4{0, 0, 1, 1};
            const glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, forwardDirWithOffset));
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4{1.f}, glm::radians(90.f), rightDir);
            const glm::vec3 upDir = rotationMatrix * glm::vec4{forwardDir, 1};

            glm::vec3 moveDir{0};
            if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
            if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
            if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
            if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
            if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
            if (glfwGetKey(window, keys.moveBack) == GLFW_PRESS) moveDir -= forwardDir;
            if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
                updateInfo.gameObject->transform->SetTranslation(glm::normalize(moveDir) * moveSpeed * updateInfo.frameInfo->frameTime + updateInfo.gameObject->transform->GetTranslation());
            }
        }

        void FocusOnObject(const ComponentUpdateInfo &updateInfo) {
            static bool _isFocusing = false;
            static glm::vec3 _targetPosition;
            static glm::vec3 _focusObjectPosition;
            if (glfwGetKey(window, keys.KEY_F) == GLFW_PRESS && !_isFocusing) {
                auto _frameInfo = *updateInfo.frameInfo;
                auto _selectedObject = _frameInfo.gameObjects.find(_frameInfo.selectedGameObjectId);
                if (_selectedObject != _frameInfo.gameObjects.end()) {
                    auto &_selectedGameObject = _selectedObject->second;
                    _focusObjectPosition = _selectedGameObject.transform->GetTranslation();
                    auto _moveTarget = _focusObjectPosition - updateInfo.gameObject->transform->GetTranslation();
                    if (glm::length(_moveTarget) < std::numeric_limits<float>::epsilon()) {
                        return;
                    }
                    float _distance = glm::length(_moveTarget);
                    auto _moveDirection = glm::normalize(_moveTarget);

                    MeshRendererComponent *_meshRendererComponent;
                    float _maxRadius = 1;
                    if (_selectedObject->second.TryGetComponent(_meshRendererComponent) && _meshRendererComponent->GetModelPtr() != nullptr) {
                        auto _selectedScale = _selectedGameObject.transform->GetScale();
                        _maxRadius = _meshRendererComponent->GetModelPtr()->GetMaxRadius() * glm::max(_selectedScale.x, glm::max(_selectedScale.y, _selectedScale.z));
                    }

                    float _keptDistance = _maxRadius / glm::tan(glm::radians(updateInfo.rendererInfo->fovY) / 2);
                    float _moveDistance = _distance - _keptDistance;
                    _targetPosition = updateInfo.gameObject->transform->GetTranslation() + _moveDirection * _moveDistance;
                    _isFocusing = true;
                }
            }
            if (_isFocusing) {
                static float _t = 0;
                _t += updateInfo.frameInfo->frameTime;
                glm::vec3 _stepTranslateTarget = (1 - _t) * updateInfo.gameObject->transform->GetTranslation() + _t * _targetPosition;
                updateInfo.gameObject->transform->SetTranslation(_stepTranslateTarget);
                glm::vec3 _translateDirection = glm::normalize(_focusObjectPosition - updateInfo.gameObject->transform->GetTranslation());
                glm::vec3 _targetRotation = Utils::VectorToRotation(_translateDirection);
                updateInfo.gameObject->transform->SetRotation(_targetRotation);
                if (glm::length(_stepTranslateTarget - _targetPosition) < 0.01) {
                    _isFocusing = false;
                    _t = 0;
                }
            }
        }
    };
}
