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
        LightComponent() {
            name = "LightComponent";
        }
        
        int lightIndex = 0;
        float lightIntensity = 1.0f;
        int lightCategory = 0;
        glm::vec3 color{};
        
    };
}

#endif