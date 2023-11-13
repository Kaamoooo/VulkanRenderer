#pragma once

#include <cassert>
#include "../MyWindow.hpp"
#include "../SwapChain.hpp"
#include "../Device.hpp"
#include "../Image.h"

namespace Kaamoo {
    class Renderer {
    public:
        Renderer(MyWindow &, Device &);

        ~Renderer();

        Renderer(const Renderer &) = delete;

        Renderer &operator=(const Renderer &) = delete;

        VkCommandBuffer beginFrame();

        void endFrame();

        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);

        void beginShadowRenderPass(VkCommandBuffer commandBuffer);

        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

        void endShadowRenderPass(VkCommandBuffer commandBuffer);
        
        void setShadowMapSynchronization(VkCommandBuffer commandBuffer);

        [[nodiscard]] std::shared_ptr<VkDescriptorImageInfo> getShadowImageInfo() const {
            return shadowImage->descriptorInfo(*shadowSampler);
        }

        bool getIsFrameStarted() const { return isFrameStarted; }

        [[nodiscard]] VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame is not in progress");
            return commandBuffers[currentFrameIndex];
        }

        VkRenderPass getSwapChainRenderPass() {
            return swapChain->getRenderPass();
        }

        VkRenderPass getShadowRenderPass() {
            return shadowRenderPass;
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame is not in progress");
            return currentFrameIndex;
        }

        float getAspectRatio() const {
            return swapChain->extentAspectRatio();
        }

    private:
        void createCommandBuffers();

        void recreateSwapChain();

        void freeCommandBuffers();

        void freeShadowResources();

        void loadShadow();

        MyWindow &myWindow;
        Device &device;
        std::unique_ptr<SwapChain> swapChain;
        std::vector<VkCommandBuffer> commandBuffers;

    private:

        uint32_t currentImageIndex;
        int currentFrameIndex = 0;
        bool isFrameStarted = false;

        std::shared_ptr<Image> shadowImage;
    public:
        const std::shared_ptr<Image> & getShadowImage() const;

        const std::shared_ptr<Sampler> &getShadowSampler() const;

    private:
        std::shared_ptr<Sampler> shadowSampler;
        VkFramebuffer shadowFrameBuffer = VK_NULL_HANDLE;
        VkRenderPass shadowRenderPass = VK_NULL_HANDLE;
    };


}