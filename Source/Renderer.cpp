//
// Created by asus on 2023/9/16.
//

#include <glm/fwd.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Renderer.h"
#include "Image.h"
#include "Application.h"


namespace Kaamoo {

    Renderer::Renderer(MyWindow &window, Device &device1) : myWindow{window}, device{device1} {

        recreateSwapChain();
        createCommandBuffers();
#ifdef RAY_TRACING
        loadOffscreenResources();
#else
        loadShadow();
#endif
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
//            loadShadow();
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
//            loadShadow();
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

        VkClearValue clearValues[2];
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(2);
        renderPassBeginInfo.pClearValues = clearValues;

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
//        renderPassBeginInfo.renderArea.extent = swapChain->getSwapChainExtent();
        renderPassBeginInfo.renderArea.extent.width = ShadowMapResolution;
        renderPassBeginInfo.renderArea.extent.height = ShadowMapResolution;

        VkClearValue clearValue{};
        clearValue.depthStencil.depth = 1;
        clearValue.depthStencil.stencil = 0;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
//        viewport.width = static_cast<float>(myWindow.getExtent().width);
//        viewport.height = static_cast<float>(myWindow.getExtent().height);
        viewport.width = static_cast<float>(ShadowMapResolution);
        viewport.height = static_cast<float>(ShadowMapResolution);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
//        scissor.extent = swapChain->getSwapChainExtent();
        scissor.extent.width = ShadowMapResolution;
        scissor.extent.height = ShadowMapResolution;
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
        commandBufferAllocateInfo.commandBufferCount = SwapChain::MAX_FRAMES_IN_FLIGHT;

        commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateCommandBuffers(device.device(), &commandBufferAllocateInfo, commandBuffers.data()) !=
            VK_SUCCESS) {
            throw std::runtime_error("Cannot allocate command buffers");
        }
    }

    void Renderer::freeShadowResources() {
        if (shadowRenderPass != VK_NULL_HANDLE)
            vkDestroyRenderPass(device.device(), shadowRenderPass, nullptr);
        if (shadowFrameBuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device.device(), shadowFrameBuffer, nullptr);
    }

    void Renderer::loadShadow() {
        freeShadowResources();

        shadowImage = std::make_shared<Image>(device);
        VkImageCreateInfo imageCreateInfo{};
        Image::setDefaultImageCreateInfo(imageCreateInfo);
        VkExtent3D shadowMapExtent{};
//        shadowMapExtent.height = swapChain->getSwapChainExtent().height;
//        shadowMapExtent.width = swapChain->getSwapChainExtent().width;
        shadowMapExtent.height = ShadowMapResolution;
        shadowMapExtent.width = ShadowMapResolution;

        shadowMapExtent.depth = 1;
        if (isCubeMap) {
            imageCreateInfo.arrayLayers = 6;
            imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }
        imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
        imageCreateInfo.extent = shadowMapExtent;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        shadowImage->createImage(imageCreateInfo);

        //create shadow image view
        auto imageViewCreateInfo = std::make_shared<VkImageViewCreateInfo>();
        shadowImage->setDefaultImageViewCreateInfo(*imageViewCreateInfo);
        if (isCubeMap) {
            imageViewCreateInfo->subresourceRange.layerCount = 6;
            imageViewCreateInfo->viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        }
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

        VkRenderPassMultiviewCreateInfo multiviewCreateInfo{};
        multiviewCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
        multiviewCreateInfo.subpassCount = 1;
        //All 6 faces of the cube map
        uint32_t viewMask = 0b00111111;
        multiviewCreateInfo.pViewMasks = &viewMask;
        multiviewCreateInfo.correlationMaskCount = 0;
        multiviewCreateInfo.pCorrelationMasks = nullptr;

        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.pNext = &multiviewCreateInfo;
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
//        framebufferCreateInfo.width = swapChain->getSwapChainExtent().width;
//        framebufferCreateInfo.height = swapChain->getSwapChainExtent().height;

        framebufferCreateInfo.width = ShadowMapResolution;
        framebufferCreateInfo.height = ShadowMapResolution;

        framebufferCreateInfo.layers = 1;
        framebufferCreateInfo.flags = 0;

        if (vkCreateFramebuffer(device.device(), &framebufferCreateInfo, nullptr, &shadowFrameBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create SHADOW frame buffer");
        }
    }

    void Renderer::freeOffscreenResources() {
        if (offscreenRenderPass != VK_NULL_HANDLE)
            vkDestroyRenderPass(device.device(), offscreenRenderPass, nullptr);
        if (offscreenFrameBuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device.device(), offscreenFrameBuffer, nullptr);
    }

    void Renderer::loadOffscreenResources() {
        freeOffscreenResources();
        {
            //Color
            offscreenImageColor = std::make_shared<Image>(device);
            VkImageCreateInfo imageCreateInfo{};
            Image::setDefaultImageCreateInfo(imageCreateInfo);
            imageCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                                    | VK_IMAGE_USAGE_STORAGE_BIT;
            VkExtent3D imageExtent{};
            imageExtent.height = swapChain->getSwapChainExtent().height;
            imageExtent.width = swapChain->getSwapChainExtent().width;
            imageExtent.depth = 1;
            imageCreateInfo.extent = imageExtent;
            offscreenImageColor->createImage(imageCreateInfo);

            VkImageViewCreateInfo imageViewCreateInfo{};
            offscreenImageColor->setDefaultImageViewCreateInfo(imageViewCreateInfo);
            imageViewCreateInfo.format = imageCreateInfo.format;
            offscreenImageColor->createImageView(imageViewCreateInfo);

            Sampler offscreenSampler(device);
            offscreenSampler.createTextureSampler();
            offscreenImageColor->sampler = offscreenSampler.getSampler();

            //depth
            offscreenImageDepth = std::make_shared<Image>(device);
            imageCreateInfo.format = VK_FORMAT_X8_D24_UNORM_PACK32;
            imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageCreateInfo.extent = imageExtent;
            offscreenImageDepth->createImage(imageCreateInfo);

            imageViewCreateInfo.format = imageCreateInfo.format;
            imageViewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
            offscreenImageDepth->createImageView(imageViewCreateInfo);
        }

        {
            device.transitionImageLayout(offscreenImageColor->getImage(),
                                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                         {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
            device.transitionImageLayout(offscreenImageDepth->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                         {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1});
        }

        {

            std::vector<VkAttachmentDescription> attachmentDescriptions(2);
            attachmentDescriptions[0].format = offscreenColorFormat;
            attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_GENERAL;
            attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachmentDescriptions[1].format = offscreenDepthFormat;
            attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpassDescription[1];
            subpassDescription[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription[0].colorAttachmentCount = 1;
            subpassDescription[0].pColorAttachments = &colorAttachmentRef;
            subpassDescription[0].pDepthStencilAttachment = &depthAttachmentRef;
            subpassDescription[0].flags = 0;
            subpassDescription[0].inputAttachmentCount = 0;
            subpassDescription[0].pInputAttachments = nullptr;
            subpassDescription[0].pResolveAttachments = nullptr;
            subpassDescription[0].preserveAttachmentCount = 0;
            subpassDescription[0].pPreserveAttachments = nullptr;

            VkSubpassDependency subpassDependency{};
            subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            subpassDependency.dstSubpass = 0;
            subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpassDependency.srcAccessMask = 0;
            subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo renderPassCreateInfo{};

            renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
            renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = subpassDescription;
            renderPassCreateInfo.dependencyCount = 1;
            renderPassCreateInfo.pDependencies = &subpassDependency;
            renderPassCreateInfo.flags = 0;

            if (vkCreateRenderPass(device.device(), &renderPassCreateInfo, nullptr, &offscreenRenderPass) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create OFFSCREEN render pass");
            }
        }

        //create frame buffer
        {
            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = offscreenRenderPass;
            std::vector<VkImageView> attachments = {*(offscreenImageColor->getImageView()),
                                                    *(offscreenImageDepth->getImageView())};
            framebufferCreateInfo.attachmentCount = attachments.size();
            framebufferCreateInfo.pAttachments = attachments.data();
            framebufferCreateInfo.width = swapChain->getSwapChainExtent().width;
            framebufferCreateInfo.height = swapChain->getSwapChainExtent().height;
            framebufferCreateInfo.layers = 1;

            if (vkCreateFramebuffer(device.device(), &framebufferCreateInfo, nullptr, &offscreenFrameBuffer) !=
                VK_SUCCESS) {
                throw std::runtime_error("failed to create OFFSCREEN frame buffer");
            }
        }
    }

    const std::shared_ptr<Image> &Renderer::getShadowImage() const {
        return shadowImage;
    }

    const std::shared_ptr<Sampler> &Renderer::getShadowSampler() const {
        return shadowSampler;
    }

    void Renderer::setShadowMapSynchronization(VkCommandBuffer commandBuffer) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = getShadowImage()->getImage();

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange = subresourceRange;

        VkPipelineStageFlagBits srcStage, dstStage;

        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = static_cast<VkPipelineStageFlagBits>(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;


        vkCmdPipelineBarrier(commandBuffer,
                             srcStage, dstStage,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

    }

    const std::shared_ptr<Image> &Renderer::getOffscreenImageColor() const {
        return offscreenImageColor;
    }
}