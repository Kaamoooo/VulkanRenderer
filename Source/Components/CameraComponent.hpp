#pragma once

#include "Component.hpp"

namespace Kaamoo {
    class CameraComponent : public Component {
    public:
        CameraComponent() {
            name = "CameraComponent";
        }

        void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
            projectionMatrix = glm::mat4{1.f};
            projectionMatrix[0][0] = 2.f / (right - left);
            projectionMatrix[1][1] = 2.f / (bottom - top);
            projectionMatrix[2][2] = 1.f / (far - near);

            projectionMatrix[3][0] = -(right + left) / (right - left);
            projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
            projectionMatrix[3][2] = -near / (far - near);
        }

        void setPerspectiveProjection(float fovY, float aspect, float near, float far) {
            const float tanHalfFovY = tan(fovY / 2);
            projectionMatrix = glm::mat4{0.f};
            projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovY);
            projectionMatrix[1][1] = 1.f / (tanHalfFovY);
            projectionMatrix[2][2] = far / (far - near);
            projectionMatrix[2][3] = 1.f;
            projectionMatrix[3][2] = -(far * near) / (far - near);
//            projectionMatrix = CorrectionMatrix * glm::perspective(fovY, aspect, near, far);
        }

        glm::mat4 getProjectionMatrix() const {
            return projectionMatrix;
        }

        glm::mat4 getViewMatrix() const {
            return viewMatrix;
        }

        glm::mat4 getInverseViewMatrix() const {
            return inverseViewMatrix;
        }

        glm::vec3 getPosition() const { return glm::vec3(inverseViewMatrix[3]); }

        void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
            //正交基，广告牌算法
            const glm::vec3 w{glm::normalize(direction)};
            const glm::vec3 u{glm::normalize(glm::cross(w, up))};
            const glm::vec3 v{glm::cross(w, u)};

            //旋转矩阵为正交矩阵，其转置即为逆
            viewMatrix = glm::mat4{1.f};
            viewMatrix[0][0] = u.x;
            viewMatrix[1][0] = u.y;
            viewMatrix[2][0] = u.z;
            viewMatrix[0][1] = v.x;
            viewMatrix[1][1] = v.y;
            viewMatrix[2][1] = v.z;
            viewMatrix[0][2] = w.x;
            viewMatrix[1][2] = w.y;
            viewMatrix[2][2] = w.z;
            viewMatrix[3][0] = -glm::dot(u, position);
            viewMatrix[3][1] = -glm::dot(v, position);
            viewMatrix[3][2] = -glm::dot(w, position);

            inverseViewMatrix = glm::mat4{1.f};
            inverseViewMatrix[0][0] = u.x;
            inverseViewMatrix[0][1] = u.y;
            inverseViewMatrix[0][2] = u.z;
            inverseViewMatrix[1][0] = v.x;
            inverseViewMatrix[1][1] = v.y;
            inverseViewMatrix[1][2] = v.z;
            inverseViewMatrix[2][0] = w.x;
            inverseViewMatrix[2][1] = w.y;
            inverseViewMatrix[2][2] = w.z;
            inverseViewMatrix[3][0] = position.x;
            inverseViewMatrix[3][1] = position.y;
            inverseViewMatrix[3][2] = position.z;
        }

        void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
            setViewDirection(position, target - position, up);
//            viewMatrix = glm::lookAt(position, target, up);
        }

        //使用YXZ tait-bryan angles
        void setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
            const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);
            const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
            const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
            const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
            viewMatrix = glm::mat4{1.f};
            viewMatrix[0][0] = u.x;
            viewMatrix[1][0] = u.y;
            viewMatrix[2][0] = u.z;
            viewMatrix[0][1] = v.x;
            viewMatrix[1][1] = v.y;
            viewMatrix[2][1] = v.z;
            viewMatrix[0][2] = w.x;
            viewMatrix[1][2] = w.y;
            viewMatrix[2][2] = w.z;
            viewMatrix[3][0] = -glm::dot(u, position);
            viewMatrix[3][1] = -glm::dot(v, position);
            viewMatrix[3][2] = -glm::dot(w, position);

            inverseViewMatrix = glm::mat4{1.f};
            inverseViewMatrix[0][0] = u.x;
            inverseViewMatrix[0][1] = u.y;
            inverseViewMatrix[0][2] = u.z;
            inverseViewMatrix[1][0] = v.x;
            inverseViewMatrix[1][1] = v.y;
            inverseViewMatrix[1][2] = v.z;
            inverseViewMatrix[2][0] = w.x;
            inverseViewMatrix[2][1] = w.y;
            inverseViewMatrix[2][2] = w.z;
            inverseViewMatrix[3][0] = position.x;
            inverseViewMatrix[3][1] = position.y;
            inverseViewMatrix[3][2] = position.z;
        }

        constexpr static glm::mat4 CorrectionMatrix = glm::mat4{
                1, 0, 0, 0,
                0, -1, 0, 0,
                0, 0, 0.5f, 0,
                0, 0, 0.5f, 1
        };

        void Update(const ComponentUpdateInfo &updateInfo) override {
            float aspectRatio = updateInfo.rendererInfo->aspectRatio;
            FrameInfo &frameInfo = *updateInfo.frameInfo;
            setViewYXZ(updateInfo.gameObject->transform->translation, updateInfo.gameObject->transform->rotation);
            setPerspectiveProjection(glm::radians(50.f), aspectRatio, 0.1f, 20.f);
            frameInfo.globalUbo.viewMatrix = getViewMatrix();
            frameInfo.globalUbo.inverseViewMatrix = getInverseViewMatrix();
            frameInfo.globalUbo.projectionMatrix = getProjectionMatrix();
#ifdef RAY_TRACING
            frameInfo.globalUbo.inverseProjectionMatrix = glm::inverse(getProjectionMatrix());
#endif
        }

    private:
        glm::mat4 projectionMatrix{1.f};
        glm::mat4 inverseProjectionMatrix{1.f};
        glm::mat4 viewMatrix{1.f};
        glm::mat4 inverseViewMatrix{1.f};
    };
}
