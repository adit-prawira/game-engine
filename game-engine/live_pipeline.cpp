#include "live_pipeline.hpp"

// std
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>

namespace live {
    // Publics
    LivePipeline::LivePipeline(LiveDevice &device, const std::string& vertexFilePath, const std::string& fragmentFilePath, const PipelineConfigInfo& configInfo): liveDevice{device} {
        this->createGraphicsPipeline(vertexFilePath, fragmentFilePath, configInfo);
    }

    PipelineConfigInfo LivePipeline::defaultPipelineConfig(uint32_t width, uint32_t height){
        PipelineConfigInfo pipelineConfigInfo = {};
        return pipelineConfigInfo;
    }

    // Privates
    std::vector<char> LivePipeline::readFile(const std::string& filePath){
        
        // read file, and when open seeked the end immediately and read it as binary
        std::filesystem::path currentPath = std::filesystem::current_path();
        
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        std::cout << "Current relative path: " << currentPath.string() << "\n";
        
        std::cout << "Is file opened: " << file.good() << "\n";
        if(!file.is_open()) throw std::runtime_error("Failed to open file: " + filePath);
        
        size_t fileSize = static_cast<size_t>(file.tellg());
        
        std::vector<char> buffer(fileSize);
        
        // read file
        file.seekg(0);
        
        // append to buffer
        file.read(buffer.data(), fileSize);
        
        // close file
        file.close();
        
        return buffer;
    }

    void LivePipeline::createGraphicsPipeline(const std::string &vertexFilePath, const std::string &fragmentFilePath, const PipelineConfigInfo& configInfo){
        auto vertexCode = this->readFile(vertexFilePath);
        auto fragmentCode = this->readFile(fragmentFilePath);
        
        std::cout << "Vertex shader size -> " << vertexCode.size() << "\n";
        std::cout << "Fragment shadert size -> " << fragmentCode.size() << "\n";
    }

    void LivePipeline::createShaderModule(const std::vector<char>& codes, VkShaderModule* shaderModule){
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = codes.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(codes.data());

        bool isCreateShaderModuleSuccess = vkCreateShaderModule(this->liveDevice.device(), &shaderModuleCreateInfo, nullptr, shaderModule) == VK_SUCCESS;
        if(!isCreateShaderModuleSuccess) throw std::runtime_error("Failed to create shader module!");
    }

}
