#pragma once
#include "engine_window.hpp"
#include "engine_pipeline.hpp"
#include "engine_device.hpp"
namespace engine {

    class App {
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;
            
            void run();
            
        private:
            EngineWindow engineWindow{WIDTH, HEIGHT, "Application Vulkan!"};
            EngineDevice engineDevice{engineWindow};
            // read compiled shader vertext and fragment file code
            EnginePipeline enginePipeline{engineDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", EnginePipeline::defaultPipelineConfig(WIDTH, HEIGHT)};
    };
}
