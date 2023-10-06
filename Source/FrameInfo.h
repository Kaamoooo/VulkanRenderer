#pragma once

#include "Camera.h"
#include <vulkan/vulkan.h>

namespace Kaamoo{
    struct FrameInfo{
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        Camera &camera;
    };
}