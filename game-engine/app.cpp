#include "app.hpp"


// std
#include <stdexcept>
#include <array>
#include <iostream>

namespace engine {
    // Publics
    App::App(){
        this->loadModels();
        this->createPipelineLayout();
        this->createPipeline();
        this->createCommandBuffers();
    }

    App::~App(){
        vkDestroyPipelineLayout(this->engineDevice.device(), this->pipelineLayout, nullptr);

    }
    
    void App::run(){
        while (!engineWindow.shouldClose()){
            glfwPollEvents();
            this->drawFrame();
        }
        vkDeviceWaitIdle(this->engineDevice.device());
    }
    

    // Privates
    void App::createPipelineLayout(){
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        bool isCreatePipelineLayoutSuccess = vkCreatePipelineLayout(this->engineDevice.device(), &pipelineLayoutCreateInfo, nullptr, &this->pipelineLayout) == VK_SUCCESS;
        if(!isCreatePipelineLayoutSuccess) throw std::runtime_error("Failed to create pipeline layout");
    }

    void App::createPipeline(){
        auto pipelineConfig = EnginePipeline::defaultPipelineConfig(this->engineSwapChain.width(), this->engineSwapChain.height());
        pipelineConfig.renderPass = this->engineSwapChain.getRenderPass();
        pipelineConfig.pipelineLayout = this->pipelineLayout;

        // read compiled shader vertext and fragment file code
        this->enginePipeline = std::make_unique<EnginePipeline>(
            this->engineDevice,
            "shaders/simple_shader.vert.spv", 
            "shaders/simple_shader.frag.spv",
            pipelineConfig
        );
    }

    void App::createCommandBuffers(){
        this->commandBuffers.resize(this->engineSwapChain.imageCount());

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        commandBufferAllocateInfo.commandPool = this->engineDevice.getCommandPool();
        commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(this->commandBuffers.size());

        bool isAllocateCommandBufferSuccess = vkAllocateCommandBuffers(this->engineDevice.device(), &commandBufferAllocateInfo, this->commandBuffers.data()) == VK_SUCCESS;
        if(!isAllocateCommandBufferSuccess) throw std::runtime_error("Failed to allocate command buffer");
        
        // Record command for each buffer
        for(int i = 0; i < this->commandBuffers.size(); i++){
            VkCommandBufferBeginInfo commandBufferBeginInfo = {};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            bool isBeginCommandBufferSuccess = vkBeginCommandBuffer(this->commandBuffers[i], &commandBufferBeginInfo) == VK_SUCCESS;
            if(!isBeginCommandBufferSuccess) throw std::runtime_error("Failed to begin recording command buffer!");

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = this->engineSwapChain.getRenderPass();
            renderPassBeginInfo.framebuffer = this->engineSwapChain.getFrameBuffer(i);
    
            renderPassBeginInfo.renderArea.offset = {0,0};
            renderPassBeginInfo.renderArea.extent = this->engineSwapChain.getSwapChainExtent();

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassBeginInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(this->commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            this->enginePipeline->bind(this->commandBuffers[i]);
            this->engineModel->bind(this->commandBuffers[i]);
            this->engineModel->draw(this->commandBuffers[i]);

            vkCmdEndRenderPass(this->commandBuffers[i]);
            bool isEndCommandBufferSuccess = vkEndCommandBuffer(this->commandBuffers[i]) == VK_SUCCESS;
            if(!isEndCommandBufferSuccess) throw std::runtime_error("Failed to record command buffer");
        }

    }

    void App::drawFrame(){        
        uint32_t imageIndex;
        auto result = this->engineSwapChain.acquireNextImage(&imageIndex);

        bool isSuccess = result == VK_SUCCESS;
        bool isSuboptimal = result == VK_SUBOPTIMAL_KHR;
        if(!isSuccess && isSuboptimal) throw std::runtime_error("Failed to acquire swap chain image!");

        // Send command to the device graphics queue while handling CPU and GPU synchronisation
        result = this->engineSwapChain.submitCommandBuffers(&this->commandBuffers[imageIndex], &imageIndex);
        bool isSubmitSuccess = result == VK_SUCCESS;
        if(!isSubmitSuccess) throw std::runtime_error("Failed to submit command buffer to device graphics queue");
    }

    void App::loadModels(){
        std::vector<EngineModel::Vertex> vertices{};
        this->sierpinski(vertices, 1, 
            {{-0.5f, 0.5f}, {0.0f, 0.0f,1.0f}}, 
            {{0.0f, -0.5f}, {1.0f, 0.0f,0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f,0.0f}}
        );
        this->engineModel = std::make_unique<EngineModel>(
            this->engineDevice,
            vertices
        );
    }

    
    void App::sierpinski(
        std::vector<EngineModel::Vertex> &vertices, 
        uint32_t depth, 
        std::pair<glm::vec2, glm::vec3> left,
        std::pair<glm::vec2, glm::vec3>  top, 
        std::pair<glm::vec2, glm::vec3>  right
    ){
        if(depth <= 0 ){
            vertices.push_back({top.first, top.second});
            vertices.push_back({right.first, right.second});
            vertices.push_back({left.first, left.second});
            return;
        }
        glm::vec2 leftTopMidVector = (left.first + top.first) / 2.0f;
        glm::vec2 topRightMidVector = (top.first + right.first) / 2.0f;
        glm::vec2 rightLeftMidVector = (right.first + left.first) /2.0f;

        // bottom left triangle
        this->sierpinski(vertices, depth-1, left, {leftTopMidVector, top.second}, {rightLeftMidVector, right.second});

        // top triangle
        this->sierpinski(vertices, depth-1, {leftTopMidVector, left.second}, top, {topRightMidVector, right.second});

        // bottom right triangle
        this->sierpinski(vertices, depth-1, {rightLeftMidVector, left.second}, {topRightMidVector, top.second}, right);
    }
}
