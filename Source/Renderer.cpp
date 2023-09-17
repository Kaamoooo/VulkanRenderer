//
// Created by asus on 2023/9/16.
//

#include "../Headers/Renderer.h"


namespace Kaamoo {


    Renderer::Renderer(MyWindow &window, Device &device1) : myWindow{window}, device{device1} {
        recreateSwapChain();
//        createPipeline();
        createCommandBuffers();
    }

    Renderer::~Renderer() {
        freeCommandBuffers();
    }

    VkCommandBuffer Renderer::beginFrame() {
        assert(!isFrameStarted && "Frame has already started");
        auto result = swapChain->acquireNextImage(&currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return nullptr;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image");
        }
        

        isFrameStarted = true;

        auto commandBuffer = getCurrentCommandBuffer();

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin command buffer");
        }
        return commandBuffer;
    }

    void Renderer::endFrame() {
        assert(isFrameStarted && "Can not call endFrame while frame is not in progress");
        auto commandBuffer = getCurrentCommandBuffer();
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }

        auto result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || myWindow.isWindowResized()) {
            myWindow.resetWindowResizedFlag();
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image");
        }
        isFrameStarted = false;
        currentFrameIndex=(currentFrameIndex+1)%SwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::recreateSwapChain() {
        auto extent = myWindow.getExtent();
        while (extent.width == 0 || extent.height == 0) {
            extent = myWindow.getExtent();
            glfwWaitEvents();
        }
        //等待逻辑设备中的所有命令队列执行完毕
        vkDeviceWaitIdle(device.device());

        if (swapChain == nullptr) {
            swapChain = std::make_unique<SwapChain>(device, extent);
        } else {
            std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
            
            //调用了std::move,将swapChain移动到另一个变量中，而移动之后原有的swapChain变为未定义
            swapChain = std::make_unique<SwapChain>(device, extent,oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*swapChain)){
                throw std::runtime_error("Swap chain's image or depth format has changed");
            }
            
        }
    }

    void Renderer::freeCommandBuffers() {
        //为什么需要显式的传入size？处于安全性考虑，避免隐式操作错误
        vkFreeCommandBuffers(device.device(), device.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()),
                             commandBuffers.data());
        commandBuffers.clear();
    }

    void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Cannot call beginSwapChainRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() &&
               "Cannot begin render pass on command buffer from a different frame");

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = swapChain->getRenderPass();
        renderPassBeginInfo.framebuffer = swapChain->getFrameBuffer(currentImageIndex);

        //Render Area即渲染的矩形区域
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = swapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(myWindow.getExtent().width);
        viewport.height = static_cast<float>(myWindow.getExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChain->getSwapChainExtent();

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    }

    void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Cannot call endSwapChainRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() &&
               "Cannot end render pass on command buffer from a different frame");

        vkCmdEndRenderPass(commandBuffer);
    }

    void Renderer::createCommandBuffers() {
        
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = nullptr;
        commandBufferAllocateInfo.commandPool = device.getCommandPool();
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 3;

        commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT );
        if (vkAllocateCommandBuffers(device.device(), &commandBufferAllocateInfo, commandBuffers.data())!=VK_SUCCESS){
            throw std::runtime_error("Cannot allocate command buffers");
        }
    }


}