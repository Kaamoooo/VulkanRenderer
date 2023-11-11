#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC

#include "../External/stb_image.h"

#include <string>
#include <vulkan/vulkan.h>
#include "Device.hpp"
#include "Sampler.h"

namespace Kaamoo {
    class Image {
    public:
        Image(Device &device);

        ~Image();

        void createTextureImage(std::string path);

        void createTextureImage(std::string path, VkImageCreateInfo createInfo);

        void createImageView();

        void createImageView(VkImageViewCreateInfo createInfo);

        std::shared_ptr<VkDescriptorImageInfo> descriptorInfo(Sampler &sampler);

        void setImage(VkImage vkImage) { this->image = vkImage; }

        VkImage getImage() const { return this->image; };

        static void setDefaultImageCreateInfo(VkImageCreateInfo &defaultCreateInfo);

        void setDefaultImageViewCreateInfo(VkImageViewCreateInfo &createInfo);

        void createImage(VkImageCreateInfo createInfo);

    private:
        Device &device;
        VkImage image;
        VkImageView imageView;
    public:
        const VkImageView *getImageView() const;

    private:
        VkDeviceMemory imageMemory{};
        int texWidth, texHeight, texChannels;

    };


}