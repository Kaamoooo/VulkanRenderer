#pragma once

#include <memory>
#include <glm/gtc/constants.hpp>
#include "../Pipeline.hpp"
#include "../Device.hpp"
#include "../Model.hpp"
#include "../GameObject.hpp"
#include "../StructureInfos.h"
#include "../Components/MeshRendererComponent.hpp"

namespace Kaamoo {
    class RenderSystem {
    public:
        RenderSystem(Device &device, VkRenderPass renderPass, std::shared_ptr<Material> material);
        
        void Init();

        ~RenderSystem();

        RenderSystem(const RenderSystem &) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;

        virtual void render(FrameInfo &frameInfo);

        template<class T>
        void UpdateGlobalUboBuffer(T &globalUbo, uint32_t frameIndex) {
            m_material->getBufferPointers()[0]->writeToIndex(&globalUbo, frameIndex);
            m_material->getBufferPointers()[0]->flushIndex(frameIndex);
        };


    protected:
        virtual void createPipeline(VkRenderPass renderPass);

        virtual void createPipelineLayout();

        //手动编译Shader，此时读取编译后的文件
        //路径是从可执行文件开始的，并非从根目录
        Device &device;
        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        std::shared_ptr<Material> m_material;
        VkRenderPass m_renderPass;
    };


}