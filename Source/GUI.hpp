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

            ImGuiStyle& style = ImGui::GetStyle();
            style.FramePadding = ImVec2(0, 0);
            style.ItemSpacing = ImVec2(0, 6); 
            style.WindowPadding = ImVec2(0, 0); 
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

        static void ShowWindow(ImVec2 windowExtent, GameObject::Map *pGameObjectsMap) {
            ImGuiWindowFlags window_flags = 0;
            window_flags |= ImGuiWindowFlags_MenuBar;
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoResize;
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(UI_LEFT_WIDTH, windowExtent.y), ImGuiCond_Always);
            
            if (!ImGui::Begin("Scene", nullptr, window_flags)) {
                ImGui::End();
                return;
            }
            if (ImGui::TreeNode("Hierarchy")) {
                if (ImGui::BeginListBox("##Hierarchy", ImVec2(-1, -1))) {
                    for (auto &gameObjectPair: *pGameObjectsMap) {
                        auto &gameObject = gameObjectPair.second;
                        ImGui::Selectable(gameObject.getName().c_str(), false);
                    }
                    ImGui::EndListBox();
                }
                ImGui::TreePop();
            }
            ImGui::End();
//            ImGui::PopStyleVar(2);
        }

        static void EndFrame(VkCommandBuffer &commandBuffer) {
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            ImGui::EndFrame();
        }

    private:
        inline static VkDescriptorPool imguiDescPool{};
    };
}