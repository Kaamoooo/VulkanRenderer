#include "GameObject.hpp"
#include "Components/LightComponent.hpp"
#include "Components/MeshRendererComponent.hpp"

namespace Kaamoo {

    GameObject GameObject::makeLight(float intensity, float radius, glm::vec3 color, int lightCategory= 0) {
        static int lightNum = 0;
        GameObject& gameObject = GameObject::createGameObject();
        
        auto* meshRendererComponent=new MeshRendererComponent(nullptr,0);
        gameObject.TryAddComponent(meshRendererComponent);
        auto* lightComponent = new LightComponent();
        gameObject.TryAddComponent(lightComponent);
        
        gameObject.transform->scale.x = radius;
        lightComponent->color = color;
        lightComponent->lightIntensity = intensity;
        lightComponent->lightIndex = lightNum;
        lightComponent->lightCategory = lightCategory;
        lightNum++;
        return std::move(gameObject);
    }

}
