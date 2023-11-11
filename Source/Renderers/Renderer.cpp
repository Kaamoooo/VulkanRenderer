//
// Created by asus on 2023/9/16.
//

#include <glm/fwd.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Renderer.h"
#include "../Image.h"
#include "../Application.hpp"


namespace Kaamoo {


    Renderer::Renderer(MyWindow &window, Device &device1) : myWindow{window}, device{device1} {

        recreateSwapChain();
        createCommandBuffers();
        loadShadow();
    }

    void Renderer::freeShadowResources() {
        if (shadowRenderPass != VK_NULL_HANDLE)
            vkDestroyRenderPass(device.device(), shadowRenderPass, nullptr);
        if (shadowFrameBuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device.device(), shadowFrameBuffer, nullptr);
    }

    Renderer::~Renderer() {
        freeCommandBuffers();
        freeShadowResources();
    }

    VkCommandBuffer Renderer::beginFrame() {
        assert(!isFrameStarted && "Frame has already started");
        auto result = swapChain->acquireNextImage(&currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            loadShadow();
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
            loadShadow();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image");
        }
        isFrameStarted = false;
        currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
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
            swapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*swapChain)) {
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
        assert(isFrameStarted && "Cannot call beginShadowRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() &&
               "Cannot begin renderShadow pass on command buffer from a different frame");

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

    void Renderer::beginShadowRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Cannot call beginShadowRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() &&
               "Cannot begin renderShadow pass on command buffer from a different frame");

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = shadowRenderPass;
        renderPassBeginInfo.framebuffer = shadowFrameBuffer;

        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = swapChain->getSwapChainExtent();

        VkClearValue clearValue{};
        clearValue.depthStencil.depth = 1;
        clearValue.depthStencil.stencil = 0;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;

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


    void Renderer::endShadowRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Cannot call endShadowRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() &&
               "Cannot end renderShadow pass on command buffer from a different frame");

        vkCmdEndRenderPass(commandBuffer);
    }


    void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Cannot call endShadowRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() &&
               "Cannot end renderShadow pass on command buffer from a different frame");

        vkCmdEndRenderPass(commandBuffer);
    }

    void Renderer::createCommandBuffers() {

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = nullptr;
        commandBufferAllocateInfo.commandPool = device.getCommandPool();
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 3;

        commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateCommandBuffers(device.device(), &commandBufferAllocateInfo, commandBuffers.data()) !=
            VK_SUCCESS) {
            throw std::runtime_error("Cannot allocate command buffers");
        }
    }

    void Renderer::loadShadow() {
        freeShadowResources();

        //create shadow image
        shadowImage = std::make_unique<Image>(device);
        VkImageCreateInfo imageCreateInfo{};
        Image::setDefaultImageCreateInfo(imageCreateInfo);
        VkExtent3D shadowMapExtent{};
        shadowMapExtent.height = swapChain->getSwapChainExtent().height;
        shadowMapExtent.width = swapChain->getSwapChainExtent().width;
        shadowMapExtent.depth = 1;
        imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
        imageCreateInfo.extent = shadowMapExtent;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        shadowImage->createImage(imageCreateInfo);

        //create shadow image view
        auto imageViewCreateInfo=std::make_shared<VkImageViewCreateInfo>();
        shadowImage->setDefaultImageViewCreateInfo(*imageViewCreateInfo);
        imageViewCreateInfo->format = VK_FORMAT_D32_SFLOAT;
        imageViewCreateInfo->components.r = VK_COMPONENT_SWIZZLE_R;
        imageViewCreateInfo->components.g = VK_COMPONENT_SWIZZLE_G;
        imageViewCreateInfo->components.b = VK_COMPONENT_SWIZZLE_B;
        imageViewCreateInfo->components.a = VK_COMPONENT_SWIZZLE_A;
        imageViewCreateInfo->subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        shadowImage->createImageView(*imageViewCreateInfo);

        //create shadow sampler
        shadowSampler = std::make_shared<Sampler>(device);
        shadowSampler->createTextureSampler();

        
        //create shadow pass
        VkAttachmentDescription attachmentDescriptions[2];
        attachmentDescriptions[0].format = VK_FORMAT_D32_SFLOAT;
        attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescriptions[0].flags = 0;
        attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription[1];
        subpassDescription[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription[0].flags = 0;
        subpassDescription[0].inputAttachmentCount = 0;
        subpassDescription[0].pInputAttachments = nullptr;
        subpassDescription[0].colorAttachmentCount = 0;
        subpassDescription[0].pColorAttachments = nullptr;
        subpassDescription[0].pResolveAttachments = nullptr;
        subpassDescription[0].pDepthStencilAttachment = &depthAttachmentRef;
        subpassDescription[0].preserveAttachmentCount = 0;
        subpassDescription[0].pPreserveAttachments = nullptr;

        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.pNext = nullptr;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = attachmentDescriptions;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = subpassDescription;
        renderPassCreateInfo.dependencyCount = 0;
        renderPassCreateInfo.pDependencies = nullptr;
        renderPassCreateInfo.flags = 0;

        if (vkCreateRenderPass(device.device(), &renderPassCreateInfo, nullptr, &shadowRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create SHADOW renderShadow pass");
        }

        //create frame buffer
        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.renderPass = shadowRenderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = shadowImage->getImageView();
        framebufferCreateInfo.width = swapChain->getSwapChainExtent().width;
        framebufferCreateInfo.height = swapChain->getSwapChainExtent().height;
        framebufferCreateInfo.layers = 1;
        framebufferCreateInfo.flags = 0;

        if (vkCreateFramebuffer(device.device(), &framebufferCreateInfo, nullptr, &shadowFrameBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create SHADOW frame buffer");
        }
    }

    const std::shared_ptr<Image> & Renderer::getShadowImage() const {
        return shadowImage;
    }

    const std::shared_ptr<Sampler> &Renderer::getShadowSampler() const {
        return shadowSampler;
    }
}