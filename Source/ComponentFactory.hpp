#include <any>
#include <rapidjson/document.h>
#include "Components/Input/ObjectMovementComponent.hpp"
#include "Components/Input/CameraMovementComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/MeshRendererComponent.hpp"
#include "Components/LightComponent.hpp"
#include "Components/RayTracingManagerComponent.hpp"
#include "Components/RigidBodyComponent.hpp"

namespace Kaamoo {
    struct ComponentName {
        inline static const std::string TransformComponent = "TransformComponent";
        inline static const std::string ObjectMovementComponent = "ObjectMovementComponent";
        inline static const std::string CameraMovementComponent = "CameraMovementComponent";
        inline static const std::string MeshRendererComponent = "MeshRendererComponent";
        inline static const std::string CameraComponent = "CameraComponent";
        inline static const std::string LightComponent = "LightComponent";
        inline static const std::string RayTracingManagerComponent = "RayTracingManagerComponent";
        inline static const std::string RigidBodyComponent = "RigidBodyComponent";
    };

    class ComponentFactory {
    public:

        //Initialize some global parameters
        Component* CreateComponent(std::unordered_map<int, rapidjson::Value>& componentsMap,const int componentId) {
            if (componentConstructorMap.empty()) {
                InitMap();
            }
            auto typeName = componentsMap[componentId]["type"].GetString();
            if (componentConstructorMap.count(typeName) == 0) {
                return nullptr;
            }
            auto componentConstructor = componentConstructorMap.find(typeName)->second;
            return (*componentConstructor)(componentsMap[componentId]);
        }
        
        void InitMap(){
            componentConstructorMap[ComponentName::MeshRendererComponent] = [](const rapidjson::Value& object)->Component*{return new MeshRendererComponent(object);};
            componentConstructorMap[ComponentName::LightComponent] = [](const rapidjson::Value& object)->Component*{return new LightComponent(object);};
            componentConstructorMap[ComponentName::ObjectMovementComponent] = [](const rapidjson::Value& object)->Component*{return new ObjectMovementComponent(Device::getDeviceSingleton()->getWindow().getGLFWwindow());};
            componentConstructorMap[ComponentName::CameraMovementComponent] = [](const rapidjson::Value& object)->Component*{return new CameraMovementComponent(Device::getDeviceSingleton()->getWindow().getGLFWwindow());};
            componentConstructorMap[ComponentName::CameraComponent] = [](const rapidjson::Value& object)->Component*{return new CameraComponent();};
            componentConstructorMap[ComponentName::RigidBodyComponent] = [](const rapidjson::Value& object)->Component*{return new RigidBodyComponent();};
#ifdef RAY_TRACING
            componentConstructorMap[ComponentName::RayTracingManagerComponent] = [](const rapidjson::Value& object)->Component*{return new RayTracingManagerComponent();};
#endif
        }

    private:
        std::unordered_map<std::string, Component*(*)(const rapidjson::Value&)> componentConstructorMap;
    };

}