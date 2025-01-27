#pragma once

#include <cassert>
#include "MyWindow.hpp"
#include "SwapChain.hpp"
#include "Device.hpp"
#include "Image.h"

namespace Kaamoo {
    class Renderer {
    public:
        const float FOV_Y = 50.f;
        const float NEAR_CLIP = 0.1f;
        const float FAR_CLIP = 20.f;
        
        Renderer(MyWindow &, Device &);

        ~Renderer();

        Renderer(const Renderer &) = delete;

        Renderer &operator=(const Renderer &) = delete;

        VkCommandBuffer beginFrame();

        void endFrame();

        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);

        void beginGizmosRenderPass(VkCommandBuffer commandBuffer);

        void beginShadowRenderPass(VkCommandBuffer commandBuffer);

        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

        void endGizmosRenderPass(VkCommandBuffer commandBuffer);

        void endShadowRenderPass(VkCommandBuffer commandBuffer);

        void setShadowMapSynchronization(VkCommandBuffer commandBuffer);

#ifdef RAY_TRACING

        void setDenoiseComputeToPostSynchronization(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        void setDenoiseRtxToComputeSynchronization(VkCommandBuffer commandBuffer, uint32_t imageIndex);

#endif

        [[nodiscard]] std::shared_ptr<VkDescriptorImageInfo> getShadowImageInfo() const {
            return shadowImage->descriptorInfo(*shadowSampler);
        }

        bool getIsFrameStarted() const { return isFrameStarted; }

        [[nodiscard]] VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame is not in progress");
            return commandBuffers[currentFrameIndex];
        }

        const VkRenderPass& getSwapChainRenderPass(){
            return swapChain->getRenderPass();
        }

        const VkRenderPass& getShadowRenderPass() const{
            return shadowRenderPass;
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame is not in progress");
            return currentFrameIndex;
        }

        float getAspectRatio() const {
            return static_cast<float>(myWindow.getCurrentExtent().width - UI_LEFT_WIDTH - UI_LEFT_WIDTH_2) / static_cast<float> (myWindow.getCurrentExtent().height);
        }

        const std::shared_ptr<Image> &getShadowImage() const;

        const std::shared_ptr<Sampler> &getShadowSampler() const;

        const std::shared_ptr<Image> &getOffscreenImageColor(int index) const {
            return m_offscreenImageColors[index];
        }

        const std::shared_ptr<Image> &getViewPosImageColor(int index) const {
            return m_viewPosImageColors[index];
        }

        const std::shared_ptr<Image> &getWorldPosImageColor(int index) const {
            return m_worldPosImage[index];
        };

        const std::shared_ptr<Image> &getDenoisingAccumulationImageColor() const {
            return m_denoisingAccumulationImage;
        };

    private:
        void createCommandBuffers();

        void recreateSwapChain();

        void freeCommandBuffers();

        void freeShadowResources();

        void loadShadow();

        void freeOffscreenResources();

        void loadOffscreenResources();

        void loadGizmos();

        MyWindow &myWindow;
        Device &device;
        std::unique_ptr<SwapChain> swapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex = 0;
        bool isFrameStarted = false;

        bool isCubeMap = true;
        const int ShadowMapResolution = 1024;

        std::shared_ptr<Image> shadowImage;
        std::shared_ptr<Sampler> shadowSampler;
        VkFramebuffer shadowFrameBuffer = VK_NULL_HANDLE;
        VkRenderPass shadowRenderPass = VK_NULL_HANDLE;

        std::vector<std::shared_ptr<Image>> m_offscreenImageColors;
        std::vector<std::shared_ptr<Image>> m_viewPosImageColors;
        std::vector<std::shared_ptr<Image>> m_worldPosImage;
        std::shared_ptr<Image> m_denoisingAccumulationImage;
        std::shared_ptr<Sampler> m_offscreenSampler;
        std::shared_ptr<Image> offscreenImageDepth;

        VkFormat offscreenColorFormat{VK_FORMAT_R32G32B32A32_SFLOAT};
        VkFormat worldPosColorFormat{VK_FORMAT_R32G32B32A32_SFLOAT};
        VkFormat offscreenDepthFormat{VK_FORMAT_D32_SFLOAT};

    };

}