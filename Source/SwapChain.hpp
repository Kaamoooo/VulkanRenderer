#pragma once

#include "Device.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <memory>

namespace Kaamoo {

    class SwapChain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
        static constexpr bool ENABLE_SHADOW = true;

        ~SwapChain();

        SwapChain(Device &deviceRef, VkExtent2D windowExtent);

        SwapChain(Device &deviceRef, VkExtent2D windowExtent, std::shared_ptr<SwapChain> previousSwapChain);

        SwapChain(const SwapChain &) = delete;

        void operator=(const SwapChain &) = delete;

        VkFramebuffer getFrameBuffer(int index) { return swapChainFrameBuffers[index]; }

        VkRenderPass& getRenderPass() { return renderPass; }

        VkRenderPass getGizmosRenderPass() { return m_gizmosRenderPass; }

        VkImageView getImageView(int index) { return swapChainImageViews[index]; }

        size_t imageCount() { return swapChainImages.size(); }

        VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }

        VkExtent2D getSwapChainExtent() { return swapChainExtent; }

        uint32_t width() { return swapChainExtent.width; }

        uint32_t height() { return swapChainExtent.height; }

        VkFormat findDepthFormat();

        VkResult acquireNextImage(uint32_t *imageIndex);

        VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

        bool compareSwapFormats(const SwapChain &swapChain) const {
            return swapChain.swapChainImageFormat == swapChainImageFormat &&
                   swapChain.swapChainDepthFormat == swapChainDepthFormat;
        }

    private:
        void init();

        void createSwapChain();

        void createImageViews();

        void createDepthResources();

        void createRenderPass();

        void createGizmosRenderPass();

        void createFramebuffers();

        void createSyncObjects();

        // Helper functions
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(
                const std::vector<VkSurfaceFormatKHR> &availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(
                const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;

        std::vector<VkFramebuffer> swapChainFrameBuffers;
        VkRenderPass renderPass;
        VkRenderPass m_gizmosRenderPass;

        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemories;
        std::vector<VkImageView> depthImageViews;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;

        Device &device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;

        std::shared_ptr<SwapChain> oldSwapChain;
    };

}  // namespace lve
