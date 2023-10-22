#include <stdexcept>
#include <memory>
#include "Image.h"
#include "Buffer.h"

namespace Kaamoo {
    void Image::createTextureImage(std::string path, VkImageCreateInfo createInfo) {
        stbi_uc *pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (pixels == nullptr) {
            throw std::runtime_error("Failed to load pixels from given texture path");
        }

        std::unique_ptr<Buffer> stagingBuffer = std::make_unique<Buffer>(device, imageSize, 1,
                                                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffer->map();
        stagingBuffer->writeToBuffer(pixels, imageSize);
        stagingBuffer->unmap();

        stbi_image_free(pixels);

        createInfo.extent.width = texWidth;
        createInfo.extent.height = texHeight;

        device.createImageWithInfo(createInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

        device.transitionImageLayout(image, createInfo.format, createInfo.initialLayout,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        device.copyBufferToImage(stagingBuffer->getBuffer(), image, static_cast<uint32_t>(texWidth),
                                 static_cast<uint32_t>(texHeight), 1);
        device.transitionImageLayout(image, createInfo.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    }


    Image::Image(Device &device) : device{device} {
    }

    Image::~Image() {
        vkDestroyImage(device.device(), image, nullptr);
        vkDestroyImageView(device.device(), imageView, nullptr);
        vkFreeMemory(device.device(), imageMemory, nullptr);
    }

    void Image::setDefaultImageCreateInfo(VkImageCreateInfo &defaultCreateInfo) {
        defaultCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        defaultCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        VkExtent3D extent3D{};
        extent3D.width = 512;
        extent3D.height = 512;
        extent3D.depth = 1;
        defaultCreateInfo.extent = extent3D;
        defaultCreateInfo.mipLevels = 1;
        defaultCreateInfo.arrayLayers = 1;
        defaultCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        defaultCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        defaultCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        defaultCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        defaultCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        defaultCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        defaultCreateInfo.flags = 0;
    }

    void Image::createTextureImageView(VkImageViewCreateInfo imageViewCreateInfo) {
        if (vkCreateImageView(device.device(), &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view");
        }
    }

    void Image::setDefaultImageViewCreateInfo(VkImageViewCreateInfo &imageViewCreateInfo) {
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    }

    void Image::createTextureImage(std::string path) {
        VkImageCreateInfo createInfo{};
        setDefaultImageCreateInfo(createInfo);
        createTextureImage(path, createInfo);
    }

    void Image::createTextureImageView() {
        VkImageViewCreateInfo createInfo{};
        setDefaultImageViewCreateInfo(createInfo);
        createTextureImageView(createInfo);
    }

    VkDescriptorImageInfo Image::descriptorInfo(Sampler &sampler) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = sampler.getSampler();
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return imageInfo;
    }


}
