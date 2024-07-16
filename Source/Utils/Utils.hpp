#pragma once

#include <typeinfo>
#include <string>
#include <glm/glm.hpp>
#include <iomanip>

namespace Kaamoo {
    using id_t = unsigned int;
    
    class Utils {
    public:
        
        template<typename T>
        static std::string TypeName(){
            return typeid(T).name();
        }
        
        static VkTransformMatrixKHR GlmMatrixToVulkanMatrix(glm::mat4 glmMatrix){
            VkTransformMatrixKHR transformMatrix;
            for(int i=0; i<3; i++){
                for(int j=0; j<4; j++){
                    transformMatrix.matrix[i][j] = glmMatrix[j][i];
                }
            }
            return transformMatrix;
        }
        
        static uint32_t alignUp(uint32_t value, uint32_t alignment){
            return (value + alignment - 1) & ~(alignment - 1);
        }
        
        static std::string FloatToString(float value){
            std::ostringstream oss;
            oss << std::setprecision(2) << value;
            return oss.str();
        }
        
        static std::string Vec3ToString(glm::vec3 value){
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << " (" << value.x << ", " << value.y << ", " << value.z << ")";
            return oss.str();
        }                    
        
        static std::string GetFileNameFromPath(const std::string& path, char separator) {
            size_t pos = path.find_last_of(separator);
            if (pos != std::string::npos) {
                return path.substr(pos + 1);
            }
            return path;
        }
    };
}