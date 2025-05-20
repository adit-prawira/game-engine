#!/bin/bash

# Load .env file
if [ -f .env ]; then
  export $(grep -v '^#' .env | xargs)
fi

echo "Compiling with VulkanSDK at path: $VULKAN_SDK_PATH"

$VULKAN_SDK_PATH/bin/glslc shaders/simple_shader.vert -o shaders/simple_shader.vert.spv
$VULKAN_SDK_PATH/bin/glslc shaders/simple_shader.frag -o shaders/simple_shader.frag.spv

