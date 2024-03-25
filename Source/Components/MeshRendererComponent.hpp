#include <string>
#include <glm/vec3.hpp>
#include <glm/fwd.hpp>
#include <glm/detail/type_mat3x3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>
#include <utility>
#include "Component.hpp"
#include "../Model.hpp"

namespace Kaamoo {
    class MeshRendererComponent : public Component {
    public:
        MeshRendererComponent(std::shared_ptr<Model> model, id_t materialID) : model(model), materialId(materialID) {
            name = "MeshRendererComponent";
        }

        id_t GetMaterialID() const { return materialId; }

        std::shared_ptr<Model> GetModelPtr() { return model; }

    private:
        id_t materialId;
        std::shared_ptr<Model> model = nullptr;

    };
}