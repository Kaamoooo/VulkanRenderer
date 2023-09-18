//
// Created by asus on 2023/9/18.
//
#include "../Headers/Camera.h"

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
}