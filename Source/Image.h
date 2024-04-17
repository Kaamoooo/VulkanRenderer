#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC

#include "../External/stb_image.h"

#include <string>
#include <vulkan/vulkan.h>
#include "Device.hpp"
#include "Sampler.h"

namespace Kaamoo {
    const struct ImageCategory{
        std::string Default="Default";
        std::string CubeMap="CubeMap";
    } ImageType;
    class Image {
    public:
        explicit Image(Device &device, std::string imageCategory = ImageType.Default);

        ~Image();

        void createTextureImage(const std::string& path);


        void createImageView();

        void createImageView(VkImageViewCreateInfo createInfo);

        std::shared_ptr<VkDescriptorImageInfo> descriptorInfo(Sampler &sampler);

        void setImage(VkImage vkImage) { this->image = vkImage; }

        VkImage getImage() const { return this->image; };

        static void setDefaultImageCreateInfo(VkImageCreateInfo &defaultCreateInfo);

        void setDefaultImageViewCreateInfo(VkImageViewCreateInfo &createInfo);

        void createImage(VkImageCreateInfo createInfo);

        const VkImageView *getImageView() const;

    private:
        Device &device;
        VkImage image;
        VkImageView imageView;
        VkDeviceMemory imageMemory{};
        int texWidth, texHeight, texChannels;
        
        std::string imageType;
        
        void createDefaultImage(const std::string& path, VkImageCreateInfo createInfo);

        void createCubeMapImage(const std::string &path, VkImageCreateInfo createInfo);
    };


}