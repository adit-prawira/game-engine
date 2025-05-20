#pragma once
#include "live_window.hpp"
#include "live_pipeline.hpp"

namespace live {

class App {
public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;
    
    void run();
    
private:
    LiveWindow liveWindow{WIDTH, HEIGHT, "Application Vulkan!"};
    
    // read compiled shader vertext and fragment file code
    LivePipeline livePipeline{"shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv"};
};
}
