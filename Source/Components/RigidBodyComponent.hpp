#pragma once

#include <optional>
#include "MeshRendererComponent.hpp"

namespace Kaamoo {
    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };

    struct Triangle {
        glm::vec3 vertices[3];
        glm::vec3 normal;
    };

    class RigidBodyComponent : public Component {
    public:
        inline const static float EPSILON = 0.0001f;

        RigidBodyComponent(const rapidjson::Value &object) {
            if (object.HasMember("isKinematic")) {
                m_isKinematic = object["isKinematic"].GetBool();
            }
            if (object.HasMember("omega")) {
                auto _omegaArray = object["omega"].GetArray();
                m_omega = glm::vec3(_omegaArray[0].GetFloat(), _omegaArray[1].GetFloat(), _omegaArray[2].GetFloat());
            }
            if (object.HasMember("velocity")) {
                auto _velocityArray = object["velocity"].GetArray();
                m_velocity = glm::vec3(_velocityArray[0].GetFloat(), _velocityArray[1].GetFloat(), _velocityArray[2].GetFloat());
            }
            name = "RigidBodyComponent";
        }

        void Loaded(GameObject *gameObject) override {
            if (!gameObject->TryGetComponent(m_meshRendererComponent)) {
                throw std::runtime_error("RigidBodyComponent needs a MeshRendererComponent");
            }
            m_transformComponent = gameObject->transform;

            glm::mat3 _I0;
            glm::vec3 _massCenter;
            float _totalMass;
            MeshComputeInertia(gameObject->transform, m_meshRendererComponent->GetModelPtr().get(), 1.0f, &_I0, &_massCenter, &_totalMass);

            m_I0 = _I0;
            m_invI0 = glm::inverse(_I0);
            m_invMass = 1.0f / _totalMass;
            m_nativeMassCenter = _massCenter;
            m_totalMass = _totalMass;

            auto &_vertices = m_meshRendererComponent->GetModelPtr()->GetVertices();
            glm::vec3 _min = _vertices[0].position, _max = _vertices[0].position;
            for (auto &_vertex: _vertices) {
                _vertex.position -= _massCenter;
                _min = glm::min(_min, _vertex.position);
                _max = glm::max(_max, _vertex.position);
            }

            m_aabb.min = _min;
            m_aabb.max = _max;
            Insert(GetAABB(gameObject->transform), gameObject);

            m_meshRendererComponent->GetModelPtr()->RefreshVertexBuffer(_vertices);
        }

        void FixedUpdate(const ComponentUpdateInfo &updateInfo) override {
            auto _startTime = std::chrono::high_resolution_clock::now();
            auto _gameObjects = GetBroadPhaseCollisions(updateInfo.gameObject);
            if (!_gameObjects.empty()) {
                for (auto &_gameObject: _gameObjects) {
                    glm::vec3 _collidedFaceNormal{};
                    glm::vec3 _collisionPoint{};
                    auto _collidedPoint = GetNarrowPhaseCollision(updateInfo.gameObject, _gameObject, _collidedFaceNormal, _collisionPoint);
                    if (_collidedPoint.has_value()) {
                        ProcessCollision(_collidedPoint.value(), _collidedFaceNormal, _collisionPoint, updateInfo.gameObject, _gameObject);
                    }
                }
            }
            auto _rotationMatrix = m_transformComponent->GetRotationMatrix();
            m_transformComponent->Translate(m_velocity * FIXED_UPDATE_INTERVAL);
            m_transformComponent->Rotate(m_omega * FIXED_UPDATE_INTERVAL);
            m_I0 = _rotationMatrix * m_I0 * glm::transpose(_rotationMatrix);
        }

        AABB GetAABB(TransformComponent *transformComponent) const {
            //Jim Arvo
            //Split the transform into a translation vector (T) and a 3x3 rotation (M).
            auto _mat4 = transformComponent->mat4();
            glm::vec3 _translation = _mat4[3];
            AABB _aabb{};
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    float _a = _mat4[j][i] * m_aabb.min[j];
                    float _b = _mat4[j][i] * m_aabb.max[j];
                    _aabb.min[i] += _a < _b ? _a : _b;
                    _aabb.max[i] += _a < _b ? _b : _a;
                }
            }
            _aabb.max += _translation;
            _aabb.min += _translation;
            return _aabb;
        }

        glm::vec3 GetMassCenter(TransformComponent *transformComponent) const {
            return transformComponent->mat4() * glm::vec4(m_nativeMassCenter, 1);
        }

        void SetVelocity(glm::vec3 velocity) {
            m_velocity = velocity;
        }

        void SetOmega(glm::vec3 omega) {
            m_omega = omega;
        }

        glm::vec3 GetVelocity() const {
            return m_velocity;
        }

        glm::vec3 GetOmega() const {
            return m_omega;
        }

        float GetInvMass() const {
            return m_invMass;
        }

        glm::mat3 GetInvI0(TransformComponent *transformComponent) const {
            auto _rotateMatrix = transformComponent->GetRotationMatrix();
            auto _I0 = _rotateMatrix * m_I0 * glm::transpose(_rotateMatrix);
            return glm::inverse(_I0);
        }

        bool IsKinematic() const {
            return m_isKinematic;
        }

    private:
        AABB m_aabb;
        TransformComponent *m_transformComponent;
        MeshRendererComponent *m_meshRendererComponent;
        glm::vec3 m_velocity{0, 0, 0};
        glm::vec3 m_omega{0, 0, 0};
        glm::vec3 m_nativeMassCenter{0};
        glm::mat3 m_I0;
        glm::mat3 m_invI0;
        float m_totalMass;
        float m_invMass;
        float m_e = 1.0f;
        float m_u = 0.5f;

        bool m_isKinematic = false;

        void ProcessCollision(glm::vec3 intersectionPoint, glm::vec3 collidedFaceNormal, glm::vec3 collisionPoint, GameObject *gameObject, GameObject *otherObject) {
            RigidBodyComponent *_otherRigidBodyComponent;
            if (!otherObject->TryGetComponent(_otherRigidBodyComponent)) {
                throw std::runtime_error("RigidBodyComponent not found");
            }
            glm::vec3 _massCenter = GetMassCenter(gameObject->transform);
            glm::vec3 _r = intersectionPoint - _massCenter;
            glm::vec3 _n = -glm::normalize(collidedFaceNormal);

            TransformComponent *_otherTransformComponent = otherObject->transform;
            glm::vec3 _velocityOther = _otherRigidBodyComponent->GetVelocity();
            glm::vec3 _omegaOther = _otherRigidBodyComponent->GetOmega();
            float _otherInvMass = _otherRigidBodyComponent->GetInvMass();
            glm::mat3 _otherInvI0 = _otherRigidBodyComponent->GetInvI0(_otherTransformComponent);
            glm::vec3 _rOther = intersectionPoint - _otherRigidBodyComponent->GetMassCenter(otherObject->transform);

            glm::vec3 _relativeVelocity = m_velocity - _velocityOther + glm::cross(m_omega, _r) - glm::cross(_omegaOther, _rOther);
            auto _invI0 = GetInvI0(m_transformComponent);

            if (glm::length(_relativeVelocity) < 0.001f || glm::dot(_relativeVelocity, _n) < 0) {
                return;
            }

            _relativeVelocity = Utils::TruncVec3(_relativeVelocity, 3);
            glm::vec3 _Vn = glm::dot(_relativeVelocity, _n) * _n;
            glm::vec3 _Vt = _relativeVelocity - _Vn;
            glm::vec3 _t;
            if (glm::length(_Vt) < EPSILON) {
                _t = glm::vec3(0);
            } else {
                _t = -glm::normalize(_Vt);
            }

            glm::vec3 _Jn = (1 + m_e) * _Vn / (m_invMass + _otherInvMass +
                                               glm::dot(_n, glm::cross(_otherInvI0 * glm::cross(_rOther, _n), _rOther) + glm::cross(_invI0 * glm::cross(_r, _n), _r)));
            glm::vec3 _Jt = _Vt / (m_invMass + _otherInvMass +
                                   glm::dot(_t, glm::cross(_otherInvI0 * glm::cross(_rOther, _t), _rOther) + glm::cross(_invI0 * glm::cross(_r, _t), _r)));
            if (glm::length(_Jt) > m_u * glm::length(_Jn)) {
                _Jt = m_u * glm::length(_Jn) * _t;
            }
            auto _J = _Jn * _n + _Jt * _t;

            m_velocity -= _J * m_invMass;
            m_omega -= _invI0 * glm::cross(_r, _J);

            if (!_otherRigidBodyComponent->IsKinematic()) {
                _velocityOther += _J * _otherInvMass;
                _otherRigidBodyComponent->SetVelocity(_velocityOther);
                _omegaOther += _otherInvI0 * glm::cross(_rOther, _J);
                _otherRigidBodyComponent->SetOmega(_omegaOther);
            }

            if (glm::length(m_omega) > 3) {
                std::cout << "omega: " << m_omega.x << ", " << m_omega.y << ", " << m_omega.z << std::endl;
            }
        }

        static std::optional<glm::vec3> GetNarrowPhaseCollision(GameObject *main, GameObject *other, glm::vec3 &normal, glm::vec3 &collisionPoint) {
            RigidBodyComponent *_mainRigidBodyComponent, *_otherRigidBodyComponent;
            MeshRendererComponent *_mainMeshRendererComponent, *_otherMeshRendererComponent;
            if (!main->TryGetComponent(_mainRigidBodyComponent) || !other->TryGetComponent(_otherRigidBodyComponent)) {
                throw std::runtime_error("Collide: RigidBodyComponent not found");
            }
            if (!main->TryGetComponent(_mainMeshRendererComponent) || !other->TryGetComponent(_otherMeshRendererComponent)) {
                throw std::runtime_error("Collide: MeshRendererComponent not found");
            }

            AABB _aabbMain = _mainRigidBodyComponent->GetAABB(main->transform);
            AABB _aabbOther = _otherRigidBodyComponent->GetAABB(other->transform);
            AABB _intersectionAABB{};
            bool _intersect = AABBIntersect(_aabbMain, _aabbOther);
            if (!_intersect) {
                return {};
            }

            for (int i = 0; i < 3; ++i) {
                std::vector<float> _coords = {_aabbMain.min[i], _aabbMain.max[i], _aabbOther.min[i], _aabbOther.max[i]};
                std::sort(_coords.begin(), _coords.end());
                _intersectionAABB.min[i] = _coords[1];
                _intersectionAABB.max[i] = _coords[2];
            }

            auto &_mainVertices = _mainMeshRendererComponent->GetModelPtr()->GetVertices();
            auto &_otherVertices = _otherMeshRendererComponent->GetModelPtr()->GetVertices();
            auto &_otherIndices = _otherMeshRendererComponent->GetModelPtr()->GetIndices();

            std::vector<glm::vec3> _mainValidVertices;
            for (int i = 0; i < _mainVertices.size(); ++i) {
                glm::vec3 _mainWorldVertex = main->transform->mat4() * glm::vec4(_mainVertices[i].position, 1);
                if (_mainWorldVertex.x > _intersectionAABB.min.x - EPSILON && _mainWorldVertex.x < _intersectionAABB.max.x + EPSILON
                    && _mainWorldVertex.y > _intersectionAABB.min.y - EPSILON && _mainWorldVertex.y < _intersectionAABB.max.y + EPSILON
                    && _mainWorldVertex.z > _intersectionAABB.min.z - EPSILON && _mainWorldVertex.z < _intersectionAABB.max.z + EPSILON) {
                    _mainValidVertices.push_back(_mainWorldVertex);
                }
            }

            std::vector<Triangle> _otherValidTriangles;
            for (int i = 0; i < _otherIndices.size(); i += 3) {
                glm::vec3 _triangleVertices[3];
                AABB _triangleAABB;
                _triangleAABB.min = glm::vec3(FLT_MAX);
                _triangleAABB.max = glm::vec3(-FLT_MAX);
                for (int j = 0; j < 3; ++j) {
                    _triangleVertices[j] = other->transform->mat4() * glm::vec4(_otherVertices[_otherIndices[i + j]].position, 1);
                    for (int k = 0; k < 3; ++k) {
                        _triangleAABB.min[k] = glm::min(_triangleAABB.min[k], _triangleVertices[j][k]);
                        _triangleAABB.max[k] = glm::max(_triangleAABB.max[k], _triangleVertices[j][k]);
                    }
                }
                _intersect = AABBIntersect(_triangleAABB, _intersectionAABB);
                if (_intersect) {
                    Triangle _triangle;
                    _triangle.vertices[0] = _triangleVertices[0];
                    _triangle.vertices[1] = _triangleVertices[1];
                    _triangle.vertices[2] = _triangleVertices[2];
                    _triangle.normal = glm::normalize(glm::cross(_triangle.vertices[1] - _triangle.vertices[0], _triangle.vertices[2] - _triangle.vertices[0]));
                    _otherValidTriangles.push_back(_triangle);
                }
            }

            glm::vec3 _rayOrigin = _mainRigidBodyComponent->GetMassCenter(main->transform);
            std::vector<glm::vec3> _intersectionPoints{};
            std::vector<glm::vec3> _collidedPoints{};
            std::vector<Triangle> _collidedTriangles{};
            for (const auto &_mainValidVertex: _mainValidVertices) {
                glm::vec3 _rayVector = _mainValidVertex - _rayOrigin;
                for (const auto &_otherValidTriangle: _otherValidTriangles) {
                    auto _intersectionPoint = RayIntersectsTriangle(_rayOrigin, _rayVector, _otherValidTriangle);
                    if (_intersectionPoint.has_value()) {
                        _collidedPoints.push_back(_mainValidVertex);
                        _intersectionPoints.push_back(_intersectionPoint.value());
                        _collidedTriangles.push_back(_otherValidTriangle);
                    }
                }
            }
            if (_intersectionPoints.empty() || _collidedTriangles.empty()) {
                return {};
            }

            glm::vec3 _averageIntersectionPoint{};
            glm::vec3 _averageCollisionPoint{};
            glm::vec3 _averageNormal{};
            for (const auto &_intersectionPoint: _intersectionPoints) {
                _averageIntersectionPoint += _intersectionPoint;
            }
            for (auto &triangle: _collidedTriangles) {
                _averageNormal += triangle.normal;
            }
            normal = glm::normalize(_averageNormal);
            for (auto &point: _collidedPoints) {
                _averageCollisionPoint += point;
            }
            _averageCollisionPoint /= _collidedPoints.size();
            collisionPoint = _averageCollisionPoint;

            _averageIntersectionPoint /= _intersectionPoints.size();
            return _averageIntersectionPoint;
        }

        static void MeshComputeInertia(TransformComponent *transformComponent, Model *model, float density, glm::mat3 *I0, glm::vec3 *massCenter, float *totalMass) {
            auto _indices = model->GetIndices();
            auto _vertices = model->GetVertices();
            assert(_indices.size() % 3 == 0 && "Indices size must be a multiple of 3");

            float _mass = 0;
            glm::vec3 _massCenter{0};
            float _Ia = 0, _Ib = 0, _Ic = 0, _Iap = 0, _Ibp = 0, _Icp = 0;
            for (int i = 0; i < _indices.size(); i += 3) {
                glm::vec3 _triangleVertices[3];
                for (int j = 0; j < 3; j++) {
                    _triangleVertices[j] = transformComponent->GetRotationMatrix() * transformComponent->GetScale() * _vertices[_indices[i + j]].position;
                }

                //Todo: The algorithm below is copied and should be understood later
                float _detJ = glm::dot(_triangleVertices[0], glm::cross(_triangleVertices[1], _triangleVertices[2]));
                float _tetVolume = _detJ / 6.0f;
                float _tetMass = density * _tetVolume;
                glm::vec3 _tetMassCenter = (_triangleVertices[0] + _triangleVertices[1] + _triangleVertices[2]) / 4.0f;

                _Ia += _detJ * (ComputeInertiaMoment(_triangleVertices, 1) + ComputeInertiaMoment(_triangleVertices, 2));
                _Ib += _detJ * (ComputeInertiaMoment(_triangleVertices, 0) + ComputeInertiaMoment(_triangleVertices, 2));
                _Ic += _detJ * (ComputeInertiaMoment(_triangleVertices, 0) + ComputeInertiaMoment(_triangleVertices, 1));
                _Iap += _detJ * (ComputeInertiaProduct(_triangleVertices, 1, 2));
                _Ibp += _detJ * (ComputeInertiaProduct(_triangleVertices, 0, 1));
                _Icp += _detJ * (ComputeInertiaProduct(_triangleVertices, 0, 2));

                _tetMassCenter *= _tetMass;
                _massCenter += _tetMassCenter;
                _mass += _tetMass;
            }

            _massCenter /= _mass;
            _Ia = density * _Ia / 60.0 - _mass * (pow(_massCenter[1], 2) + pow(_massCenter[2], 2));
            _Ib = density * _Ib / 60.0 - _mass * (pow(_massCenter[0], 2) + pow(_massCenter[2], 2));
            _Ic = density * _Ic / 60.0 - _mass * (pow(_massCenter[0], 2) + pow(_massCenter[1], 2));
            _Iap = density * _Iap / 120.0 - _mass * _massCenter[1] * _massCenter[2];
            _Ibp = density * _Ibp / 120.0 - _mass * _massCenter[0] * _massCenter[1];
            _Icp = density * _Icp / 120.0 - _mass * _massCenter[0] * _massCenter[2];

            glm::mat3 _I = glm::mat3(
                    _Ia, -_Ibp, -_Icp,
                    -_Ibp, _Ib, -_Iap,
                    -_Icp, -_Iap, _Ic
            );

            //Todo: Plan mesh has no mass, so I assigned a maximum value to it for now
            if (_mass < EPSILON) {
                _mass = 1000000;
                _massCenter = glm::vec3(0);
            }

            *I0 = _I;
            *massCenter = _massCenter;
            *totalMass = _mass;
        }

        static float ComputeInertiaMoment(glm::vec3 p[3], int i) {
            return pow(p[0][i], 2) + p[1][i] * p[2][i]
                   + pow(p[1][i], 2) + p[0][i] * p[2][i]
                   + pow(p[2][i], 2) + p[0][i] * p[1][i];
        }

        static float ComputeInertiaProduct(glm::vec3 p[3], int i, int j) {
            return 2 * p[0][i] * p[0][j] + p[1][i] * p[2][j] + p[2][i] * p[1][j]
                   + 2 * p[1][i] * p[1][j] + p[0][i] * p[2][j] + p[2][i] * p[0][j]
                   + 2 * p[2][i] * p[2][j] + p[0][i] * p[1][j] + p[1][i] * p[0][j];
        }

        static bool AABBIntersect(const AABB &a, const AABB &b) {
            glm::vec3 _aMid = {(a.min.x + a.max.x) / 2,
                               (a.min.y + a.max.y) / 2,
                               (a.min.z + a.max.z) / 2};
            glm::vec3 _bMid = {(b.min.x + b.max.x) / 2,
                               (b.min.y + b.max.y) / 2,
                               (b.min.z + b.max.z) / 2};
            glm::vec3 _diff = _aMid - _bMid;
            bool _isIntersect = true;
            for (int i = 0; i < 3; ++i) {
                if ((abs(_diff[i]) - ((a.max[i] - a.min[i]) / 2 + (b.max[i] - b.min[i]) / 2)) > EPSILON) {
                    _isIntersect = false;
                    break;
                }
            }
            return _isIntersect;
        }

        static AABB MakeAABB(glm::vec3 min, glm::vec3 max) {
            AABB _aabb;
            _aabb.min = min - EPSILON;
            _aabb.max = max + EPSILON;
            return _aabb;
        }

        //Möller–Trumbore ray-triangle intersection algorithm
        static std::optional<glm::vec3> RayIntersectsTriangle(const glm::vec3 ray_origin,
                                                              const glm::vec3 ray_vector,
                                                              const Triangle triangle) {
            constexpr float epsilon = std::numeric_limits<float>::epsilon();

            glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
            glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];
            glm::vec3 ray_cross_e2 = cross(ray_vector, edge2);
            float det = dot(edge1, ray_cross_e2);

            if (det > -epsilon && det < epsilon)
                return {};    // This ray is parallel to this triangle.

            float inv_det = 1.0 / det;
            glm::vec3 s = ray_origin - triangle.vertices[0];
            float u = inv_det * dot(s, ray_cross_e2);

            if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon))
                return {};

            glm::vec3 s_cross_e1 = cross(s, edge1);
            float v = inv_det * dot(ray_vector, s_cross_e1);

            if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
                return {};

            // At this stage we can compute t to find out where the intersection point is on the line.
            float t = inv_det * dot(edge2, s_cross_e1);

            if (t > epsilon) // ray intersection
            {
                return glm::vec3(ray_origin + ray_vector * t);
            } else // This means that there is a line intersection but not a ray intersection.
                return {};
        }

#ifdef RAY_TRACING

        void SetUI(std::vector<GameObjectDesc> *, FrameInfo &frameInfo) override {
            ImGui::Text("Velocity:");
            ImGui::SameLine(90);
            ImGui::InputFloat3("##Velocity", &m_velocity.x);

            ImGui::Text("Omega:");
            ImGui::SameLine(90);
            ImGui::InputFloat3("##Omega", &m_omega.x);

            ImGui::Text("Mass:");
            ImGui::SameLine(90);
            ImGui::InputFloat("##Mass", &m_totalMass);
        }

#else

        void SetUI(Material::Map *, FrameInfo &frameInfo) override {
            ImGui::Text("Velocity:");
            ImGui::SameLine(90);
            ImGui::InputFloat3("##Velocity", &m_velocity.x);

            ImGui::Text("Omega:");
            ImGui::SameLine(90);
            ImGui::InputFloat3("##Omega", &m_omega.x);

            ImGui::Text("Mass:");
            ImGui::SameLine(90);
            ImGui::InputFloat("##Mass", &m_totalMass);
        }

#endif


//Octree
    private:
        struct SplitEntry {
            float min;
            float max;
        };

        struct Node {
            SplitEntry splitEntry[3];
            std::vector<GameObject *> gameObjects{};
            std::vector<Node *> children{};
        };
        inline static Node *root = nullptr;
        static const int MAX_DEPTH = 12;

        static void Insert(AABB aabb, GameObject *gameObject) {
            if (root == nullptr) {
                root = new Node();
                root->splitEntry[0] = {-100, +100};
                root->splitEntry[1] = {-100, +100};
                root->splitEntry[2] = {-100, +100};
            }
            InsertRecursive(aabb, gameObject, root, 0);
        }

//Todo: Simplify the collider of complex mesh.
        static void InsertRecursive(AABB aabb, GameObject *gameObject, Node *node, int depth) {
            if (node->children.size() != 0) {
                for (auto &_childNode: node->children) {
                    InsertRecursive(aabb, gameObject, _childNode, depth + 1);
                }
                return;
            }

            bool _isInside = true;
            for (int i = 0; i < 3; ++i) {
                if (aabb.min[i] < node->splitEntry[i].min || aabb.max[i] > node->splitEntry[i].max) {
                    _isInside = false;
                    break;
                }
            }

            glm::vec3 _min = {node->splitEntry[0].min, node->splitEntry[1].min, node->splitEntry[2].min};
            glm::vec3 _max = {node->splitEntry[0].max, node->splitEntry[1].max, node->splitEntry[2].max};
            AABB _sectorAABB = MakeAABB(_min, _max);
            bool _isIntersect = AABBIntersect(aabb, _sectorAABB);

            if (_isInside || _isIntersect) {
                node->gameObjects.push_back(gameObject);
            }

            if (node->gameObjects.size() > 2 && depth < MAX_DEPTH) {
                Split(node, depth);
            }
        }

        static std::vector<GameObject *> GetBroadPhaseCollisions(GameObject *gameObject) {
            std::vector<GameObject *> _gameObjects;
            GetBroadPhaseCollisionsRecursive(gameObject, root, _gameObjects);
            return _gameObjects;
        }

        static void GetBroadPhaseCollisionsRecursive(GameObject *gameObject, Node *node, std::vector<GameObject *> &_gameObjects) {
            if (node->children.size() != 0) {
                for (auto &_childNode: node->children) {
                    GetBroadPhaseCollisionsRecursive(gameObject, _childNode, _gameObjects);
                }
                return;
            }

            for (auto &_gameObject: node->gameObjects) {
                if (_gameObject == gameObject) {
                    for (auto &_gameObject1: node->gameObjects) {
                        if (_gameObject1 == gameObject) {
                            continue;
                        }
                        RigidBodyComponent *_rigidBodyComponentMain;
                        if (!gameObject->TryGetComponent(_rigidBodyComponentMain)) {
                            throw std::runtime_error("RigidBodyComponent not found");
                        }
                        RigidBodyComponent *_itemRigidBodyComponent;
                        if (_gameObject1->TryGetComponent(_itemRigidBodyComponent)) {
                            if (AABBIntersect(_rigidBodyComponentMain->GetAABB(gameObject->transform), _itemRigidBodyComponent->GetAABB(_gameObject1->transform))) {
                                _gameObjects.push_back(_gameObject1);
                            }
                        }
                    }
                    break;
                }
            }
        }

        static void Split(Node *node, int depth) {

            glm::vec3 _mid = {(node->splitEntry[0].min + node->splitEntry[0].max) / 2,
                              (node->splitEntry[1].min + node->splitEntry[1].max) / 2,
                              (node->splitEntry[2].min + node->splitEntry[2].max) / 2};
            //x+,y+,z+
            Node *_child = new Node();
            _child->splitEntry[0] = {_mid.x, node->splitEntry[0].max};
            _child->splitEntry[1] = {_mid.y, node->splitEntry[1].max};
            _child->splitEntry[2] = {_mid.z, node->splitEntry[2].max};
            node->children.push_back(_child);

            //x+,y+,z-
            _child = new Node();
            _child->splitEntry[0] = {_mid.x, node->splitEntry[0].max};
            _child->splitEntry[1] = {_mid.y, node->splitEntry[1].max};
            _child->splitEntry[2] = {node->splitEntry[2].min, _mid.z};
            node->children.push_back(_child);

            //x+,y-,z+
            _child = new Node();
            _child->splitEntry[0] = {_mid.x, node->splitEntry[0].max};
            _child->splitEntry[1] = {node->splitEntry[1].min, _mid.y};
            _child->splitEntry[2] = {_mid.z, node->splitEntry[2].max};
            node->children.push_back(_child);

            //x+,y-,z-
            _child = new Node();
            _child->splitEntry[0] = {_mid.x, node->splitEntry[0].max};
            _child->splitEntry[1] = {node->splitEntry[1].min, _mid.y};
            _child->splitEntry[2] = {node->splitEntry[2].min, _mid.z};
            node->children.push_back(_child);

            //x-,y+,z+
            _child = new Node();
            _child->splitEntry[0] = {node->splitEntry[0].min, _mid.x};
            _child->splitEntry[1] = {_mid.y, node->splitEntry[1].max};
            _child->splitEntry[2] = {_mid.z, node->splitEntry[2].max};
            node->children.push_back(_child);

            //x-,y+,z-
            _child = new Node();
            _child->splitEntry[0] = {node->splitEntry[0].min, _mid.x};
            _child->splitEntry[1] = {_mid.y, node->splitEntry[1].max};
            _child->splitEntry[2] = {node->splitEntry[2].min, _mid.z};
            node->children.push_back(_child);

            //x-,y-,z+
            _child = new Node();
            _child->splitEntry[0] = {node->splitEntry[0].min, _mid.x};
            _child->splitEntry[1] = {node->splitEntry[1].min, _mid.y};
            _child->splitEntry[2] = {_mid.z, node->splitEntry[2].max};
            node->children.push_back(_child);

            //x-,y-,z-
            _child = new Node();
            _child->splitEntry[0] = {node->splitEntry[0].min, _mid.x};
            _child->splitEntry[1] = {node->splitEntry[1].min, _mid.y};
            _child->splitEntry[2] = {node->splitEntry[2].min, _mid.z};
            node->children.push_back(_child);

            auto _gameObjects = std::move(node->gameObjects);
            node->gameObjects.clear();
            for (const auto &_gameObject: _gameObjects) {
                RigidBodyComponent *_rigidBodyComponent;
                if (_gameObject->TryGetComponent(_rigidBodyComponent)) {
                    InsertRecursive(_rigidBodyComponent->GetAABB(_gameObject->transform), _gameObject, node, depth);
                }
            }
        }
    };
}
