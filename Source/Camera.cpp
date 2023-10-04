//
// Created by asus on 2023/9/18.
//
#include "Camera.h"

namespace Kaamoo{

    //vulkan只会显示x,y在-1~1,z在0~1区间的像素
    void Camera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
        projectionMatrix = glm::mat4{1.f};
        projectionMatrix[0][0] = 2.f/(right-left);
        projectionMatrix[1][1] = 2.f/(bottom-top);
        projectionMatrix[2][2] = 1.f/(far-near);
        
        projectionMatrix[3][0] = -(right+left)/(right-left);
        projectionMatrix[3][1] = -(bottom+top)/(bottom-top);
        projectionMatrix[3][2] = -near/(far-near);
        
    }

    void Camera::setPerspectiveProjection(float fovY, float aspect, float near, float far) {
        const float tanHalfFovY = tan(fovY/2);
        projectionMatrix=glm::mat4{0.f};
        projectionMatrix[0][0] = 1.f/(aspect*tanHalfFovY);
        projectionMatrix[1][1] = 1.f/(tanHalfFovY);
        projectionMatrix[2][2] = far/(far-near);
        projectionMatrix[2][3] = 1.f;
        projectionMatrix[3][2] = -(far*near)/(far-near);
    }

    void Camera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
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
    }

    void Camera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
        setViewDirection(position, target - position, up);
    }

    void Camera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
        //这里角度均取负，代表往相反方向旋转
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
    }

}