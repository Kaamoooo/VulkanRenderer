#pragma once

#include "MeshRendererComponent.hpp"

namespace Kaamoo {
    class RigidBodyComponent : public Component {
    public:
        RigidBodyComponent() {
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
            MeshComputeInertia(m_meshRendererComponent->GetModelPtr().get(), 1.0f, &_I0, &_massCenter, &_totalMass);

            m_invI0 = glm::inverse(_I0);
            m_invMass = 1.0f / _totalMass;
            
            auto& _vertices = m_meshRendererComponent->GetModelPtr()->GetVertices();
            for (auto &_vertex : _vertices) {
                _vertex.position -= _massCenter;
            }
            m_meshRendererComponent->GetModelPtr()->RefreshVertexBuffer(_vertices);
        }

        void FixedUpdate(const ComponentUpdateInfo &updateInfo) override {
            m_transformComponent->Translate(m_velocity * FIXED_UPDATE_INTERVAL);
            auto _rotationMatrix = m_transformComponent->GetRotationMatrix();
            glm::vec3 _omega = _rotationMatrix * m_invI0 * glm::transpose(_rotationMatrix) * m_angularMomentum;
            m_transformComponent->Rotate(_omega * FIXED_UPDATE_INTERVAL);
        }

        static void MeshComputeInertia(Model *model, float density, glm::mat3 *I0, glm::vec3 *massCenter, float *totalMass) {
            auto _indices = model->GetIndices();
            auto _vertices = model->GetVertices();
            assert(_indices.size() % 3 == 0 && "Indices size must be a multiple of 3");

            float _mass = 0;
            glm::vec3 _massCenter{0};
            float _Ia = 0, _Ib = 0, _Ic = 0, _Iap = 0, _Ibp = 0, _Icp = 0;
            for (int i = 0; i < _indices.size(); i += 3) {
                glm::vec3 _triangleVertices[3];
                for (int j = 0; j < 3; j++) {
                    _triangleVertices[j] = _vertices[_indices[i + j]].position;
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

                _massCenter += _tetMass * _tetMassCenter;
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

            *I0 = _I;
            *massCenter = _massCenter;
            *totalMass = _mass;
        }

    private:
        TransformComponent *m_transformComponent;
        MeshRendererComponent *m_meshRendererComponent;
        glm::vec3 m_velocity{0, 0, 0};
        glm::vec3 m_massCenter{0};
        glm::vec3 m_angularMomentum{0.5, 0, 0};

        glm::mat3 m_invI0;
        float m_invMass;

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
    };
}
