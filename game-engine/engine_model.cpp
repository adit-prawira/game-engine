#include "engine_model.hpp"

// std
#include <cassert>
#include <cstring>

namespace engine {

  // Publics
  EngineModel::EngineModel(EngineDevice &device, const std::vector<Vertex> &vertices): engineDevice{device}{
    this->createVertexBuffers(vertices);
  }

  EngineModel::~EngineModel(){
    vkDestroyBuffer(this->engineDevice.device(), this->vertexBuffer, nullptr);
    vkFreeMemory(this->engineDevice.device(), this->vertexBufferMemory, nullptr);
  }

  void EngineModel::bind(VkCommandBuffer commandBuffer){
    VkBuffer buffers[] = {this->vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
  }

  void EngineModel::draw(VkCommandBuffer commandBuffer){
    vkCmdDraw(commandBuffer, this->vertexCount, 1, 0 ,0);
  }

  std::vector<VkVertexInputBindingDescription> EngineModel::Vertex::getBindingDescriptions(){
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
  }

  std::vector<VkVertexInputAttributeDescription> EngineModel::Vertex::getAttributeDescriptions(){
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    return attributeDescriptions;
  }

  // Privates
  void EngineModel::createVertexBuffers(const std::vector<Vertex> &vertices){
    // add minimum requirement where there are required atleast 3 vertices
    this->vertexCount = static_cast<uint32_t>(vertices.size());
    assert(this->vertexCount >= 3 && "Vertex must contain atleast 3 vertices");

    // Get the total number of bytes required for the vertex buffer to store all the vertices
    VkDeviceSize bufferSize = sizeof(vertices[0]) * this->vertexCount;
    this->engineDevice.createBuffer(
      bufferSize, 
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      this->vertexBuffer,
      this->vertexBufferMemory
    );

    void *data;
    vkMapMemory(this->engineDevice.device(), this->vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(this->engineDevice.device(), this->vertexBufferMemory);
  }
}