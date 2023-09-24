﻿#pragma once

#include "Device.hpp"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Kaamoo {
    class Model {
    public:
        Model(const Model &model) = delete;

        Model &operator=(const Model &model) = delete;

        struct Vertex {
            glm::vec3 position;
            glm::vec3 color;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        };
        
        struct Builder{
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
        };

        Model(Device &device, const std::vector<Vertex> &vertices);
//        Model(Device &device, const Builder& builder);

        ~Model();

        void bind(VkCommandBuffer commandBuffer);

        void draw(VkCommandBuffer commandBuffer);

    private:
        void createVertexBuffers(const std::vector<Vertex> &vertices);

        Device &device;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        uint32_t vertexCount;
    };
}