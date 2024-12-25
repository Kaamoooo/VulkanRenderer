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

        TransformComponent *transform;

        ~GameObject() {
            for (auto &component: m_components) {
                delete component;
            }
        }

        static GameObject &createGameObject(std::string name = "GameObject") {
            static id_t currentID = 0;
            auto *gameObject = new GameObject(currentID++, std::move(name));
            auto *t = new TransformComponent();
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

        std::vector<Component *> getComponents() { return m_components; }

        template<typename T, typename std::enable_if<std::is_base_of<Component, T>::value, int>::type = 0>
        void TryAddComponent(T *component) {
            for (auto &item: m_components) {
                if (item->GetName() == component->GetName()) {
                    throw std::runtime_error(
                            "Game Object " + name + " already has component type " + component->GetName());
                }
            }
            m_components.push_back(component);
        }

        template<typename T>
        bool TryGetComponent(T *&component) {
            for (auto &item: m_components) {
                T *targetComponent = dynamic_cast<T *>(item);
                if (targetComponent != nullptr) {
                    component = targetComponent;
                    return true;
                }
            }
            return false;
        }

        void OnLoad() {
            for (auto &component: m_components) {
                component->OnLoad(this);
            }
        }

        void Loaded() {
            for (auto &component: m_components) {
                component->Loaded(this);
            }
        }

        void Awake(ComponentAwakeInfo awakeInfo) {
            for (auto &component: m_components) {
                component->Awake(awakeInfo);
            }
        }

        void Start(const ComponentUpdateInfo &updateInfo) {
            for (auto &component: m_components) {
                component->Start(updateInfo);
            }
        }

        void Update(const ComponentUpdateInfo &updateInfo) {
            for (auto &component: m_components) {
                component->Update(updateInfo);
            }
        }

        void LateUpdate(const ComponentUpdateInfo &updateInfo) {
            for (auto &component: m_components) {
                component->LateUpdate(updateInfo);
            }
        }

        void FixedUpdate(const ComponentUpdateInfo &updateInfo) {
            for (auto &component: m_components) {
                component->FixedUpdate(updateInfo);
            }
        }

        void LateFixedUpdate(const ComponentUpdateInfo &updateInfo) {
            for (auto &component: m_components) {
                component->LateFixedUpdate(updateInfo);
            }
        }

    private:
        std::vector<Component *> m_components;

        id_t id;

        std::string name;

        GameObject(id_t id, std::string name = "GameObject") : id{id}, name{std::move(name)} {};
    };

    class HierarchyTree {
    public:
        struct Node {
            int id;
            GameObject *gameObject;
            int parentTransformId;
            std::vector<Node *> children;
        };
        static const int ROOT_ID = -1;
        static const int DEFAULT_TRANSFORM_ID = -2;

        HierarchyTree() {
            m_root = new Node{ROOT_ID, nullptr};
        };

        Node *GetRoot() {
            return m_root;
        }

        bool AddNode(int parentId, int childId, GameObject *childGameObject) {

            auto parentNode = FindNode(parentId, m_root);

            if (parentNode == nullptr) {
                auto node = new Node{childId, childGameObject, parentId};
                m_fakeNodes.push_back(node);
                m_root->children.push_back(node);
            } else {
                auto node = new Node{childId, childGameObject};
                parentNode->children.push_back(node);
                for (auto &fakeNode: m_fakeNodes) {
                    if (childGameObject->transform->GetTransformId() == fakeNode->parentTransformId) {
                        node->children.push_back(fakeNode);
                        auto it = std::find_if(m_root->children.begin(), m_root->children.end(), [&fakeNode](const Node *node) {
                            return node->id == fakeNode->id;
                        });
                        if (it != m_root->children.end()) {
                            m_root->children.erase(it);
                        }

                        it = std::find(m_fakeNodes.begin(), m_fakeNodes.end(), fakeNode);
                        if (it != m_fakeNodes.end()) {
                            m_fakeNodes.erase(it);
                        }
                    }
                }
            }

            return true;
        }


    private:
        Node *m_root = nullptr;
        std::vector<Node *> m_fakeNodes;

        Node *FindNode(int id, Node *tmpNode) {
            if (tmpNode->id == id) {
                return tmpNode;
            }
            for (auto &child: tmpNode->children) {
                auto node = FindNode(id, child);
                if (node != nullptr) {
                    return node;
                }
            }
            return nullptr;
        }
    };

}

#endif
