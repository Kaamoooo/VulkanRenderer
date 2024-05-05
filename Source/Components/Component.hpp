#pragma once

#include <string>
#include "../StructureInfos.h"

namespace Kaamoo {
    
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

    protected:
        std::string name = "Component";
    };
}