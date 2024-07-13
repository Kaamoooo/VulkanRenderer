#pragma once

#define RAY_TRACING

#include "MyWindow.hpp"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Kaamoo {

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;

        bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };

    class Device {
    public:
        const bool enableValidationLayers = true;
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceProperties2 properties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

        Device(MyWindow &window);

        ~Device();

        Device(const Device &) = delete;

        void operator=(const Device &) = delete;

        Device(Device &&) = delete;

        Device &operator=(Device &&) = delete;

        static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);

        static Device *getDeviceSingleton() { return deviceSingleton; }

        VkInstance getInstance() { return instance; }

        VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }

        QueueFamilyIndices getQueueFamilyIndices() { return indices; }

        MyWindow &getWindow() { return window; }

        VkCommandPool getCommandPool() { return commandPool; }

        VkDevice device() { return device_; }

        VkSurfaceKHR surface() { return surface_; }

        VkQueue graphicsQueue() { return graphicsQueue_; }

        VkQueue presentQueue() { return presentQueue_; }
        
        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

        VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);

        VkCommandBuffer beginSingleTimeCommands();

        void endSingleTimeCommands(VkCommandBuffer &commandBuffer);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

        void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);

        void createImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

        VkPipelineStageFlagBits pipelineStageForLayout(VkImageLayout layout);

        VkAccessFlags accessFlagsForImageLayout(VkImageLayout layout);


#ifdef RAY_TRACING
        VkPhysicalDeviceExternalMemoryHostPropertiesEXT externalMemoryHostProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

        VkDeviceAddress getAccelerationStructureAddressKHR(VkAccelerationStructureKHR &accelerationStructure) {
            VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
            addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            addressInfo.accelerationStructure = accelerationStructure;
            return pfn_vkGetAccelerationStructureDeviceAddressKHR(device_, &addressInfo);
        }

#endif
    private:
        inline static Device *deviceSingleton;
        VkInstance instance;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        QueueFamilyIndices indices;

        VkDebugUtilsMessengerEXT debugMessenger;
        MyWindow &window;
        VkCommandPool commandPool;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        VkQueue presentQueue_;


        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                            VK_KHR_MULTIVIEW_EXTENSION_NAME,
                                                            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
//                                                            VK_NV_RAY_TRACING_VALIDATION_EXTENSION_NAME,
//                                                            VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                                            VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
                                                            VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME,
                                                            VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME
        };


        void createInstance();

        void setupDebugMessenger();

        void createSurface();

        void pickPhysicalDevice();

        void createLogicalDevice();

        void createCommandPool();

        bool isDeviceSuitable(VkPhysicalDevice device);

        std::vector<const char *> getRequiredExtensions();

        bool checkValidationLayerSupport();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

        void hasGflwRequiredInstanceExtensions();

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        void loadExtensionFunctions();


    public:
#ifdef RAY_TRACING
        inline static PFN_vkBuildAccelerationStructuresKHR pfn_vkBuildAccelerationStructuresKHR = 0;
        inline static PFN_vkCmdBuildAccelerationStructuresIndirectKHR pfn_vkCmdBuildAccelerationStructuresIndirectKHR = 0;
        inline static PFN_vkCmdBuildAccelerationStructuresKHR pfn_vkCmdBuildAccelerationStructuresKHR = 0;
        inline static PFN_vkCmdCopyAccelerationStructureKHR pfn_vkCmdCopyAccelerationStructureKHR = 0;
        inline static PFN_vkCmdCopyAccelerationStructureToMemoryKHR pfn_vkCmdCopyAccelerationStructureToMemoryKHR = 0;
        inline static PFN_vkCmdCopyMemoryToAccelerationStructureKHR pfn_vkCmdCopyMemoryToAccelerationStructureKHR = 0;
        inline static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pfn_vkCmdWriteAccelerationStructuresPropertiesKHR = 0;
        inline static PFN_vkCopyAccelerationStructureKHR pfn_vkCopyAccelerationStructureKHR = 0;
        inline static PFN_vkCopyAccelerationStructureToMemoryKHR pfn_vkCopyAccelerationStructureToMemoryKHR = 0;
        inline static PFN_vkCopyMemoryToAccelerationStructureKHR pfn_vkCopyMemoryToAccelerationStructureKHR = 0;
        inline static PFN_vkCreateAccelerationStructureKHR pfn_vkCreateAccelerationStructureKHR = 0;
        inline static PFN_vkDestroyAccelerationStructureKHR pfn_vkDestroyAccelerationStructureKHR = 0;
        inline static PFN_vkGetAccelerationStructureBuildSizesKHR pfn_vkGetAccelerationStructureBuildSizesKHR = 0;
        inline static PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_vkGetAccelerationStructureDeviceAddressKHR = 0;
        inline static PFN_vkGetDeviceAccelerationStructureCompatibilityKHR pfn_vkGetDeviceAccelerationStructureCompatibilityKHR = 0;
        inline static PFN_vkWriteAccelerationStructuresPropertiesKHR pfn_vkWriteAccelerationStructuresPropertiesKHR = 0;

        inline static PFN_vkCmdSetRayTracingPipelineStackSizeKHR pfn_vkCmdSetRayTracingPipelineStackSizeKHR = 0;
        inline static PFN_vkCmdTraceRaysIndirectKHR pfn_vkCmdTraceRaysIndirectKHR = 0;
        inline static PFN_vkCmdTraceRaysKHR pfn_vkCmdTraceRaysKHR = 0;
        inline static PFN_vkCreateRayTracingPipelinesKHR pfn_vkCreateRayTracingPipelinesKHR = 0;
        inline static PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = 0;
        inline static PFN_vkGetRayTracingShaderGroupHandlesKHR pfn_vkGetRayTracingShaderGroupHandlesKHR = 0;
        inline static PFN_vkGetRayTracingShaderGroupStackSizeKHR pfn_vkGetRayTracingShaderGroupStackSizeKHR = 0;
#endif
    };

}