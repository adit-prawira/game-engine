#pragma once

#include "engine_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <vector>

namespace engine {
  // Read created vertex data file on the CPU
  // then copy over data to our device GPU to be rendered efficiently
  class EngineModel {
    public:

      struct Vertex {
        glm::vec2 position;
        glm::vec3 color;
        
        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
      };

      EngineModel(EngineDevice &device, const std::vector<Vertex> &vertices);
      ~EngineModel();

      EngineModel(const EngineModel &) = delete;
      EngineModel &operator = (const EngineModel &) = delete;

      void bind(VkCommandBuffer comandBuffer);
      void draw(VkCommandBuffer commandBuffer);

    private:
      EngineDevice &engineDevice;
      uint32_t vertexCount;

      // Buffer and the memory are separated to have full control in memory management
      // otherwise it is easy to hit the maxMemoryAllocationCount within VkDevice
      VkBuffer vertexBuffer;
      VkDeviceMemory vertexBufferMemory;

      void createVertexBuffers(const std::vector<Vertex> &vertices);

  };
}