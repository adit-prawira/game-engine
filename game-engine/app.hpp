#pragma once
#include "engine_window.hpp"
#include "engine_pipeline.hpp"
#include "engine_device.hpp"
#include "engine_swap_chain.hpp"
#include "engine_model.hpp"

// std
#include <memory>
#include <vector>

namespace engine {

    class App {
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;
            
            App();
            ~App();
            
            App(const App &) = delete;
            App &operator=(const App &)=delete;

            void run();


        private:
            EngineWindow engineWindow{WIDTH, HEIGHT, "Application Vulkan!"};
            EngineDevice engineDevice{engineWindow};
            EngineSwapChain engineSwapChain{engineDevice, this->engineWindow.getExtent()};

            std::unique_ptr<EnginePipeline> enginePipeline;
            VkPipelineLayout pipelineLayout;
            std::vector<VkCommandBuffer> commandBuffers;

            std::unique_ptr<EngineModel> engineModel;

            void createPipelineLayout();
            void createPipeline();
            void createCommandBuffers();
            void drawFrame();
            void loadModels();

            void sierpinski(
                std::vector<EngineModel::Vertex> &vertices, 
                uint32_t depth, 
                std::pair<glm::vec2, glm::vec3> left,
                std::pair<glm::vec2, glm::vec3>  top, 
                std::pair<glm::vec2, glm::vec3>  right);

    };
}
