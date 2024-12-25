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
        inline const static glm::vec3 GRAVITY = glm::vec3(0, 0.98f, 0);

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
            if (object.HasMember("useGravity")) {
                m_useGravity = object["useGravity"].GetBool();
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
            float _maxRadius = 0;
            for (auto &_vertex: _vertices) {
                _vertex.position -= _massCenter;
                _min = glm::min(_min, _vertex.position);
                _max = glm::max(_max, _vertex.position);
                _maxRadius = glm::max(_maxRadius, glm::length(_vertex.position));
            }

            m_aabb.min = _min;
            m_aabb.max = _max;
            Insert(GetAABB(gameObject->transform), gameObject);

            m_meshRendererComponent->GetModelPtr()->RefreshVertexBuffer(_vertices);
            m_meshRendererComponent->GetModelPtr()->SetMaxRadius(_maxRadius);
        }

        //Todo: Damn gravity!
        void FixedUpdate(const ComponentUpdateInfo &updateInfo) override {
            auto _massCenter = GetMassCenter(m_transformComponent);
            if (m_useGravity) {
                AddJ(_massCenter, FIXED_UPDATE_INTERVAL * m_totalMass * GRAVITY);
            }
            for (auto &pair: m_momentum) {
                auto _r = std::get<0>(pair) - _massCenter;
                m_velocity += std::get<1>(pair) * m_invMass;
                m_omega += m_invI0 * glm::cross(_r, std::get<1>(pair));
            }
            m_momentum.clear();

            updated = false;
            auto _startTime = std::chrono::high_resolution_clock::now();
            auto _gameObjects = GetBroadPhaseCollisions(updateInfo.gameObject);
            if (!_gameObjects.empty()) {
                for (auto &_gameObjectPair: _gameObjects) {
                    auto _gameObject = _gameObjectPair.first;
                    RigidBodyComponent *_otherRigidBodyComponent;
                    if (!_gameObject->TryGetComponent(_otherRigidBodyComponent)) {
                        throw std::runtime_error("RigidBodyComponent not found");
                    }
                    if (_otherRigidBodyComponent->GetCollisionMap().find(updateInfo.gameObject) != _otherRigidBodyComponent->GetCollisionMap().end()) {
                        continue;
                    }
                    glm::vec3 _collidedFaceNormal{};
                    auto _collidedPoint = GetNarrowPhaseCollision(updateInfo.gameObject, _gameObject, _collidedFaceNormal);
                    if (_collidedPoint.has_value()) {
                        ProcessCollision(_otherRigidBodyComponent, _collidedPoint.value(), _collidedFaceNormal, updateInfo.gameObject, _gameObject);
                    }
                }
            }
            if (glm::length(m_velocity) < EPSILON) {
                m_velocity = glm::vec3(0);
            }
            if (glm::length(m_omega) < EPSILON) {
                m_omega = glm::vec3(0);
            }
            auto _rotationMatrix = m_transformComponent->GetRotationMatrix();
            m_transformComponent->Translate(m_velocity * FIXED_UPDATE_INTERVAL);
            m_transformComponent->Rotate(m_omega * FIXED_UPDATE_INTERVAL);
            m_I0 = _rotationMatrix * m_I0 * glm::transpose(_rotationMatrix);

            gameObjectAABBs[updateInfo.gameObject] = GetAABB(m_transformComponent);
        }

        void LateFixedUpdate(const ComponentUpdateInfo &updateInfo) override {
            if (!updated) {
                UpdateOctree();
                updated = true;
            }
            m_collisionMap.clear();
        }

        //Todo: Static objects do not need to reconstruct

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

        auto GetJ() const {
            return m_momentum;
        }

        void AddJ(glm::vec3 position, glm::vec3 j) {
            m_momentum.push_back(std::make_tuple(position, j));
        }

        std::unordered_map<GameObject *, int> GetCollisionMap() const {
            return m_collisionMap;
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
        TransformComponent *m_transformComponent;
        MeshRendererComponent *m_meshRendererComponent;
        std::unordered_map<GameObject *, int> m_collisionMap;
        AABB m_aabb;

        glm::vec3 m_velocity{0, 0, 0};
        glm::vec3 m_omega{0, 0, 0};
        glm::vec3 m_nativeMassCenter{0};
        std::vector<std::tuple<glm::vec3, glm::vec3>> m_momentum;
        glm::mat3 m_I0;
        glm::mat3 m_invI0;
        float m_totalMass;
        float m_invMass;
        float m_e = 0.7f;
        float m_u = 0.5f;

        bool m_isKinematic = false;
        bool m_useGravity = false;

        void ProcessCollision(RigidBodyComponent *otherRigidBodyComponent, glm::vec3 intersectionPoint, glm::vec3 collidedFaceNormal, GameObject *gameObject, GameObject *otherObject) {
            glm::vec3 _massCenter = GetMassCenter(gameObject->transform);
            glm::vec3 _r = Utils::TruncSmallValues(intersectionPoint - _massCenter, EPSILON);
            glm::vec3 _n = Utils::TruncSmallValues(glm::normalize(collidedFaceNormal), EPSILON);

//Todo: Subtle swizzles
            TransformComponent *_otherTransformComponent = otherObject->transform;
            glm::vec3 _velocityOther = otherRigidBodyComponent->GetVelocity();
            glm::vec3 _omegaOther = otherRigidBodyComponent->GetOmega();
            float _otherInvMass = otherRigidBodyComponent->GetInvMass();
            glm::mat3 _otherInvI0 = otherRigidBodyComponent->GetInvI0(_otherTransformComponent);
            glm::vec3 _rOther =
                    Utils::TruncSmallValues(intersectionPoint - otherRigidBodyComponent->GetMassCenter(otherObject->transform), EPSILON);

            glm::vec3 _relativeVelocity = m_velocity - _velocityOther + glm::cross(m_omega, _r) - glm::cross(_omegaOther, _rOther);
            auto _invI0 = GetInvI0(m_transformComponent);

            if (glm::length(_relativeVelocity) < 0.001f || glm::dot(_relativeVelocity, -_n) < 0) {
                return;
            }

            _relativeVelocity = Utils::TruncVec3(_relativeVelocity, 3);
            glm::vec3 _Vn = glm::dot(_relativeVelocity, _n) * _n;
            glm::vec3 _Vt = _relativeVelocity - _Vn;
            glm::vec3 _t;
            if (glm::length(_Vt) < EPSILON) {
                _t = glm::vec3(0);
            } else {
                _t = glm::normalize(_Vt);
            }

            glm::vec3 _J = glm::vec3(0);

            if (IsKinematic()) {
                _J = -(1 + m_e) * _Vn /
                     (_otherInvMass + glm::dot(_n, glm::cross(_otherInvI0 * glm::cross(_rOther, _n), _rOther)));
                otherRigidBodyComponent->AddJ(intersectionPoint, -_J);
            } else if (otherRigidBodyComponent->IsKinematic()) {
                _J = -(1 + m_e) * _Vn /
                     (m_invMass + glm::dot(_n, glm::cross(_invI0 * glm::cross(_r, _n), _r)));
                AddJ(intersectionPoint, _J);
            } else {
                _J = -(1 + m_e) * _Vn /
                     (m_invMass + _otherInvMass + glm::dot(_n, glm::cross(_invI0 * glm::cross(_r, _n), _r)) + glm::dot(_n, glm::cross(_otherInvI0 * glm::cross(_rOther, _n), _rOther)));
                AddJ(intersectionPoint, _J);
                otherRigidBodyComponent->AddJ(intersectionPoint, -_J);
            }

            m_collisionMap[otherObject] = 1;
        }

        static std::optional<glm::vec3> GetNarrowPhaseCollision(GameObject *main, GameObject *other, glm::vec3 &normal) {
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
            auto &_mainIndices = _mainMeshRendererComponent->GetModelPtr()->GetIndices();
            auto &_otherVertices = _otherMeshRendererComponent->GetModelPtr()->GetVertices();
            auto &_otherIndices = _otherMeshRendererComponent->GetModelPtr()->GetIndices();

            auto getValidTriangles = [](GameObject *object, std::vector<Model::Vertex> &vertices, std::vector<unsigned int> &indices, const AABB &_intersectionAABB) {
                std::vector<Triangle> _validTriangles;
                for (int i = 0; i < indices.size(); i += 3) {
                    glm::vec3 _triangleVertices[3];
                    AABB _triangleAABB;
                    _triangleAABB.min = glm::vec3(FLT_MAX);
                    _triangleAABB.max = glm::vec3(-FLT_MAX);
                    for (int j = 0; j < 3; ++j) {
                        _triangleVertices[j] = object->transform->mat4() * glm::vec4(vertices[indices[i + j]].position, 1);
                        for (int k = 0; k < 3; ++k) {
                            _triangleAABB.min[k] = glm::min(_triangleAABB.min[k], _triangleVertices[j][k]);
                            _triangleAABB.max[k] = glm::max(_triangleAABB.max[k], _triangleVertices[j][k]);
                        }
                    }
                    if (AABBIntersect(_triangleAABB, _intersectionAABB)) {
                        Triangle _triangle;
                        _triangle.vertices[0] = _triangleVertices[0];
                        _triangle.vertices[1] = _triangleVertices[1];
                        _triangle.vertices[2] = _triangleVertices[2];
                        _triangle.normal = glm::normalize(glm::cross(_triangle.vertices[1] - _triangle.vertices[0], _triangle.vertices[2] - _triangle.vertices[0]));
                        _validTriangles.push_back(_triangle);
                    }
                }
                return _validTriangles;
            };
            auto _mainValidTriangles = getValidTriangles(main, _mainVertices, _mainIndices, _intersectionAABB);
            auto _otherValidTriangles = getValidTriangles(other, _otherVertices, _otherIndices, _intersectionAABB);

            glm::vec3 _center = _mainRigidBodyComponent->GetMassCenter(main->transform);
            std::vector<glm::vec3> _intersectionPoints;
            std::unordered_map<glm::vec3, int, Utils::Vec3Hash, Utils::Vec3Equal> _intersectionPointsMap{};
            glm::vec3 _centroid{};
            std::vector<Triangle> _collidedTriangles{};

            for (const auto &_mainValidTriangle: _mainValidTriangles) {
                for (int i = 0; i < 3; ++i) {
                    for (const auto &_otherValidTriangle: _otherValidTriangles) {
                        auto _intersectionPoint = RayIntersectsTriangle(_mainValidTriangle.vertices[i], _mainValidTriangle.vertices[(i + 1) % 3], _otherValidTriangle);
                        if (_intersectionPoint.has_value()) {
                            if (_intersectionPointsMap.find(_intersectionPoint.value()) == _intersectionPointsMap.end()) {
                                _intersectionPointsMap[_intersectionPoint.value()] = 1;
                                _intersectionPoints.push_back(_intersectionPoint.value());
                                _centroid += _intersectionPoint.value();
                            }
                            _collidedTriangles.push_back(_otherValidTriangle);
                        }
                    }
                }
            }
            if (_intersectionPointsMap.empty() || _collidedTriangles.empty()) {
                return {};
            }

            _centroid /= _intersectionPointsMap.size();
            glm::vec3 _u = glm::normalize(_intersectionPoints[0] - _centroid);
            glm::vec3 _v = glm::normalize(glm::cross(_u, glm::cross(_u, glm::normalize(_intersectionPoints[1] - _centroid))));
            std::sort(_intersectionPoints.begin(), _intersectionPoints.end(), [&](const glm::vec3 &a, const glm::vec3 &b) {
                float _angleA = glm::atan(glm::dot(a - _centroid, _v), glm::dot(a - _centroid, _u));
                float _angleB = glm::atan(glm::dot(b - _centroid, _v), glm::dot(b - _centroid, _u));
                return _angleA > _angleB;
            });
            glm::vec3 _averageIntersectionPoint{};
            float totalArea = 0;
            for (int i = 0; i < _intersectionPoints.size(); ++i) {
                auto _p1 = _intersectionPoints[i], _p2 = _intersectionPoints[(i + 1) % _intersectionPoints.size()];
                auto _v1 = _p1 - _centroid, _v2 = _p2 - _centroid;
                float _s = 0.5 * glm::length(glm::cross(_v1, _v2));
                _averageIntersectionPoint += _s * (_centroid + _p1 + _p2) / 3.0f;
                totalArea += _s;
            }
            _averageIntersectionPoint /= totalArea;

            if (totalArea == 0) return {};

            glm::vec3 _averageNormal{};
            for (auto &triangle: _collidedTriangles) {
                _averageNormal += triangle.normal;
            }
            normal = glm::normalize(_averageNormal);
            if (std::isnan(normal.x) || std::isnan(normal.y) || std::isnan(normal.z)) {
                return {};
            }

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
                    _triangleVertices[j] = transformComponent->GetScale() * _vertices[_indices[i + j]].position;
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
//            if (_mass < EPSILON) {
//                _mass = 1000000;
//                _massCenter = glm::vec3(0);
//            }

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
                                                              const glm::vec3 ray_target,
                                                              const Triangle triangle) {
            constexpr float epsilon = std::numeric_limits<float>::epsilon();
            auto ray_vector = ray_target - ray_origin;
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

//            if (t > epsilon) // ray intersection
//            {
//                return glm::vec3(ray_origin + ray_vector * t);
//            } else // This means that there is a line intersection but not a ray intersection.
//                return {};
            if (t < 1 + EPSILON && t > -EPSILON) {
                return glm::vec3(ray_origin + ray_vector * t);
            } else {
                return {};
            }
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
        inline static bool updated = false;
        inline static std::unordered_map<GameObject *, AABB> gameObjectAABBs = {};
        struct SplitEntry {
            float min;
            float max;
        };

        struct Node {
            SplitEntry splitEntry[3];
            std::vector<GameObject *> gameObjects{};
            std::vector<std::shared_ptr<Node>> children{};
        };
        inline static std::shared_ptr<Node> root = nullptr;
        static const int MAX_DEPTH = 4;

        static void InitRoot() {
            if (root == nullptr) {
                root = std::make_shared<Node>();
                root->splitEntry[0] = {-100, +100};
                root->splitEntry[1] = {-100, +100};
                root->splitEntry[2] = {-100, +100};
            }
        }

        static void Insert(AABB aabb, GameObject *gameObject) {
            InitRoot();
            InsertRecursive(aabb, gameObject, root, 0);
        }

//Todo: Simplify the collider of complex mesh.
        static void InsertRecursive(AABB aabb, GameObject *gameObject, std::shared_ptr<Node> node, int depth) {
            glm::vec3 _min = {node->splitEntry[0].min, node->splitEntry[1].min, node->splitEntry[2].min};
            glm::vec3 _max = {node->splitEntry[0].max, node->splitEntry[1].max, node->splitEntry[2].max};
            AABB _sectorAABB = MakeAABB(_min, _max);
            bool _isIntersect = AABBIntersect(aabb, _sectorAABB);

            if (!_isIntersect) return;

            if (!node->children.empty()) {
                for (auto &_childNode: node->children) {
                    InsertRecursive(aabb, gameObject, _childNode, depth + 1);
                }
                return;
            }

            if (_isIntersect) {
                node->gameObjects.push_back(gameObject);
            }

            if (node->gameObjects.size() > 2 && depth < MAX_DEPTH) {
                Split(node, depth);
            }
        }

        static std::unordered_map<GameObject *, int> GetBroadPhaseCollisions(GameObject *gameObject) {
            std::unordered_map<GameObject *, int> _gameObjects;
            GetBroadPhaseCollisionsRecursive(gameObject, root, _gameObjects);
            return _gameObjects;
        }

        static void GetBroadPhaseCollisionsRecursive(GameObject *gameObject, std::shared_ptr<Node> node, std::unordered_map<GameObject *, int> &_gameObjects) {
            if (node->children.size() != 0) {
                for (auto &_childNode: node->children) {
                    GetBroadPhaseCollisionsRecursive(gameObject, _childNode, _gameObjects);
                }
                return;
            }

            for (auto &_gameObject: node->gameObjects) {
                if (_gameObject == gameObject) {
                    for (auto &_gameObject1: node->gameObjects) {
                        if (_gameObject1 == gameObject || _gameObjects.find(_gameObject1) != _gameObjects.end()) {
                            continue;
                        }
                        RigidBodyComponent *_rigidBodyComponentMain;
                        if (!gameObject->TryGetComponent(_rigidBodyComponentMain)) {
                            throw std::runtime_error("RigidBodyComponent not found");
                        }
                        RigidBodyComponent *_itemRigidBodyComponent;
                        if (_gameObject1->TryGetComponent(_itemRigidBodyComponent)) {
                            if (AABBIntersect(_rigidBodyComponentMain->GetAABB(gameObject->transform), _itemRigidBodyComponent->GetAABB(_gameObject1->transform))) {
                                _gameObjects[_gameObject1] = 1;
                            }
                        }
                    }
                    break;
                }
            }
        }

        static void Split(std::shared_ptr<Node> node, int depth) {

            glm::vec3 _mid = {(node->splitEntry[0].min + node->splitEntry[0].max) / 2,
                              (node->splitEntry[1].min + node->splitEntry[1].max) / 2,
                              (node->splitEntry[2].min + node->splitEntry[2].max) / 2};
            //x+,y+,z+
            auto _child = std::make_shared<Node>();
            _child->splitEntry[0] = {_mid.x, node->splitEntry[0].max};
            _child->splitEntry[1] = {_mid.y, node->splitEntry[1].max};
            _child->splitEntry[2] = {_mid.z, node->splitEntry[2].max};
            node->children.push_back(std::move(_child));

            //x+,y+,z-
            _child = std::make_shared<Node>();
            _child->splitEntry[0] = {_mid.x, node->splitEntry[0].max};
            _child->splitEntry[1] = {_mid.y, node->splitEntry[1].max};
            _child->splitEntry[2] = {node->splitEntry[2].min, _mid.z};
            node->children.push_back(std::move(_child));

            //x+,y-,z+
            _child = std::make_shared<Node>();
            _child->splitEntry[0] = {_mid.x, node->splitEntry[0].max};
            _child->splitEntry[1] = {node->splitEntry[1].min, _mid.y};
            _child->splitEntry[2] = {_mid.z, node->splitEntry[2].max};
            node->children.push_back(std::move(_child));

            //x+,y-,z-
            _child = std::make_shared<Node>();
            _child->splitEntry[0] = {_mid.x, node->splitEntry[0].max};
            _child->splitEntry[1] = {node->splitEntry[1].min, _mid.y};
            _child->splitEntry[2] = {node->splitEntry[2].min, _mid.z};
            node->children.push_back(std::move(_child));

            //x-,y+,z+
            _child = std::make_shared<Node>();
            _child->splitEntry[0] = {node->splitEntry[0].min, _mid.x};
            _child->splitEntry[1] = {_mid.y, node->splitEntry[1].max};
            _child->splitEntry[2] = {_mid.z, node->splitEntry[2].max};
            node->children.push_back(std::move(_child));

            //x-,y+,z-
            _child = std::make_shared<Node>();
            _child->splitEntry[0] = {node->splitEntry[0].min, _mid.x};
            _child->splitEntry[1] = {_mid.y, node->splitEntry[1].max};
            _child->splitEntry[2] = {node->splitEntry[2].min, _mid.z};
            node->children.push_back(std::move(_child));

            //x-,y-,z+
            _child = std::make_shared<Node>();
            _child->splitEntry[0] = {node->splitEntry[0].min, _mid.x};
            _child->splitEntry[1] = {node->splitEntry[1].min, _mid.y};
            _child->splitEntry[2] = {_mid.z, node->splitEntry[2].max};
            node->children.push_back(std::move(_child));

            //x-,y-,z-
            _child = std::make_shared<Node>();
            _child->splitEntry[0] = {node->splitEntry[0].min, _mid.x};
            _child->splitEntry[1] = {node->splitEntry[1].min, _mid.y};
            _child->splitEntry[2] = {node->splitEntry[2].min, _mid.z};
            node->children.push_back(std::move(_child));

            auto _gameObjects = std::move(node->gameObjects);
            node->gameObjects.clear();
            for (const auto &_gameObject: _gameObjects) {
                RigidBodyComponent *_rigidBodyComponent;
                if (_gameObject->TryGetComponent(_rigidBodyComponent)) {
                    InsertRecursive(_rigidBodyComponent->GetAABB(_gameObject->transform), _gameObject, node, depth);
                }
            }
        }

        static void UpdateOctree() {
            root = nullptr;
            InitRoot();
            for (auto &_gameObjectAABB: gameObjectAABBs) {
                Insert(_gameObjectAABB.second, _gameObjectAABB.first);
            }
        }
    };
}
