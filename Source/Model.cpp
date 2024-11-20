#include "Model.hpp"

//引用tiny obj loader库读取模型
#define TINYOBJLOADER_IMPLEMENTATION

#include "../External/tiny_obj_loader.h"
#include <iostream>
#include "Untils.h"
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/hash.hpp>

namespace std {
    //模板特化
    template<>
    struct hash<Kaamoo::Model::Vertex> {
        //重载哈希函数对象的调用运算符
        size_t operator()(Kaamoo::Model::Vertex const &vertex) const {
            size_t seed = 0;
//            Kaamoo::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
//            Kaamoo::hashCombine(seed, vertex.position);
//            return seed;
//            return ((hash<glm::vec3>()(vertex.position) ^
//                     (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
//                   (hash<glm::vec3>()(vertex.normal) << 1) ^
//                   (hash<glm::vec2>()(vertex.uv) << 1); 
            seed = hash<glm::vec3>()(vertex.position);
            return seed;
        }
    };
}


namespace Kaamoo {
    Model::Model(Kaamoo::Device &device, const Builder &builder) : device{device} {
        static uint32_t modelIndex = 0;
        indexReference = modelIndex++;
        createVertexBuffers(builder.vertices);
        createIndexBuffers(builder.indices);
        m_vertices = builder.vertices;
        m_indices = builder.indices;
    }

    void Model::draw(VkCommandBuffer commandBuffer) {
        if (hasIndexBuffer) {
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        } else {
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    void Model::bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = {vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if (hasIndexBuffer) {
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void Model::createIndexBuffers(const std::vector<uint32_t> &indices) {
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = !indices.empty();
        if (!hasIndexBuffer)return;

        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        uint32_t indexSize = sizeof(indices[0]);

        Buffer stagingBuffer(device, indexSize, indexCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void *) indices.data());
#ifdef RAY_TRACING
        indexBuffer = std::make_unique<Buffer>(
                device, indexSize, indexCount,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | rayTracingFlags,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
#else
        indexBuffer = std::make_unique<Buffer>(
                device, indexSize, indexCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
#endif

        device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }

    std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescriptions[0].stride = sizeof(Vertex);
        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        //position,color,normal,uv
        attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0});
        attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
        attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
        attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, smoothedNormal)});
        attributeDescriptions.push_back({4, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

        return attributeDescriptions;
    }

    void Model::Builder::loadModel(const std::string &filePath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto &shape: shapes) {
            for (int i = 0; i < shape.mesh.indices.size(); i++) {
                const auto &index = shape.mesh.indices[i];

                Vertex vertex{};

                if (index.vertex_index >= 0) {
                    vertex.position = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.color = {
                            attrib.colors[3 * index.vertex_index + 0],
                            attrib.colors[3 * index.vertex_index + 1],
                            attrib.colors[3 * index.vertex_index + 2],
                    };
                }

                if (index.normal_index >= 0) {
                    vertex.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                    };
                }

                if (i % 3 == 2) {
                    auto idx0 = shape.mesh.indices[i - 2];
                    auto idx1 = shape.mesh.indices[i - 1];
                    auto idx2 = shape.mesh.indices[i];

                    glm::vec3 v0 = glm::vec3(
                            attrib.vertices[3 * idx0.vertex_index + 0],
                            attrib.vertices[3 * idx0.vertex_index + 1],
                            attrib.vertices[3 * idx0.vertex_index + 2]
                    );
                    glm::vec3 v1 = glm::vec3(
                            attrib.vertices[3 * idx1.vertex_index + 0],
                            attrib.vertices[3 * idx1.vertex_index + 1],
                            attrib.vertices[3 * idx1.vertex_index + 2]
                    );
                    glm::vec3 v2 = glm::vec3(
                            attrib.vertices[3 * idx2.vertex_index + 0],
                            attrib.vertices[3 * idx2.vertex_index + 1],
                            attrib.vertices[3 * idx2.vertex_index + 2]
                    );

                    glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

                    auto back = indices.size() - 1;
                    vertices[indices[back - 1]].smoothedNormal += faceNormal;
                    vertices[indices[back]].smoothedNormal += faceNormal;
                    vertex.smoothedNormal += faceNormal;
                }

                if (index.texcoord_index >= 0) {
                    vertex.uv = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
        
    }
}
