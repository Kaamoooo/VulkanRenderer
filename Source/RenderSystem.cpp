#include "../Headers/RenderSystem.h"

namespace Kaamoo {

    //定义constant data,是暂存的且会被销毁
    //constant data对于频繁改变的数据性能较高，然而尺寸有限，同时对于模型合批处理起来比较困难
    //由于Shader中的constant data变量是按16bytes来分别存储的，即不满16字节的变量也要存储16字节，所以此处必须添加alignas(16)来与Shader匹配，表示内存地址是16的倍数
    struct SimplePushConstantData {
        //按对角线初始化
        glm::mat4 transform{1.f};
        alignas(16)glm::vec3 color;
    };

    RenderSystem::RenderSystem(Device &device, VkRenderPass renderPass) : device{device} {
        createPipelineLayout();
        createPipeline(renderPass);
    }

    RenderSystem::~RenderSystem() {
        vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    }

    void RenderSystem::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);
        
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        //指定传给Shader的常量
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(device.device(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout");
        }

    }

    void RenderSystem::createPipeline(VkRenderPass renderPass) {
        PipelineConfigureInfo pipelineConfigureInfo{};
        Pipeline::setDefaultPipelineConfigureInfo(pipelineConfigureInfo);
        pipelineConfigureInfo.renderPass = renderPass;
        pipelineConfigureInfo.pipelineLayout = pipelineLayout;
        pipeline = std::make_unique<Pipeline>(
                device,
                "../Shaders/MyShader.vert.spv",
                "../Shaders/MyShader.frag.spv",
                pipelineConfigureInfo
        );
    }

    void RenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject> &gameObjects,
                                         const Camera &camera) {
        pipeline->bind(commandBuffer);
        
        auto projectionView = camera.getProjectionMatrix()*camera.getViewMatrix();
        
        for (auto &obj: gameObjects) {
            SimplePushConstantData pushConstantData{};
            pushConstantData.color = obj.color;
            pushConstantData.transform = projectionView * obj.transform.mat4();

            vkCmdPushConstants(commandBuffer, pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(SimplePushConstantData),
                               &pushConstantData);
            obj.model->bind(commandBuffer);
            obj.model->draw(commandBuffer);
        }
    }
}