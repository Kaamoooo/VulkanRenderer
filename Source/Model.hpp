#pragma once

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
            float maxRadius = 0.0f;
            void loadModel(const std::string &filePath);
        };

        static std::unique_ptr<Model> createModelFromFile(Device &device, const std::string &filePath) {
            Builder builder;
            builder.loadModel(filePath);
            return std::make_unique<Model>(device, builder);
        }

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

        std::vector<Vertex> &GetVertices() { return m_vertices; }

        std::vector<uint32_t> &GetIndices() { return m_indices; }
        
        void RefreshVertexBuffer(const std::vector<Vertex> &vertices){
            vertexCount = static_cast<uint32_t>(vertices.size());

            uint32_t vertexSize = sizeof(vertices[0]);
            uint32_t bufferSize = vertexSize * vertexCount;

            Buffer stagingBuffer(device, vertexSize, vertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            stagingBuffer.map();
            stagingBuffer.writeToBuffer((void *) vertices.data());
            
            device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
        }
        
        void createVertexBuffers(const std::vector<Vertex> &vertices){
            vertexCount = static_cast<uint32_t>(vertices.size());

            assert(vertexCount >= 3 && "vertex count must be at least 3");
            uint32_t vertexSize = sizeof(vertices[0]);
            uint32_t bufferSize = vertexSize * vertexCount;

            Buffer stagingBuffer(device, vertexSize, vertexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            stagingBuffer.map();
            stagingBuffer.writeToBuffer((void *) vertices.data());

#ifdef RAY_TRACING
            vertexBuffer = std::make_unique<Buffer>(
                device, vertexSize, vertexCount,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | rayTracingFlags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
#else
            vertexBuffer = std::make_unique<Buffer>(
                    device, vertexSize, vertexCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
#endif
            device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
        }

        void createIndexBuffers(const std::vector<uint32_t> &indices);
        
        float GetMaxRadius() const { return m_maxRadius; }
        
        void SetMaxRadius(float maxRadius) { m_maxRadius = maxRadius; }

    private:

#ifdef RAY_TRACING
        const VkBufferUsageFlags flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        const VkBufferUsageFlags rayTracingFlags =
                flags | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
#endif

        Device &device;
        std::string name;

        std::unique_ptr<Buffer> vertexBuffer;
        uint32_t vertexCount{};

        bool hasIndexBuffer = true;
        std::unique_ptr<Buffer> indexBuffer;
        uint32_t indexCount{};

        uint32_t indexReference;

        std::vector<Vertex> m_vertices{};
        std::vector<uint32_t> m_indices{};
        
        float m_maxRadius;
    };
}