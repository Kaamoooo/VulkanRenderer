﻿#pragma  once

#include "RenderSystem.h"

namespace Kaamoo {
    class GrassSystem : public RenderSystem {
    public:
        GrassSystem(Device &device, VkRenderPass renderPass, std::shared_ptr<Material> material) ;

    private:
        void createPipelineLayout() override;

        void createPipeline(VkRenderPass renderPass) override;

        void render(FrameInfo &frameInfo) override;
      
        GameObject *moveObject= nullptr;
    };
}


