#include "live_pipeline.hpp"

// std
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>

namespace live {

    LivePipeline::LivePipeline(const std::string& vertexFilePath, const std::string& fragmentFilePath){
        this->createGraphicsPipeline(vertexFilePath, fragmentFilePath);
    }

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

    void LivePipeline::createGraphicsPipeline(const std::string &vertexFilePath, const std::string &fragmentFilePath){
        auto vertexCode = this->readFile(vertexFilePath);
        auto fragmentCode = this->readFile(fragmentFilePath);
        
        std::cout << "Vertex shader size -> " << vertexCode.size() << "\n";
        std::cout << "Fragment shadert size -> " << fragmentCode.size() << "\n";
    }
}
