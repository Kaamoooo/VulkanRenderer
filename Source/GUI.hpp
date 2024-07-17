#include <iomanip>
#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_glfw.h"
#include "Imgui/imgui_impl_vulkan.h"

namespace Kaamoo {
    class GUI {
    public:
        GUI() = delete;

        static void Destroy() {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            vkDestroyDescriptorPool(Device::getDeviceSingleton()->device(), imguiDescPool, nullptr);
        }

        static void Init(Renderer &renderer, MyWindow &myWindow) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            auto device = Device::getDeviceSingleton();
            VkDescriptorPoolSize pool_sizes[] =
                    {
                            {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
                            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
                            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
                            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
                            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
                            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
                            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
                            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
                    };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1000;
            pool_info.poolSizeCount = std::size(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            vkCreateDescriptorPool(device->device(), &pool_info, nullptr, &imguiDescPool);

            ImGui::StyleColorsDark();
            ImGuiIO &io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\arial.ttf)", 16.0f);
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = device->getInstance();
            init_info.PhysicalDevice = device->getPhysicalDevice();
            init_info.Device = device->device();
            init_info.QueueFamily = device->getQueueFamilyIndices().graphicsFamily;
            init_info.Queue = device->graphicsQueue();
            init_info.DescriptorPool = imguiDescPool;
            init_info.MinImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
            init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            init_info.Allocator = nullptr;
            init_info.RenderPass = renderer.getSwapChainRenderPass();
            ImGui_ImplGlfw_InitForVulkan(myWindow.getGLFWwindow(), true);
            ImGui_ImplVulkan_Init(&init_info);
            ImGui_ImplVulkan_CreateFontsTexture();

            ImGuiStyle &style = ImGui::GetStyle();
            style.FramePadding = ImVec2(3, 3);
            style.ItemSpacing = ImVec2(0, 6);
            style.WindowPadding = ImVec2(10, 0);
            style.IndentSpacing = 8;
//            style.ItemInnerSpacing = ImVec2(0, 0);
        };

        static void BeginFrame(ImVec2 windowExtent) {
            ImGui_ImplVulkan_SetMinImageCount(SwapChain::MAX_FRAMES_IN_FLIGHT);
            ImGuiIO &io = ImGui::GetIO();
            io.DisplaySize = ImVec2(UI_LEFT_WIDTH, windowExtent.y);
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

/*
 *  | UI_LEFT_WIDTH_2 | UI_LEFT_WIDTH | SCENE_WIDTH     
 *       Inspector        Hierarchy       Scene
 * */

        static void ShowWindow(ImVec2 windowExtent, GameObject::Map *pGameObjectsMap,std::vector<GameObjectDesc>* pGameObjectDescs) {
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoResize;

            ImGui::SetNextWindowPos(ImVec2(UI_LEFT_WIDTH_2, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(UI_LEFT_WIDTH, windowExtent.y), ImGuiCond_Always);

            ImGui::Begin("Scene", nullptr, window_flags);
            if (ImGui::TreeNode("Hierarchy")) {
                if (ImGui::BeginListBox("##Hierarchy", ImVec2(-1, -1))) {
                    for (auto &gameObjectPair: *pGameObjectsMap) {
                        auto &gameObject = gameObjectPair.second;
                        if (ImGui::Selectable(gameObject.getName().c_str())) {
                            selectedId = gameObjectPair.first;
                            bSelected = true;
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::TreePop();
            }
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(UI_LEFT_WIDTH_2, windowExtent.y), ImGuiCond_Always);
            ImGui::Begin("Inspector", nullptr, window_flags);
            if (bSelected) {
                auto &gameObject = pGameObjectsMap->at(selectedId);
                ImGui::Text("Name:");
                ImGui::SameLine(70);
                ImGui::Text(gameObject.getName().c_str());

                //Transform is a special component of game object, so I handle it separately.
                if (ImGui::TreeNode("Transform")) {
                    ImGui::Text("Position:");
                    ImGui::SameLine(90);
                    ImGui::InputFloat3("##Position", &gameObject.transform->translation.x);

                    ImGui::Text("Rotation:");
                    ImGui::SameLine(90);
                    glm::vec3 rotationByDegrees = glm::degrees(gameObject.transform->rotation);
                    ImGui::InputFloat3("##Rotation", &rotationByDegrees.x);
                    gameObject.transform->rotation = glm::radians(rotationByDegrees);

                    ImGui::Text("Scale:");
                    ImGui::SameLine(90);
                    ImGui::InputFloat3("##Scale", &gameObject.transform->scale.x);

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Components")) {
                    for (auto &component: gameObject.getComponents()) {
                        if (component->GetName() == ComponentName::TransformComponent)continue;
                        if (ImGui::TreeNode(component->GetName().c_str())) {
                            component->SetUI(pGameObjectDescs);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::End();
        }

        static void EndFrame(VkCommandBuffer &commandBuffer) {
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            ImGui::EndFrame();
        }

    private:
        inline static VkDescriptorPool imguiDescPool{};
        inline static bool bSelected;
        inline static id_t selectedId;
    };
}