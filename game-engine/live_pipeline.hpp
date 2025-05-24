#pragma once
#include "live_device.hpp"

// std
#include <string>
#include <vector>

namespace live {
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

    class LivePipeline {
        public:
            LivePipeline(LiveDevice &device,  const std::string& vertexFilePath, const std::string& fragmentFilePath, const PipelineConfigInfo& configInfo);
            ~LivePipeline();
            
            LivePipeline(const LivePipeline&) = delete;
            void operator = (const LivePipeline&) = delete; 
            static PipelineConfigInfo defaultPipelineConfig(uint32_t width, uint32_t height);
        private:
            static std::vector<char> readFile(const std::string& filePath);
            
            void createGraphicsPipeline(const std::string& vertexFilePath, const std::string& fragmentFilePath, const PipelineConfigInfo& configInfo);
            
            void createShaderModule(const std::vector<char>& codes, VkShaderModule* shaderModule);

            // Pipeline need device to exist, but this is an aggregation where it can exist independently from the parent
            LiveDevice& liveDevice;
            VkPipeline graphicsPipeline;
            VkShaderModule vertexShaderModule;
            VkShaderModule fragmentShadeModule;

    };
}
