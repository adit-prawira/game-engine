#pragma once
#include "engine_device.hpp"

// std
#include <string>
#include <vector>

namespace engine {
    struct PipelineConfigInfo {
        VkViewport viewPort;
        VkRect2D scissor;
        VkPipelineViewportStateCreateInfo pipelineViewPortStateCreateInfo;
        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
        VkPipelineMultisampleStateCreateInfo pipelineMultiSampleStateCreateInfo;
        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo;
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;
        uint32_t subpass = 0;
    };

    class EnginePipeline {
        public:
            EnginePipeline(EngineDevice &device,  const std::string& vertexFilePath, const std::string& fragmentFilePath, const PipelineConfigInfo& configInfo);
            ~EnginePipeline();

            EnginePipeline(const EnginePipeline&) = delete;
            void operator = (const EnginePipeline&) = delete; 
            static PipelineConfigInfo defaultPipelineConfig(uint32_t width, uint32_t height);
        private:
            static std::vector<char> readFile(const std::string& filePath);
            
            void createGraphicsPipeline(const std::string& vertexFilePath, const std::string& fragmentFilePath, const PipelineConfigInfo& configInfo);
            
            void createShaderModule(const std::vector<char>& codes, VkShaderModule* shaderModule);

            // Pipeline need device to exist, but this is an aggregation where it can exist independently from the parent
            EngineDevice& engineDevice;
            VkPipeline graphicsPipeline;
            VkShaderModule vertexShaderModule;
            VkShaderModule fragmentShadeModule;

    };
}
