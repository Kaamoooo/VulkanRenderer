﻿#include <typeinfo>
#include <string>

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
    };
}