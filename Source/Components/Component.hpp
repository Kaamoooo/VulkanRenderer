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

#include "Imgui/imgui.h"
#include "../Utils/Utils.hpp"

namespace Kaamoo {
    class GameObject;

    struct ComponentAwakeInfo {
        GameObject *gameObject;
    };

    struct ComponentUpdateInfo {
        GameObject *gameObject;
        FrameInfo *frameInfo;
        RendererInfo *rendererInfo;
    };

    class Component {
    public:
        virtual std::string GetName() { return name; }

        virtual ~Component() = default;

        virtual void OnLoad(GameObject *gameObject) {};

        virtual void Loaded(GameObject *gameObject) {};

        virtual void Awake(const ComponentAwakeInfo &awakeInfo) {};

        virtual void Start(const ComponentUpdateInfo &updateInfo) {};

        virtual void Update(const ComponentUpdateInfo &updateInfo) {};

        virtual void LateUpdate(const ComponentUpdateInfo &updateInfo) {};

        virtual void SetUI(std::vector<GameObjectDesc>*){};

    protected:
        std::string name = "Component";

    };

}