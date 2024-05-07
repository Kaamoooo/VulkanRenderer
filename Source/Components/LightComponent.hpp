#ifndef LIGHT_COMPONENT_INCLUDED
#define LIGHT_COMPONENT_INCLUDED

#include <string>
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <glm/detail/type_mat3x3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>
#include <utility>
#include "Component.hpp"
#include "../Model.hpp"

namespace Kaamoo {
    
    class LightComponent : public Component {
    public:
        inline static int lightNum = 0;
        
        LightComponent() {
            name = "LightComponent";
        }
        
        LightComponent(const rapidjson::Value &object) {
            name = "LightComponent";
            //color, it is stored as an array
            auto colorArray = object["color"].GetArray();
            color = glm::vec3(colorArray[0].GetFloat(), colorArray[1].GetFloat(), colorArray[2].GetFloat());
            auto colorCategoryString = object["category"].GetString();
            lightCategory = lightCategoryMap[colorCategoryString];
            lightIntensity = object["intensity"].GetFloat();
            lightNum++;
        }
        
        int lightIndex = 0;
        float lightIntensity = 1.0f;
        int lightCategory = 0;
        glm::vec3 color{};
        
    private:
        std::unordered_map<std::string, int> lightCategoryMap = {
            {"Point", LightCategory::POINT_LIGHT},
            {"Directional", LightCategory::DIRECTIONAL_LIGHT},
        };
    };
}

#endif