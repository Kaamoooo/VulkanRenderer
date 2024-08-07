﻿#pragma once

#include <glm/glm.hpp>
#include "Buffer.h"
#include <memory>
#include <iostream>
#include <unordered_map>

namespace Kaamoo {
    class Model {
    public:

        inline static std::unordered_map<std::string, std::shared_ptr<Model>> models{};

        Model(const Model &model) = delete;

        Model &operator=(const Model &model) = delete;

        inline static const std::string BaseModelsPath = "../Models/";

        struct Vertex {
            //Memory alignment for closest hit shader
            alignas(16) glm::vec3 position;
            alignas(16) glm::vec3 color;
            alignas(16) glm::vec3 normal;
            alignas(16) glm::vec3 smoothedNormal = glm::vec3(0.0f);
            alignas(16) glm::vec2 uv;

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex &other) const {
//                return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
                return position == other.position;
            }
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
            glm::vec3 scaleOrigin;

            void loadModel(const std::string &filePath);
        };

        static std::unique_ptr<Model> createModelFromFile(Device &device, const std::string &filePath);

        Model(Device &device, const Builder &builder);

        void bind(VkCommandBuffer commandBuffer);

        void draw(VkCommandBuffer commandBuffer);

        std::unique_ptr<Buffer> &getVertexBuffer() { return vertexBuffer; }

        std::unique_ptr<Buffer> &getIndexBuffer() { return indexBuffer; }

        uint32_t getPrimitiveCount() const { return indexCount / 3; }

        uint32_t getVertexCount() const { return vertexCount; }

        uint32_t getIndexCount() const { return indexCount; }

        uint32_t getIndexReference() const { return indexReference; }

        std::string SetName(const std::string &name) { return this->name = name; }

        std::string GetName() const { return name; }

    private:
        void createVertexBuffers(const std::vector<Vertex> &vertices);

        void createIndexBuffers(const std::vector<uint32_t> &indices);

#ifdef RAY_TRACING
        const VkBufferUsageFlags flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VkBufferUsageFlags rayTracingFlags =
                flags | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
#endif

        Device &device;

        std::unique_ptr<Buffer> vertexBuffer;
        uint32_t vertexCount{};

        bool hasIndexBuffer = true;
        std::unique_ptr<Buffer> indexBuffer;
        uint32_t indexCount{};

        uint32_t indexReference;

        std::string name;
    };
}