#pragma once

#include <vulkan/vulkan.h>
#include "Device.hpp"

namespace Kaamoo {
    class Sampler {
    public:
        //Todo: Make a sampler pool
        Sampler(Device &device) : device{device} {};

        ~Sampler();

        void createTextureSampler();

        void createTextureSampler(VkSamplerCreateInfo createInfo);

        void setDefaultSamplerCreateInfo(VkSamplerCreateInfo &createInfo) const;

        VkSampler getSampler() const {
            return sampler;
        }

    private:
        VkSampler sampler;
        Device &device;
    };

}