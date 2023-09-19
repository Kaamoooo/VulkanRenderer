#include "../Headers/Model.hpp"

namespace Kaamoo {
    Model::Model(Kaamoo::Device &device, const std::vector<Vertex> &vertices) : device{device} {
        createVertexBuffers(vertices);
    }

    Model::~Model() {
        vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
        vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
    }

    void Model::draw(VkCommandBuffer commandBuffer) {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }

    void Model::bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {vertexBuffer};
        VkDeviceSize offsets[]={0};
        vkCmdBindVertexBuffers(commandBuffer,0,1,buffers,offsets);
    }

    void Model::createVertexBuffers(const std::vector<Vertex> &vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "vertex count must be at least 3");
        //uint64_t
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
        device.createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            vertexBuffer,
                            vertexBufferMemory
        );

        void *data;
        vkMapMemory(device.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device.device(), vertexBufferMemory);
    }

    Model::Model(Device &device, const Model::Builder &builder) {

    }

    std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding=0;
        bindingDescriptions[0].inputRate=VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescriptions[0].stride=sizeof(Vertex);
        return bindingDescriptions;
    }

    //绑定Attribute,例如我们为顶点制定了position和color,这里的Descriptions数量就应该为2
    std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
        attributeDescriptions[0].binding=0;
        attributeDescriptions[0].offset=0;
        attributeDescriptions[0].format=VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].location=0;

        attributeDescriptions[1].binding=0;
        attributeDescriptions[1].offset= offsetof(Vertex,color);
        attributeDescriptions[1].format=VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].location=1;
        return attributeDescriptions;
    }
}
