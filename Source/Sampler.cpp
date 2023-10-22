#include "Sampler.h"
#include "Device.hpp"

namespace Kaamoo{


    void Sampler::createTextureSampler() {
        VkSamplerCreateInfo defaultCreateInfo{};
        setDefaultSamplerCreateInfo(defaultCreateInfo);
        createTextureSampler(defaultCreateInfo);
    }

    void Sampler::createTextureSampler(VkSamplerCreateInfo createInfo) {
        if (vkCreateSampler(device.device(),&createInfo, nullptr,&sampler)!=VK_SUCCESS){
            throw std::runtime_error("failed to create sampler");
        }
    }

    void Sampler::setDefaultSamplerCreateInfo(VkSamplerCreateInfo &createInfo) {
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.magFilter = VK_FILTER_LINEAR;
        createInfo.minFilter = VK_FILTER_LINEAR;
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        createInfo.anisotropyEnable = VK_TRUE;
        createInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        //使用归一化的UV坐标
        createInfo.unnormalizedCoordinates = VK_FALSE;
        createInfo.compareEnable = VK_FALSE;
        createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.mipLodBias = 0.0f;
        createInfo.minLod = 0.0f;
        createInfo.maxLod = 0.0f;

    }

    Sampler::~Sampler() {
        vkDestroySampler(device.device(),sampler, nullptr);
    }
}