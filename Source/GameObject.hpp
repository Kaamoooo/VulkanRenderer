﻿#pragma once

#include <memory>
#include "Model.hpp"
#include "Material.hpp"
#include "Components/Component.hpp"
#include "Utils/Utils.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/LightComponent.hpp"
#include "Components/MeshRendererComponent.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <utility>

namespace Kaamoo {

    class GameObject {
    public:
        using Map = std::unordered_map<id_t, GameObject>;

        TransformComponent* transform;

        ~GameObject(){
            for(auto& component: components){
                delete component;
            }
        }
        
        static GameObject &createGameObject(std::string name = "GameObject") {
            static id_t currentID = 0;
            auto *gameObject = new GameObject(currentID++, std::move(name));
            auto* t = new TransformComponent();
            gameObject->TryAddComponent(t);
            gameObject->transform = t;
            return *gameObject;
        }

        static GameObject makeLight(float intensity, float radius, glm::vec3 color, int lightCategory= 0) {
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
        
        

        id_t getId() { return id; }

        //移动构造函数
        GameObject(GameObject &&) = default;

        GameObject &operator=(GameObject &) = delete;

        GameObject &operator=(GameObject &&) = default;

        std::string getName() const { return name; }

        void setName(std::string name) { GameObject::name = std::move(name); }

        template<typename T, typename std::enable_if<std::is_base_of<Component, T>::value, int>::type = 0>
        void TryAddComponent(T* component) {
            for (auto &item: components) {
                if (dynamic_cast<T *>(item) != nullptr) {
                    throw std::runtime_error(
                            "Game Object " + name + " already has component type " + Utils::TypeName<T>());
                }
            }
            components.push_back(component);
        }

        template<typename T>
        bool TryGetComponent(T*& component) {
            for (auto& item: components) {
                T *targetComponent = dynamic_cast<T *>(item);
                if (targetComponent != nullptr) {
                    component = targetComponent;
                    return true;
                }
            }
            return false;
        }

    private:
        std::vector<Component*> components;

        id_t id;

        std::string name;

        GameObject(id_t id, std::string name = "GameObject") : id{id}, name{std::move(name)} {};
    };
}