#include "Model.hpp"

namespace Kaamoo {
    Model::Model(Kaamoo::Device &device, const Builder &builder) : device{device} {
        createVertexBuffers(builder.vertices);
        createIndexBuffers(builder.indices);
    }

    Model::~Model() {
        vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
        vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
        if (hasIndexBuffer) {
            vkDestroyBuffer(device.device(), indexBuffer, nullptr);
            vkFreeMemory(device.device(), indexBufferMemory, nullptr);
        }
    }

    void Model::draw(VkCommandBuffer commandBuffer) {
        //有index buffer就使用
        if (hasIndexBuffer) {
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        } else {
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    void Model::bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if (hasIndexBuffer) {
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void Model::createVertexBuffers(const std::vector<Vertex> &vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());

        assert(vertexCount >= 3 && "vertex count must be at least 3");
        //uint64_t
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                //host表示CPU与GPU的内存可以相绑定，性能较低
                //coherent表示CPU对应内存改变时，GPU对应内存也改变
                //因此使用staging buffer来进行中转，将缓冲区最终映射到device local的buffer中，性能最高(当然是对于静态数据而言)
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingBuffer,
                            stagingMemory
        );

        void *data;
        vkMapMemory(device.device(), stagingMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device.device(), stagingMemory);

        device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        device.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
        vkFreeMemory(device.device(), stagingMemory, nullptr);
    }

    void Model::createIndexBuffers(const std::vector<uint32_t> &indices) {
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = !indices.empty();

        if (!hasIndexBuffer)return;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            stagingBuffer,
                            stagingMemory
        );

        void *data;
        vkMapMemory(device.device(), stagingMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device.device(), stagingMemory);

        device.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        device.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
        vkFreeMemory(device.device(), stagingMemory, nullptr);
    }


    std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescriptions[0].stride = sizeof(Vertex);
        return bindingDescriptions;
    }

//绑定Attribute,例如我们为顶点制定了position和color,这里的Descriptions数量就应该为2
    std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].offset = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].location = 0;

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].location = 1;
        return attributeDescriptions;
    }

}
