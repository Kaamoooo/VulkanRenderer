#pragma once

#include <typeinfo>
#include <string>
#include <glm/glm.hpp>
#include <iomanip>
#include <algorithm>

namespace Kaamoo {
    using id_t = unsigned int;

    class Utils {
    public:

        template<typename T>
        static std::string TypeName() {
            return typeid(T).name();
        }

        static glm::vec3 VectorToRotation(const glm::vec3 &target) {
            // 归一化目标向量
            glm::vec3 _direction = glm::normalize(target);

            // 计算 Yaw 和 Pitch
            float yaw = std::atan2(_direction.x, _direction.z);  // 水平方向 (绕y轴)
            float pitch = std::asin(-_direction.y);            // 垂直方向 (绕x轴)

            // Roll 通常为 0，除非有额外信息
            float roll = 0.0f;

            return {pitch, yaw, roll};
        }

        static glm::vec3 TruncVec3(const glm::vec3 &vec, int precision) {
            float scale = std::pow(10.0f, precision);
            return glm::vec3(
                    std::trunc(vec.x * scale) / scale,
                    std::trunc(vec.y * scale) / scale,
                    std::trunc(vec.z * scale) / scale
            );
        }
        
        static glm::vec3 TruncSmallValues(glm::vec3 vec, float threshold) {
            if (std::abs(vec.x) < threshold) vec.x = 0;
            if (std::abs(vec.y) < threshold) vec.y = 0;
            if (std::abs(vec.z) < threshold) vec.z = 0;
            return vec;
        }

        static glm::mat3 SkewSymmetric(const glm::vec3 &a) {
            return glm::mat3(
                    0, -a.z, a.y,
                    a.z, 0, -a.x,
                    -a.y, a.x, 0
            );
        }

        static VkTransformMatrixKHR GlmMatrixToVulkanMatrix(glm::mat4 glmMatrix) {
            VkTransformMatrixKHR transformMatrix;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 4; j++) {
                    transformMatrix.matrix[i][j] = glmMatrix[j][i];
                }
            }
            return transformMatrix;
        }

        static uint32_t alignUp(uint32_t value, uint32_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

        static std::string FloatToString(float value) {
            std::ostringstream oss;
            oss << std::setprecision(2) << value;
            return oss.str();
        }

        static float StringToFloat(const std::string &value) {
            return std::stof(value);
        }

        static std::string Vec3ToString(glm::vec3 value) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << " (" << value.x << ", " << value.y << ", " << value.z << ")";
            return oss.str();
        }

        // 1,1,1 -> Glm::vec3(1,1,1)
        static glm::vec3 StringToVec3(const std::string &value) {
            glm::vec3 vec;
            std::string str = value;
            size_t pos = str.find(",");
            vec.x = std::stof(str.substr(0, pos));
            str = str.substr(pos + 1);
            pos = str.find(",");
            vec.y = std::stof(str.substr(0, pos));
            vec.z = std::stof(str.substr(pos + 1));
            return vec;
        }

        static std::string GetFileNameFromPath(const std::string &path, char separator) {
            size_t pos = path.find_last_of(separator);
            if (pos != std::string::npos) {
                return path.substr(pos + 1);
            }
            return path;
        }

        static void ClampFloat(float &value, float min, float max) {
            if (value < min) value = min;
            else if (value > max) value = max;
        }

        static void ClampVec3(glm::vec3 &value, float min, float max) {
            ClampFloat(value.x, min, max);
            ClampFloat(value.y, min, max);
            ClampFloat(value.z, min, max);
        }

        static glm::mat4 GetRotateDirectionMatrix(glm::vec3 rotation) {
            const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);
            const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
            const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
            const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
            auto rotateMatrix = glm::mat4{1.f};
            rotateMatrix[0][0] = u.x;
            rotateMatrix[0][1] = u.y;
            rotateMatrix[0][2] = u.z;
            rotateMatrix[1][0] = v.x;
            rotateMatrix[1][1] = v.y;
            rotateMatrix[1][2] = v.z;
            rotateMatrix[2][0] = w.x;
            rotateMatrix[2][1] = w.y;
            rotateMatrix[2][2] = w.z;
            return rotateMatrix;
        }
    };
}