#include "engine_pipeline.hpp"

// std
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <cassert>

namespace engine {
    // Publics
    EnginePipeline::EnginePipeline(EngineDevice &device, const std::string& vertexFilePath, const std::string& fragmentFilePath, const PipelineConfigInfo& configInfo): engineDevice{device} {
        this->createGraphicsPipeline(vertexFilePath, fragmentFilePath, configInfo);
    }

    EnginePipeline::~EnginePipeline(){
        vkDestroyShaderModule(this->engineDevice.device(), this->vertexShaderModule, nullptr);
        vkDestroyShaderModule(this->engineDevice.device(), this->fragmentShadeModule, nullptr);
        vkDestroyPipeline(this->engineDevice.device(), this->graphicsPipeline, nullptr);
    }

    PipelineConfigInfo EnginePipeline::defaultPipelineConfig(uint32_t width, uint32_t height){
        PipelineConfigInfo pipelineConfigInfo = {};
        pipelineConfigInfo.pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipelineConfigInfo.pipelineInputAssemblyStateCreateInfo.flags = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipelineConfigInfo.pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        // View ports
        pipelineConfigInfo.viewPort.x = 0.0f;
        pipelineConfigInfo.viewPort.y = 0.0f;

        pipelineConfigInfo.viewPort.width = static_cast<uint32_t>(width);
        pipelineConfigInfo.viewPort.height = static_cast<uint32_t>(height);

        pipelineConfigInfo.viewPort.minDepth = 0.0f;
        pipelineConfigInfo.viewPort.maxDepth = 1.0f;

        pipelineConfigInfo.scissor.offset = {0,0};
        pipelineConfigInfo.scissor.extent = {width, height};

        pipelineConfigInfo.pipelineViewPortStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipelineConfigInfo.pipelineViewPortStateCreateInfo.viewportCount = 1;
        pipelineConfigInfo.pipelineViewPortStateCreateInfo.pViewports = &pipelineConfigInfo.viewPort;
        pipelineConfigInfo.pipelineViewPortStateCreateInfo.scissorCount = 1;
        pipelineConfigInfo.pipelineViewPortStateCreateInfo.pScissors = &pipelineConfigInfo.scissor;

        // Rasterization
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f; // Optional
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f; // Optional
        pipelineConfigInfo.pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisample
        pipelineConfigInfo.pipelineMultiSampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineConfigInfo.pipelineMultiSampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        pipelineConfigInfo.pipelineMultiSampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineConfigInfo.pipelineMultiSampleStateCreateInfo.minSampleShading = 1.0f;
        pipelineConfigInfo.pipelineMultiSampleStateCreateInfo.pSampleMask = nullptr;
        pipelineConfigInfo.pipelineMultiSampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        pipelineConfigInfo.pipelineMultiSampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

        // Color blending
        pipelineConfigInfo.pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pipelineConfigInfo.pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
        pipelineConfigInfo.pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineConfigInfo.pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineConfigInfo.pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        pipelineConfigInfo.pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineConfigInfo.pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineConfigInfo.pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.attachmentCount = 1;
        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.pAttachments = &pipelineConfigInfo.pipelineColorBlendAttachmentState;
        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
        pipelineConfigInfo.pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

        
        return pipelineConfigInfo;
    }

    // Privates
    std::vector<char> EnginePipeline::readFile(const std::string& filePath){
        
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

    void EnginePipeline::createGraphicsPipeline(const std::string &vertexFilePath, const std::string &fragmentFilePath, const PipelineConfigInfo& configInfo){
        assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline:: no pipelineLayout provided in configInfo");
        assert(configInfo.renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline:: no renderPass provided in configInfo");

        auto vertexCode = this->readFile(vertexFilePath);
        auto fragmentCode = this->readFile(fragmentFilePath);
        
        this->createShaderModule(vertexCode, &this->vertexShaderModule);
        this->createShaderModule(fragmentCode, &this->fragmentShadeModule);
        
        VkPipelineShaderStageCreateInfo shaderStages[2];
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = this->vertexShaderModule;
        shaderStages[0].pName = "main";
        shaderStages[0].flags = 0;
        shaderStages[0].pNext = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;


        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = this->fragmentShadeModule;
        shaderStages[1].pName = "main";
        shaderStages[1].flags = 0;
        shaderStages[1].pNext = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
        pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
        pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
        pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;

        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.stageCount = 2;
        graphicsPipelineCreateInfo.pStages = shaderStages;
        graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &configInfo.pipelineInputAssemblyStateCreateInfo;
        graphicsPipelineCreateInfo.pViewportState = &configInfo.pipelineViewPortStateCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &configInfo.pipelineRasterizationStateCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &configInfo.pipelineMultiSampleStateCreateInfo;
        graphicsPipelineCreateInfo.pColorBlendState = &configInfo.pipelineColorBlendStateCreateInfo;
        graphicsPipelineCreateInfo.pDepthStencilState = &configInfo.pipelineDepthStencilStateCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = nullptr;

        graphicsPipelineCreateInfo.layout = configInfo.pipelineLayout;
        graphicsPipelineCreateInfo.renderPass = configInfo.renderPass;
        graphicsPipelineCreateInfo.subpass = configInfo.subpass;
        
        graphicsPipelineCreateInfo.basePipelineIndex = -1;
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

        bool isCreatGraphicsPipelineSuccessful = vkCreateGraphicsPipelines(this->engineDevice.device(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &this->graphicsPipeline) == VK_SUCCESS;
        if(!isCreatGraphicsPipelineSuccessful) throw std::runtime_error("Failed to create graphics pipeline!");
    }

    void EnginePipeline::createShaderModule(const std::vector<char>& codes, VkShaderModule* shaderModule){
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = codes.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(codes.data());

        bool isCreateShaderModuleSuccess = vkCreateShaderModule(this->engineDevice.device(), &shaderModuleCreateInfo, nullptr, shaderModule) == VK_SUCCESS;
        if(!isCreateShaderModuleSuccess) throw std::runtime_error("Failed to create shader module!");
    }

}