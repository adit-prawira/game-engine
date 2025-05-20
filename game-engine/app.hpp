#pragma once
#include "live_window.hpp"

namespace live {

class App {
public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;
    
    void run();
    
private:
    LiveWindow liveWindow{WIDTH, HEIGHT, "Application Vulkan!"};
};
}
