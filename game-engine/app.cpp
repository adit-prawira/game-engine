#include "app.hpp"

namespace live {
    void App::run(){
        while (!liveWindow.shouldClose()){
            glfwPollEvents();
        }
    }
}
