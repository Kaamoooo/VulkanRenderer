#pragma once

#include <string>
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <glm/detail/type_mat3x3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>
#include <rapidjson/document.h>
#include <iostream>
#include "../StructureInfos.h"

namespace Kaamoo {
    class GameObject;

    struct ComponentUpdateInfo {
        
        GameObject *gameObject;
        FrameInfo *frameInfo;
        RendererInfo *rendererInfo;
    };

    class Component {
    public:
        virtual std::string GetName() { return name; }

        virtual ~Component() = default;

        virtual void Update(const ComponentUpdateInfo &updateInfo) {};
        
        virtual void LateUpdate(const ComponentUpdateInfo &updateInfo) {};

    protected:
        std::string name = "Component";

    };

}