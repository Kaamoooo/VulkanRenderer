#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC

#include "../External/stb_image.h"

#include <string>
#include <vulkan/vulkan.h>
#include "Device.hpp"

namespace Kaamoo {
    class Image {
    public:
        Image(Device &device);
        Image(Device &device,VkImageCreateInfo&);

        ~Image();

        void createTextureImage(std::string path);
        static void setDefaultImageCreateInfo(VkImageCreateInfo& defaultCreateInfo);
        
    private:
        Device &device;
        VkImage image;
        VkDeviceMemory textureImageMemory;
        VkImageCreateInfo createInfo{};
        VkDeviceMemory imageMemory{};
        int texWidth, texHeight, texChannels;
        
    };


}