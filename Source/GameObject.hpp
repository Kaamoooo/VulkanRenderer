#ifndef GAME_OBJECT_INCLUDED
#define GAME_OBJECT_INCLUDED

#include <memory>
#include "Utils/Utils.hpp"
#include "Components/TransformComponent.hpp"

namespace Kaamoo {
    class Component;
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
                if (item->GetName()==component->GetName()) {
                    throw std::runtime_error(
                            "Game Object " + name + " already has component type " + component->GetName());
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
        
        void Update(const ComponentUpdateInfo& updateInfo){
            for(auto& component: components){
                component->Update(updateInfo);
            }
        }
        
        void LateUpdate(const ComponentUpdateInfo& updateInfo){
            for(auto& component: components){
                component->LateUpdate(updateInfo);
            }
        }

    private:
        std::vector<Component*> components;

        id_t id;

        std::string name;

        GameObject(id_t id, std::string name = "GameObject") : id{id}, name{std::move(name)} {};
    };
}

#endif
