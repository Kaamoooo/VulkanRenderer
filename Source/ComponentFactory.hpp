#include <any>
#include <rapidjson/document.h>
#include "Components/Input/ObjectMovementComponent.hpp"
#include "Components/Input/CameraMovementComponent.hpp"
#include "Components/CameraComponent.hpp"

namespace Kaamoo {

    class ComponentFactory {
    public:
        struct ComponentName {
            inline static const std::string ObjectMovementComponent = "ObjectMovementComponent";
            inline static const std::string CameraMovementComponent = "CameraMovementComponent";
            inline static const std::string MeshRendererComponent = "MeshRendererComponent";
            inline static const std::string CameraComponent = "CameraComponent";
            inline static const std::string LightComponent = "LightComponent";
        };

        //Initialize some global parameters
        Component* CreateComponent(std::unordered_map<int, rapidjson::Value>& componentsMap,const int componentId) {
            if (componentConstructorMap.empty()) {
                InitMap();
            }
            auto typeName = componentsMap[componentId]["type"].GetString();
            auto componentConstructor = componentConstructorMap.find(typeName)->second;
            return (*componentConstructor)(componentsMap[componentId]);
        }
        
        void InitMap(){
            componentConstructorMap[ComponentName::MeshRendererComponent] = [](const rapidjson::Value& object)->Component*{return new MeshRendererComponent(object);};
            componentConstructorMap[ComponentName::LightComponent] = [](const rapidjson::Value& object)->Component*{return new LightComponent(object);};
            componentConstructorMap[ComponentName::ObjectMovementComponent] = [](const rapidjson::Value& object)->Component*{return new ObjectMovementComponent(Application::getWindow().getGLFWwindow());};
            componentConstructorMap[ComponentName::CameraMovementComponent] = [](const rapidjson::Value& object)->Component*{return new CameraMovementComponent(Device::getDeviceSingleton()->getWindow().getGLFWwindow());};
            componentConstructorMap[ComponentName::CameraComponent] = [](const rapidjson::Value& object)->Component*{return new CameraComponent();};
        }

    private:
        std::unordered_map<std::string, Component*(*)(const rapidjson::Value&)> componentConstructorMap;
    };

}