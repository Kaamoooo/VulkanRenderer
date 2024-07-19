#pragma once

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

    class LightComponent : public Component {
    public:

        LightComponent() {
            name = "LightComponent";
        }

        LightComponent(const rapidjson::Value &object) {
            name = "LightComponent";
            auto colorArray = object["color"].GetArray();
            color = glm::vec3(colorArray[0].GetFloat(), colorArray[1].GetFloat(), colorArray[2].GetFloat());
            auto jsonLightTypeStr = object["category"].GetString();
            lightCategory = lightCategoryMap[jsonLightTypeStr];
            lightTypeStr = jsonLightTypeStr;
            lightIntensity = object["intensity"].GetFloat();
            lightIndex = lightNum;
            lightNum++;
        }

        void Update(const ComponentUpdateInfo &updateInfo) override {
            assert(lightIndex < MAX_LIGHT_NUM && "光源数目过多");

            Light light{};
            auto &transform = updateInfo.gameObject->transform;
            if (lightCategory == LightCategory::POINT_LIGHT) {
                auto rotateLight = glm::rotate(glm::mat4{1.f}, updateInfo.frameInfo->frameTime, glm::vec3(0, 1.f, 0));
//                transform->translation = glm::vec3(rotateLight * glm::vec4(transform->translation, 1));
                transform->translation = glm::vec3(glm::vec4(transform->translation, 1));
                light.lightCategory = LightCategory::POINT_LIGHT;
            } else {
                light.lightCategory = LightCategory::DIRECTIONAL_LIGHT;
            }

            light.position = glm::vec4(transform->translation, 1.f);
            auto identity = glm::mat4(1.f);
            auto rotateMatrix = glm::rotate(identity, transform->rotation.y, {0, 1, 0});
            rotateMatrix = glm::rotate(rotateMatrix, transform->rotation.x, {1, 0, 0});
            rotateMatrix = glm::rotate(rotateMatrix, transform->rotation.z, {0, 0, 1});
            light.color = glm::vec4(color, lightIntensity);
            light.direction = rotateMatrix * glm::vec4(0, 0, 1, 0);
            updateInfo.frameInfo->globalUbo.lights[lightIndex] = light;
        }

#ifdef RAY_TRACING
        void SetUI(std::vector<GameObjectDesc>*) override {
            ImGui::Text("Color:");
            ImGui::SameLine(90);
            ImGui::InputFloat3("##Color", &color.x);

            ImGui::Text("Intensity:");
            ImGui::SameLine(90);
            ImGui::InputFloat("##Intensity", &lightIntensity);

            ImGui::Text("Type:");
            ImGui::SameLine(90);
            ImGui::Text(lightTypeStr.c_str());
        }
#else

        void SetUI(Material::Map *) override {
            ImGui::Text("Color:");
            ImGui::SameLine(90);
            ImGui::InputFloat3("##Color", &color.x);

            ImGui::Text("Intensity:");
            ImGui::SameLine(90);
            ImGui::InputFloat("##Intensity", &lightIntensity);

            ImGui::Text("Type:");
            ImGui::SameLine(90);
            ImGui::Text(lightTypeStr.c_str());
        }

#endif

        static int GetLightNum() {
            return lightNum;
        }

    private:
        inline static int lightNum = 0;

        int lightIndex = 0;
        float lightIntensity = 1.0f;
        int lightCategory = 0;
        std::string lightTypeStr;
        glm::vec3 color{};

        std::unordered_map<std::string, int> lightCategoryMap = {
                {"Point",       LightCategory::POINT_LIGHT},
                {"Directional", LightCategory::DIRECTIONAL_LIGHT},
        };


    public:
        const glm::vec3 getColor() const {
            return color;
        }

        int getLightCategory() const {
            return lightCategory;
        }

        float getLightIntensity() const {
            return lightIntensity;
        }

        int getLightIndex() const {
            return lightIndex;
        }

        int getLightNum() {
            return lightNum;
        }
    };
}
